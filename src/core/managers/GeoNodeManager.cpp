#include "GeoNodeManager.h"
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
#include <algorithm>

GeoNodeManager::GeoNodeManager(Geo3D* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_initialized(false)
    , m_visible(true)
    , m_vertexVisible(true)
    , m_edgeVisible(true)
    , m_faceVisible(true)
    , m_kdTreeDirty(true)
{
    initializeNodes();
}

void GeoNodeManager::initializeNodes()
{
    if (!m_initialized) {
        // 创建节点层次结构
        m_osgNode = new osg::Group();
        m_drawableGroup = new osg::Group();
        m_transformNode = new osg::MatrixTransform();
        m_controlPointsNode = new osg::Group();
        
        // 初始化点线面节点
        m_vertexNode = new osg::Group();
        m_edgeNode = new osg::Group();
        m_faceNode = new osg::Group();
        
        setupNodeHierarchy();
        setupNodeNames();
        
        m_initialized = true;
    }
}

void GeoNodeManager::setupNodeHierarchy()
{
    // 建立节点层次结构
    m_osgNode->addChild(m_transformNode.get());
    m_transformNode->addChild(m_drawableGroup.get());
    m_transformNode->addChild(m_controlPointsNode.get());
    
    // 将点线面节点添加到变换节点下
    m_transformNode->addChild(m_vertexNode.get());
    m_transformNode->addChild(m_edgeNode.get());
    m_transformNode->addChild(m_faceNode.get());
}

void GeoNodeManager::addChild(osg::ref_ptr<osg::Node> child)
{
    if (child.valid() && m_drawableGroup.valid()) {
        m_drawableGroup->addChild(child.get());
        emit nodeStructureChanged();
    }
}

void GeoNodeManager::removeChild(osg::ref_ptr<osg::Node> child)
{
    if (child.valid() && m_drawableGroup.valid()) {
        m_drawableGroup->removeChild(child.get());
        emit nodeStructureChanged();
    }
}

void GeoNodeManager::clearChildren()
{
    if (m_drawableGroup.valid()) {
        m_drawableGroup->removeChildren(0, m_drawableGroup->getNumChildren());
        emit nodeStructureChanged();
    }
}

void GeoNodeManager::setGeometry(osg::ref_ptr<osg::Geometry> geometry)
{
    // 清除旧的几何体
    clearGeometry();
    
    if (geometry.valid()) {
        m_geometry = geometry;
        
        // 创建Geode来包装Geometry
        osg::ref_ptr<osg::Geode> geode = new osg::Geode();
        geode->addDrawable(geometry.get());
        geode->setName("main_geometry");
        
        // 添加到绘制组
        m_drawableGroup->addChild(geode.get());
        
        emit geometryChanged();
    }
}

void GeoNodeManager::clearGeometry()
{
    if (m_drawableGroup.valid()) {
        m_drawableGroup->removeChildren(0, m_drawableGroup->getNumChildren());
        m_geometry = nullptr;
        emit geometryChanged();
    }
}

void GeoNodeManager::addVertexGeometry(osg::Drawable* drawable)
{
    if (!drawable || !m_vertexNode.valid()) return;
    
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(drawable);
    geode->setName("vertex_geode");
    m_vertexNode->addChild(geode.get());
    
    // 标记KDTree需要更新
    m_kdTreeDirty = true;
}

void GeoNodeManager::addEdgeGeometry(osg::Drawable* drawable)
{
    if (!drawable || !m_edgeNode.valid()) return;
    
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(drawable);
    geode->setName("edge_geode");
    m_edgeNode->addChild(geode.get());
    
    // 标记KDTree需要更新
    m_kdTreeDirty = true;
}

void GeoNodeManager::addFaceGeometry(osg::Drawable* drawable)
{
    if (!drawable || !m_faceNode.valid()) return;
    
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(drawable);
    geode->setName("face_geode");
    m_faceNode->addChild(geode.get());
    
    // 标记KDTree需要更新
    m_kdTreeDirty = true;
}

void GeoNodeManager::clearVertexGeometries()
{
    if (m_vertexNode.valid()) {
        m_vertexNode->removeChildren(0, m_vertexNode->getNumChildren());
        m_kdTreeDirty = true;
    }
}

void GeoNodeManager::clearEdgeGeometries()
{
    if (m_edgeNode.valid()) {
        m_edgeNode->removeChildren(0, m_edgeNode->getNumChildren());
        m_kdTreeDirty = true;
    }
}

void GeoNodeManager::clearFaceGeometries()
{
    if (m_faceNode.valid()) {
        m_faceNode->removeChildren(0, m_faceNode->getNumChildren());
        m_kdTreeDirty = true;
    }
}

void GeoNodeManager::clearAllGeometries()
{
    clearVertexGeometries();
    clearEdgeGeometries();
    clearFaceGeometries();
    clearKdTree();
}

void GeoNodeManager::setTransformMatrix(const osg::Matrix& matrix)
{
    if (m_transformNode.valid()) {
        m_transformNode->setMatrix(matrix);
        emit transformChanged();
    }
}

osg::Matrix GeoNodeManager::getTransformMatrix() const
{
    if (m_transformNode.valid()) {
        return m_transformNode->getMatrix();
    }
    return osg::Matrix::identity();
}

void GeoNodeManager::resetTransform()
{
    setTransformMatrix(osg::Matrix::identity());
}

void GeoNodeManager::setupNodeNames()
{
    // 设置节点名称，用于拾取系统识别
    if (m_osgNode.valid()) {
        m_osgNode->setName("geo3d_root");
    }
    if (m_transformNode.valid()) {
        m_transformNode->setName("geo3d_transform");
    }
    if (m_drawableGroup.valid()) {
        m_drawableGroup->setName("geo3d_drawable");
    }
    if (m_controlPointsNode.valid()) {
        m_controlPointsNode->setName("geo3d_controls");
    }
    if (m_vertexNode.valid()) {
        m_vertexNode->setName("vertex_group");
    }
    if (m_edgeNode.valid()) {
        m_edgeNode->setName("edge_group");
    }
    if (m_faceNode.valid()) {
        m_faceNode->setName("face_group");
    }
}

void GeoNodeManager::setNodeName(const std::string& name)
{
    if (m_osgNode.valid()) {
        m_osgNode->setName(name);
    }
}

std::string GeoNodeManager::getNodeName() const
{
    if (m_osgNode.valid()) {
        return m_osgNode->getName();
    }
    return "";
}

void GeoNodeManager::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        updateNodeVisibility();
        emit visibilityChanged();
    }
}

