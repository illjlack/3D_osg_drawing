#include "GeoSnapPointManager.h"
#include "../GeometryBase.h"
#include <algorithm>
#include <cmath>
#include <osg/Geometry>
#include <osg/Array>
#include <osg/Geode>

GeoSnapPointManager::GeoSnapPointManager(Geo3D* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_snapThreshold(0.15f)
    , m_snapEnabled(true)
    , m_snapPointsVisible(false)
    , m_snapPointSize(0.05f)
    , m_snapPointColor(1.0f, 1.0f, 0.0f, 1.0f) // 默认黄色
    , m_autoUpdateEnabled(true)
    , m_needsUpdate(true)
{
    initializeSnapSettings();
}

void GeoSnapPointManager::initializeSnapSettings()
{
    // 初始化所有捕捉点类型为启用状态
    m_snapTypeEnabled[SNAP_CONTROL_POINT] = true;
    m_snapTypeEnabled[SNAP_VERTEX] = true;
    m_snapTypeEnabled[SNAP_EDGE_ENDPOINT] = true;
    m_snapTypeEnabled[SNAP_EDGE_MIDPOINT] = true;
    m_snapTypeEnabled[SNAP_EDGE_QUARTER] = false;  // 默认关闭1/4点
    m_snapTypeEnabled[SNAP_FACE_CENTER] = true;
    m_snapTypeEnabled[SNAP_BOUNDING_BOX] = false;  // 默认关闭包围盒点
    m_snapTypeEnabled[SNAP_CUSTOM] = true;
}

void GeoSnapPointManager::addSnapPoint(const glm::vec3& position, SnapPointType type, int featureIndex)
{
    SnapPoint snapPoint(position, type, featureIndex);
    addSnapPoint(snapPoint);
}

void GeoSnapPointManager::addSnapPoint(const SnapPoint& snapPoint)
{
    if (!isSnapTypeActive(snapPoint.type)) {
        return;
    }
    
    int index = static_cast<int>(m_snapPoints.size());
    m_snapPoints.push_back(snapPoint);
    updateSnapPositions();
    
    emit snapPointAdded(index, snapPoint);
}

void GeoSnapPointManager::removeSnapPoint(int index)
{
    if (index >= 0 && index < static_cast<int>(m_snapPoints.size())) {
        m_snapPoints.erase(m_snapPoints.begin() + index);
        updateSnapPositions();
        emit snapPointRemoved(index);
    }
}

void GeoSnapPointManager::clearSnapPoints()
{
    if (!m_snapPoints.empty()) {
        m_snapPoints.clear();
        m_snapPositions.clear();
        emit snapPointsCleared();
    }
}

void GeoSnapPointManager::clearSnapPointsByType(SnapPointType type)
{
    auto it = std::remove_if(m_snapPoints.begin(), m_snapPoints.end(),
                            [type](const SnapPoint& point) { return point.type == type; });
    
    if (it != m_snapPoints.end()) {
        m_snapPoints.erase(it, m_snapPoints.end());
        updateSnapPositions();
        emit snapPointsUpdated();
    }
}

void GeoSnapPointManager::updateSnapPoints()
{
    if (!m_autoUpdateEnabled || !m_parent) {
        return;
    }
    
    // 清除旧的自动生成的捕捉点（保留手动添加的）
    auto it = std::remove_if(m_snapPoints.begin(), m_snapPoints.end(),
                            [](const SnapPoint& point) { return point.type != SNAP_CUSTOM; });
    m_snapPoints.erase(it, m_snapPoints.end());
    
    // 生成各种类型的捕捉点
    generateControlPointSnaps();
    generateVertexSnaps();
    generateEdgeSnaps();
    generateFaceSnaps();
    generateBoundingBoxSnaps();
    
    // 移除重复点
    removeDuplicateSnapPoints();
    
    // 按优先级排序
    sortSnapPointsByPriority();
    
    updateSnapPositions();
    m_needsUpdate = false;
    
    emit snapPointsUpdated();
}

void GeoSnapPointManager::generateControlPointSnaps()
{
    if (!isSnapTypeActive(SNAP_CONTROL_POINT) || !m_parent) {
        return;
    }
    
    auto* controlManager = m_parent->getControlPointManager();
    if (controlManager) {
        const auto& controlPoints = controlManager->getControlPoints();
        for (int i = 0; i < static_cast<int>(controlPoints.size()); ++i) {
            SnapPoint snapPoint(controlPoints[i].position, SNAP_CONTROL_POINT, i, 0.1f);
            m_snapPoints.push_back(snapPoint);
        }
    }
}

void GeoSnapPointManager::generateVertexSnaps()
{
    if (!isSnapTypeActive(SNAP_VERTEX) || !m_parent) {
        return;
    }
    
    generateSnapPointsFromGeometry();
}

void GeoSnapPointManager::generateEdgeSnaps()
{
    if (!isSnapTypeActive(SNAP_EDGE_ENDPOINT) && !isSnapTypeActive(SNAP_EDGE_MIDPOINT)) {
        return;
    }
    
    if (!m_parent) return;
    
    auto* nodeManager = m_parent->getNodeManager();
    if (!nodeManager) return;
    
    auto edgeNode = nodeManager->getEdgeNode();
    if (!edgeNode.valid()) return;
    
    // 遍历边节点获取顶点数据
    for (unsigned int i = 0; i < edgeNode->getNumChildren(); ++i) {
        auto child = edgeNode->getChild(i);
        if (auto geode = dynamic_cast<osg::Geode*>(child)) {
            for (unsigned int j = 0; j < geode->getNumDrawables(); ++j) {
                if (auto geometry = dynamic_cast<osg::Geometry*>(geode->getDrawable(j))) {
                    if (auto vertices = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray())) {
                        std::vector<glm::vec3> edgeVertices;
                        for (unsigned int k = 0; k < vertices->size(); ++k) {
                            const osg::Vec3& vertex = (*vertices)[k];
                            edgeVertices.push_back(glm::vec3(vertex.x(), vertex.y(), vertex.z()));
                        }
                        addEdgeSnapPoints(edgeVertices);
                    }
                }
            }
        }
    }
}

