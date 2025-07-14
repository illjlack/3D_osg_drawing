#include "GeometryBase.h"
#include "managers/GeoStateManager.h"
#include "managers/GeoNodeManager.h"
#include "managers/GeoMaterialManager.h"
#include "managers/GeoSnapPointManager.h"
#include "managers/GeoControlPointManager.h"
#include "managers/GeoBoundingBoxManager.h"
#include "managers/GeoRenderManager.h"
#include <osg/Array>
#include <osg/Shape>
#include <osg/PositionAttitudeTransform>
#include <osg/Geode>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ========================================= 基类 Geo3D =========================================
Geo3D::Geo3D()
    : m_geoType(Geo_Undefined3D)
    , m_tempPoint(0, 0, 0)
    , m_geometryDirty(true)
    , m_initialized(false)
    , m_parametersChanged(false)
{
    setupManagers();
    initialize();
}

Geo3D::~Geo3D()
{
    // 管理器会自动销毁，不需要手动删除
}

void Geo3D::setupManagers()
{
    // 创建各个管理器
    m_stateManager = std::make_unique<GeoStateManager>(this);
    m_nodeManager = std::make_unique<GeoNodeManager>(this);
    m_materialManager = std::make_unique<GeoMaterialManager>(this);
    m_snapPointManager = std::make_unique<GeoSnapPointManager>(this);
    m_controlPointManager = std::make_unique<GeoControlPointManager>(this);
    m_boundingBoxManager = std::make_unique<GeoBoundingBoxManager>(this);
    m_renderManager = std::make_unique<GeoRenderManager>(this);
    
    // 连接管理器之间的信号
    connectManagerSignals();
}

void Geo3D::connectManagerSignals()
{
    // 状态管理器信号连接
    connect(m_stateManager.get(), &GeoStateManager::stateChanged, 
            this, [this](int, int) { emit stateChanged(this); });
    
    // 控制点管理器信号连接
    connect(m_controlPointManager.get(), &GeoControlPointManager::controlPointsChanged,
            this, [this]() { emit geometryUpdated(this); });
    
    // 包围盒管理器信号连接
    connect(m_boundingBoxManager.get(), &GeoBoundingBoxManager::boundingBoxChanged,
            this, [this]() { emit geometryUpdated(this); });
    
    // 材质管理器信号连接
    connect(m_materialManager.get(), &GeoMaterialManager::materialChanged,
            this, [this]() { emit parametersChanged(this); });
    
    // 渲染管理器信号连接
    connect(m_renderManager.get(), &GeoRenderManager::renderModeChanged,
            this, [this](GeoRenderManager::RenderMode) { emit geometryUpdated(this); });
}

// ========================================= 控制点管理接口 =========================================
const std::vector<Point3D>& Geo3D::getControlPoints() const
{
    return m_controlPointManager->getControlPoints();
}

void Geo3D::addControlPoint(const Point3D& point)
{
    m_controlPointManager->addControlPoint(point);
    markGeometryDirty();
}

void Geo3D::setControlPoint(int index, const Point3D& point)
{
    m_controlPointManager->setControlPoint(index, point);
    markGeometryDirty();
}

void Geo3D::removeControlPoint(int index)
{
    m_controlPointManager->removeControlPoint(index);
    markGeometryDirty();
}

void Geo3D::clearControlPoints()
{
    m_controlPointManager->clearControlPoints();
    markGeometryDirty();
}

bool Geo3D::hasControlPoints() const
{
    return m_controlPointManager->hasControlPoints();
}

// ========================================= 状态管理接口 =========================================
bool Geo3D::isStateInitialized() const
{
    return m_stateManager->isStateInitialized();
}

bool Geo3D::isStateComplete() const
{
    return m_stateManager->isStateComplete();
}

bool Geo3D::isStateInvalid() const
{
    return m_stateManager->isStateInvalid();
}

bool Geo3D::isStateSelected() const
{
    return m_stateManager->isStateSelected();
}

bool Geo3D::isStateEditing() const
{
    return m_stateManager->isStateEditing();
}

void Geo3D::setStateInitialized()
{
    m_stateManager->setStateInitialized();
}

void Geo3D::setStateComplete()
{
    m_stateManager->setStateComplete();
}

void Geo3D::setStateInvalid()
{
    m_stateManager->setStateInvalid();
}

void Geo3D::setStateSelected()
{
    m_stateManager->setStateSelected();
}

void Geo3D::setStateEditing()
{
    m_stateManager->setStateEditing();
}

void Geo3D::clearStateComplete()
{
    m_stateManager->clearStateComplete();
}

void Geo3D::clearStateInvalid()
{
    m_stateManager->clearStateInvalid();
}

void Geo3D::clearStateSelected()
{
    m_stateManager->clearStateSelected();
}

void Geo3D::clearStateEditing()
{
    m_stateManager->clearStateEditing();
}

// ========================================= 变换管理 =========================================
void Geo3D::setTransform(const Transform3D& transform)
{
    m_transform = transform;
    markGeometryDirty();
}

// ========================================= 包围盒管理接口 =========================================
const BoundingBox3D& Geo3D::getBoundingBox() const
{
    return m_boundingBoxManager->getBoundingBox();
}

// ========================================= 节点管理接口 =========================================
osg::ref_ptr<osg::Group> Geo3D::getOSGNode() const
{
    return m_nodeManager->getOSGNode();
}

osg::ref_ptr<osg::Group> Geo3D::getVertexNode() const
{
    return m_nodeManager->getVertexNode();
}

osg::ref_ptr<osg::Group> Geo3D::getEdgeNode() const
{
    return m_nodeManager->getEdgeNode();
}

osg::ref_ptr<osg::Group> Geo3D::getFaceNode() const
{
    return m_nodeManager->getFaceNode();
}