void GeoNodeManager::setVertexVisible(bool visible)
{
    if (m_vertexVisible != visible) {
        m_vertexVisible = visible;
        updateNodeVisibility();
    }
}

void GeoNodeManager::setEdgeVisible(bool visible)
{
    if (m_edgeVisible != visible) {
        m_edgeVisible = visible;
        updateNodeVisibility();
    }
}

void GeoNodeManager::setFaceVisible(bool visible)
{
    if (m_faceVisible != visible) {
        m_faceVisible = visible;
        updateNodeVisibility();
    }
}

bool GeoNodeManager::isVertexVisible() const
{
    return m_vertexVisible && m_visible;
}

bool GeoNodeManager::isEdgeVisible() const
{
    return m_edgeVisible && m_visible;
}

bool GeoNodeManager::isFaceVisible() const
{
    return m_faceVisible && m_visible;
}

void GeoNodeManager::updateNodes()
{
    if (!m_initialized) {
        initializeNodes();
    }
    
    updateNodeVisibility();
    emit nodeStructureChanged();
}

void GeoNodeManager::updateControlPointsVisualization()
{
    if (!m_controlPointsNode.valid() || !m_parent) {
        return;
    }
    
    // 清除旧的控制点可视化
    m_controlPointsNode->removeChildren(0, m_controlPointsNode->getNumChildren());
    
    // 如果对象在编辑状态且控制点管理器存在
    if (m_parent->isStateEditing() && m_parent->getControlPointManager()) {
        auto* controlManager = m_parent->getControlPointManager();
        
        if (controlManager->areControlPointsVisible()) {
            const auto& controlPoints = controlManager->getControlPoints();
            float size = controlManager->getControlPointSize();
            const Color3D& color = controlManager->getControlPointColor();
            
            for (const Point3D& point : controlPoints) {
                createControlPointVisualization(point, size, color);
            }
        }
    }
    
    emit controlPointsVisibilityChanged();
}

