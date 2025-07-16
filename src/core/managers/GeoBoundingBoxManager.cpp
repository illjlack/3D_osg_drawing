#include "GeoBoundingBoxManager.h"
#include "../GeometryBase.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

GeoBoundingBoxManager::GeoBoundingBoxManager(Geo3D* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_autoUpdate(true)
    , m_updateMode(3) // 默认全部更新
    , m_dirty(true)
    , m_visible(false)
    , m_wireframeColor(1.0f, 0.0f, 1.0f, 1.0f) // 默认紫色
    , m_wireframeWidth(1.0f)
    , m_controlPointsVisible(true)  // 默认显示控制点
    , m_controlPointSize(0.1f)
    , m_controlPointColor(1.0f, 1.0f, 0.0f, 1.0f) // 默认黄色
    , m_centerCached(false)
    , m_sizeCached(false)
    , m_radiusCached(false)
{
    initializeBoundingBox();
}

void GeoBoundingBoxManager::initializeBoundingBox()
{
    m_boundingBox = BoundingBox3D();
    markDirty();
}

void GeoBoundingBoxManager::setBoundingBox(const BoundingBox3D& box)
{
    if (m_boundingBox.min.x != box.min.x || m_boundingBox.min.y != box.min.y || m_boundingBox.min.z != box.min.z ||
        m_boundingBox.max.x != box.max.x || m_boundingBox.max.y != box.max.y || m_boundingBox.max.z != box.max.z) {
        
        m_boundingBox = box;
        
        // 清除缓存
        m_centerCached = false;
        m_sizeCached = false;
        m_radiusCached = false;
        
        emit boundingBoxChanged();
    }
}

void GeoBoundingBoxManager::updateBoundingBox()
{
    if (!m_autoUpdate) return;
    
    switch (m_updateMode) {
        case 0:
            updateFromControlPoints();
            break;
        case 1:
            updateFromGeometry();
            break;
        case 2:
            updateFromChildren();
            break;
        case 3:
        default:
            updateFromControlPoints();
            updateFromGeometry();
            updateFromChildren();
            break;
    }
    
    clearDirty();
    emit boundingBoxUpdated();
}

void GeoBoundingBoxManager::updateFromControlPoints()
{
    if (!m_parent) return;
    
    auto* controlManager = m_parent->mm_controlPoint();
    if (controlManager && controlManager->hasControlPoints()) {
        const auto& controlPoints = controlManager->getControlPoints();
        
        std::vector<glm::vec3> points;
        points.reserve(controlPoints.size());
        for (const auto& cp : controlPoints) {
            points.push_back(cp.position);
        }
        
        calculateBoundingBoxFromPoints(points);
    }
}

void GeoBoundingBoxManager::updateFromGeometry()
{
    if (!m_parent) return;
    
    calculateBoundingBoxFromOSGGeometry();
}

void GeoBoundingBoxManager::updateFromChildren()
{
    // 如果有子对象，这里可以更新子对象的包围盒
    // 当前实现为占位符
}

void GeoBoundingBoxManager::forceUpdate()
{
    markDirty();
    updateBoundingBox();
}

glm::vec3 GeoBoundingBoxManager::getCenter() const
{
    if (!m_centerCached) {
        if (m_boundingBox.isValid()) {
            m_cachedCenter = m_boundingBox.center();
        } else {
            m_cachedCenter = glm::vec3(0.0f);
        }
        m_centerCached = true;
    }
    return m_cachedCenter;
}

glm::vec3 GeoBoundingBoxManager::getSize() const
{
    if (!m_sizeCached) {
        if (m_boundingBox.isValid()) {
            m_cachedSize = m_boundingBox.size();
        } else {
            m_cachedSize = glm::vec3(0.0f);
        }
        m_sizeCached = true;
    }
    return m_cachedSize;
}

float GeoBoundingBoxManager::getRadius() const
{
    if (!m_radiusCached) {
        if (m_boundingBox.isValid()) {
            glm::vec3 size = getSize();
            m_cachedRadius = glm::length(size) * 0.5f;
        } else {
            m_cachedRadius = 0.0f;
        }
        m_radiusCached = true;
    }
    return m_cachedRadius;
}

float GeoBoundingBoxManager::getDiagonal() const
{
    if (m_boundingBox.isValid()) {
        glm::vec3 size = getSize();
        return glm::length(size);
    }
    return 0.0f;
}

void GeoBoundingBoxManager::expand(const glm::vec3& point)
{
    m_boundingBox.expand(point);
    
    // 清除缓存
    m_centerCached = false;
    m_sizeCached = false;
    m_radiusCached = false;
    
    emit boundingBoxChanged();
}

void GeoBoundingBoxManager::expand(const BoundingBox3D& other)
{
    m_boundingBox.expand(other);
    
    // 清除缓存
    m_centerCached = false;
    m_sizeCached = false;
    m_radiusCached = false;
    
    emit boundingBoxChanged();
}

void GeoBoundingBoxManager::expand(float margin)
{
    if (m_boundingBox.isValid() && margin > 0.0f) {
        glm::vec3 offset(margin);
        m_boundingBox.min -= offset;
        m_boundingBox.max += offset;
        
        // 清除缓存
        m_centerCached = false;
        m_sizeCached = false;
        m_radiusCached = false;
        
        emit boundingBoxChanged();
    }
}

void GeoBoundingBoxManager::contract(float margin)
{
    if (m_boundingBox.isValid() && margin > 0.0f) {
        glm::vec3 offset(margin);
        glm::vec3 newMin = m_boundingBox.min + offset;
        glm::vec3 newMax = m_boundingBox.max - offset;
        
        // 确保包围盒仍然有效
        if (newMin.x < newMax.x && newMin.y < newMax.y && newMin.z < newMax.z) {
            m_boundingBox.min = newMin;
            m_boundingBox.max = newMax;
            
            // 清除缓存
            m_centerCached = false;
            m_sizeCached = false;
            m_radiusCached = false;
            
            emit boundingBoxChanged();
        }
    }
}

void GeoBoundingBoxManager::reset()
{
    m_boundingBox = BoundingBox3D();
    
    // 清除缓存
    m_centerCached = false;
    m_sizeCached = false;
    m_radiusCached = false;
    
    emit boundingBoxInvalidated();
}

void GeoBoundingBoxManager::transform(const glm::mat4& matrix)
{
    if (!m_boundingBox.isValid()) return;
    
    // 变换包围盒的8个角点
    std::vector<glm::vec3> corners = getCorners();
    
    // 重置包围盒
    reset();
    
    // 变换每个角点并重新计算包围盒
    for (const auto& corner : corners) {
        glm::vec4 transformedCorner = matrix * glm::vec4(corner, 1.0f);
        glm::vec3 newCorner = glm::vec3(transformedCorner) / transformedCorner.w;
        expand(newCorner);
    }
}

void GeoBoundingBoxManager::translate(const glm::vec3& offset)
{
    if (m_boundingBox.isValid()) {
        m_boundingBox.min += offset;
        m_boundingBox.max += offset;
        
        // 更新缓存的中心点
        if (m_centerCached) {
            m_cachedCenter += offset;
        }
        
        emit boundingBoxChanged();
    }
}

void GeoBoundingBoxManager::scale(const glm::vec3& scale)
{
    if (m_boundingBox.isValid()) {
        glm::vec3 center = getCenter();
        
        // 相对于中心缩放
        m_boundingBox.min = center + (m_boundingBox.min - center) * scale;
        m_boundingBox.max = center + (m_boundingBox.max - center) * scale;
        
        // 清除大小和半径缓存
        m_sizeCached = false;
        m_radiusCached = false;
        
        emit boundingBoxChanged();
    }
}

void GeoBoundingBoxManager::scale(float uniformScale)
{
    scale(glm::vec3(uniformScale));
}

bool GeoBoundingBoxManager::isValid() const
{
    return m_boundingBox.isValid();
}

bool GeoBoundingBoxManager::isEmpty() const
{
    return !isValid();
}

bool GeoBoundingBoxManager::contains(const glm::vec3& point) const
{
    return m_boundingBox.isValid() && 
           point.x >= m_boundingBox.min.x && point.x <= m_boundingBox.max.x &&
           point.y >= m_boundingBox.min.y && point.y <= m_boundingBox.max.y &&
           point.z >= m_boundingBox.min.z && point.z <= m_boundingBox.max.z;
}

bool GeoBoundingBoxManager::contains(const BoundingBox3D& other) const
{
    return m_boundingBox.isValid() && other.isValid() &&
           other.min.x >= m_boundingBox.min.x && other.max.x <= m_boundingBox.max.x &&
           other.min.y >= m_boundingBox.min.y && other.max.y <= m_boundingBox.max.y &&
           other.min.z >= m_boundingBox.min.z && other.max.z <= m_boundingBox.max.z;
}

bool GeoBoundingBoxManager::intersects(const BoundingBox3D& other) const
{
    if (!m_boundingBox.isValid() || !other.isValid()) {
        return false;
    }
    
    return !(m_boundingBox.max.x < other.min.x || m_boundingBox.min.x > other.max.x ||
             m_boundingBox.max.y < other.min.y || m_boundingBox.min.y > other.max.y ||
             m_boundingBox.max.z < other.min.z || m_boundingBox.min.z > other.max.z);
}

bool GeoBoundingBoxManager::intersects(const Ray3D& ray) const
{
    if (!m_boundingBox.isValid()) return false;
    
    // 射线与AABB包围盒的相交测试
    glm::vec3 invDir = 1.0f / ray.direction;
    glm::vec3 t1 = (m_boundingBox.min - ray.origin) * invDir;
    glm::vec3 t2 = (m_boundingBox.max - ray.origin) * invDir;
    
    glm::vec3 tMin = glm::min(t1, t2);
    glm::vec3 tMax = glm::max(t1, t2);
    
    float tNear = glm::max(glm::max(tMin.x, tMin.y), tMin.z);
    float tFar = glm::min(glm::min(tMax.x, tMax.y), tMax.z);
    
    return tNear <= tFar && tFar >= 0;
}

float GeoBoundingBoxManager::distanceToPoint(const glm::vec3& point) const
{
    if (!m_boundingBox.isValid()) return FLT_MAX;
    
    glm::vec3 closest = closestPointTo(point);
    return glm::distance(point, closest);
}

float GeoBoundingBoxManager::distanceToBoundingBox(const BoundingBox3D& other) const
{
    if (!m_boundingBox.isValid() || !other.isValid()) {
        return FLT_MAX;
    }
    
    if (intersects(other)) {
        return 0.0f;
    }
    
    // 计算两个包围盒之间的最短距离
    glm::vec3 delta1 = glm::max(m_boundingBox.min - other.max, glm::vec3(0.0f));
    glm::vec3 delta2 = glm::max(other.min - m_boundingBox.max, glm::vec3(0.0f));
    glm::vec3 delta = delta1 + delta2;
    
    return glm::length(delta);
}

glm::vec3 GeoBoundingBoxManager::closestPointTo(const glm::vec3& point) const
{
    if (!m_boundingBox.isValid()) return point;
    
    return glm::clamp(point, m_boundingBox.min, m_boundingBox.max);
}

std::vector<glm::vec3> GeoBoundingBoxManager::getCorners() const
{
    std::vector<glm::vec3> corners;
    if (!m_boundingBox.isValid()) return corners;
    
    corners.reserve(8);
    glm::vec3 min = m_boundingBox.min;
    glm::vec3 max = m_boundingBox.max;
    
    corners.push_back(glm::vec3(min.x, min.y, min.z));
    corners.push_back(glm::vec3(max.x, min.y, min.z));
    corners.push_back(glm::vec3(min.x, max.y, min.z));
    corners.push_back(glm::vec3(max.x, max.y, min.z));
    corners.push_back(glm::vec3(min.x, min.y, max.z));
    corners.push_back(glm::vec3(max.x, min.y, max.z));
    corners.push_back(glm::vec3(min.x, max.y, max.z));
    corners.push_back(glm::vec3(max.x, max.y, max.z));
    
    return corners;
}

glm::vec3 GeoBoundingBoxManager::getCorner(int index) const
{
    if (index < 0 || index >= 8 || !m_boundingBox.isValid()) {
        return glm::vec3(0.0f);
    }
    
    auto corners = getCorners();
    return corners[index];
}

std::vector<glm::vec3> GeoBoundingBoxManager::getFaceCenter(int faceIndex) const
{
    std::vector<glm::vec3> centers;
    if (!m_boundingBox.isValid() || faceIndex < 0 || faceIndex >= 6) {
        return centers;
    }
    
    glm::vec3 min = m_boundingBox.min;
    glm::vec3 max = m_boundingBox.max;
    glm::vec3 center = getCenter();
    
    switch (faceIndex) {
        case 0: // -X face
            centers.push_back(glm::vec3(min.x, center.y, center.z));
            break;
        case 1: // +X face
            centers.push_back(glm::vec3(max.x, center.y, center.z));
            break;
        case 2: // -Y face
            centers.push_back(glm::vec3(center.x, min.y, center.z));
            break;
        case 3: // +Y face
            centers.push_back(glm::vec3(center.x, max.y, center.z));
            break;
        case 4: // -Z face
            centers.push_back(glm::vec3(center.x, center.y, min.z));
            break;
        case 5: // +Z face
            centers.push_back(glm::vec3(center.x, center.y, max.z));
            break;
    }
    
    return centers;
}

std::vector<glm::vec3> GeoBoundingBoxManager::getFaceNormal(int faceIndex) const
{
    std::vector<glm::vec3> normals;
    if (faceIndex < 0 || faceIndex >= 6) {
        return normals;
    }
    
    switch (faceIndex) {
        case 0: // -X face
            normals.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
            break;
        case 1: // +X face
            normals.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
            break;
        case 2: // -Y face
            normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
            break;
        case 3: // +Y face
            normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        case 4: // -Z face
            normals.push_back(glm::vec3(0.0f, 0.0f, -1.0f));
            break;
        case 5: // +Z face
            normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
            break;
    }
    
    return normals;
}

std::vector<std::pair<glm::vec3, glm::vec3>> GeoBoundingBoxManager::getEdges() const
{
    std::vector<std::pair<glm::vec3, glm::vec3>> edges;
    if (!m_boundingBox.isValid()) return edges;
    
    auto corners = getCorners();
    
    // 12条边
    edges.reserve(12);
    
    // 底面的4条边
    edges.push_back({corners[0], corners[1]});
    edges.push_back({corners[1], corners[3]});
    edges.push_back({corners[3], corners[2]});
    edges.push_back({corners[2], corners[0]});
    
    // 顶面的4条边
    edges.push_back({corners[4], corners[5]});
    edges.push_back({corners[5], corners[7]});
    edges.push_back({corners[7], corners[6]});
    edges.push_back({corners[6], corners[4]});
    
    // 垂直的4条边
    edges.push_back({corners[0], corners[4]});
    edges.push_back({corners[1], corners[5]});
    edges.push_back({corners[2], corners[6]});
    edges.push_back({corners[3], corners[7]});
    
    return edges;
}

std::pair<glm::vec3, glm::vec3> GeoBoundingBoxManager::getEdge(int edgeIndex) const
{
    auto edges = getEdges();
    if (edgeIndex >= 0 && edgeIndex < static_cast<int>(edges.size())) {
        return edges[edgeIndex];
    }
    return {glm::vec3(0.0f), glm::vec3(0.0f)};
}

void GeoBoundingBoxManager::setAutoUpdate(bool enabled)
{
    if (m_autoUpdate != enabled) {
        m_autoUpdate = enabled;
        
        if (enabled && m_dirty) {
            updateBoundingBox();
        }
    }
}

void GeoBoundingBoxManager::setUpdateMode(int mode)
{
    m_updateMode = std::clamp(mode, 0, 3);
}

void GeoBoundingBoxManager::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        updateBoundingBoxVisualization();
        emit visibilityChanged(visible);
    }
}