// ========================================= 捕捉点管理接口 =========================================
const std::vector<glm::vec3>& Geo3D::getSnapPoints() const
{
    return m_snapPointManager->getSnapPositions();
}

void Geo3D::addSnapPoint(const glm::vec3& point)
{
    m_snapPointManager->addSnapPoint(point);
}

void Geo3D::clearSnapPoints()
{
    m_snapPointManager->clearSnapPoints();
}

void Geo3D::updateSnapPoints()
{
    m_snapPointManager->updateSnapPoints();
}

// ========================================= 渲染管理接口 =========================================
void Geo3D::setShowPoints(bool show)
{
    m_renderManager->setShowPoints(show);
    m_parameters.showPoints = show;
}

void Geo3D::setShowEdges(bool show)
{
    m_renderManager->setShowEdges(show);
    m_parameters.showEdges = show;
}

void Geo3D::setShowFaces(bool show)
{
    m_renderManager->setShowFaces(show);
    m_parameters.showFaces = show;
}

bool Geo3D::isShowPoints() const
{
    return m_renderManager->isShowPoints();
}

bool Geo3D::isShowEdges() const
{
    return m_renderManager->isShowEdges();
}

bool Geo3D::isShowFaces() const
{
    return m_renderManager->isShowFaces();
}

// ========================================= 参数管理 =========================================
void Geo3D::setParameters(const GeoParameters3D& params)
{
    m_parameters = params;
    m_parametersChanged = true;
    markGeometryDirty();
    
    // 通知各个管理器参数变化
    m_materialManager->setMaterial(params.material);
    m_renderManager->setShowPoints(params.showPoints);
    m_renderManager->setShowEdges(params.showEdges);
    m_renderManager->setShowFaces(params.showFaces);
    
    emit parametersChanged(this);
}

// ========================================= 事件处理 =========================================
void Geo3D::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 默认实现，子类可以重写
}

void Geo3D::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 默认实现，子类可以重写
}

void Geo3D::mouseReleaseEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 默认实现，子类可以重写
}

void Geo3D::keyPressEvent(QKeyEvent* event)
{
    // 默认实现，子类可以重写
}

void Geo3D::keyReleaseEvent(QKeyEvent* event)
{
    // 默认实现，子类可以重写
}

// ========================================= 拾取测试 =========================================
bool Geo3D::hitTest(const Ray3D& ray, PickResult3D& result) const
{
    // 默认实现，子类可以重写
    return false;
}

// ========================================= 绘制完成 =========================================
void Geo3D::completeDrawing()
{
    setStateComplete();
    clearStateEditing();
    updateGeometry();
    
    emit drawingCompleted(this);
}

// ========================================= 点线面几何体管理 =========================================
void Geo3D::addVertexGeometry(osg::Drawable* drawable)
{
    m_nodeManager->addVertexGeometry(drawable);
}

void Geo3D::addEdgeGeometry(osg::Drawable* drawable)
{
    m_nodeManager->addEdgeGeometry(drawable);
}

void Geo3D::addFaceGeometry(osg::Drawable* drawable)
{
    m_nodeManager->addFaceGeometry(drawable);
}

void Geo3D::clearVertexGeometries()
{
    m_nodeManager->clearVertexGeometries();
}

void Geo3D::clearEdgeGeometries()
{
    m_nodeManager->clearEdgeGeometries();
}

void Geo3D::clearFaceGeometries()
{
    m_nodeManager->clearFaceGeometries();
}

// ========================================= 实用方法 =========================================
// 添加缺失的方法
void Geo3D::updateFeatureVisibility()
{
    m_renderManager->updateFeatureVisibility();
}

void Geo3D::setupNodeNames()
{
    m_nodeManager->setupNodeNames();
}

// ========================================= 初始化和更新 =========================================
void Geo3D::initialize()
{
    if (!m_initialized)
    {
        m_parameters.resetToGlobal();
        setStateInitialized();
        m_initialized = true;
    }
}

void Geo3D::updateOSGNode()
{
    if (!m_initialized)
        initialize();
    
    if (isGeometryDirty())
    {
        // 清除旧的几何体
        m_nodeManager->clearGeometry();
        
        // 创建新的几何体
        osg::ref_ptr<osg::Geometry> geometry = createGeometry();
        if (geometry.valid())
        {
            m_nodeManager->setGeometry(geometry);
            
            // 应用材质
            m_materialManager->updateMaterial();
            
            // 更新节点名称
            setupNodeNames();
            
            // 更新捕捉点
            updateSnapPoints();
            
            // 更新包围盒
            m_boundingBoxManager->updateBoundingBox();
            
            // 更新特征可见性
            updateFeatureVisibility();
        }
        
        clearGeometryDirty();
    }
    
    // 更新控制点可视化
    m_nodeManager->updateControlPointsVisualization();
}

// ========================================= 辅助函数 =========================================
osg::Vec3 Geo3D::glmToOsgVec3(const glm::vec3& v) const
{
    return osg::Vec3(v.x, v.y, v.z);
}

osg::Vec4 Geo3D::glmToOsgVec4(const glm::vec4& v) const
{
    return osg::Vec4(v.x, v.y, v.z, v.w);
}

glm::vec3 Geo3D::osgToGlmVec3(const osg::Vec3& v) const
{
    return glm::vec3(v.x(), v.y(), v.z());
}

glm::vec4 Geo3D::osgToGlmVec4(const osg::Vec4& v) const
{
    return glm::vec4(v.x(), v.y(), v.z(), v.w());
}

// ============================================================================
// 工厂函数（临时实现）
// ============================================================================

#include "../util/GeometryFactory.h"

Geo3D* createGeo3D(DrawMode3D mode)
{
    return GeometryFactory::createGeometry(mode);
} 