#include "GeoControlPointManager.h"
#include "../GeometryBase.h"
#include <stdexcept>
#include <algorithm>

GeoControlPointManager::GeoControlPointManager(Geo3D* parent)
    : QObject(parent)
    , m_parent(parent)
{
    /**
    * 默认有一个空的阶段
    */
    m_stages.emplace_back();
}

bool GeoControlPointManager::addControlPoint(const Point3D& point)
{
    assert(stageSize() <= getStageDescriptors().size() && "不应该超过限制的绘制阶段");
    assert(currentStageIdx() >= 0 &&
           currentStagePointSize() <= getStageDescriptor(currentStageIdx()).maxControlPoints && "不应该超过限制的点数");

    /**
    * 应用约束函数（如果存在）
    */
    Point3D constrainedPoint = point;
    const auto& currentDescriptor = getStageDescriptor(currentStageIdx());

    if (currentDescriptor.constraint) 
    {
        constrainedPoint = currentDescriptor.constraint(point, m_stages);
    }

    /**
    * 因为一个阶段至少能容纳一个，且达到上限后自动切换下一阶段
    * 所以不需要太多判断
    */
    currentStage().emplace_back(constrainedPoint);
    if (currentStagePointSize() == currentDescriptor.maxControlPoints)
    {
        nextStage();
    }
    emit controlPointChanged();
    return true;
}

bool GeoControlPointManager::undoLastControlPoint()
{
    assert(m_stages.size() && "初始化时已经不为空了");

    // 第一阶段为空时，不做撤销
    if (stageSize() <= 1 && !currentStagePointSize())
        return false;

    if (currentStagePointSize())
    {
        currentStage().pop_back();
        emit controlPointChanged();
    }
    else
    {
        assert(stageSize() > 1 && "已经做了非空判断了");
        
        // 这两步应该是原子操作，不允许一个当前阶段是满的状态（除了绘制完成）
        m_stages.pop_back();
        currentStage().pop_back();
    }
    return true;
}

bool GeoControlPointManager::nextStage()
{
    if (currentStagePointSize() < getStageDescriptor(currentStageIdx()).minControlPoints)
    {
        getState()->setStateInvalid();
        return false;
    }

    assert(stageSize() <= getStageDescriptors().size() && "如果超过了就不会结束");

    if (stageSize() == getStageDescriptors().size())
    {
        getState()->setStateComplete();
        return false;
    }

    m_stages.emplace_back();
    return true;
}

void GeoControlPointManager::setTempPoint(const Point3D& point)
{
    assert(!getState()->isStateComplete() && "应该在没有绘制完成时调用");
    m_tempPoint = point;
    emit controlPointChanged();
}

bool GeoControlPointManager::setControlPoint(int globalIndex, const Point3D& point)
{
    assert(globalIndex >= 0 && "索引应该非负");
    assert(getState()->isStateComplete() && "应该在绘制完成后调用");


    auto updataControlPoint = [&]()
    {
        for (int stageIdx = 0; stageIdx < m_stages.size(); stageIdx++)
        {
            const auto& currentDescriptor = getStageDescriptor(stageIdx);
            if (!currentDescriptor.constraint)continue;
            for (auto& point : m_stages[stageIdx])
            {
                point = currentDescriptor.constraint(point, m_stages);
            }
        }
    };



    for (int stageIdx = 0; stageIdx < m_stages.size(); stageIdx++)
    {
        if (globalIndex < m_stages[stageIdx].size())
        {
            m_stages[stageIdx][globalIndex] = point;
            // 控制点点更新，按顺序，后续依次用约束更新,为了方便全部更新
            emit controlPointChanged();
            return true;
        }
        globalIndex -= m_stages[stageIdx].size();
    }



    assert(false && "索引越界");
    return false;
}

const std::vector<std::vector<Point3D>>& GeoControlPointManager::getAllStageControlPoints()
{
    if (getState()->isStateComplete())
    {
        return m_stages;
    }
    else
    {
        assert(currentStagePointSize() < getStageDescriptor(currentStageIdx()).maxControlPoints && "未完成绘制时控制点不应该满");

        // 约束临时点
        const auto& currentDescriptor = getStageDescriptor(currentStageIdx());
        if (currentDescriptor.constraint)
        {
            m_tempPoint = currentDescriptor.constraint(m_tempPoint, m_stages);
        }

        m_stagesTemp = m_stages;
        m_stagesTemp.back().emplace_back(m_tempPoint);
        return m_stagesTemp;
    }
}

const StageDescriptors& GeoControlPointManager::getStageDescriptors() const
{
    assert(m_parent);
    return m_parent->getStageDescriptors();
}

const StageDescriptor& GeoControlPointManager::getStageDescriptor(int idx) const
{
    assert(m_parent && idx < getStageDescriptors().size());
    return getStageDescriptors()[idx];
}

GeoStateManager* GeoControlPointManager::getState() const
{
    assert(m_parent);
    return m_parent->mm_state();
}

