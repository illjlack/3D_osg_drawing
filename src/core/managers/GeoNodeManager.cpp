#include "GeoNodeManager.h"
#include "../GeometryBase.h"
#include <osg/Geode>
#include <osg/Material>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/NodeVisitor>
#include <osg/ComputeBoundsVisitor>
#include <osg/KdTree>
#include "../../util/LogManager.h"

GeoNodeManager::GeoNodeManager(Geo3D* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_initialized(false)
{
    initializeNodes();
}

void GeoNodeManager::initializeNodes()
{
    if (!m_initialized) {
        // 创建根节点和变换节点
        m_osgNode = new osg::Group();
        m_transformNode = new osg::MatrixTransform();
        m_osgNode->addChild(m_transformNode.get());
        
        // 创建几何体节点
        m_vertexGeometry = new osg::Geometry();
        m_edgeGeometry = new osg::Geometry();
        m_faceGeometry = new osg::Geometry();
        m_controlPointsGeometry = new osg::Geometry();
        
        setupNodeHierarchy();
        m_initialized = true;
    }
}

void GeoNodeManager::setupNodeHierarchy()
{
    // 将几何体节点添加到变换节点
    m_transformNode->addChild(m_vertexGeometry.get());
    m_transformNode->addChild(m_edgeGeometry.get());
    m_transformNode->addChild(m_faceGeometry.get());
    m_transformNode->addChild(m_controlPointsGeometry.get());
}

void GeoNodeManager::clearVertexGeometry()
{
    if (m_vertexGeometry.valid()) {
        m_vertexGeometry->removePrimitiveSet(0, m_vertexGeometry->getNumPrimitiveSets());
        m_vertexGeometry->setVertexArray(nullptr);
        m_vertexGeometry->setColorArray(nullptr);
        m_vertexGeometry->setShape(nullptr);  // 清除KdTree
        emit geometryChanged();
    }
}

void GeoNodeManager::clearEdgeGeometry()
{
    if (m_edgeGeometry.valid()) {
        m_edgeGeometry->removePrimitiveSet(0, m_edgeGeometry->getNumPrimitiveSets());
        m_edgeGeometry->setVertexArray(nullptr);
        m_edgeGeometry->setColorArray(nullptr);
        m_edgeGeometry->setShape(nullptr);  // 清除KdTree
        emit geometryChanged();
    }
}

void GeoNodeManager::clearFaceGeometry()
{
    if (m_faceGeometry.valid()) {
        m_faceGeometry->removePrimitiveSet(0, m_faceGeometry->getNumPrimitiveSets());
        m_faceGeometry->setVertexArray(nullptr);
        m_faceGeometry->setColorArray(nullptr);
        m_faceGeometry->setShape(nullptr);  // 清除KdTree
        emit geometryChanged();
    }
}

void GeoNodeManager::clearControlPointsGeometry()
{
    if (m_controlPointsGeometry.valid()) {
        m_controlPointsGeometry->removePrimitiveSet(0, m_controlPointsGeometry->getNumPrimitiveSets());
        m_controlPointsGeometry->setVertexArray(nullptr);
        m_controlPointsGeometry->setColorArray(nullptr);
        emit geometryChanged();
    }
}

void GeoNodeManager::clearAllGeometries()
{
    clearVertexGeometry();
    clearEdgeGeometry();
    clearFaceGeometry();
    clearControlPointsGeometry();
    clearSpatialIndex();
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
    return m_transformNode.valid() ? m_transformNode->getMatrix() : osg::Matrix::identity();
}

void GeoNodeManager::resetTransform()
{
    setTransformMatrix(osg::Matrix::identity());
}

void GeoNodeManager::setVisible(bool visible)
{
    if (m_transformNode.valid()) {
        m_transformNode->setNodeMask(visible ? 0xffffffff : 0x0);
        emit visibilityChanged();
    }
}

bool GeoNodeManager::isVisible() const
{
    return m_transformNode.valid() ? m_transformNode->getNodeMask() != 0x0 : false;
}

void GeoNodeManager::setVertexVisible(bool visible)
{
    if (m_vertexGeometry.valid()) {
        m_vertexGeometry->setNodeMask(visible ? 0xffffffff : 0x0);
        emit visibilityChanged();
    }
}

void GeoNodeManager::setEdgeVisible(bool visible)
{
    if (m_edgeGeometry.valid()) {
        m_edgeGeometry->setNodeMask(visible ? 0xffffffff : 0x0);
        emit visibilityChanged();
    }
}

void GeoNodeManager::setFaceVisible(bool visible)
{
    if (m_faceGeometry.valid()) {
        m_faceGeometry->setNodeMask(visible ? 0xffffffff : 0x0);
        emit visibilityChanged();
    }
}

void GeoNodeManager::setControlPointsVisible(bool visible)
{
    if (m_controlPointsGeometry.valid()) {
        m_controlPointsGeometry->setNodeMask(visible ? 0xffffffff : 0x0);
        emit visibilityChanged();
    }
}

bool GeoNodeManager::isVertexVisible() const
{
    return m_vertexGeometry.valid() ? m_vertexGeometry->getNodeMask() != 0x0 : false;
}

bool GeoNodeManager::isEdgeVisible() const
{
    return m_edgeGeometry.valid() ? m_edgeGeometry->getNodeMask() != 0x0 : false;
}

bool GeoNodeManager::isFaceVisible() const
{
    return m_faceGeometry.valid() ? m_faceGeometry->getNodeMask() != 0x0 : false;
}

bool GeoNodeManager::isControlPointsVisible() const
{
    return m_controlPointsGeometry.valid() ? m_controlPointsGeometry->getNodeMask() != 0x0 : false;
}

void GeoNodeManager::updateSpatialIndex()
{
    if (m_vertexGeometry.valid()) buildKdTreeForGeometry(m_vertexGeometry.get());
    if (m_edgeGeometry.valid()) buildKdTreeForGeometry(m_edgeGeometry.get());
    if (m_faceGeometry.valid()) buildKdTreeForGeometry(m_faceGeometry.get());
}

void GeoNodeManager::clearSpatialIndex()
{
    if (m_vertexGeometry.valid()) m_vertexGeometry->setShape(nullptr);
    if (m_edgeGeometry.valid()) m_edgeGeometry->setShape(nullptr);
    if (m_faceGeometry.valid()) m_faceGeometry->setShape(nullptr);
}

void GeoNodeManager::buildKdTreeForGeometry(osg::Geometry* geometry)
{
    if (!geometry
        || !geometry->getVertexArray()
        || geometry->getVertexArray()->getNumElements() == 0)
        return;

    // 1) 创建一个 KdTree 实例
    osg::ref_ptr<osg::KdTree> kdTree = new osg::KdTree;

    // 2) 调用它自己的 build() 方法（注意签名要用 BuildOptions）
    osg::KdTree::BuildOptions opts;
    if (kdTree->build(opts, geometry))
    {
        // 3) 挂到 geometry 上
        geometry->setShape(kdTree.get());
    }
    else
    {
        LOG_INFO("KdTree 构建失败", "GEO");
    }
}