void GeoBoundingBoxManager::setVisibleForSelection(bool selected)
{
    // 当对象被选中时，显示包围盒和控制点
    setVisible(selected);
    setControlPointsVisible(selected);
}

void GeoBoundingBoxManager::setWireframeColor(const Color3D& color)
{
    if (m_wireframeColor.r != color.r || 
        m_wireframeColor.g != color.g || 
        m_wireframeColor.b != color.b || 
        m_wireframeColor.a != color.a) {
        m_wireframeColor = color;
        updateBoundingBoxVisualization();
        emit colorChanged();
    }
}

void GeoBoundingBoxManager::setWireframeWidth(float width)
{
    if (m_wireframeWidth != width) {
        m_wireframeWidth = std::max(0.1f, width);
        updateBoundingBoxVisualization();
    }
}

float GeoBoundingBoxManager::getVolume() const
{
    if (m_boundingBox.isValid()) {
        glm::vec3 size = getSize();
        return size.x * size.y * size.z;
    }
    return 0.0f;
}

float GeoBoundingBoxManager::getSurfaceArea() const
{
    if (m_boundingBox.isValid()) {
        glm::vec3 size = getSize();
        return 2.0f * (size.x * size.y + size.y * size.z + size.z * size.x);
    }
    return 0.0f;
}