void GeoSnapPointManager::generateFaceSnaps()
{
    if (!isSnapTypeActive(SNAP_FACE_CENTER) || !m_parent) {
        return;
    }
    
    auto* nodeManager = m_parent->getNodeManager();
    if (!nodeManager) return;
    
    auto faceNode = nodeManager->getFaceNode();
    if (!faceNode.valid()) return;
    
    // 遍历面节点获取顶点数据
    for (unsigned int i = 0; i < faceNode->getNumChildren(); ++i) {
        auto child = faceNode->getChild(i);
        if (auto geode = dynamic_cast<osg::Geode*>(child)) {
            for (unsigned int j = 0; j < geode->getNumDrawables(); ++j) {
                if (auto geometry = dynamic_cast<osg::Geometry*>(geode->getDrawable(j))) {
                    if (auto vertices = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray())) {
                        std::vector<glm::vec3> faceVertices;
                        for (unsigned int k = 0; k < vertices->size(); ++k) {
                            const osg::Vec3& vertex = (*vertices)[k];
                            faceVertices.push_back(glm::vec3(vertex.x(), vertex.y(), vertex.z()));
                        }
                        addFaceSnapPoints(faceVertices);
                    }
                }
            }
        }
    }
}

void GeoSnapPointManager::generateBoundingBoxSnaps()
{
    if (!isSnapTypeActive(SNAP_BOUNDING_BOX) || !m_parent) {
        return;
    }
    
    auto* boundingBoxManager = m_parent->getBoundingBoxManager();
    if (boundingBoxManager && boundingBoxManager->isValid()) {
        const auto& bbox = boundingBoxManager->getBoundingBox();
        auto corners = boundingBoxManager->getCorners();
        
        for (int i = 0; i < static_cast<int>(corners.size()); ++i) {
            SnapPoint snapPoint(corners[i], SNAP_BOUNDING_BOX, i, 0.8f);
            m_snapPoints.push_back(snapPoint);
        }
        
        // 添加包围盒中心点
        glm::vec3 center = boundingBoxManager->getCenter();
        SnapPoint centerPoint(center, SNAP_BOUNDING_BOX, -1, 0.7f);
        m_snapPoints.push_back(centerPoint);
    }
}

