
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
#include "../Enums3D.h"

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
        
        // 设置用户数据，存储指向父几何体对象的指针
        m_vertexGeometry->setUserData(m_parent);
        m_edgeGeometry->setUserData(m_parent);
        m_faceGeometry->setUserData(m_parent);
        m_controlPointsGeometry->setUserData(m_parent);
        m_boundingBoxGeometry->setUserData(m_parent);

        m_transformNode->addChild(m_vertexGeometry.get());
        m_transformNode->addChild(m_edgeGeometry.get());
        m_transformNode->addChild(m_faceGeometry.get());
        m_transformNode->addChild(m_controlPointsGeometry.get());
        m_transformNode->addChild(m_boundingBoxGeometry.get());

        // 只可见，因为没有绘制完成，不能被拾取
        m_osgNode->setNodeMask(NODE_MASK_NOSELECT);
        // 设置各个几何体的专用mask
        m_vertexGeometry->setNodeMask(NODE_MASK_VERTEX);
        m_edgeGeometry->setNodeMask(NODE_MASK_EDGE);
        m_faceGeometry->setNodeMask(NODE_MASK_FACE);
        // 设置初始状态
        m_controlPointsGeometry->setNodeMask(NODE_MASK_NONE);
        m_boundingBoxGeometry->setNodeMask(NODE_MASK_NONE);

        m_initialized = true;
        
        // 确保包围盒可见性状态正确
        updateBoundingBoxVisibility();
    }
}

void GeoNodeManager::clearVertexGeometry()
{
    if (m_vertexGeometry.valid()) {
        m_vertexGeometry->removePrimitiveSet(0, m_vertexGeometry->getNumPrimitiveSets());
        m_vertexGeometry->setVertexArray(nullptr);
        m_vertexGeometry->setColorArray(nullptr);
        // m_vertexGeometry->setShape(nullptr);  // 清除KdTree (点几何不构建KdTree)
        emit geometryChanged();
    }
}

void GeoNodeManager::clearEdgeGeometry()
{
    if (m_edgeGeometry.valid()) {
        m_edgeGeometry->removePrimitiveSet(0, m_edgeGeometry->getNumPrimitiveSets());
        m_edgeGeometry->setVertexArray(nullptr);
        m_edgeGeometry->setColorArray(nullptr);
        // m_edgeGeometry->setShape(nullptr);  // 清除KdTree (线几何不构建KdTree)
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
        // 设置变换节点的可见性 - 显示所有几何元素或完全隐藏
        m_transformNode->setNodeMask(visible ? NODE_MASK_ALL_VISIBLE : NODE_MASK_NONE);
    }
    
    // 如果设置为可见，恢复各个几何体的默认mask
    if (visible) {
        if (m_vertexGeometry.valid()) {
            m_vertexGeometry->setNodeMask(NODE_MASK_VERTEX);
        }
        if (m_edgeGeometry.valid()) {
            m_edgeGeometry->setNodeMask(NODE_MASK_EDGE);
        }
        if (m_faceGeometry.valid()) {
            m_faceGeometry->setNodeMask(NODE_MASK_FACE);
        }
        // 控制点和包围盒保持隐藏状态
        if (m_controlPointsGeometry.valid()) {
            m_controlPointsGeometry->setNodeMask(NODE_MASK_NONE);
        }
        if (m_boundingBoxGeometry.valid()) {
            m_boundingBoxGeometry->setNodeMask(NODE_MASK_NONE);
        }
    }
}

bool GeoNodeManager::isVisible() const
{
    return m_transformNode.valid() ? m_transformNode->getNodeMask() != 0x0 : false;
}

void GeoNodeManager::setVertexVisible(bool visible)
{
    if (m_vertexGeometry.valid()) {
        m_vertexGeometry->setNodeMask(visible ? NODE_MASK_VERTEX : NODE_MASK_NONE);
    }
}

void GeoNodeManager::setEdgeVisible(bool visible)
{
    if (m_edgeGeometry.valid()) {
        m_edgeGeometry->setNodeMask(visible ? NODE_MASK_EDGE : NODE_MASK_NONE);
    }
}