glm::vec3 GeoBoundingBoxManager::getExtent() const
{
    return getSize();
}

float GeoBoundingBoxManager::getAspectRatio() const
{
    if (m_boundingBox.isValid()) {
        glm::vec3 size = getSize();
        float minSize = std::min({size.x, size.y, size.z});
        float maxSize = std::max({size.x, size.y, size.z});
        return minSize > 0.0f ? maxSize / minSize : 1.0f;
    }
    return 1.0f;
}

bool GeoBoundingBoxManager::validateBoundingBox() const
{
    return m_boundingBox.isValid();
}

void GeoBoundingBoxManager::correctBoundingBox()
{
    if (m_boundingBox.min.x > m_boundingBox.max.x) {
        std::swap(m_boundingBox.min.x, m_boundingBox.max.x);
    }
    if (m_boundingBox.min.y > m_boundingBox.max.y) {
        std::swap(m_boundingBox.min.y, m_boundingBox.max.y);
    }
    if (m_boundingBox.min.z > m_boundingBox.max.z) {
        std::swap(m_boundingBox.min.z, m_boundingBox.max.z);
    }
    
    // 清除缓存
    m_centerCached = false;
    m_sizeCached = false;
    m_radiusCached = false;
    
    emit boundingBoxChanged();
}

QString GeoBoundingBoxManager::toString() const
{
    if (m_boundingBox.isValid()) {
        return QString("BBox[(%1,%2,%3) - (%4,%5,%6)]")
               .arg(m_boundingBox.min.x).arg(m_boundingBox.min.y).arg(m_boundingBox.min.z)
               .arg(m_boundingBox.max.x).arg(m_boundingBox.max.y).arg(m_boundingBox.max.z);
    }
    return "BBox[Invalid]";
}

