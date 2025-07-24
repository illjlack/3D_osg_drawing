
#include "GeoNodeManager.h"
#include "../GeometryBase.h"
#include <osg/Geode>
#include <osg/Material>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/PolygonMode>
#include <osg/NodeVisitor>
#include <osg/ComputeBoundsVisitor>
#include <osg/KdTree>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include "../../util/LogManager.h"
#include "../Enums3D.h"
#include <algorithm>

GeoNodeManager::GeoNodeManager(Geo3D* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_initialized(false)
    , m_selected(false)  // 初始状态为未选中
{
    initializeNodes();
}

void GeoNodeManager::initializeNodes()
{
    if (!m_initialized) {
        // 创建根节点和变换节点
        m_osgNode = new osg::Group();
        m_osgNode->setName(NodeTags3D::ROOT_GROUP);
        
        m_transformNode = new osg::MatrixTransform();
        m_transformNode->setName(NodeTags3D::TRANSFORM_NODE);
        m_osgNode->addChild(m_transformNode.get());
        
        // 创建几何体节点并设置标记
        m_vertexGeometry = new osg::Geometry();
        m_vertexGeometry->setName(NodeTags3D::VERTEX_GEOMETRY);
        
        m_edgeGeometry = new osg::Geometry();
        m_edgeGeometry->setName(NodeTags3D::EDGE_GEOMETRY);
        
        m_faceGeometry = new osg::Geometry();
        m_faceGeometry->setName(NodeTags3D::FACE_GEOMETRY);
        
        m_controlPointsGeometry = new osg::Geometry();
        m_controlPointsGeometry->setName(NodeTags3D::CONTROL_POINTS_GEOMETRY);
        
        m_boundingBoxGeometry = new osg::Geometry();
        m_boundingBoxGeometry->setName(NodeTags3D::BOUNDING_BOX_GEOMETRY);
        
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
        
        // 设置控制点和包围盒的渲染属性
        setupControlPointsRendering();
        setupBoundingBoxRendering();

        m_initialized = true;
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

// 点线面可见性由GeoRenderManager管理

void GeoNodeManager::setSelected(bool selected)
{
    if (m_selected == selected) return;
    
    m_selected = selected;
    
    // 选中时显示包围盒和控制点，未选中时隐藏
    if (m_controlPointsGeometry.valid()) {
        m_controlPointsGeometry->setNodeMask(selected ? NODE_MASK_CONTROL_POINTS : NODE_MASK_NONE);
    }
    
    if (m_boundingBoxGeometry.valid()) {
        m_boundingBoxGeometry->setNodeMask(selected ? NODE_MASK_BOUNDING_BOX : NODE_MASK_NONE);
    }
}

// 批量可见性控制已删除，由GeoRenderManager统一管理点线面可见性

void GeoNodeManager::updateSpatialIndex()
{
    // 只在绘制完成时才构建KdTree，提高性能
    if (!m_parent->mm_state()->isStateComplete()) {
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
        // 计算包围盒的尺寸，用于确定扩展量
        osg::Vec3 size = boundingBox.corner(7) - boundingBox.corner(0); // 对角线向量
        double maxDimension = std::max({size.x(), size.y(), size.z()});
        double expandAmount = maxDimension * 0.05; // 扩展5%
        
        // 确保最小扩展量，避免包围盒太小时看不清
        expandAmount = std::max(expandAmount, 0.1);
        
        // 向外扩展包围盒
        osg::BoundingBox expandedBox = boundingBox;
        expandedBox.expandBy(osg::Vec3(expandAmount, expandAmount, expandAmount));
        
        createBoundingBoxGeometry(expandedBox);
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

// 包围盒可见性现在由setSelected方法统一管理

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
    colors->push_back(osg::Vec4(1.0, 1.0, 0.0, 1.0)); // 黄色

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

void GeoNodeManager::setOSGNode(osg::ref_ptr<osg::Node> node)
{
    if (!node.valid()) {
        LOG_INFO("尝试设置空的OSG节点", "几何体管理");
        return;
    }
    
    // 设置节点的用户数据，指向父几何体对象
    node->setUserData(m_parent);
    
    // 设置节点掩码为可见且可拾取
    node->setNodeMask(NODE_MASK_ALL);
    
    // 检查是否为Group节点且有名字
    osg::Group* groupNode = dynamic_cast<osg::Group*>(node.get());
    if (groupNode && !node->getName().empty()) {
        // Group节点有名字，递归搜索标记组件
        findAndAssignNodeComponents(node.get());
        
        // 如果名字匹配ROOT_GROUP，设置为根节点
        if (node->getName() == NodeTags3D::ROOT_GROUP) {
            m_osgNode = groupNode;
            LOG_INFO("将传入的Group节点设置为根节点", "几何体管理");
        } else {
            // 其他命名Group节点，添加到现有结构中
            if (m_transformNode.valid()) {
                m_transformNode->addChild(node.get());
            } else if (m_osgNode.valid()) {
                m_osgNode->addChild(node.get());
            }
            LOG_INFO(QString("将命名Group节点添加到现有结构中: %1")
                    .arg(QString::fromStdString(node->getName())), "几何体管理");
        }
    } else {
        // 不是Group或没有名字，直接添加到变换节点下并标记为面节点
        if (m_transformNode.valid()) {
            // 设置面节点mask（用于拾取系统识别）
            node->setNodeMask(NODE_MASK_FACE);
            m_transformNode->addChild(node.get());
            LOG_INFO("将节点添加到变换节点下并设置面几何体mask", "几何体管理");
        } else if (m_osgNode.valid()) {
            node->setNodeMask(NODE_MASK_FACE);
            m_osgNode->addChild(node.get());
            LOG_INFO("将节点添加到根节点下并设置面几何体mask", "几何体管理");
        }
    }
    
    LOG_INFO(QString("成功设置外部节点，节点名称: %1")
            .arg(QString::fromStdString(node->getName())), "几何体管理");
    
    // 更新几何体和空间索引
    updateGeometries();
    
    // 通知几何体已更改
    emit geometryChanged();
}

void GeoNodeManager::findAndAssignNodeComponents(osg::Node* node)
{
    if (!node) return;
    
    // 直接根据节点名称标记识别组件
    std::string nodeName = node->getName();
    
    // 检查是否为变换节点
    if (nodeName == NodeTags3D::TRANSFORM_NODE) {
        osg::MatrixTransform* transform = dynamic_cast<osg::MatrixTransform*>(node);
        if (transform) {
            m_transformNode = transform;
            LOG_INFO("找到变换节点", "几何体管理");
        }
    }
    
    // 检查是否为几何体节点
    osg::Geometry* geometry = dynamic_cast<osg::Geometry*>(node);
    if (geometry) {
        // 直接根据标记识别几何体类型
        if (nodeName == NodeTags3D::VERTEX_GEOMETRY) {
            m_vertexGeometry = geometry;
            LOG_INFO("找到顶点几何体", "几何体管理");
        }
        else if (nodeName == NodeTags3D::EDGE_GEOMETRY) {
            m_edgeGeometry = geometry;
            LOG_INFO("找到边几何体", "几何体管理");
        }
        else if (nodeName == NodeTags3D::FACE_GEOMETRY) {
            m_faceGeometry = geometry;
            LOG_INFO("找到面几何体", "几何体管理");
        }
        else if (nodeName == NodeTags3D::CONTROL_POINTS_GEOMETRY) {
            m_controlPointsGeometry = geometry;
            LOG_INFO("找到控制点几何体", "几何体管理");
        }
        else if (nodeName == NodeTags3D::BOUNDING_BOX_GEOMETRY) {
            m_boundingBoxGeometry = geometry;
            LOG_INFO("找到包围盒几何体", "几何体管理");
        }
        else {
            LOG_INFO(QString("未识别的几何体节点: %1").arg(QString::fromStdString(nodeName)), "几何体管理");
        }
        
        // 设置几何体的用户数据
        geometry->setUserData(m_parent);
    }
    
    // 递归遍历子节点
    osg::Group* group = dynamic_cast<osg::Group*>(node);
    if (group) {
        for (unsigned int i = 0; i < group->getNumChildren(); ++i) {
            findAndAssignNodeComponents(group->getChild(i));
        }
    }
}

void GeoNodeManager::setupControlPointsRendering()
{
    if (!m_controlPointsGeometry.valid()) return;
    
    // 创建控制点的渲染状态
    auto stateSet = m_controlPointsGeometry->getOrCreateStateSet();
    
    // 创建控制点材质 - 红色核心，黄色光晕
    auto material = new osg::Material();
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));  // 红色核心
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 0.0f, 0.5f));  // 黄色光晕
    material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f)); // 黄色高光
    material->setShininess(osg::Material::FRONT_AND_BACK, 32.0f);
    
    stateSet->setAttributeAndModes(material, osg::StateAttribute::ON);
    
    // 设置点大小 - 控制点比普通点大
    auto pointSize = new osg::Point(8.0f);
    stateSet->setAttributeAndModes(pointSize, osg::StateAttribute::ON);
    
    // 启用点平滑
    stateSet->setMode(GL_POINT_SMOOTH, osg::StateAttribute::ON);
    
    // 关闭光照以获得更亮的颜色
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    // 确保控制点总是在前面显示
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    
    LOG_INFO("控制点渲染设置完成", "节点管理");
}

void GeoNodeManager::setupBoundingBoxRendering()
{
    if (!m_boundingBoxGeometry.valid()) return;
    
    // 创建包围盒的渲染状态
    auto stateSet = m_boundingBoxGeometry->getOrCreateStateSet();
    
    // 创建包围盒材质 - 黄色线框
    auto material = new osg::Material();
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));  // 黄色
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 0.0f, 0.3f));  // 暗黄色环境光
    material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4(0.2f, 0.2f, 0.0f, 1.0f)); // 微弱发光
    
    stateSet->setAttributeAndModes(material, osg::StateAttribute::ON);
    
    // 设置线宽
    auto lineWidth = new osg::LineWidth(2.0f);
    stateSet->setAttributeAndModes(lineWidth, osg::StateAttribute::ON);
    
    // 启用线平滑
    stateSet->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    
    // 关闭光照以获得更亮的颜色
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    // 设置深度测试，确保包围盒正确显示
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    
    // 设置为线框模式
    auto polygonMode = new osg::PolygonMode();
    polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
    stateSet->setAttributeAndModes(polygonMode, osg::StateAttribute::ON);
    
    LOG_INFO("包围盒渲染设置完成", "节点管理");
}