void GeoNodeManager::rebuildNodeStructure()
{
    if (m_initialized) {
        clearChildren();
        clearAllGeometries();
        m_controlPointsNode->removeChildren(0, m_controlPointsNode->getNumChildren());
        
        setupNodeHierarchy();
        setupNodeNames();
        updateNodeVisibility();
        
        emit nodeStructureChanged();
    }
}

int GeoNodeManager::getChildCount() const
{
    if (m_osgNode.valid()) {
        return static_cast<int>(m_osgNode->getNumChildren());
    }
    return 0;
}

bool GeoNodeManager::hasChildren() const
{
    return getChildCount() > 0;
}

bool GeoNodeManager::hasGeometry() const
{
    return m_geometry.valid();
}

void GeoNodeManager::optimizeNodes()
{
    // 实施节点优化
    if (m_osgNode.valid()) {
        // 这里可以添加OSG的优化器
        // osgUtil::Optimizer optimizer;
        // optimizer.optimize(m_osgNode.get());
    }
}

void GeoNodeManager::compactNodes()
{
    // 压缩节点结构，移除空的中间节点
    // 这是一个简化的实现
    optimizeNodes();
}

void GeoNodeManager::updateNodeVisibility()
{
    if (!m_osgNode.valid()) return;
    
    // 设置主节点可见性
    m_osgNode->setNodeMask(m_visible ? 0xffffffff : 0x0);
    
    // 设置点线面节点可见性
    if (m_vertexNode.valid()) {
        m_vertexNode->setNodeMask(isVertexVisible() ? 0xffffffff : 0x0);
    }
    if (m_edgeNode.valid()) {
        m_edgeNode->setNodeMask(isEdgeVisible() ? 0xffffffff : 0x0);
    }
    if (m_faceNode.valid()) {
        m_faceNode->setNodeMask(isFaceVisible() ? 0xffffffff : 0x0);
    }
}

void GeoNodeManager::createControlPointVisualization(const Point3D& point, float size, const Color3D& color)
{
    if (!m_controlPointsNode.valid()) return;
    
    // 创建控制点的球体可视化
    osg::ref_ptr<osg::Geode> controlPointGeode = new osg::Geode();
    osg::ref_ptr<osg::ShapeDrawable> sphere = new osg::ShapeDrawable(
        new osg::Sphere(osg::Vec3(point.x(), point.y(), point.z()), size));
    
    sphere->setColor(osg::Vec4(color.r, color.g, color.b, color.a));
    controlPointGeode->addDrawable(sphere.get());
    controlPointGeode->setName("control_point");
    
    m_controlPointsNode->addChild(controlPointGeode.get());
} 

// KDTree管理实现
void GeoNodeManager::buildKdTree()
{
    if (!m_kdTreeDirty) return;
    
    // 收集几何体数据
    collectGeometryData();
    
    if (m_geometryInfos.empty()) {
        m_kdTree = nullptr;
        return;
    }
    
    // 创建KDTree
    m_kdTree = new osg::KdTree();
    
    // 简化实现：只构建一个包含所有几何体的KDTree
    // 创建一个临时的Geode来包含所有Drawable
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    for (const auto& info : m_geometryInfos) {
        if (m_geometryVisibility[info.index]) {
            geode->addDrawable(info.drawable.get());
        }
    }
    
    if (geode->getNumDrawables() > 0) {
        osg::KdTree::BuildOptions buildOptions;
        for (unsigned int i = 0; i < geode->getNumDrawables(); ++i) {
            osg::Drawable* drawable = geode->getDrawable(i);
            osg::Geometry* geometry = dynamic_cast<osg::Geometry*>(drawable);
            if (geometry) {
                m_kdTree->build(buildOptions, geometry);
            }
        }
    }
    
    m_kdTreeDirty = false;
    emit kdTreeUpdated();
}

