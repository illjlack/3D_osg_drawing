
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
#include <osg/PagedLOD>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include "../../util/LogManager.h"
#include "../Enums3D.h"
#include <algorithm>

// ============= 构造函数 =============

GeoNodeManager::GeoNodeManager(osg::ref_ptr<Geo3D> parent)
    : QObject(parent)
    , m_parent(parent)
    , m_initialized(false)
    , m_selected(false)
{
    initializeNodes();
}

// ============= 几何体管理 =============

void GeoNodeManager::clearVertexGeometry()
{
    if (m_vertexGeometry.valid()) {
        m_vertexGeometry->removePrimitiveSet(0, m_vertexGeometry->getNumPrimitiveSets());
        m_vertexGeometry->setVertexArray(nullptr);
        m_vertexGeometry->setColorArray(nullptr);
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

void GeoNodeManager::updateGeometries()
{
    m_parent->buildControlPointGeometries();
    m_parent->buildVertexGeometries();
    m_parent->buildEdgeGeometries();
    m_parent->buildFaceGeometries();

    // 更新包围盒
    updateBoundingBoxGeometry();

    if (!m_parent->mm_state()->isStateComplete())return;
    // 绘制完成才建索引
    // 更新空间索引
    updateSpatialIndex();
}

// ============= 节点设置 =============

void GeoNodeManager::setOSGNode(osg::ref_ptr<osg::Node> node)
{
    if (!node.valid()) {
        LOG_ERROR("传入的节点为空", "几何体管理");
        return;
    }

    LOG_INFO("开始设置OSG节点", "几何体管理");

    // 检查是否为Group类型且有名字
    osg::Group* groupNode = dynamic_cast<osg::Group*>(node.get());
    if (groupNode && node->getName() == NodeTags3D::ROOT_GROUP) {
        // 有名字的Group → 按标记搜索组件
        LOG_INFO("检测到命名Group节点，搜索标记组件", "几何体管理");
        findAndAssignNodeComponents(node.get());
    } else {
        // 不是Group或没有名字，直接添加到变换节点下并标记为面节点
        if (m_transformNode.valid()) {
            // 设置面节点mask（用于拾取系统识别）
            node->setNodeMask(NODE_MASK_FACE);
            node->setUserData(m_parent);
            m_transformNode->addChild(node.get());
            LOG_INFO("将节点添加到变换节点下并设置面几何体mask", "几何体管理");
        } else if (m_osgNode.valid()) {
            node->setNodeMask(NODE_MASK_FACE);
            node->setUserData(m_parent);
            m_osgNode->addChild(node.get());
            LOG_INFO("将节点添加到根节点下并设置面几何体mask", "几何体管理");
        }
    }
    LOG_INFO("OSG节点设置完成", "几何体管理");

    // 外部对象加载的时候不会触发节点的重计算,包围盒也就没有算

    // 设置控制点和包围盒的渲染属性
    setupControlPointsRendering();
    setupBoundingBoxRendering();

    updateBoundingBoxGeometry();
}

// ============= 选中状态管理 =============

void GeoNodeManager::setSelected(bool selected)
{
    if (m_selected == selected) return;
    
    m_selected = selected;
    
    if (selected) {
        // 选中时显示包围盒和控制点
        if (m_boundingBoxGeometry.valid()) {
            m_boundingBoxGeometry->setNodeMask(NODE_MASK_BOUNDING_BOX);
        }
        if (m_controlPointsGeometry.valid()) {
            m_controlPointsGeometry->setNodeMask(NODE_MASK_CONTROL_POINTS);
        }
        LOG_INFO("几何体已选中，显示包围盒和控制点", "选择管理");
    } else {
        // 取消选中时隐藏包围盒和控制点
        if (m_boundingBoxGeometry.valid()) {
            m_boundingBoxGeometry->setNodeMask(NODE_MASK_NONE);
        }
        if (m_controlPointsGeometry.valid()) {
            m_controlPointsGeometry->setNodeMask(NODE_MASK_NONE);
        }
        LOG_INFO("几何体取消选中，隐藏包围盒和控制点", "选择管理");
    }
}

// ============= 公共槽函数 =============

void GeoNodeManager::onDrawingCompleted()
{
    // 绘制完成后，允许节点被拾取
    if (m_osgNode.valid()) {
        m_osgNode->setNodeMask(NODE_MASK_ALL_VISIBLE);
        LOG_INFO("绘制完成，节点已可拾取", "几何体管理");
    }

    // 第一次建立索引
    updateSpatialIndex();
}

// ============= 私有函数：初始化 =============

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

// ============= 私有函数：外部节点处理 =============

void GeoNodeManager::findAndAssignNodeComponents(osg::Node* node)
{
    if (!node) return;

    // 递归查找具有特定标记的子节点
    class ComponentFinder : public osg::NodeVisitor
    {
    public:
        ComponentFinder(GeoNodeManager* manager) 
            : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
            , m_manager(manager) {}

        virtual void apply(osg::Group& group) override
        {
            group.setUserData(m_manager->m_parent);
            const std::string& name = group.getName();
            if (name == NodeTags3D::ROOT_GROUP) {
                // 找到根节点，直接赋值给m_osgNode
                m_manager->m_osgNode = &group;
                LOG_INFO("找到并分配根节点组", "几何体管理");
            }
            traverse(group);
        }

        virtual void apply(osg::MatrixTransform& transform) override
        {
            transform.setUserData(m_manager->m_parent);
            const std::string& name = transform.getName();
            if (name == NodeTags3D::TRANSFORM_NODE) {
                // 找到变换节点
                m_manager->m_transformNode = &transform;
                LOG_INFO("找到并分配变换节点", "几何体管理");
            }
            traverse(transform);
        }

        virtual void apply(osg::Geometry& geometry) override
        {
            geometry.setUserData(m_manager->m_parent);
            const std::string& name = geometry.getName();
            if (name == NodeTags3D::VERTEX_GEOMETRY) {
                m_manager->m_vertexGeometry = &geometry;
                LOG_INFO("找到并分配点几何体", "几何体管理");
            } else if (name == NodeTags3D::EDGE_GEOMETRY) {
                m_manager->m_edgeGeometry = &geometry;
                LOG_INFO("找到并分配线几何体", "几何体管理");
            } else if (name == NodeTags3D::FACE_GEOMETRY) {
                m_manager->m_faceGeometry = &geometry;
                LOG_INFO("找到并分配面几何体", "几何体管理");
            } else if (name == NodeTags3D::CONTROL_POINTS_GEOMETRY) {
                m_manager->m_controlPointsGeometry = &geometry;
                LOG_INFO("找到并分配控制点几何体", "几何体管理");
            } else if (name == NodeTags3D::BOUNDING_BOX_GEOMETRY) {
                m_manager->m_boundingBoxGeometry = &geometry;
                LOG_INFO("找到并分配包围盒几何体", "几何体管理");
            }
        }

    private:
        GeoNodeManager* m_manager;
    };

    ComponentFinder finder(this);
    node->accept(finder);
    
    /**
    * 关于继承public osg::Referenced导致的bug:
    * 这个是在对象内部维护一个计数，对于特定api的调用，会ref和unref计数。目的是osg节点等对象自动析构（因为他们都是用特定api绑定的）。
    * 
    * 然后setUserData就是其中一个api, 在节点上挂载一个自定义的用户数据，用来共享并自动析构。
    * 
    * 因为Geo3D持有多个node节点，我需要 在场景树（node的）中拾取功能能通过node找到Geo3D。
    * 然后就使用了setUserData保存对象指针。但是在这里，因为所有节点都析构更换，所以最后Geo3D也就被析构了。
    * 
    * (!所以这又有一个bug, 我说怎么拾取不了（没全部更换的没用触发析构，程序还能运行），更换节点的节点忘设置对象的指针了，不然这里应该不会被析构，
    * 总会持有。。。。好像又解释了为什么之前一关程序就崩溃，重复析构了。。)
    * 
    * 其实我要保存的是一个普通的独占数据，用键值对setUserValue就行。
    * 
    * 
    * 所有对同一个 osg::Referenced 对象调用的 ref()／unref()，都作用在它内部那一个 _refCount 上，无论这些调用是来自你手动用 ref_ptr，还是来自 OSG 的各种容器或*setter API。
    * 所以用setUserData这个也没用问题
    * 问题在于裸指针和ref_ptr()混用
    */
}

// ============= 私有函数：空间索引管理 =============

void GeoNodeManager::updateSpatialIndex()
{
#ifdef __linux__
    // Linux平台优化：仅在几何体发生实质变化时才重建空间索引
    // 减少KdTree重建频率，提升性能
    if (!m_parent->mm_state()->isStateComplete()) {
        LOG_INFO("几何体未完成，跳过空间索引构建", "空间索引");
        return;
    }
    
    // 检查几何体是否有顶点数据，避免无效的索引构建
    bool needRebuild = false;
    
    if (m_edgeGeometry.valid() && m_edgeGeometry->getVertexArray() && 
        m_edgeGeometry->getVertexArray()->getNumElements() > 0) {
        if (!m_edgeGeometry->getShape()) {
            needRebuild = true;
        }
    }
    
    if (m_faceGeometry.valid() && m_faceGeometry->getVertexArray() && 
        m_faceGeometry->getVertexArray()->getNumElements() > 0) {
        if (!m_faceGeometry->getShape()) {
            needRebuild = true;
        }
    }
    
    if (needRebuild) {
        LOG_INFO("Linux平台：开始构建必要的空间索引", "空间索引");
        // 为边和面几何体构建KdTree以加速射线拾取
        buildKdTreeForGeometry(m_edgeGeometry.get());
        buildKdTreeForGeometry(m_faceGeometry.get());
    }
#else
    // 其他平台保持原有行为
    // 为边和面几何体构建KdTree以加速射线拾取
    buildKdTreeForGeometry(m_edgeGeometry.get());
    buildKdTreeForGeometry(m_faceGeometry.get());
#endif
}

void GeoNodeManager::clearSpatialIndex()
{
    if (m_edgeGeometry.valid()) {
        m_edgeGeometry->setShape(nullptr);
    }
    if (m_faceGeometry.valid()) {
        m_faceGeometry->setShape(nullptr);
    }
}

void GeoNodeManager::buildKdTreeForGeometry(osg::Geometry* geometry)
{
    if (!geometry || !geometry->getVertexArray()) return;

    osg::ref_ptr<osg::KdTree> kdTree = new osg::KdTree;
    osg::KdTree::BuildOptions buildOptions;
    if (kdTree->build(buildOptions, geometry)) {
        geometry->setShape(kdTree.get());
        LOG_INFO("为几何体构建KdTree成功", "空间索引");
    } else {
        LOG_WARNING("为几何体构建KdTree失败", "空间索引");
    }
}

// ============= 私有函数：包围盒管理 =============

void GeoNodeManager::updateBoundingBoxGeometry()
{
    if (!m_transformNode.valid()) return;


    osg::ComputeBoundsVisitor visitor;

    // 指定在“剔除”阶段遍历，这样 PagedLOD 会加载它自己
    visitor.setVisitorType(osg::NodeVisitor::CULL_VISITOR);
    // 遍历所有子节点（包括 Group、Geode、Geometry…）
    visitor.setTraversalMode(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);
    // 设置遍历mask为所有节点，确保不会因为mask问题跳过节点
    visitor.setTraversalMask(0xFFFFFFFF);

    m_transformNode->accept(visitor);

    osg::BoundingBox boundingBox = visitor.getBoundingBox();

    if (boundingBox.valid()) {
        createBoundingBoxGeometry(boundingBox);
    }
}

void GeoNodeManager::createBoundingBoxGeometry(const osg::BoundingBox& boundingBox)
{
    if (!m_boundingBoxGeometry.valid() || !boundingBox.valid()) return;

    // 清除现有数据
    m_boundingBoxGeometry->removePrimitiveSet(0, m_boundingBoxGeometry->getNumPrimitiveSets());

    // 创建包围盒的8个顶点
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(8);
    const osg::Vec3& min = boundingBox._min;
    const osg::Vec3& max = boundingBox._max;

    (*vertices)[0] = osg::Vec3(min.x(), min.y(), min.z());
    (*vertices)[1] = osg::Vec3(max.x(), min.y(), min.z());
    (*vertices)[2] = osg::Vec3(max.x(), max.y(), min.z());
    (*vertices)[3] = osg::Vec3(min.x(), max.y(), min.z());
    (*vertices)[4] = osg::Vec3(min.x(), min.y(), max.z());
    (*vertices)[5] = osg::Vec3(max.x(), min.y(), max.z());
    (*vertices)[6] = osg::Vec3(max.x(), max.y(), max.z());
    (*vertices)[7] = osg::Vec3(min.x(), max.y(), max.z());

    m_boundingBoxGeometry->setVertexArray(vertices.get());

    // 创建线框
    osg::ref_ptr<osg::DrawElementsUShort> lineIndices = 
        new osg::DrawElementsUShort(osg::PrimitiveSet::LINES);

    // 底面
    lineIndices->push_back(0); lineIndices->push_back(1);
    lineIndices->push_back(1); lineIndices->push_back(2);
    lineIndices->push_back(2); lineIndices->push_back(3);
    lineIndices->push_back(3); lineIndices->push_back(0);

    // 顶面
    lineIndices->push_back(4); lineIndices->push_back(5);
    lineIndices->push_back(5); lineIndices->push_back(6);
    lineIndices->push_back(6); lineIndices->push_back(7);
    lineIndices->push_back(7); lineIndices->push_back(4);

    // 竖直边
    lineIndices->push_back(0); lineIndices->push_back(4);
    lineIndices->push_back(1); lineIndices->push_back(5);
    lineIndices->push_back(2); lineIndices->push_back(6);
    lineIndices->push_back(3); lineIndices->push_back(7);

    m_boundingBoxGeometry->addPrimitiveSet(lineIndices.get());
}

// ============= 私有函数：渲染设置 =============

void GeoNodeManager::setupControlPointsRendering()
{
    if (!m_controlPointsGeometry.valid()) return;

    osg::ref_ptr<osg::StateSet> stateSet = m_controlPointsGeometry->getOrCreateStateSet();
    
#ifdef __linux__
    // Linux平台简化控制点渲染
    // 设置点的大小，使用较小的尺寸减少填充率压力
    osg::ref_ptr<osg::Point> pointSize = new osg::Point(3.0f);
    stateSet->setAttributeAndModes(pointSize.get(), osg::StateAttribute::ON);
    
    // 简化材质设置，只设置基本颜色
    osg::ref_ptr<osg::Material> material = new osg::Material();
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));  // 黄色
    stateSet->setAttributeAndModes(material.get(), osg::StateAttribute::ON);
    
    // 关闭光照，减少计算开销
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
    
    LOG_INFO("Linux平台：使用简化的控制点渲染", "几何体管理");
#else
    // 设置点的大小和颜色
    osg::ref_ptr<osg::Point> pointSize = new osg::Point(4.0f);
    stateSet->setAttributeAndModes(pointSize.get(), osg::StateAttribute::ON);
    
    // 设置材质
    osg::ref_ptr<osg::Material> material = new osg::Material();
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));  // 黄色
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0.8f, 0.8f, 0.0f, 1.0f));
    stateSet->setAttributeAndModes(material.get(), osg::StateAttribute::ON);
    
    // 关闭光照
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
#endif
}

