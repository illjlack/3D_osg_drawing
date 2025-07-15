#include "GeoNodeManager.h"
#include "OctreeManager.h"
#include "../GeometryBase.h"
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Shape>
#include <osg/Material>
#include <osg/LineWidth>
#include <osg/Point>
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
    m_octreeManager = std::make_unique<OctreeManager>(parent);
    if (m_octreeManager) {
        connect(m_octreeManager.get(), &OctreeManager::octreeUpdated, this, &GeoNodeManager::kdTreeUpdated);
        connect(m_octreeManager.get(), &OctreeManager::geometryDataChanged, this, &GeoNodeManager::geometryChanged);
        connect(m_octreeManager.get(), &OctreeManager::visibilityChanged, this, &GeoNodeManager::visibilityChanged);
    }
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
    
    // 添加到八叉树管理器
    if (m_octreeManager) {
        GeoOctreeNodeInfo info;
        info.drawable = drawable;
        info.node = geode.get();
        info.geoObject = m_parent;
        info.geometryType = 0; // 点
        info.boundingBox = m_octreeManager->getGeometryBoundingBox(drawable);
        m_octreeManager->addGeometryData(info);
    }
    
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
    
    // 添加到八叉树管理器
    if (m_octreeManager) {
        GeoOctreeNodeInfo info;
        info.drawable = drawable;
        info.node = geode.get();
        info.geoObject = m_parent;
        info.geometryType = 1; // 线
        info.boundingBox = m_octreeManager->getGeometryBoundingBox(drawable);
        m_octreeManager->addGeometryData(info);
    }
    
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
    
    // 添加到八叉树管理器
    if (m_octreeManager) {
        GeoOctreeNodeInfo info;
        info.drawable = drawable;
        info.node = geode.get();
        info.geoObject = m_parent;
        info.geometryType = 2; // 面
        info.boundingBox = m_octreeManager->getGeometryBoundingBox(drawable);
        m_octreeManager->addGeometryData(info);
    }
    
    // 标记KDTree需要更新
    m_kdTreeDirty = true;
}

void GeoNodeManager::clearVertexGeometries()
{
    if (m_vertexNode.valid()) {
        m_vertexNode->removeChildren(0, m_vertexNode->getNumChildren());
        
        // 清除八叉树中的点几何体数据
        if (m_octreeManager) {
            m_octreeManager->clearGeometryData();
        }
        
        m_kdTreeDirty = true;
    }
}

void GeoNodeManager::clearEdgeGeometries()
{
    if (m_edgeNode.valid()) {
        m_edgeNode->removeChildren(0, m_edgeNode->getNumChildren());
        
        // 清除八叉树中的线几何体数据
        if (m_octreeManager) {
            m_octreeManager->clearGeometryData();
        }
        
        m_kdTreeDirty = true;
    }
}

void GeoNodeManager::clearFaceGeometries()
{
    if (m_faceNode.valid()) {
        m_faceNode->removeChildren(0, m_faceNode->getNumChildren());
        
        // 清除八叉树中的面几何体数据
        if (m_octreeManager) {
            m_octreeManager->clearGeometryData();
        }
        
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

// 八叉树管理 - 委托给OctreeManager
void GeoNodeManager::buildKdTree()
{
    if (m_octreeManager) {
        m_octreeManager->buildOctree();
    }
}

void GeoNodeManager::updateKdTree()
{
    if (m_octreeManager) {
        m_octreeManager->updateOctree();
    }
}

void GeoNodeManager::clearKdTree()
{
    if (m_octreeManager) {
        m_octreeManager->clearOctree();
    }
} 