void GeoBoundingBoxManager::fromString(const QString& str)
{
    // 简单的解析实现
    // 格式: BBox[(x1,y1,z1) - (x2,y2,z2)]
    // 这里只是占位符，实际实现需要更复杂的解析
}

void GeoBoundingBoxManager::calculateBoundingBoxFromPoints(const std::vector<glm::vec3>& points)
{
    if (points.empty()) return;
    
    BoundingBox3D newBox;
    for (const auto& point : points) {
        newBox.expand(point);
    }
    
    setBoundingBox(newBox);
}

void GeoBoundingBoxManager::calculateBoundingBoxFromOSGGeometry()
{
    if (!m_parent) return;
    
    auto* nodeManager = m_parent->mm_node();
    if (!nodeManager) return;
    
    auto geometry = nodeManager->getGeometry();
    if (!geometry.valid()) return;
    
    // 从OSG几何体计算包围盒
    auto boundingSphere = geometry->getBound();
    if (boundingSphere.valid()) {
        float radius = boundingSphere.radius();
        glm::vec3 center(boundingSphere.center().x(), 
                        boundingSphere.center().y(), 
                        boundingSphere.center().z());
        
        BoundingBox3D newBox(center - glm::vec3(radius), 
                            center + glm::vec3(radius));
        setBoundingBox(newBox);
    }
}

