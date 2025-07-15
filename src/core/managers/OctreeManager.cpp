#include "OctreeManager.h"
#include "../GeometryBase.h"
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Shape>
#include <osg/Material>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/KdTree>
#include <osg/NodeVisitor>
#include <osg/ComputeBoundsVisitor>
#include <osg/Geometry>
#include <osg/PrimitiveSet>
#include <osg/Vec3>
#include <osg/BoundingBox>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <functional>

OctreeManager::OctreeManager(Geo3D* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_octreeRoot(nullptr)
    , m_maxGeometriesPerNode(8)
    , m_maxDepth(8)
    , m_octreeDirty(true)
    , m_cachedTotalNodes(0)
    , m_cachedLeafNodes(0)
    , m_statsDirty(true)
{
}

void OctreeManager::buildOctree()
{
    if (!m_octreeDirty) return;
    
    // 收集几何体数据
    collectGeometryData();
    
    if (m_geometryInfos.empty()) {
        m_octreeRoot = nullptr;
        return;
    }
    
    // 计算所有几何体的总包围盒
    osg::BoundingBox totalBounds;
    for (const auto& info : m_geometryInfos) {
        if (m_geometryVisibility[info.index]) {
            totalBounds.expandBy(info.boundingBox);
        }
    }
    
    if (!totalBounds.valid()) {
        m_octreeRoot = nullptr;
        return;
    }
    
    // 创建根节点
    m_octreeRoot = std::make_unique<OctreeNode>(totalBounds, m_maxGeometriesPerNode, m_maxDepth, 0);
    
    // 收集可见的几何体
    std::vector<GeoOctreeNodeInfo> visibleGeometries;
    for (size_t i = 0; i < m_geometryInfos.size(); ++i) {
        if (m_geometryVisibility[i]) {
            visibleGeometries.push_back(m_geometryInfos[i]);
        }
    }
    
    // 构建八叉树
    buildOctreeRecursive(m_octreeRoot, visibleGeometries);
    
    m_octreeDirty = false;
    m_statsDirty = true;
    emit octreeUpdated();
}

void OctreeManager::buildOctreeRecursive(std::unique_ptr<OctreeNode>& node, const std::vector<GeoOctreeNodeInfo>& geometries)
{
    if (geometries.empty()) {
        return;
    }
    
    // 如果几何体数量少于最大数量或已达到最大深度，则作为叶子节点
    if (geometries.size() <= static_cast<size_t>(node->maxGeometries) || 
        node->currentDepth >= node->maxDepth) {
        node->geometries = geometries;
        node->isLeaf = true;
        return;
    }
    
    // 分割节点
    splitNode(node, geometries);
}

void OctreeManager::splitNode(std::unique_ptr<OctreeNode>& node, const std::vector<GeoOctreeNodeInfo>& geometries)
{
    // 计算子节点的包围盒
    std::vector<osg::BoundingBox> childBounds = calculateChildBounds(node->bounds);
    
    // 为每个子节点分配几何体
    std::vector<std::vector<GeoOctreeNodeInfo>> childGeometries(8);
    
    for (const auto& geometry : geometries) {
        // 找到几何体属于哪个子节点
        for (int i = 0; i < 8; ++i) {
            if (isBoxIntersectBox(geometry.boundingBox, childBounds[i])) {
                childGeometries[i].push_back(geometry);
            }
        }
    }
    
    // 创建子节点
    node->children.resize(8);
    for (int i = 0; i < 8; ++i) {
        if (!childGeometries[i].empty()) {
            node->children[i] = std::make_unique<OctreeNode>(
                childBounds[i], 
                node->maxGeometries, 
                node->maxDepth, 
                node->currentDepth + 1
            );
            buildOctreeRecursive(node->children[i], childGeometries[i]);
        }
    }
    
    node->isLeaf = false;
}

std::vector<osg::BoundingBox> OctreeManager::calculateChildBounds(const osg::BoundingBox& parentBounds)
{
    std::vector<osg::BoundingBox> childBounds(8);
    
    osg::Vec3 center = (parentBounds._min + parentBounds._max) * 0.5f;
    osg::Vec3 halfSize = (parentBounds._max - parentBounds._min) * 0.5f;
    
    // 八个子节点的包围盒
    // 0: 前下左, 1: 前下右, 2: 前上左, 3: 前上右
    // 4: 后下左, 5: 后下右, 6: 后上左, 7: 后上右
    childBounds[0] = osg::BoundingBox(parentBounds._min, center);
    childBounds[1] = osg::BoundingBox(
        osg::Vec3(center.x(), parentBounds._min.y(), parentBounds._min.z()),
        osg::Vec3(parentBounds._max.x(), center.y(), center.z())
    );
    childBounds[2] = osg::BoundingBox(
        osg::Vec3(parentBounds._min.x(), center.y(), parentBounds._min.z()),
        osg::Vec3(center.x(), parentBounds._max.y(), center.z())
    );
    childBounds[3] = osg::BoundingBox(
        osg::Vec3(center.x(), center.y(), parentBounds._min.z()),
        osg::Vec3(parentBounds._max.x(), parentBounds._max.y(), center.z())
    );
    childBounds[4] = osg::BoundingBox(
        osg::Vec3(parentBounds._min.x(), parentBounds._min.y(), center.z()),
        osg::Vec3(center.x(), center.y(), parentBounds._max.z())
    );
    childBounds[5] = osg::BoundingBox(
        osg::Vec3(center.x(), parentBounds._min.y(), center.z()),
        osg::Vec3(parentBounds._max.x(), center.y(), parentBounds._max.z())
    );
    childBounds[6] = osg::BoundingBox(
        osg::Vec3(parentBounds._min.x(), center.y(), center.z()),
        osg::Vec3(center.x(), parentBounds._max.y(), parentBounds._max.z())
    );
    childBounds[7] = osg::BoundingBox(center, parentBounds._max);
    
    return childBounds;
}

void OctreeManager::updateOctree()
{
    m_octreeDirty = true;
    buildOctree();
}

void OctreeManager::clearOctree()
{
    m_octreeRoot = nullptr;
    m_geometryInfos.clear();
    m_geometryVisibility.clear();
    m_octreeDirty = true;
    m_statsDirty = true;
}

void OctreeManager::rebuildOctree()
{
    clearOctree();
    buildOctree();
}

void OctreeManager::collectGeometryData()
{
    m_geometryInfos.clear();
    m_geometryVisibility.clear();
    
    if (!m_parent) return;
    
    // 这里需要从GeoNodeManager获取几何体数据
    // 由于我们分离了八叉树管理，这里需要与GeoNodeManager协作
    // 暂时使用简化实现
    
    // 实际实现中，这里应该从GeoNodeManager获取几何体数据
    // 并转换为GeoOctreeNodeInfo格式
}

void OctreeManager::addGeometryData(const GeoOctreeNodeInfo& info)
{
    GeoOctreeNodeInfo newInfo = info;
    newInfo.index = m_geometryInfos.size();
    m_geometryInfos.push_back(newInfo);
    m_geometryVisibility.push_back(true);
    m_octreeDirty = true;
}

void OctreeManager::clearGeometryData()
{
    m_geometryInfos.clear();
    m_geometryVisibility.clear();
    m_octreeDirty = true;
}

std::vector<GeoOctreeNodeInfo> OctreeManager::queryOctree(const osg::Vec3& point, float radius)
{
    std::vector<GeoOctreeNodeInfo> results;
    
    if (!m_octreeRoot || m_octreeDirty) {
        buildOctree();
    }
    
    if (!m_octreeRoot) return results;
    
    queryOctreeRecursive(m_octreeRoot.get(), point, radius, results);
    return results;
}

void OctreeManager::queryOctreeRecursive(const OctreeNode* node, const osg::Vec3& point, float radius, std::vector<GeoOctreeNodeInfo>& results) const
{
    if (!node) return;
    
    // 检查查询点是否在节点包围盒内
    if (!isPointInBox(point, node->bounds)) {
        return;
    }
    
    // 如果是叶子节点，检查所有几何体
    if (node->isLeaf) {
        for (const auto& geometry : node->geometries) {
            if (!m_geometryVisibility[geometry.index]) continue;
            
            osg::Vec3 center = getGeometryCenter(geometry.drawable.get());
            float distance = calculateDistance(point, center);
            
            if (distance <= radius) {
                results.push_back(geometry);
            }
        }
    } else {
        // 递归查询子节点
        for (const auto& child : node->children) {
            if (child) {
                queryOctreeRecursive(child.get(), point, radius, results);
            }
        }
    }
}

std::vector<GeoOctreeNodeInfo> OctreeManager::queryOctreeRay(const osg::Vec3& start, const osg::Vec3& direction, float maxDistance)
{
    std::vector<GeoOctreeNodeInfo> results;
    
    if (!m_octreeRoot || m_octreeDirty) {
        buildOctree();
    }
    
    if (!m_octreeRoot) return results;
    
    queryOctreeRayRecursive(m_octreeRoot.get(), start, direction, maxDistance, results);
    return results;
}

void OctreeManager::queryOctreeRayRecursive(const OctreeNode* node, const osg::Vec3& start, const osg::Vec3& direction, float maxDistance, std::vector<GeoOctreeNodeInfo>& results) const
{
    if (!node) return;
    
    // 检查射线是否与节点包围盒相交
    if (!isRayIntersectBox(start, direction, node->bounds)) {
        return;
    }
    
    // 如果是叶子节点，检查所有几何体
    if (node->isLeaf) {
        for (const auto& geometry : node->geometries) {
            if (!m_geometryVisibility[geometry.index]) continue;
            
            osg::Vec3 center = getGeometryCenter(geometry.drawable.get());
            
            // 计算到射线的距离
            osg::Vec3 toCenter = center - start;
            float projection = toCenter * direction;
            
            if (projection < 0 || projection > maxDistance) continue;
            
            osg::Vec3 closestPoint = start + direction * projection;
            float distance = calculateDistance(center, closestPoint);
            
            // 如果距离在合理范围内，认为相交
            if (distance <= 1.0f) {
                results.push_back(geometry);
            }
        }
    } else {
        // 递归查询子节点
        for (const auto& child : node->children) {
            if (child) {
                queryOctreeRayRecursive(child.get(), start, direction, maxDistance, results);
            }
        }
    }
}

std::vector<GeoOctreeNodeInfo> OctreeManager::queryOctreeBox(const osg::BoundingBox& box)
{
    std::vector<GeoOctreeNodeInfo> results;
    
    if (!m_octreeRoot || m_octreeDirty) {
        buildOctree();
    }
    
    if (!m_octreeRoot) return results;
    
    queryOctreeBoxRecursive(m_octreeRoot.get(), box, results);
    return results;
}

void OctreeManager::queryOctreeBoxRecursive(const OctreeNode* node, const osg::BoundingBox& box, std::vector<GeoOctreeNodeInfo>& results) const
{
    if (!node) return;
    
    // 检查包围盒是否相交
    if (!isBoxIntersectBox(node->bounds, box)) {
        return;
    }
    
    // 如果是叶子节点，检查所有几何体
    if (node->isLeaf) {
        for (const auto& geometry : node->geometries) {
            if (!m_geometryVisibility[geometry.index]) continue;
            
            if (isBoxIntersectBox(geometry.boundingBox, box)) {
                results.push_back(geometry);
            }
        }
    } else {
        // 递归查询子节点
        for (const auto& child : node->children) {
            if (child) {
                queryOctreeBoxRecursive(child.get(), box, results);
            }
        }
    }
}

GeoOctreeNodeInfo OctreeManager::findClosestGeometry(const osg::Vec3& point)
{
    GeoOctreeNodeInfo closest;
    float minDistance = std::numeric_limits<float>::max();
    
    if (!m_octreeRoot || m_octreeDirty) {
        buildOctree();
    }
    
    if (!m_octreeRoot) return closest;
    
    // 查询附近的几何体
    std::vector<GeoOctreeNodeInfo> nearby = queryOctree(point, 10.0f);
    
    for (const auto& info : nearby) {
        if (!m_geometryVisibility[info.index]) continue;
        
        osg::Vec3 center = getGeometryCenter(info.drawable.get());
        float distance = calculateDistance(center, point);
        
        if (distance < minDistance) {
            minDistance = distance;
            closest = info;
        }
    }
    
    return closest;
}

void OctreeManager::setGeometryVisible(int index, bool visible)
{
    if (index < 0 || index >= static_cast<int>(m_geometryVisibility.size())) {
        return;
    }
    
    m_geometryVisibility[index] = visible;
    m_octreeDirty = true;
    emit visibilityChanged();
}

bool OctreeManager::isGeometryVisible(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_geometryVisibility.size())) {
        return false;
    }
    
    return m_geometryVisibility[index];
}