void GeoNodeManager::setFaceVisible(bool visible)
{
    if (m_faceGeometry.valid()) {
        m_faceGeometry->setNodeMask(visible ? NODE_MASK_FACE : NODE_MASK_NONE);
    }
}

void GeoNodeManager::setControlPointsVisible(bool visible)
{
    if (m_controlPointsGeometry.valid()) {
        m_controlPointsGeometry->setNodeMask(visible ? NODE_MASK_CONTROL_POINTS : NODE_MASK_NONE);
    }
}

void GeoNodeManager::setBoundingBoxVisible(bool visible)
{
    if (m_boundingBoxGeometry.valid()) {
        m_boundingBoxGeometry->setNodeMask(visible ? NODE_MASK_BOUNDING_BOX : NODE_MASK_NONE);
    }
}

bool GeoNodeManager::isVertexVisible() const
{
    return m_vertexGeometry.valid() ? (m_vertexGeometry->getNodeMask() & NODE_MASK_VERTEX) != 0 : false;
}

bool GeoNodeManager::isEdgeVisible() const
{
    return m_edgeGeometry.valid() ? (m_edgeGeometry->getNodeMask() & NODE_MASK_EDGE) != 0 : false;
}

bool GeoNodeManager::isFaceVisible() const
{
    return m_faceGeometry.valid() ? (m_faceGeometry->getNodeMask() & NODE_MASK_FACE) != 0 : false;
}

bool GeoNodeManager::isControlPointsVisible() const
{
    return m_controlPointsGeometry.valid() ? (m_controlPointsGeometry->getNodeMask() & NODE_MASK_CONTROL_POINTS) != 0 : false;
}

bool GeoNodeManager::isBoundingBoxVisible() const
{
    return m_boundingBoxGeometry.valid() ? (m_boundingBoxGeometry->getNodeMask() & NODE_MASK_BOUNDING_BOX) != 0 : false;
}

void GeoNodeManager::setGeometryMask(unsigned int mask)
{
    // 根据mask设置各个几何体的可见性
    if (m_vertexGeometry.valid()) {
        m_vertexGeometry->setNodeMask((mask & NODE_MASK_VERTEX) ? NODE_MASK_VERTEX : NODE_MASK_NONE);
    }
    if (m_edgeGeometry.valid()) {
        m_edgeGeometry->setNodeMask((mask & NODE_MASK_EDGE) ? NODE_MASK_EDGE : NODE_MASK_NONE);
    }
    if (m_faceGeometry.valid()) {
        m_faceGeometry->setNodeMask((mask & NODE_MASK_FACE) ? NODE_MASK_FACE : NODE_MASK_NONE);
    }
    if (m_controlPointsGeometry.valid()) {
        m_controlPointsGeometry->setNodeMask((mask & NODE_MASK_CONTROL_POINTS) ? NODE_MASK_CONTROL_POINTS : NODE_MASK_NONE);
    }
    if (m_boundingBoxGeometry.valid()) {
        m_boundingBoxGeometry->setNodeMask((mask & NODE_MASK_BOUNDING_BOX) ? NODE_MASK_BOUNDING_BOX : NODE_MASK_NONE);
    }
}

unsigned int GeoNodeManager::getGeometryMask() const
{
    unsigned int mask = 0;
    if (isVertexVisible()) mask |= NODE_MASK_VERTEX;
    if (isEdgeVisible()) mask |= NODE_MASK_EDGE;
    if (isFaceVisible()) mask |= NODE_MASK_FACE;
    if (isControlPointsVisible()) mask |= NODE_MASK_CONTROL_POINTS;
    if (isBoundingBoxVisible()) mask |= NODE_MASK_BOUNDING_BOX;
    return mask;
}

void GeoNodeManager::showOnlyVertices()
{
    setGeometryMask(NODE_MASK_VERTEX);
}

void GeoNodeManager::showOnlyEdges()
{
    setGeometryMask(NODE_MASK_EDGE);
}