void GeoBoundingBoxManager::updateBoundingBoxVisualization()
{
    // 这里可以添加包围盒线框的可视化
    // 现在只是占位符
    if (m_parent && m_parent->mm_node()) {
        // 可以在这里创建包围盒的线框可视化
        // 暂时不实现，因为高亮系统会处理包围盒显示
    }
}

void GeoBoundingBoxManager::createBoundingBoxWireframe()
{
    // 创建包围盒线框可视化
    // 这是一个占位符方法
}

void GeoBoundingBoxManager::markDirty()
{
    m_dirty = true;
    
    if (m_autoUpdate) {
        updateBoundingBox();
    }
} 

void GeoBoundingBoxManager::setControlPointsVisible(bool visible)
{
    if (m_controlPointsVisible != visible)
    {
        m_controlPointsVisible = visible;
        updateBoundingBoxVisualization();
        emit boundingBoxChanged();
    }
}

void GeoBoundingBoxManager::setControlPointSize(float size)
{
    if (m_controlPointSize != size)
    {
        m_controlPointSize = size;
        updateBoundingBoxVisualization();
        emit boundingBoxChanged();
    }
}

void GeoBoundingBoxManager::setControlPointColor(const Color3D& color)
{
    if (m_controlPointColor.r != color.r || m_controlPointColor.g != color.g || 
        m_controlPointColor.b != color.b || m_controlPointColor.a != color.a)
    {
        m_controlPointColor = color;
        updateBoundingBoxVisualization();
        emit boundingBoxChanged();
    }
}