std::vector<GeoOctreeNodeInfo> OctreeManager::getVisibleGeometries() const
{
    std::vector<GeoOctreeNodeInfo> visible;
    
    for (size_t i = 0; i < m_geometryInfos.size(); ++i) {
        if (m_geometryVisibility[i]) {
            visible.push_back(m_geometryInfos[i]);
        }
    }
    
    return visible;
}

void OctreeManager::setMaxGeometriesPerNode(int maxGeo)
{
    if (m_maxGeometriesPerNode != maxGeo) {
        m_maxGeometriesPerNode = maxGeo;
        m_octreeDirty = true;
    }
}

void OctreeManager::setMaxDepth(int maxDepth)
{
    if (m_maxDepth != maxDepth) {
        m_maxDepth = maxDepth;
        m_octreeDirty = true;
    }
}

int OctreeManager::getOctreeDepth() const
{
    if (!m_octreeRoot) return 0;
    
    // 计算最大深度
    int maxDepth = 0;
    std::function<void(const OctreeNode*, int)> traverse = [&](const OctreeNode* node, int depth) {
        if (!node) return;
        maxDepth = std::max(maxDepth, depth);
        for (const auto& child : node->children) {
            if (child) {
                traverse(child.get(), depth + 1);
            }
        }
    };
    
    traverse(m_octreeRoot.get(), 0);
    return maxDepth;
}