void GeoNodeManager::showOnlyFaces()
{
    setGeometryMask(NODE_MASK_FACE);
}

void GeoNodeManager::showAllGeometries()
{
    setGeometryMask(NODE_MASK_ALL_GEOMETRY);
}

void GeoNodeManager::hideAllGeometries()
{
    setGeometryMask(0x0);
}

void GeoNodeManager::updateSpatialIndex()
{
    // 只在绘制完成时才构建KdTree，提高性能
    if (!m_parent->isDrawingComplete()) {
        return;
    }
    
    // 为几何体构建OSG的KdTree，让OSG内部拾取系统使用
    // 注释掉点和线的KdTree构建，只构建面的KdTree
    // if (m_vertexGeometry.valid()) buildKdTreeForGeometry(m_vertexGeometry.get());
    // if (m_edgeGeometry.valid()) buildKdTreeForGeometry(m_edgeGeometry.get());
    if (m_faceGeometry.valid()) buildKdTreeForGeometry(m_faceGeometry.get());
}

void GeoNodeManager::clearSpatialIndex()
{
    // 为几何体清除OSG的KdTree
    // 注释掉点和线的KdTree清除，只清除面的KdTree
    // if (m_vertexGeometry.valid()) m_vertexGeometry->setShape(nullptr);
    // if (m_edgeGeometry.valid()) m_edgeGeometry->setShape(nullptr);
    if (m_faceGeometry.valid()) m_faceGeometry->setShape(nullptr);
}

void GeoNodeManager::buildKdTreeForGeometry(osg::Geometry* geometry)
{
    if (!geometry
        || !geometry->getVertexArray()
        || geometry->getVertexArray()->getNumElements() == 0)
    {
        LOG_INFO("KdTree 构建跳过：几何体无效或顶点为空", "GEO");
        return;
    }

    // 1) 创建一个 KdTree 实例
    osg::ref_ptr<osg::KdTree> kdTree = new osg::KdTree;

    // 2) 调用它自己的 build() 方法（注意签名要用 BuildOptions）
    osg::KdTree::BuildOptions opts;
    opts._maxNumLevels = 16;  // 设置最大层级
    opts._targetNumTrianglesPerLeaf = 10;  // 设置每个叶子节点的目标三角形数
    
    if (kdTree->build(opts, geometry))
    {
        // 3) 挂到 geometry 上
        geometry->setShape(kdTree.get());
        LOG_INFO("KdTree 构建成功", "GEO");
    }
    else
    {
        // 尝试使用默认参数重新构建
        osg::KdTree::BuildOptions defaultOpts;
        if (kdTree->build(defaultOpts, geometry))
        {
            geometry->setShape(kdTree.get());
            LOG_INFO("KdTree 使用默认参数构建成功", "GEO");
        }
        else
        {
            LOG_ERROR("KdTree 构建失败，几何体可能过于复杂或顶点数据有问题", "GEO");
            // 不设置Shape，保持为nullptr
        }
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
        
        // 更新包围盒可见性状态
        updateBoundingBoxVisibility();
    } else {
        clearBoundingBoxGeometry();
    }
}

void GeoNodeManager::updateGeometries()
{
    m_parent->updateGeometries();
    updateSpatialIndex();
    updateBoundingBoxGeometry();
    updateBoundingBoxVisibility();
}

void GeoNodeManager::updateBoundingBoxVisibility()
{
    if (!m_boundingBoxGeometry.valid()) return;
    
    // 检查父对象是否被选中，如果是则显示包围盒
    if (m_parent && m_parent->mm_state() && m_parent->mm_state()->isStateSelected()) {
        setBoundingBoxVisible(true);
    } else {
        setBoundingBoxVisible(false);
    }
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

void GeoNodeManager::onDrawingCompleted()
{
    // 绘制完成后，根节点节点的掩码为NODE_MASK_ALL，使其既可见又可拾取
    if (m_osgNode.valid()) {
        m_osgNode->setNodeMask(NODE_MASK_ALL);
    }
}