glm::vec3 GeoSnapPointManager::findNearestSnapPoint(const glm::vec3& position, float threshold) const
{
    if (!m_snapEnabled || m_snapPoints.empty()) {
        return position;
    }
    
    int index = findNearestSnapPointIndex(position, threshold);
    if (index >= 0) {
        return m_snapPoints[index].position;
    }
    
    return position;
}

int GeoSnapPointManager::findNearestSnapPointIndex(const glm::vec3& position, float threshold) const
{
    if (!m_snapEnabled || m_snapPoints.empty()) {
        return -1;
    }
    
    int nearestIndex = -1;
    float minDistance = threshold;
    
    for (int i = 0; i < static_cast<int>(m_snapPoints.size()); ++i) {
        if (!m_snapPoints[i].enabled) continue;
        
        glm::vec3 diff = m_snapPoints[i].position - position;
        float distance = glm::length(diff);
        
        if (distance < minDistance) {
            minDistance = distance;
            nearestIndex = i;
        }
    }
    
    return nearestIndex;
}

bool GeoSnapPointManager::hasSnapPointNear(const glm::vec3& position, float threshold) const
{
    return findNearestSnapPointIndex(position, threshold) >= 0;
}

std::vector<int> GeoSnapPointManager::findSnapPointsInRange(const glm::vec3& position, float range) const
{
    std::vector<int> indices;
    
    for (int i = 0; i < static_cast<int>(m_snapPoints.size()); ++i) {
        if (!m_snapPoints[i].enabled) continue;
        
        glm::vec3 diff = m_snapPoints[i].position - position;
        float distance = glm::length(diff);
        
        if (distance <= range) {
            indices.push_back(i);
        }
    }
    
    return indices;
}

void GeoSnapPointManager::setSnapThreshold(float threshold)
{
    if (m_snapThreshold != threshold) {
        m_snapThreshold = std::max(0.001f, threshold);
        emit snapThresholdChanged(m_snapThreshold);
    }
}

void GeoSnapPointManager::setSnapEnabled(bool enabled)
{
    if (m_snapEnabled != enabled) {
        m_snapEnabled = enabled;
        emit snapEnabledChanged(enabled);
    }
}

void GeoSnapPointManager::setSnapTypeEnabled(SnapPointType type, bool enabled)
{
    if (m_snapTypeEnabled[type] != enabled) {
        m_snapTypeEnabled[type] = enabled;
        
        if (m_autoUpdateEnabled) {
            updateSnapPoints();
        }
        
        emit snapTypeEnabledChanged(type, enabled);
    }
}

bool GeoSnapPointManager::isSnapTypeEnabled(SnapPointType type) const
{
    auto it = m_snapTypeEnabled.find(type);
    return it != m_snapTypeEnabled.end() ? it->second : false;
}

void GeoSnapPointManager::enableAllSnapTypes()
{
    for (auto& pair : m_snapTypeEnabled) {
        pair.second = true;
    }
    
    if (m_autoUpdateEnabled) {
        updateSnapPoints();
    }
}

void GeoSnapPointManager::disableAllSnapTypes()
{
    for (auto& pair : m_snapTypeEnabled) {
        pair.second = false;
    }
    
    clearSnapPoints();
}

void GeoSnapPointManager::setSnapPointPriority(int index, float priority)
{
    if (index >= 0 && index < static_cast<int>(m_snapPoints.size())) {
        m_snapPoints[index].priority = priority;
        sortSnapPointsByPriority();
    }
}

float GeoSnapPointManager::getSnapPointPriority(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_snapPoints.size())) {
        return m_snapPoints[index].priority;
    }
    return 1.0f;
}

void GeoSnapPointManager::sortSnapPointsByPriority()
{
    std::sort(m_snapPoints.begin(), m_snapPoints.end(),
             [](const SnapPoint& a, const SnapPoint& b) {
                 return a.priority < b.priority;
             });
    updateSnapPositions();
}

