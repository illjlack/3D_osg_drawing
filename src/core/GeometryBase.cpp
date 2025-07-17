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

    m_controlPointManager = std::make_unique<GeoControlPointManager>(this);
    m_renderManager = std::make_unique<GeoRenderManager>(this);
    
    // 连接管理器之间的信号
    connectManagerSignals();
}

void Geo3D::connectManagerSignals()
{
    // 确保所有管理器都已创建
    if (!m_stateManager || !m_nodeManager || !m_materialManager || 
        !m_controlPointManager || !m_renderManager) {
        qWarning() << "Geo3D::connectManagerSignals: 某些管理器未初始化";
        return;
    }
    
    // ==================== 状态管理器信号连接 ====================
    
    // 状态完成时通知控制点管理器
    connect(m_stateManager.get(), &GeoStateManager::stateCompleted,
            this, [this]() {
                if (m_controlPointManager) {
                    // 绘制完成时清理临时点
                    m_controlPointManager->clearTempPoint();
                }
            });
    
    // 编辑状态变化时通知渲染管理器
    connect(m_stateManager.get(), &GeoStateManager::editingStarted,
            this, [this]() {
                if (m_renderManager) {
                    m_renderManager->setHighlighted(true);
                }
                // 编辑开始时显示控制点
                if (m_nodeManager) {
                    m_nodeManager->setControlPointsVisible(true);
                }
            });
    
    connect(m_stateManager.get(), &GeoStateManager::editingFinished,
            this, [this]() {
                if (m_renderManager) {
                    m_renderManager->setHighlighted(false);
                }
                // 编辑结束时隐藏控制点
                if (m_nodeManager) {
                    m_nodeManager->setControlPointsVisible(false);
                }
            });
    
    // 选中状态变化时通知渲染管理器
    connect(m_stateManager.get(), &GeoStateManager::stateSelected,
            this, [this]() {
                if (m_renderManager) {
                    m_renderManager->setHighlighted(true);
                }
            });
    
    connect(m_stateManager.get(), &GeoStateManager::stateDeselected,
            this, [this]() {
                if (m_renderManager) {
                    m_renderManager->setHighlighted(false);
                }
            });
    
    // ==================== 节点管理器信号连接 ====================
    connect(m_nodeManager.get(), &GeoNodeManager::geometryChanged,
            this, [this]() {
                // 几何体变化时更新渲染
                if (m_renderManager) {
                    m_renderManager->forceRenderUpdate();
                }
                // 几何体变化时更新材质
                if (m_materialManager) {
                    m_materialManager->updateOSGMaterial();
                }
            });
    
    connect(m_nodeManager.get(), &GeoNodeManager::transformChanged,
            this, [this]() {
                // 变换变化时更新渲染
                if (m_renderManager) {
                    m_renderManager->updateRender();
                }
            });
    
    connect(m_nodeManager.get(), &GeoNodeManager::visibilityChanged,
            this, [this]() {
                // 可见性变化时更新渲染
                if (m_renderManager) {
                    m_renderManager->updateRender();
                }
            });
    
    // ==================== 材质管理器信号连接 ====================
    connect(m_materialManager.get(), &GeoMaterialManager::materialChanged,
            this, [this]() {
                // 材质变化时更新渲染
                if (m_renderManager) {
                    m_renderManager->updateRender();
                }
            });
    
    connect(m_materialManager.get(), &GeoMaterialManager::colorChanged,
            this, [this]() {
                // 颜色变化时更新渲染
                if (m_renderManager) {
                    m_renderManager->updateRender();
                }
            });
    
    connect(m_materialManager.get(), &GeoMaterialManager::linePropertiesChanged,
            this, [this]() {
                // 线条属性变化时更新渲染
                if (m_renderManager) {
                    m_renderManager->updateRender();
                }
            });
    
    connect(m_materialManager.get(), &GeoMaterialManager::pointPropertiesChanged,
            this, [this]() {
                // 点属性变化时更新渲染
                if (m_renderManager) {
                    m_renderManager->updateRender();
                }
            });
    
    connect(m_materialManager.get(), &GeoMaterialManager::facePropertiesChanged,
            this, [this]() {
                // 面属性变化时更新渲染
                if (m_renderManager) {
                    m_renderManager->updateRender();
                }
            });
    
    connect(m_materialManager.get(), &GeoMaterialManager::blendingChanged,
            this, [this]() {
                // 混合模式变化时更新渲染
                if (m_renderManager) {
                    m_renderManager->updateRender();
                }
            });
    
    connect(m_materialManager.get(), &GeoMaterialManager::renderModeChanged,
            this, [this]() {
                // 渲染模式变化时通知渲染管理器
                if (m_renderManager) {
                    m_renderManager->applyRenderMode();
                }
            });
    
    // ==================== 渲染管理器信号连接 ====================
    connect(m_renderManager.get(), &GeoRenderManager::renderModeChanged,
            this, [this](GeoRenderManager::RenderMode mode) {
                // 渲染模式变化时更新材质
                if (m_materialManager) {
                    m_materialManager->updateRenderingAttributes();
                }
            });
    
    connect(m_renderManager.get(), &GeoRenderManager::visibilityChanged,
            this, [this](bool visible) {
                // 可见性变化时更新节点管理器
                if (m_nodeManager) {
                    m_nodeManager->setVisible(visible);
                }
            });
    
    connect(m_renderManager.get(), &GeoRenderManager::highlightChanged,
            this, [this](bool highlighted) {
                // 高亮变化时更新材质
                if (m_materialManager) {
                    m_materialManager->updateOSGMaterial();
                }
            });
    
    connect(m_renderManager.get(), &GeoRenderManager::renderUpdateRequired,
            this, [this]() {
                // 渲染更新请求时强制更新
                if (m_renderManager) {
                    m_renderManager->forceRenderUpdate();
                }
            });
    
    connect(m_renderManager.get(), &GeoRenderManager::renderQualityChanged,
            this, [this](int quality) {
                // 渲染质量变化时更新材质
                if (m_materialManager) {
                    m_materialManager->updateRenderingAttributes();
                }
            });
    
    connect(m_renderManager.get(), &GeoRenderManager::renderOptimizationSuggested,
            this, [this]() {
                // 渲染优化建议时执行优化
                if (m_renderManager) {
                    m_renderManager->optimizeRendering();
                }
            });
    
    qDebug() << "Geo3D::connectManagerSignals: 所有管理器信号连接完成";
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
}

// ========================================= 初始化和更新 =========================================
void Geo3D::initialize()
{
    m_parameters.resetToGlobal();
    mm_state()->setStateInitialized();
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

// ==================== 公共接口方法实现 ====================

void Geo3D::checkAndEmitDrawingComplete()
{
    // 检查控制点是否有效
    if (!mm_controlPoint()->hasControlPoints()) {
        qWarning() << "Geo3D::checkAndEmitDrawingComplete: 控制点无效";
        return;
    }
    
    // 检查是否绘制完成
    if (isDrawingComplete() && areControlPointsValid()) {
        // 发送绘制完成信号
        mm_state()->setStateComplete();
        qDebug() << "Geo3D::checkAndEmitDrawingComplete: 绘制完成信号已发送";
    } else {
        qDebug() << "Geo3D::checkAndEmitDrawingComplete: 绘制尚未完成";
    }
}

void Geo3D::updateGeometries()
{
    buildVertexGeometries();
    buildEdgeGeometries();
    buildFaceGeometries();
}

// ============================================================================
// 工厂函数（临时实现）
// ============================================================================

#include "../util/GeometryFactory.h"

Geo3D* createGeo3D(DrawMode3D mode)
{
    return GeometryFactory::createGeometry(mode);
}
