#include "GeoControlPointManager.h"
#include "../GeometryBase.h"
#include <stdexcept>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

GeoControlPointManager::GeoControlPointManager(Geo3D* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_minimumPointsRequired(1)
    , m_previewActive(false)
    , m_controlPointsVisible(true)
    , m_controlPointSize(0.1f)
    , m_controlPointColor(1.0f, 0.0f, 0.0f, 1.0f) // 默认红色
    , m_tempPoint(Point3D(glm::vec3(0))) // 初始化临时点
{
}

Point3D GeoControlPointManager::getControlPoint(int index) const
{
    validateIndex(index);
    return m_controlPoints[index];
}

void GeoControlPointManager::addControlPoint(const Point3D& point)
{
    int index = static_cast<int>(m_controlPoints.size());
    m_controlPoints.push_back(point);
    
    notifyGeometryChanged();
    updateControlPointVisualization();
    
    emit controlPointAdded(index, point);
}

void GeoControlPointManager::insertControlPoint(int index, const Point3D& point)
{
    if (index < 0 || index > static_cast<int>(m_controlPoints.size())) {
        throw std::out_of_range("Control point index out of range");
    }
    
    m_controlPoints.insert(m_controlPoints.begin() + index, point);
    
    notifyGeometryChanged();
    updateControlPointVisualization();
    
    emit controlPointAdded(index, point);
}

void GeoControlPointManager::setControlPoint(int index, const Point3D& point)
{
    validateIndex(index);
    
    Point3D oldPoint = m_controlPoints[index];
    m_controlPoints[index] = point;
    
    notifyGeometryChanged();
    updateControlPointVisualization();
    
    emit controlPointChanged(index, oldPoint, point);
}

void GeoControlPointManager::removeControlPoint(int index)
{
    validateIndex(index);
    
    m_controlPoints.erase(m_controlPoints.begin() + index);
    
    notifyGeometryChanged();
    updateControlPointVisualization();
    
    emit controlPointRemoved(index);
}

void GeoControlPointManager::removeLastControlPoint()
{
    if (!m_controlPoints.empty()) {
        removeControlPoint(static_cast<int>(m_controlPoints.size() - 1));
    }
}

void GeoControlPointManager::clearControlPoints()
{
    if (!m_controlPoints.empty()) {
        m_controlPoints.clear();
        
        notifyGeometryChanged();
        updateControlPointVisualization();
        
        emit controlPointsCleared();
    }
}

int GeoControlPointManager::findNearestControlPoint(const Point3D& point, float threshold) const
{
    int nearestIndex = -1;
    float minDistance = threshold;
    
    for (int i = 0; i < static_cast<int>(m_controlPoints.size()); ++i) {
        glm::vec3 diff = m_controlPoints[i].position - point.position;
        float distance = glm::length(diff);
        
        if (distance < minDistance) {
            minDistance = distance;
            nearestIndex = i;
        }
    }
    
    return nearestIndex;
}

bool GeoControlPointManager::isValidIndex(int index) const
{
    return index >= 0 && index < static_cast<int>(m_controlPoints.size());
}

void GeoControlPointManager::translateControlPoints(const glm::vec3& offset)
{
    for (auto& point : m_controlPoints) {
        point.position += offset;
    }
    
    notifyGeometryChanged();
    updateControlPointVisualization();
    
    emit controlPointsTransformed();
}

void GeoControlPointManager::rotateControlPoints(const glm::vec3& axis, float angle, const glm::vec3& center)
{
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, axis);
    glm::mat4 translate1 = glm::translate(glm::mat4(1.0f), -center);
    glm::mat4 translate2 = glm::translate(glm::mat4(1.0f), center);
    glm::mat4 transform = translate2 * rotation * translate1;
    
    transformControlPoints(transform);
}