int OctreeManager::getTotalNodes() const
{
    if (m_statsDirty) {
        calculateOctreeStats(m_octreeRoot.get(), m_cachedTotalNodes, m_cachedLeafNodes);
        m_statsDirty = false;
    }
    return m_cachedTotalNodes;
}

int OctreeManager::getLeafNodes() const
{
    if (m_statsDirty) {
        calculateOctreeStats(m_octreeRoot.get(), m_cachedTotalNodes, m_cachedLeafNodes);
        m_statsDirty = false;
    }
    return m_cachedLeafNodes;
}

int OctreeManager::getTotalGeometries() const
{
    return static_cast<int>(m_geometryInfos.size());
}

void OctreeManager::calculateOctreeStats(const OctreeNode* node, int& totalNodes, int& leafNodes) const
{
    if (!node) return;
    
    totalNodes++;
    if (node->isLeaf) {
        leafNodes++;
    } else {
        for (const auto& child : node->children) {
            if (child) {
                calculateOctreeStats(child.get(), totalNodes, leafNodes);
            }
        }
    }
}

void OctreeManager::printOctreeStats() const
{
    std::cout << "=== 八叉树统计信息 ===" << std::endl;
    std::cout << "总节点数: " << getTotalNodes() << std::endl;
    std::cout << "叶子节点数: " << getLeafNodes() << std::endl;
    std::cout << "总几何体数: " << getTotalGeometries() << std::endl;
    std::cout << "八叉树深度: " << getOctreeDepth() << std::endl;
    std::cout << "每节点最大几何体数: " << m_maxGeometriesPerNode << std::endl;
    std::cout << "最大深度: " << m_maxDepth << std::endl;
    std::cout << "=====================" << std::endl;
}

