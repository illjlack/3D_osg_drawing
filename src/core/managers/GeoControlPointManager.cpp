#include "GeoControlPointManager.h"
#include "../GeometryBase.h"
#include <stdexcept>
#include <algorithm>

GeoControlPointManager::GeoControlPointManager(Geo3D* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_currentStage(0)
{
    // 默认初始化一个阶段
    m_stageControlPoints.resize(1);
    m_stageTempPoints.resize(1, Point3D(glm::vec3(0)));
}

// ==================== 多阶段管理接口实现 ====================

void GeoControlPointManager::setStageDescriptors(const std::vector<StageDescriptor>& descriptors)
{
    m_stageDescriptors = descriptors;
    
    // 调整数据结构大小
    m_stageControlPoints.resize(descriptors.size());
    m_stageTempPoints.resize(descriptors.size(), Point3D(glm::vec3(0)));
    m_tempStageControlPointsList.resize(descriptors.size());
    
    // 重置当前阶段
    m_currentStage = 0;
}

const std::vector<StageDescriptor>& GeoControlPointManager::getStageDescriptors() const
{
    return m_stageDescriptors;
}

int GeoControlPointManager::getCurrentStage() const
{
    return m_currentStage;
}

bool GeoControlPointManager::nextStage()
{
    if (!canAdvanceToNextStage()) {
        return false;
    }
    
    m_currentStage++;
    emit stageChanged(m_currentStage);
    
    // 检查是否所有阶段都完成
    if (isAllStagesComplete()) {
        emit allStagesCompleted();
    }
    
    notifyGeometryChanged();
    return true;
}

bool GeoControlPointManager::canAdvanceToNextStage() const
{
    // 检查当前阶段是否完成
    if (!isCurrentStageComplete()) {
        return false;
    }
    
    // 检查是否还有下一个阶段
    return m_currentStage < static_cast<int>(m_stageDescriptors.size()) - 1;
}

bool GeoControlPointManager::isCurrentStageComplete() const
{
    if (m_stageDescriptors.empty() || m_currentStage >= static_cast<int>(m_stageDescriptors.size())) {
        return false;
    }
    
    const StageDescriptor& desc = m_stageDescriptors[m_currentStage];
    int currentCount = getStageControlPointCount(m_currentStage);
    
    // 检查是否满足最小控制点数量
    if (currentCount < desc.minControlPoints) {
        return false;
    }
    
    // 如果有最大限制，检查是否达到最大数量
    if (desc.maxControlPoints > 0 && currentCount >= desc.maxControlPoints) {
        return true;
    }
    
    // 如果没有最大限制，当前阶段永远不会自动完成，需要手动切换
    return currentCount >= desc.minControlPoints;
}

bool GeoControlPointManager::isAllStagesComplete() const
{
    if (m_stageDescriptors.empty()) {
        return false;
    }
    
    // 检查所有阶段是否都满足最小控制点要求
    for (int i = 0; i < static_cast<int>(m_stageDescriptors.size()); ++i) {
        const StageDescriptor& desc = m_stageDescriptors[i];
        int stageCount = getStageControlPointCount(i);
        if (stageCount < desc.minControlPoints) {
            return false;
        }
    }
    
    return true;
}

const StageDescriptor* GeoControlPointManager::getCurrentStageDescriptor() const
{
    if (m_stageDescriptors.empty() || m_currentStage >= static_cast<int>(m_stageDescriptors.size())) {
        return nullptr;
    }
    
    return &m_stageDescriptors[m_currentStage];
}

// ==================== 控制点访问接口实现 ====================

const std::vector<Point3D>& GeoControlPointManager::getStageControlPoints(int stage) const
{
    validateStageIndex(stage);
    
    // 如果绘制未完成且有临时点，返回包含临时点的控制点列表
    if (!isDrawingComplete() && hasCurrentStageTempPoint() && stage == m_currentStage) {
        m_tempStageControlPointsList[stage] = m_stageControlPoints[stage];
        m_tempStageControlPointsList[stage].push_back(m_stageTempPoints[stage]);
        return m_tempStageControlPointsList[stage];
    }
    
    return m_stageControlPoints[stage];
}

const std::vector<Point3D>& GeoControlPointManager::getCurrentStageControlPoints() const
{
    return getStageControlPoints(m_currentStage);
}

const std::vector<std::vector<Point3D>>& GeoControlPointManager::getAllStageControlPoints() const
{
    return m_stageControlPoints;
}

Point3D GeoControlPointManager::getStageControlPoint(int stage, int index) const
{
    validateStageIndex(stage);
    
    // 如果是当前阶段，且绘制未完成，且索引是最后一个，可能是临时点
    if (stage == m_currentStage && !isDrawingComplete() && hasCurrentStageTempPoint() && 
        index == static_cast<int>(m_stageControlPoints[stage].size())) {
        return m_stageTempPoints[stage];
    }
    
    validateControlPointIndex(stage, index);
    return m_stageControlPoints[stage][index];
}

int GeoControlPointManager::getCurrentStageControlPointCount() const
{
    return getStageControlPointCount(m_currentStage);
}

int GeoControlPointManager::getStageControlPointCount(int stage) const
{
    validateStageIndex(stage);
    return static_cast<int>(m_stageControlPoints[stage].size());
}

// ==================== 控制点操作接口实现 ====================

bool GeoControlPointManager::addControlPointToCurrentStage(const Point3D& point)
{
    if (m_stageDescriptors.empty()) {
        return false;
    }
    
    const StageDescriptor& desc = m_stageDescriptors[m_currentStage];
    int currentCount = getCurrentStageControlPointCount();
    
    // 检查是否超过最大限制
    if (desc.maxControlPoints > 0 && currentCount >= desc.maxControlPoints) {
        return false;
    }
    
    m_stageControlPoints[m_currentStage].push_back(point);
    
    // 检查当前阶段是否完成
    if (isCurrentStageComplete()) {
        emit stageCompleted(m_currentStage);
        
        // 如果达到最大控制点数，自动进入下一阶段
        if (desc.maxControlPoints > 0 && getCurrentStageControlPointCount() >= desc.maxControlPoints) {
            nextStage();
        }
    }
    
    notifyGeometryChanged();
    return true;
}

bool GeoControlPointManager::setStageControlPoint(int stage, int index, const Point3D& point)
{
    validateStageIndex(stage);
    validateControlPointIndex(stage, index);
    
    m_stageControlPoints[stage][index] = point;
    notifyGeometryChanged();
    return true;
}

bool GeoControlPointManager::removeLastControlPointFromCurrentStage()
{
    if (m_stageControlPoints[m_currentStage].empty()) {
        return false;
    }
    
    m_stageControlPoints[m_currentStage].pop_back();
    notifyGeometryChanged();
    return true;
}

void GeoControlPointManager::clearAllControlPoints()
{
    for (auto& stagePoints : m_stageControlPoints) {
        stagePoints.clear();
    }
    
    for (auto& tempPoint : m_stageTempPoints) {
        tempPoint = Point3D(glm::vec3(0));
    }
    
    m_currentStage = 0;
    notifyGeometryChanged();
}

void GeoControlPointManager::clearCurrentStageControlPoints()
{
    if (!m_stageControlPoints.empty() && m_currentStage < static_cast<int>(m_stageControlPoints.size())) {
        m_stageControlPoints[m_currentStage].clear();
        clearCurrentStageTempPoint();
        notifyGeometryChanged();
    }
}

// ==================== 临时点管理接口实现 ====================

void GeoControlPointManager::setCurrentStageTempPoint(const Point3D& point)
{
    if (m_currentStage < static_cast<int>(m_stageTempPoints.size())) {
        if (m_stageTempPoints[m_currentStage].position != point.position) {
            m_stageTempPoints[m_currentStage] = point;
            notifyGeometryChanged();
        }
    }
}

void GeoControlPointManager::clearCurrentStageTempPoint()
{
    if (m_currentStage < static_cast<int>(m_stageTempPoints.size())) {
        if (m_stageTempPoints[m_currentStage].position != glm::vec3(0)) {
            m_stageTempPoints[m_currentStage] = Point3D(glm::vec3(0));
            notifyGeometryChanged();
        }
    }
}

bool GeoControlPointManager::hasCurrentStageTempPoint() const
{
    if (m_currentStage >= static_cast<int>(m_stageTempPoints.size())) {
        return false;
    }
    
    return m_stageTempPoints[m_currentStage].position != glm::vec3(0);
}

// ==================== 兼容性接口实现 ====================

const std::vector<Point3D>& GeoControlPointManager::getControlPoints() const
{
    // 将所有阶段的控制点扁平化为一个数组
    m_flattenedControlPoints.clear();
    
    for (int stage = 0; stage < static_cast<int>(m_stageControlPoints.size()); ++stage) {
        const auto& stagePoints = getStageControlPoints(stage); // 这会包含临时点
        m_flattenedControlPoints.insert(m_flattenedControlPoints.end(), 
                                       stagePoints.begin(), stagePoints.end());
    }
    
    return m_flattenedControlPoints;
}

Point3D GeoControlPointManager::getControlPoint(int index) const
{
    const auto& allPoints = getControlPoints();
    if (index < 0 || index >= static_cast<int>(allPoints.size())) {
        throw std::out_of_range("Control point index out of range: " + std::to_string(index));
    }
    return allPoints[index];
}

int GeoControlPointManager::getControlPointCountWithoutTempPoint() const
{
    int totalCount = 0;
    for (const auto& stagePoints : m_stageControlPoints) {
        totalCount += static_cast<int>(stagePoints.size());
    }
    return totalCount;
}

bool GeoControlPointManager::hasControlPoints() const
{
    for (const auto& stagePoints : m_stageControlPoints) {
        if (!stagePoints.empty()) {
            return true;
        }
    }
    
    // 检查是否有临时点
    return hasCurrentStageTempPoint();
}

void GeoControlPointManager::addControlPoint(const Point3D& point)
{
    addControlPointToCurrentStage(point);
}

void GeoControlPointManager::setControlPoint(int index, const Point3D& point)
{
    // 兼容性实现：将索引转换为阶段和阶段内索引
    int currentIndex = 0;
    for (int stage = 0; stage < static_cast<int>(m_stageControlPoints.size()); ++stage) {
        int stageSize = static_cast<int>(m_stageControlPoints[stage].size());
        if (index < currentIndex + stageSize) {
            setStageControlPoint(stage, index - currentIndex, point);
            return;
        }
        currentIndex += stageSize;
    }
    
    throw std::out_of_range("Control point index out of range: " + std::to_string(index));
}

void GeoControlPointManager::removeControlPoint(int index)
{
    // 简化实现：只支持移除当前阶段的最后一个控制点
    if (index == getControlPointCountWithoutTempPoint() - 1) {
        removeLastControlPointFromCurrentStage();
    }
}

void GeoControlPointManager::clearControlPoints()
{
    clearAllControlPoints();
}

void GeoControlPointManager::setTempPoint(const Point3D& point)
{
    setCurrentStageTempPoint(point);
}

void GeoControlPointManager::clearTempPoint()
{
    clearCurrentStageTempPoint();
}

// ==================== 查询和验证接口实现 ====================

int GeoControlPointManager::findNearestControlPoint(const Point3D& point, float threshold) const
{
    int globalIndex = -1;
    float minDistance = threshold;
    int currentIndex = 0;
    
    // 搜索所有阶段的控制点
    for (int stage = 0; stage < static_cast<int>(m_stageControlPoints.size()); ++stage) {
        const auto& stagePoints = m_stageControlPoints[stage];
        
        for (int i = 0; i < static_cast<int>(stagePoints.size()); ++i) {
            glm::vec3 diff = stagePoints[i].position - point.position;
            float distance = glm::length(diff);
            
            if (distance < minDistance) {
                minDistance = distance;
                globalIndex = currentIndex + i;
            }
        }
        currentIndex += static_cast<int>(stagePoints.size());
    }
    
    // 检查当前阶段的临时点
    if (hasCurrentStageTempPoint()) {
        glm::vec3 diff = m_stageTempPoints[m_currentStage].position - point.position;
        float distance = glm::length(diff);
        
        if (distance < minDistance) {
            minDistance = distance;
            globalIndex = currentIndex;
        }
    }
    
    return globalIndex;
}

bool GeoControlPointManager::isValidStageIndex(int stage) const
{
    return stage >= 0 && stage < static_cast<int>(m_stageControlPoints.size());
}

bool GeoControlPointManager::isValidControlPointIndex(int stage, int index) const
{
    if (!isValidStageIndex(stage)) {
        return false;
    }
    
    int maxIndex = static_cast<int>(m_stageControlPoints[stage].size());
    
    // 如果是当前阶段且有临时点，最大索引加1
    if (stage == m_currentStage && hasCurrentStageTempPoint()) {
        maxIndex++;
    }
    
    return index >= 0 && index < maxIndex;
}

void GeoControlPointManager::notifyGeometryChanged()
{
    // 通知父对象几何体需要更新
    if (m_parent && m_parent->mm_state()) {
        m_parent->mm_node()->updateGeometries();
    }
}

// ==================== 私有方法实现 ====================

void GeoControlPointManager::validateStageIndex(int stage) const
{
    if (!isValidStageIndex(stage)) {
        throw std::out_of_range("Stage index out of range: " + std::to_string(stage));
    }
}

void GeoControlPointManager::validateControlPointIndex(int stage, int index) const
{
    if (!isValidControlPointIndex(stage, index)) {
        throw std::out_of_range("Control point index out of range: stage=" + 
                               std::to_string(stage) + ", index=" + std::to_string(index));
    }
}

bool GeoControlPointManager::isDrawingComplete() const
{
    // 检查绘制是否完成状态
    return m_parent && m_parent->mm_state() && m_parent->mm_state()->isStateComplete();
} 