void GeoControlPointManager::scaleControlPoints(const glm::vec3& scale, const glm::vec3& center)
{
    glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);
    glm::mat4 translate1 = glm::translate(glm::mat4(1.0f), -center);
    glm::mat4 translate2 = glm::translate(glm::mat4(1.0f), center);
    glm::mat4 transform = translate2 * scaling * translate1;
    
    transformControlPoints(transform);
}

void GeoControlPointManager::transformControlPoints(const glm::mat4& matrix)
{
    for (auto& point : m_controlPoints) {
        glm::vec4 pos = glm::vec4(point.position, 1.0f);
        pos = matrix * pos;
        point.position = glm::vec3(pos) / pos.w;
    }
    
    notifyGeometryChanged();
    updateControlPointVisualization();
    
    emit controlPointsTransformed();
}

bool GeoControlPointManager::validateControlPoints() const
{
    // 检查最小点数要求
    if (!isMinimumPointsMet()) {
        return false;
    }
    
    // 检查是否有重复点
    for (size_t i = 0; i < m_controlPoints.size(); ++i) {
        for (size_t j = i + 1; j < m_controlPoints.size(); ++j) {
            glm::vec3 diff = m_controlPoints[i].position - m_controlPoints[j].position;
            if (glm::length(diff) < 1e-6f) {
                return false; // 有重复点
            }
        }
    }
    
    return true;
}

bool GeoControlPointManager::isMinimumPointsMet() const
{
    return static_cast<int>(m_controlPoints.size()) >= m_minimumPointsRequired;
}

int GeoControlPointManager::getMinimumPointsRequired() const
{
    return m_minimumPointsRequired;
}

void GeoControlPointManager::setMinimumPointsRequired(int count)
{
    m_minimumPointsRequired = std::max(0, count);
}

void GeoControlPointManager::startPreview()
{
    if (!m_previewActive) {
        m_previewActive = true;
        emit previewStarted();
    }
}

void GeoControlPointManager::stopPreview()
{
    if (m_previewActive) {
        m_previewActive = false;
        emit previewStopped();
    }
}

void GeoControlPointManager::setControlPointsVisible(bool visible)
{
    if (m_controlPointsVisible != visible) {
        m_controlPointsVisible = visible;
        updateControlPointVisualization();
        emit visibilityChanged(visible);
    }
}

void GeoControlPointManager::setControlPointSize(float size)
{
    if (m_controlPointSize != size) {
        m_controlPointSize = std::max(0.01f, size);
        updateControlPointVisualization();
    }
}

void GeoControlPointManager::setControlPointColor(const Color3D& color)
{
    if (m_controlPointColor.r != color.r || 
        m_controlPointColor.g != color.g || 
        m_controlPointColor.b != color.b || 
        m_controlPointColor.a != color.a) {
        m_controlPointColor = color;
        updateControlPointVisualization();
    }
}

void GeoControlPointManager::validateIndex(int index) const
{
    if (!isValidIndex(index)) {
        throw std::out_of_range("Control point index out of range: " + std::to_string(index));
    }
}

void GeoControlPointManager::notifyGeometryChanged()
{
    // 通知父对象几何体需要更新
    if (m_parent) {
        m_parent->markGeometryDirty();
    }
}

void GeoControlPointManager::updateControlPointVisualization()
{
    // 这里会在节点管理器中实现具体的可视化更新
    // 现在只是占位符，实际实现会调用节点管理器的方法
    if (m_parent && m_parent->mm_node()) {
        m_parent->mm_node()->updateControlPointsVisualization();
    }
}

// ==================== 临时点管理 ====================

void GeoControlPointManager::setTempPoint(const Point3D& point)
{
    if (m_tempPoint.position != point.position) {
        m_tempPoint = point;
        notifyGeometryChanged();
        updateControlPointVisualization();
        emit controlPointsChanged();
    }
}

void GeoControlPointManager::clearTempPoint()
{
    if (m_tempPoint.position != glm::vec3(0)) {
        m_tempPoint = Point3D(glm::vec3(0));
        notifyGeometryChanged();
        updateControlPointVisualization();
        emit controlPointsChanged();
    }
} 