void GeoNodeManager::setupBoundingBoxRendering()
{
    if (!m_boundingBoxGeometry.valid()) return;

    osg::ref_ptr<osg::StateSet> stateSet = m_boundingBoxGeometry->getOrCreateStateSet();
    
#ifdef __linux__
    // Linux平台简化包围盒渲染
    // 使用较小的线宽，减少填充率压力
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth(1.5f);
    stateSet->setAttributeAndModes(lineWidth.get(), osg::StateAttribute::ON);
    
    // 简化材质设置，只设置基本颜色
    osg::ref_ptr<osg::Material> material = new osg::Material();
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));  // 黄色
    stateSet->setAttributeAndModes(material.get(), osg::StateAttribute::ON);
    
    // 关闭光照，减少计算开销
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
    
    LOG_INFO("Linux平台：使用简化的包围盒渲染", "几何体管理");
#else
    // 设置线宽
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth(2.0f);
    stateSet->setAttributeAndModes(lineWidth.get(), osg::StateAttribute::ON);
    
    // 设置材质
    osg::ref_ptr<osg::Material> material = new osg::Material();
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));  // 黄色
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0.8f, 0.8f, 0.0f, 1.0f));
    stateSet->setAttributeAndModes(material.get(), osg::StateAttribute::ON);
    
    // 关闭光照
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
#endif
}