osg::ref_ptr<osg::Node> OctreeManager::createOctreeVisualization() const
{
    if (!m_octreeRoot) return nullptr;
    
    osg::ref_ptr<osg::Group> group = new osg::Group();
    group->setName("octree_visualization");
    
    // 使用正确的重载版本
    createNodeVisualization(m_octreeRoot.get(), group.get());
    
    return group;
}

osg::ref_ptr<osg::Node> OctreeManager::createNodeVisualization(const OctreeNode* node) const
{
    if (!node) return nullptr;
    
    osg::ref_ptr<osg::Group> group = new osg::Group();
    createNodeVisualization(node, group.get());
    return group;
}

void OctreeManager::createNodeVisualization(const OctreeNode* node, osg::Group* parent) const
{
    if (!node || !parent) return;
    
    // 创建节点包围盒的可视化
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    
    // 创建线框立方体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    
    const osg::BoundingBox& bb = node->bounds;
    
    // 立方体的8个顶点
    vertices->push_back(osg::Vec3(bb._min.x(), bb._min.y(), bb._min.z()));
    vertices->push_back(osg::Vec3(bb._max.x(), bb._min.y(), bb._min.z()));
    vertices->push_back(osg::Vec3(bb._max.x(), bb._max.y(), bb._min.z()));
    vertices->push_back(osg::Vec3(bb._min.x(), bb._max.y(), bb._min.z()));
    vertices->push_back(osg::Vec3(bb._min.x(), bb._min.y(), bb._max.z()));
    vertices->push_back(osg::Vec3(bb._max.x(), bb._min.y(), bb._max.z()));
    vertices->push_back(osg::Vec3(bb._max.x(), bb._max.y(), bb._max.z()));
    vertices->push_back(osg::Vec3(bb._min.x(), bb._max.y(), bb._max.z()));
    
    geometry->setVertexArray(vertices.get());
    
    // 创建线框索引
    osg::ref_ptr<osg::DrawElementsUInt> lines = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES, 0);
    
    // 12条边
    // 底面
    lines->push_back(0); lines->push_back(1);
    lines->push_back(1); lines->push_back(2);
    lines->push_back(2); lines->push_back(3);
    lines->push_back(3); lines->push_back(0);
    
    // 顶面
    lines->push_back(4); lines->push_back(5);
    lines->push_back(5); lines->push_back(6);
    lines->push_back(6); lines->push_back(7);
    lines->push_back(7); lines->push_back(4);
    
    // 竖直边
    lines->push_back(0); lines->push_back(4);
    lines->push_back(1); lines->push_back(5);
    lines->push_back(2); lines->push_back(6);
    lines->push_back(3); lines->push_back(7);
    
    geometry->addPrimitiveSet(lines.get());
    
    // 设置颜色
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    if (node->isLeaf) {
        colors->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f)); // 红色表示叶子节点
    } else {
        colors->push_back(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f)); // 绿色表示内部节点
    }
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    
    geode->addDrawable(geometry.get());
    parent->addChild(geode.get());
    
    // 递归创建子节点可视化
    if (!node->isLeaf) {
        for (const auto& child : node->children) {
            if (child) {
                createNodeVisualization(child.get(), parent);
            }
        }
    }
}

