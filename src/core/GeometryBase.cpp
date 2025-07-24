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
    m_controlPointManager = std::make_unique<GeoControlPointManager>(this);
    m_renderManager = std::make_unique<GeoRenderManager>(this);
    
    // 连接管理器之间的信号
    connectManagerSignals();
}

void Geo3D::connectManagerSignals()
{
    // 状态完成时通知控制点管理器
    connect(m_stateManager.get(), &GeoStateManager::stateCompleted,
            this, [this]() {

            });
    
    // 状态完成时通知节点管理器设置节点可拾取
    connect(m_stateManager.get(), &GeoStateManager::stateCompleted,
            m_nodeManager.get(), &GeoNodeManager::onDrawingCompleted);
    
    // 编辑状态变化时通知节点管理器
    connect(m_stateManager.get(), &GeoStateManager::editingStarted,
            this, [this]() {
                if (m_nodeManager) {
                    m_nodeManager->setSelected(true);  // 编辑时选中（显示包围盒和控制点）
                }
            });
    
    connect(m_stateManager.get(), &GeoStateManager::editingFinished,
            this, [this]() {
                // 编辑结束时保持选中状态，不改变显示
            });
    
    // 选中状态变化时通知节点管理器
    connect(m_stateManager.get(), &GeoStateManager::stateSelected,
            this, [this]() {
                if (m_nodeManager) {
                    m_nodeManager->setSelected(true);  // 选中时显示包围盒和控制点
                }
            });
    
    connect(m_stateManager.get(), &GeoStateManager::stateDeselected,
            this, [this]() {
                if (m_nodeManager) {
                    m_nodeManager->setSelected(false); // 取消选中时隐藏包围盒和控制点
                }
            });

    connect(m_controlPointManager.get(), &GeoControlPointManager::controlPointChanged,
        this, [this]() {
            updateGeometries();
        });

    qDebug() << "Geo3D::connectManagerSignals: 所有管理器信号连接完成";
}

// ========================================= 参数管理 =========================================
void Geo3D::setParameters(const GeoParameters3D& params)
{
    GeoParameters3D constrainedParams = params;
    
    // 应用显示约束：确保至少有一个组件可见
    constrainedParams.enforceVisibilityConstraint();
    
    // 检查是否有需要重新计算几何体的参数变化
    bool needsGeometryRecalculation = false;
    
    if (m_parameters.pointShape != constrainedParams.pointShape) {
        needsGeometryRecalculation = true;
    }
    
    if (m_parameters.subdivisionLevel != constrainedParams.subdivisionLevel) {
        needsGeometryRecalculation = true;
    }
    
    // 如果需要重新计算几何体，先更新参数，然后重建几何体
    if (needsGeometryRecalculation) {
        m_parameters = constrainedParams;
        if (m_nodeManager) {
            m_nodeManager->updateGeometries();
        }
    } else {
        // 只更新参数
        m_parameters = constrainedParams;
    }
    
    // 使用简化的渲染管理器接口更新所有渲染参数
    if (m_renderManager) {
        m_renderManager->updateRenderingParameters(constrainedParams);
    }
    
    m_parametersChanged = true;
}

// ========================================= 初始化和更新 =========================================
void Geo3D::initialize()
{
    // 获得此时的全局参数
    m_parameters.resetToGlobal();
    mm_state()->setStateInitialized();
}

void Geo3D::updateGeometries()
{
    buildControlPointGeometries();
    buildVertexGeometries();
    buildEdgeGeometries();
    buildFaceGeometries();
}

void Geo3D::buildControlPointGeometries()
{
    mm_node()->clearControlPointsGeometry();

    // 获取所有控制点用于绘制
    const auto& controlPointss = mm_controlPoint()->getAllStageControlPoints();

    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getControlPointsGeometry();
    if (!geometry.valid())
    {
        return;
    }

    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;

    // 添加所有控制点
    for (auto& points : controlPointss)
        for (auto& point : points)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        }

    geometry->setVertexArray(vertices);
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

// ============================================================================
// 工厂函数（临时实现）
// ============================================================================

#include "../util/GeometryFactory.h"

Geo3D* createGeo3D(DrawMode3D mode)
{
    return GeometryFactory::createGeometry(mode);
}