void GeoSnapPointManager::setSnapPointsVisible(bool visible)
{
    if (m_snapPointsVisible != visible) {
        m_snapPointsVisible = visible;
        updateSnapPointVisualization();
        emit snapPointsVisibilityChanged(visible);
    }
}

void GeoSnapPointManager::setSnapPointSize(float size)
{
    if (m_snapPointSize != size) {
        m_snapPointSize = std::max(0.01f, size);
        updateSnapPointVisualization();
    }
}

void GeoSnapPointManager::setSnapPointColor(const Color3D& color)
{
    if (m_snapPointColor.r != color.r || 
        m_snapPointColor.g != color.g || 
        m_snapPointColor.b != color.b || 
        m_snapPointColor.a != color.a) {
        m_snapPointColor = color;
        updateSnapPointVisualization();
    }
}

bool GeoSnapPointManager::validateSnapPoints() const
{
    // 检查捕捉点是否有效
    for (const auto& point : m_snapPoints) {
        if (std::isnan(point.position.x) || 
            std::isnan(point.position.y) || 
            std::isnan(point.position.z)) {
            return false;
        }
    }
    return true;
}

void GeoSnapPointManager::removeInvalidSnapPoints()
{
    auto it = std::remove_if(m_snapPoints.begin(), m_snapPoints.end(),
                            [](const SnapPoint& point) {
                                return std::isnan(point.position.x) || 
                                       std::isnan(point.position.y) || 
                                       std::isnan(point.position.z);
                            });
    
    if (it != m_snapPoints.end()) {
        m_snapPoints.erase(it, m_snapPoints.end());
        updateSnapPositions();
        emit snapPointsUpdated();
    }
}

void GeoSnapPointManager::removeDuplicateSnapPoints(float tolerance)
{
    if (m_snapPoints.size() <= 1) return;
    
    std::vector<SnapPoint> uniquePoints;
    
    for (const auto& point : m_snapPoints) {
        bool isDuplicate = false;
        for (const auto& unique : uniquePoints) {
            glm::vec3 diff = point.position - unique.position;
            if (glm::length(diff) < tolerance) {
                isDuplicate = true;
                break;
            }
        }
        
        if (!isDuplicate) {
            uniquePoints.push_back(point);
        }
    }
    
    if (uniquePoints.size() != m_snapPoints.size()) {
        m_snapPoints = std::move(uniquePoints);
        updateSnapPositions();
        emit snapPointsUpdated();
    }
}

int GeoSnapPointManager::getSnapPointCountByType(SnapPointType type) const
{
    return static_cast<int>(std::count_if(m_snapPoints.begin(), m_snapPoints.end(),
                                         [type](const SnapPoint& point) { return point.type == type; }));
}

std::vector<GeoSnapPointManager::SnapPointType> GeoSnapPointManager::getActiveSnapTypes() const
{
    std::vector<SnapPointType> activeTypes;
    for (const auto& pair : m_snapTypeEnabled) {
        if (pair.second) {
            activeTypes.push_back(pair.first);
        }
    }
    return activeTypes;
}

void GeoSnapPointManager::transformSnapPoints(const glm::mat4& matrix)
{
    for (auto& point : m_snapPoints) {
        glm::vec4 pos = glm::vec4(point.position, 1.0f);
        pos = matrix * pos;
        point.position = glm::vec3(pos) / pos.w;
    }
    
    updateSnapPositions();
    emit snapPointsUpdated();
}

void GeoSnapPointManager::translateSnapPoints(const glm::vec3& offset)
{
    for (auto& point : m_snapPoints) {
        point.position += offset;
    }
    
    updateSnapPositions();
    emit snapPointsUpdated();
}

void GeoSnapPointManager::updateSnapPositions()
{
    m_snapPositions.clear();
    m_snapPositions.reserve(m_snapPoints.size());
    
    for (const auto& point : m_snapPoints) {
        m_snapPositions.push_back(point.position);
    }
}

