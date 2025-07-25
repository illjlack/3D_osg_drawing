#include "GeoControlPointManager.h"
#include "../GeometryBase.h"
#include <stdexcept>
#include <algorithm>
#include <sstream> // Added for serialization/deserialization

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

std::string GeoControlPointManager::serializeControlPoints() const
{
    std::ostringstream oss;
    
    // 保存阶段数量（只保存已完成的阶段，不保存当前正在进行的空阶段）
    size_t completedStages = m_stages.size();
    if (!m_stages.empty() && m_stages.back().empty()) {
        completedStages--; // 不保存最后的空阶段
    }
    
    oss << completedStages;
    
    // 保存每个阶段的控制点
    for (size_t stageIdx = 0; stageIdx < completedStages; ++stageIdx) {
        const auto& stage = m_stages[stageIdx];
        oss << ";" << stage.size(); // 该阶段的点数量
        
        for (const auto& point : stage) {
            oss << ";" << point.x() << "," << point.y() << "," << point.z();
        }
    }
    
    return oss.str();
}

bool GeoControlPointManager::deserializeControlPoints(const std::string& data)
{
    if (data.empty()) {
        // 空数据，保持默认状态（一个空阶段）
        return true;
    }
    
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;
    
    // 分割字符串
    while (std::getline(iss, token, ';')) {
        tokens.push_back(token);
    }
    
    if (tokens.empty()) {
        return false;
    }
    
    try {
        // 读取阶段数量
        size_t stageCount = std::stoull(tokens[0]);
        
        // 清空现有数据
        m_stages.clear();
        
        size_t tokenIdx = 1;
        
        // 读取每个阶段的数据
        for (size_t stageIdx = 0; stageIdx < stageCount; ++stageIdx) {
            if (tokenIdx >= tokens.size()) {
                return false; // 数据不完整
            }
            
            // 读取该阶段的点数量
            size_t pointCount = std::stoull(tokens[tokenIdx++]);
            
            std::vector<Point3D> stagePoints;
            
            // 读取该阶段的所有点
            for (size_t pointIdx = 0; pointIdx < pointCount; ++pointIdx) {
                if (tokenIdx >= tokens.size()) {
                    return false; // 数据不完整
                }
                
                // 解析点坐标 "x,y,z"
                std::string pointStr = tokens[tokenIdx++];
                std::istringstream pointStream(pointStr);
                std::string coord;
                std::vector<std::string> coords;
                
                while (std::getline(pointStream, coord, ',')) {
                    coords.push_back(coord);
                }
                
                if (coords.size() != 3) {
                    return false; // 坐标格式错误
                }
                
                double x = std::stod(coords[0]);
                double y = std::stod(coords[1]);
                double z = std::stod(coords[2]);
                
                stagePoints.emplace_back(x, y, z);
            }
            
            m_stages.push_back(stagePoints);
        }

        // 不重新触发计算，因为保存在节点里，省的发生什么错误
        // emit controlPointChanged();
        return true;
        
    } catch (const std::exception& e) {
        // 解析失败，恢复默认状态
        m_stages.clear();
        m_stages.emplace_back();
        return false;
    }
}