osg::Vec3 OctreeManager::getGeometryCenter(osg::Drawable* drawable) const
{
    if (!drawable) return osg::Vec3();
    
    // 计算几何体的包围盒中心
    osg::ComputeBoundsVisitor cbv;
    drawable->accept(cbv);
    osg::BoundingBox bb = cbv.getBoundingBox();
    
    return (bb._min + bb._max) * 0.5f;
}

osg::BoundingBox OctreeManager::getGeometryBoundingBox(osg::Drawable* drawable) const
{
    if (!drawable) return osg::BoundingBox();
    
    // 计算几何体的包围盒
    osg::ComputeBoundsVisitor cbv;
    drawable->accept(cbv);
    return cbv.getBoundingBox();
}

bool OctreeManager::isPointInBox(const osg::Vec3& point, const osg::BoundingBox& box) const
{
    return point.x() >= box._min.x() && point.x() <= box._max.x() &&
           point.y() >= box._min.y() && point.y() <= box._max.y() &&
           point.z() >= box._min.z() && point.z() <= box._max.z();
}

bool OctreeManager::isBoxIntersectBox(const osg::BoundingBox& box1, const osg::BoundingBox& box2) const
{
    return !(box1._max.x() < box2._min.x() || box1._min.x() > box2._max.x() ||
             box1._max.y() < box2._min.y() || box1._min.y() > box2._max.y() ||
             box1._max.z() < box2._min.z() || box1._min.z() > box2._max.z());
}

bool OctreeManager::isRayIntersectBox(const osg::Vec3& start, const osg::Vec3& direction, const osg::BoundingBox& box) const
{
    // 简化的射线包围盒相交测试
    // 这里可以实现更精确的射线包围盒相交算法
    return true; // 简化实现，总是返回true
}

float OctreeManager::calculateDistance(const osg::Vec3& point1, const osg::Vec3& point2) const
{
    return (point1 - point2).length();
} 