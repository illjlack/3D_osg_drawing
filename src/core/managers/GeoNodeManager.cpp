#include "GeoNodeManager.h"
#include "../GeometryBase.h"
#include <osg/Geode>
#include <osg/Material>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/NodeVisitor>
#include <osg/ComputeBoundsVisitor>
#include <osg/KdTree>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Array>
#include <osg/PrimitiveSet>
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
        m_boundingBoxGeometry = new osg::Geometry();
        
        m_transformNode->addChild(m_vertexGeometry.get());
        m_transformNode->addChild(m_edgeGeometry.get());
        m_transformNode->addChild(m_faceGeometry.get());
        m_transformNode->addChild(m_controlPointsGeometry.get());
        m_transformNode->addChild(m_boundingBoxGeometry.get());

        m_initialized = true;
    }
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

void GeoNodeManager::clearBoundingBoxGeometry()
{
    if (m_boundingBoxGeometry.valid()) {
        m_boundingBoxGeometry->removePrimitiveSet(0, m_boundingBoxGeometry->getNumPrimitiveSets());
        m_boundingBoxGeometry->setVertexArray(nullptr);
        m_boundingBoxGeometry->setColorArray(nullptr);
        emit geometryChanged();
    }
}

void GeoNodeManager::clearAllGeometries()
{
    clearVertexGeometry();
    clearEdgeGeometry();
    clearFaceGeometry();
    clearControlPointsGeometry();
    clearBoundingBoxGeometry();
    clearSpatialIndex();
}

void GeoNodeManager::setTransformMatrix(const osg::Matrix& matrix)
{
    if (m_transformNode.valid()) {
        m_transformNode->setMatrix(matrix);
        
        // 注意：变换矩阵改变时不需要重建空间索引
        // 因为：
        // 1. KdTree 存储的是几何体的局部坐标
        // 2. OSG 的 CullVisitor 会自动应用变换矩阵进行视锥体剔除
        // 3. 几何体的顶点数据本身没有改变
        
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

void GeoNodeManager::setBoundingBoxVisible(bool visible)
{
    if (m_boundingBoxGeometry.valid()) {
        m_boundingBoxGeometry->setNodeMask(visible ? 0xffffffff : 0x0);
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

bool GeoNodeManager::isBoundingBoxVisible() const
{
    return m_boundingBoxGeometry.valid() ? m_boundingBoxGeometry->getNodeMask() != 0x0 : false;
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

void GeoNodeManager::updateBoundingBoxGeometry()
{
    if (!m_boundingBoxGeometry.valid()) return;

    // 计算所有几何体的包围盒
    osg::BoundingBox boundingBox;
    
    // 从顶点几何体计算包围盒
    if (m_vertexGeometry.valid() && m_vertexGeometry->getVertexArray()) {
        osg::ComputeBoundsVisitor boundsVisitor;
        m_vertexGeometry->accept(boundsVisitor);
        osg::BoundingBox vertexBounds = boundsVisitor.getBoundingBox();
        boundingBox.expandBy(vertexBounds);
    }
    
    // 从边几何体计算包围盒
    if (m_edgeGeometry.valid() && m_edgeGeometry->getVertexArray()) {
        osg::ComputeBoundsVisitor boundsVisitor;
        m_edgeGeometry->accept(boundsVisitor);
        osg::BoundingBox edgeBounds = boundsVisitor.getBoundingBox();
        boundingBox.expandBy(edgeBounds);
    }
    
    // 从面几何体计算包围盒
    if (m_faceGeometry.valid() && m_faceGeometry->getVertexArray()) {
        osg::ComputeBoundsVisitor boundsVisitor;
        m_faceGeometry->accept(boundsVisitor);
        osg::BoundingBox faceBounds = boundsVisitor.getBoundingBox();
        boundingBox.expandBy(faceBounds);
    }

    // 如果包围盒有效，创建包围盒几何体
    if (boundingBox.valid()) {
        createBoundingBoxGeometry(boundingBox);
    } else {
        clearBoundingBoxGeometry();
    }
}

void GeoNodeManager::updateGeometries()
{
    m_parent->updateGeometries();
    updateSpatialIndex();
    updateBoundingBoxGeometry();
}

void GeoNodeManager::createBoundingBoxGeometry(const osg::BoundingBox& boundingBox)
{
    if (!m_boundingBoxGeometry.valid()) return;

    // 清除现有几何体
    m_boundingBoxGeometry->removePrimitiveSet(0, m_boundingBoxGeometry->getNumPrimitiveSets());
    m_boundingBoxGeometry->setVertexArray(nullptr);
    m_boundingBoxGeometry->setColorArray(nullptr);

    // 创建包围盒的8个顶点
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->push_back(osg::Vec3(boundingBox.xMin(), boundingBox.yMin(), boundingBox.zMin()));
    vertices->push_back(osg::Vec3(boundingBox.xMax(), boundingBox.yMin(), boundingBox.zMin()));
    vertices->push_back(osg::Vec3(boundingBox.xMax(), boundingBox.yMax(), boundingBox.zMin()));
    vertices->push_back(osg::Vec3(boundingBox.xMin(), boundingBox.yMax(), boundingBox.zMin()));
    vertices->push_back(osg::Vec3(boundingBox.xMin(), boundingBox.yMin(), boundingBox.zMax()));
    vertices->push_back(osg::Vec3(boundingBox.xMax(), boundingBox.yMin(), boundingBox.zMax()));
    vertices->push_back(osg::Vec3(boundingBox.xMax(), boundingBox.yMax(), boundingBox.zMax()));
    vertices->push_back(osg::Vec3(boundingBox.xMin(), boundingBox.yMax(), boundingBox.zMax()));

    // 创建颜色数组（线框颜色）
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f)); // 黄色

    // 创建线框索引
    osg::ref_ptr<osg::DrawElementsUInt> lines = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES, 0);
    
    // 底面的4条边
    lines->push_back(0); lines->push_back(1);
    lines->push_back(1); lines->push_back(2);
    lines->push_back(2); lines->push_back(3);
    lines->push_back(3); lines->push_back(0);
    
    // 顶面的4条边
    lines->push_back(4); lines->push_back(5);
    lines->push_back(5); lines->push_back(6);
    lines->push_back(6); lines->push_back(7);
    lines->push_back(7); lines->push_back(4);
    
    // 连接顶面和底面的4条边
    lines->push_back(0); lines->push_back(4);
    lines->push_back(1); lines->push_back(5);
    lines->push_back(2); lines->push_back(6);
    lines->push_back(3); lines->push_back(7);

    // 设置几何体数据
    m_boundingBoxGeometry->setVertexArray(vertices.get());
    m_boundingBoxGeometry->setColorArray(colors.get(), osg::Array::BIND_OVERALL);
    m_boundingBoxGeometry->addPrimitiveSet(lines.get());

    emit geometryChanged();
}