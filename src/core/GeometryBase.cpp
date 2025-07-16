#include "GeometryBase.h"
#include "../util/LogManager.h"
#include "../util/MathUtils.h"
#include "../util/OSGUtils.h"
#include <osg/ComputeBoundsVisitor>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/PolygonMode>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/MatrixTransform>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>
#include <osg/NodeVisitor>
#include <osg/UserDataContainer>
#include <osg/Timer>
#include <osgViewer/Viewer>
#include <osg/LineSegment>
#include <osg/Geode>
#include <algorithm>
#include <cmath>
#include <random>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QObject>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ========================================= 基类 Geo3D =========================================
Geo3D::Geo3D()
    : m_geoType(Geo_Undefined3D)
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
    connect(m_stateManager.get(), &GeoStateManager::stateCompleted, 
            this, [this]() { });
    
    // 控制点管理器信号连接
    connect(m_controlPointManager.get(), &GeoControlPointManager::controlPointsChanged,
            this, [this]() { });
    
    // 包围盒管理器信号连接
    connect(m_boundingBoxManager.get(), &GeoBoundingBoxManager::boundingBoxChanged,
            this, [this]() { });
    
    // 材质管理器信号连接
    connect(m_materialManager.get(), &GeoMaterialManager::materialChanged,
            this, [this]() { });
    
    // 渲染管理器信号连接
    connect(m_renderManager.get(), &GeoRenderManager::renderModeChanged,
            this, [this](GeoRenderManager::RenderMode) { });
}

// ========================================= 参数管理 =========================================
void Geo3D::setParameters(const GeoParameters3D& params)
{
    m_parameters = params;
    m_parametersChanged = true;
    
    // 通知各个管理器参数变化
    m_materialManager->setMaterial(params.material);
    m_renderManager->setShowPoints(params.showPoints);
    m_renderManager->setShowEdges(params.showEdges);
    m_renderManager->setShowFaces(params.showFaces);
    
    // 参数变化后重新构建KDTree
    if (m_nodeManager) {
        m_nodeManager->updateSpatialIndex();
    }
    
}

// ========================================= 初始化和更新 =========================================
void Geo3D::initialize()
{
    m_parameters.resetToGlobal();
    mm_state()->setStateInitialized();
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


// ========================================= 事件处理虚函数实现 =========================================
void Geo3D::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 默认实现
}

void Geo3D::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 默认实现
}

void Geo3D::keyPressEvent(QKeyEvent* event)
{
    // 默认实现
}

void Geo3D::keyReleaseEvent(QKeyEvent* event)
{
    // 默认实现
}

// ============================================================================
// 工厂函数（临时实现）
// ============================================================================

#include "../util/GeometryFactory.h"

Geo3D* createGeo3D(DrawMode3D mode)
{
    return GeometryFactory::createGeometry(mode);
}