std::vector<glm::vec3> GeoBoundingBoxManager::getControlPointPositions() const
{
    std::vector<glm::vec3> positions;
    
    if (!m_boundingBox.isValid())
        return positions;
    
    // 返回包围盒的8个角点
    positions = getCorners();
    
    return positions;
}

int GeoBoundingBoxManager::findNearestControlPoint(const glm::vec3& point, float threshold) const
{
    if (!m_boundingBox.isValid())
        return -1;
    
    std::vector<glm::vec3> corners = getCorners();
    int nearestIndex = -1;
    float minDistance = threshold;
    
    for (int i = 0; i < static_cast<int>(corners.size()); ++i)
    {
        glm::vec3 diff = corners[i] - point;
        float distance = glm::length(diff);
        
        if (distance < minDistance)
        {
            minDistance = distance;
            nearestIndex = i;
        }
    }
    
    return nearestIndex;
}

glm::vec3 GeoBoundingBoxManager::getControlPointPosition(int index) const
{
    if (!isValidControlPointIndex(index))
        return glm::vec3(0.0f);
    
    std::vector<glm::vec3> corners = getCorners();
    return corners[index];
}

bool GeoBoundingBoxManager::isValidControlPointIndex(int index) const
{
    return index >= 0 && index < 8; // 包围盒有8个角点
} 