void GeoSnapPointManager::generateSnapPointsFromGeometry()
{
    if (!m_parent) return;
    
    auto* nodeManager = m_parent->getNodeManager();
    if (!nodeManager) return;
    
    auto vertexNode = nodeManager->getVertexNode();
    if (!vertexNode.valid()) return;
    
    // 从顶点节点提取捕捉点
    for (unsigned int i = 0; i < vertexNode->getNumChildren(); ++i) {
        auto child = vertexNode->getChild(i);
        if (auto geode = dynamic_cast<osg::Geode*>(child)) {
            for (unsigned int j = 0; j < geode->getNumDrawables(); ++j) {
                if (auto geometry = dynamic_cast<osg::Geometry*>(geode->getDrawable(j))) {
                    if (auto vertices = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray())) {
                        for (unsigned int k = 0; k < vertices->size(); ++k) {
                            const osg::Vec3& vertex = (*vertices)[k];
                            SnapPoint snapPoint(glm::vec3(vertex.x(), vertex.y(), vertex.z()), 
                                              SNAP_VERTEX, k, 0.2f);
                            m_snapPoints.push_back(snapPoint);
                        }
                    }
                }
            }
        }
    }
}

void GeoSnapPointManager::addEdgeSnapPoints(const std::vector<glm::vec3>& vertices)
{
    for (size_t i = 0; i < vertices.size() - 1; ++i) {
        const glm::vec3& v1 = vertices[i];
        const glm::vec3& v2 = vertices[i + 1];
        
        // 端点
        if (isSnapTypeActive(SNAP_EDGE_ENDPOINT)) {
            SnapPoint endpoint1(v1, SNAP_EDGE_ENDPOINT, static_cast<int>(i * 2), 0.2f);
            SnapPoint endpoint2(v2, SNAP_EDGE_ENDPOINT, static_cast<int>(i * 2 + 1), 0.2f);
            m_snapPoints.push_back(endpoint1);
            m_snapPoints.push_back(endpoint2);
        }
        
        // 中点
        if (isSnapTypeActive(SNAP_EDGE_MIDPOINT)) {
            glm::vec3 midpoint = (v1 + v2) * 0.5f;
            SnapPoint midSnapPoint(midpoint, SNAP_EDGE_MIDPOINT, static_cast<int>(i), 0.3f);
            m_snapPoints.push_back(midSnapPoint);
        }
        
        // 1/4和3/4点
        if (isSnapTypeActive(SNAP_EDGE_QUARTER)) {
            glm::vec3 quarter1 = v1 * 0.75f + v2 * 0.25f;
            glm::vec3 quarter3 = v1 * 0.25f + v2 * 0.75f;
            SnapPoint quarter1Point(quarter1, SNAP_EDGE_QUARTER, static_cast<int>(i * 2), 0.4f);
            SnapPoint quarter3Point(quarter3, SNAP_EDGE_QUARTER, static_cast<int>(i * 2 + 1), 0.4f);
            m_snapPoints.push_back(quarter1Point);
            m_snapPoints.push_back(quarter3Point);
        }
    }
}

void GeoSnapPointManager::addFaceSnapPoints(const std::vector<glm::vec3>& vertices)
{
    if (vertices.size() >= 3) {
        // 计算面的质心
        glm::vec3 centroid(0.0f);
        for (const auto& vertex : vertices) {
            centroid += vertex;
        }
        centroid /= static_cast<float>(vertices.size());
        
        SnapPoint centerPoint(centroid, SNAP_FACE_CENTER, 0, 0.3f);
        m_snapPoints.push_back(centerPoint);
    }
}

bool GeoSnapPointManager::isSnapTypeActive(SnapPointType type) const
{
    auto it = m_snapTypeEnabled.find(type);
    return m_snapEnabled && (it != m_snapTypeEnabled.end() ? it->second : false);
}

void GeoSnapPointManager::updateSnapPointVisualization()
{
    // 这个方法会在需要时由节点管理器调用
    // 现在只是占位符
    if (m_parent && m_parent->getNodeManager()) {
        // 可以在这里添加捕捉点的可视化代码
    }
} 