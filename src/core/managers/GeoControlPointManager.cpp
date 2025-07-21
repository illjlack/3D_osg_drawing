#include "GeoControlPointManager.h"
#include "../GeometryBase.h"
#include <stdexcept>
#include <algorithm>

GeoControlPointManager::GeoControlPointManager(Geo3D* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_tempPoint(Point3D(glm::vec3(0))) // 初始化临时点
{
}

// 控制点访问 - 对外统一接口
const std::vector<Point3D>& GeoControlPointManager::getControlPoints() const
{
    // 如果绘制未完成且有临时点，返回包含临时点的控制点列表
    if (!isDrawingComplete() && m_tempPoint.position != glm::vec3(0)) {
        // 使用成员变量而不是static变量，避免多个几何体之间的冲突
        m_tempControlPointsList = m_controlPoints;
        m_tempControlPointsList.push_back(m_tempPoint);
        return m_tempControlPointsList;
    }
    return m_controlPoints;
}

Point3D GeoControlPointManager::getControlPoint(int index) const
{
    validateIndex(index);
    
    // 如果绘制未完成且有临时点，且索引是最后一个，返回临时点
    if (!isDrawingComplete() && m_tempPoint.position != glm::vec3(0) && 
        index == static_cast<int>(m_controlPoints.size())) {
        return m_tempPoint;
    }
    
    return m_controlPoints[index];
}

int GeoControlPointManager::getControlPointCountWithoutTempPoint() const
{
    int count = static_cast<int>(m_controlPoints.size());
    
    // 如果绘制未完成且有临时点，计数加1 （返回真正控制点个数）
    //if (!isDrawingComplete() && m_tempPoint.position != glm::vec3(0)) {
    //    count++;
    //}
    
    return count;
}

bool GeoControlPointManager::hasControlPoints() const
{
    return !m_controlPoints.empty() || 
           (!isDrawingComplete() && m_tempPoint.position != glm::vec3(0));
}

void GeoControlPointManager::addControlPoint(const Point3D& point)
{
    m_controlPoints.push_back(point);
    
    notifyGeometryChanged();
}

void GeoControlPointManager::setControlPoint(int index, const Point3D& point)
{
    validateIndex(index);
    
    // 如果绘制未完成且有临时点，且索引是最后一个，设置临时点
    if (!isDrawingComplete() && m_tempPoint.position != glm::vec3(0) && 
        index == static_cast<int>(m_controlPoints.size())) {
        setTempPoint(point);
        return;
    }
    
    m_controlPoints[index] = point;
    
    notifyGeometryChanged();
}

void GeoControlPointManager::removeControlPoint(int index)
{
    validateIndex(index);
    
    // 如果绘制未完成且有临时点，且索引是最后一个，清除临时点
    if (!isDrawingComplete() && m_tempPoint.position != glm::vec3(0) && 
        index == static_cast<int>(m_controlPoints.size())) {
        clearTempPoint();
        return;
    }
    
    m_controlPoints.erase(m_controlPoints.begin() + index);
    
    notifyGeometryChanged();
}

void GeoControlPointManager::clearControlPoints()
{
    if (!m_controlPoints.empty() || 
        (!isDrawingComplete() && m_tempPoint.position != glm::vec3(0))) {
        m_controlPoints.clear();
        clearTempPoint();
        
        notifyGeometryChanged();
    }
}

int GeoControlPointManager::findNearestControlPoint(const Point3D& point, float threshold) const
{
    int nearestIndex = -1;
    float minDistance = threshold;
    
    // 搜索实际控制点
    for (int i = 0; i < static_cast<int>(m_controlPoints.size()); ++i) {
        glm::vec3 diff = m_controlPoints[i].position - point.position;
        float distance = glm::length(diff);
        
        if (distance < minDistance) {
            minDistance = distance;
            nearestIndex = i;
        }
    }
    
    // 如果绘制未完成且有临时点，也搜索临时点
    if (!isDrawingComplete() && m_tempPoint.position != glm::vec3(0)) {
        glm::vec3 diff = m_tempPoint.position - point.position;
        float distance = glm::length(diff);
        
        if (distance < minDistance) {
            minDistance = distance;
            nearestIndex = static_cast<int>(m_controlPoints.size());
        }
    }
    
    return nearestIndex;
}

bool GeoControlPointManager::isValidIndex(int index) const
{
    int maxIndex = static_cast<int>(m_controlPoints.size());
    
    // 如果绘制未完成且有临时点，最大索引加1
    if (!isDrawingComplete() && m_tempPoint.position != glm::vec3(0)) {
        maxIndex++;
    }
    
    return index >= 0 && index < maxIndex;
}

void GeoControlPointManager::setTempPoint(const Point3D& point)
{
    if (m_tempPoint.position != point.position) {
        m_tempPoint = point;
        notifyGeometryChanged();
    }
}

void GeoControlPointManager::clearTempPoint()
{
    if (m_tempPoint.position != glm::vec3(0)) {
        m_tempPoint = Point3D(glm::vec3(0));
        notifyGeometryChanged();
    }
}

void GeoControlPointManager::notifyGeometryChanged()
{
    // 通知父对象几何体需要更新
    if (m_parent && m_parent->mm_state()) {
        m_parent->mm_node()->updateGeometries();
    }
}

void GeoControlPointManager::validateIndex(int index) const
{
    if (!isValidIndex(index)) {
        throw std::out_of_range("Control point index out of range: " + std::to_string(index));
    }
}

bool GeoControlPointManager::isDrawingComplete() const
{
    // 检查绘制是否完成状态
    return m_parent && m_parent->mm_state() && m_parent->mm_state()->isStateComplete();
} 