void GeoNodeManager::updateKdTree()
{
    m_kdTreeDirty = true;
    buildKdTree();
}

void GeoNodeManager::clearKdTree()
{
    m_kdTree = nullptr;
    m_geometryInfos.clear();
    m_geometryVisibility.clear();
    m_kdTreeDirty = true;
}

void GeoNodeManager::collectGeometryData()
{
    m_geometryInfos.clear();
    m_geometryVisibility.clear();
    
    // 收集点几何体
    if (m_vertexNode.valid()) {
        for (unsigned int i = 0; i < m_vertexNode->getNumChildren(); ++i) {
            osg::Node* child = m_vertexNode->getChild(i);
            if (osg::Geode* geode = dynamic_cast<osg::Geode*>(child)) {
                for (unsigned int j = 0; j < geode->getNumDrawables(); ++j) {
                    osg::Drawable* drawable = geode->getDrawable(j);
                    if (drawable) {
                        GeoKdTreeNodeInfo info;
                        info.drawable = drawable;
                        info.node = child;
                        info.geoObject = m_parent;
                        info.geometryType = 0; // 点
                        info.index = m_geometryInfos.size();
                        
                        m_geometryInfos.push_back(info);
                        m_geometryVisibility.push_back(m_vertexVisible);
                    }
                }
            }
        }
    }
    
    // 收集线几何体
    if (m_edgeNode.valid()) {
        for (unsigned int i = 0; i < m_edgeNode->getNumChildren(); ++i) {
            osg::Node* child = m_edgeNode->getChild(i);
            if (osg::Geode* geode = dynamic_cast<osg::Geode*>(child)) {
                for (unsigned int j = 0; j < geode->getNumDrawables(); ++j) {
                    osg::Drawable* drawable = geode->getDrawable(j);
                    if (drawable) {
                        GeoKdTreeNodeInfo info;
                        info.drawable = drawable;
                        info.node = child;
                        info.geoObject = m_parent;
                        info.geometryType = 1; // 线
                        info.index = m_geometryInfos.size();
                        
                        m_geometryInfos.push_back(info);
                        m_geometryVisibility.push_back(m_edgeVisible);
                    }
                }
            }
        }
    }
    
    // 收集面几何体
    if (m_faceNode.valid()) {
        for (unsigned int i = 0; i < m_faceNode->getNumChildren(); ++i) {
            osg::Node* child = m_faceNode->getChild(i);
            if (osg::Geode* geode = dynamic_cast<osg::Geode*>(child)) {
                for (unsigned int j = 0; j < geode->getNumDrawables(); ++j) {
                    osg::Drawable* drawable = geode->getDrawable(j);
                    if (drawable) {
                        GeoKdTreeNodeInfo info;
                        info.drawable = drawable;
                        info.node = child;
                        info.geoObject = m_parent;
                        info.geometryType = 2; // 面
                        info.index = m_geometryInfos.size();
                        
                        m_geometryInfos.push_back(info);
                        m_geometryVisibility.push_back(m_faceVisible);
                    }
                }
            }
        }
    }
}



osg::Vec3 GeoNodeManager::getGeometryCenter(osg::Drawable* drawable) const
{
    if (!drawable) return osg::Vec3();
    
    // 计算几何体的包围盒中心
    osg::ComputeBoundsVisitor cbv;
    drawable->accept(cbv);
    osg::BoundingBox bb = cbv.getBoundingBox();
    
    return (bb._min + bb._max) * 0.5f;
}

bool GeoNodeManager::isGeometryInFrustum(const osg::Vec3& center, float radius) const
{
    // 简单的视锥体剔除检查
    // 这里可以根据需要实现更复杂的视锥体剔除
    return true;
}

// 快速查询功能实现
std::vector<GeoKdTreeNodeInfo> GeoNodeManager::queryKdTree(const osg::Vec3& point, float radius)
{
    std::vector<GeoKdTreeNodeInfo> results;
    
    if (!m_kdTree.valid() || m_kdTreeDirty) {
        buildKdTree();
    }
    
    if (!m_kdTree.valid()) return results;
    
    // 简化实现：直接遍历所有可见几何体
    for (size_t i = 0; i < m_geometryInfos.size(); ++i) {
        if (!m_geometryVisibility[i]) continue;
        
        const auto& info = m_geometryInfos[i];
        osg::Vec3 center = getGeometryCenter(info.drawable.get());
        float distance = (center - point).length();
        
        if (distance <= radius) {
            results.push_back(info);
        }
    }
    
    return results;
}

std::vector<GeoKdTreeNodeInfo> GeoNodeManager::queryKdTreeRay(const osg::Vec3& start, const osg::Vec3& direction, float maxDistance)
{
    std::vector<GeoKdTreeNodeInfo> results;
    
    if (!m_kdTree.valid() || m_kdTreeDirty) {
        buildKdTree();
    }
    
    if (!m_kdTree.valid()) return results;
    
    // 简化实现：直接遍历所有可见几何体
    for (size_t i = 0; i < m_geometryInfos.size(); ++i) {
        if (!m_geometryVisibility[i]) continue;
        
        const auto& info = m_geometryInfos[i];
        osg::Vec3 center = getGeometryCenter(info.drawable.get());
        
        // 计算到射线的距离
        osg::Vec3 toCenter = center - start;
        float projection = toCenter * direction;
        
        if (projection < 0 || projection > maxDistance) continue; // 在射线范围外
        
        osg::Vec3 closestPoint = start + direction * projection;
        float distance = (center - closestPoint).length();
        
        // 如果距离在合理范围内，认为相交
        if (distance <= 1.0f) { // 1.0f作为相交阈值
            results.push_back(info);
        }
    }
    
    return results;
}

GeoKdTreeNodeInfo GeoNodeManager::findClosestGeometry(const osg::Vec3& point)
{
    GeoKdTreeNodeInfo closest;
    float minDistance = std::numeric_limits<float>::max();
    
    if (!m_kdTree.valid() || m_kdTreeDirty) {
        buildKdTree();
    }
    
    if (!m_kdTree.valid()) return closest;
    
    // 查询附近的几何体
    std::vector<GeoKdTreeNodeInfo> nearby = queryKdTree(point, 10.0f);
    
    for (const auto& info : nearby) {
        if (!m_geometryVisibility[info.index]) continue;
        
        osg::Vec3 center = getGeometryCenter(info.drawable.get());
        float distance = (center - point).length();
        
        if (distance < minDistance) {
            minDistance = distance;
            closest = info;
        }
    }
    
    return closest;
}

// 几何体可见性查询
bool GeoNodeManager::isGeometryVisible(int type, int index) const
{
    if (index < 0 || index >= static_cast<int>(m_geometryVisibility.size())) {
        return false;
    }
    
    return m_geometryVisibility[index];
}

void GeoNodeManager::setGeometryVisible(int type, int index, bool visible)
{
    if (index < 0 || index >= static_cast<int>(m_geometryVisibility.size())) {
        return;
    }
    
    m_geometryVisibility[index] = visible;
    
    // 更新对应节点的可见性
    if (index < static_cast<int>(m_geometryInfos.size())) {
        const auto& info = m_geometryInfos[index];
        if (info.node.valid()) {
            info.node->setNodeMask(visible ? 0xffffffff : 0x0);
        }
    }
}

std::vector<GeoKdTreeNodeInfo> GeoNodeManager::getVisibleGeometries() const
{
    std::vector<GeoKdTreeNodeInfo> visible;
    
    for (size_t i = 0; i < m_geometryInfos.size(); ++i) {
        if (m_geometryVisibility[i]) {
            visible.push_back(m_geometryInfos[i]);
        }
    }
    
    return visible;
} 