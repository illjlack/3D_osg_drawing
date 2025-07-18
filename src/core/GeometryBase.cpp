﻿#include "GeometryBase.h"
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
                // 选中时显示包围盒
                if (m_nodeManager) {
                    m_nodeManager->updateBoundingBoxVisibility();
                }
            });
    
    connect(m_stateManager.get(), &GeoStateManager::stateDeselected,
            this, [this]() {
                if (m_renderManager) {
                    m_renderManager->setHighlighted(false);
                }
                // 取消选中时隐藏包围盒
                if (m_nodeManager) {
                    m_nodeManager->updateBoundingBoxVisibility();
                }
            });
    qDebug() << "Geo3D::connectManagerSignals: 所有管理器信号连接完成";
}

// ========================================= 参数管理 =========================================
void Geo3D::setParameters(const GeoParameters3D& params)
{
    if (m_renderManager) {
        if (m_parameters.pointColor != params.pointColor) {
            m_renderManager->setPointColor(params.pointColor);
        }
        if (m_parameters.pointSize != params.pointSize) {
            m_renderManager->setPointSize(params.pointSize);
        }
        if (m_parameters.pointShape != params.pointShape) {
            
        }
        if (m_parameters.lineColor != params.lineColor) {
            m_renderManager->setEdgeColor(params.lineColor);
        }
        if (m_parameters.lineWidth != params.lineWidth) {
            m_renderManager->setLineWidth(params.lineWidth);
        }
        if (m_parameters.lineStyle != params.lineStyle) {
            
        }
        if (m_parameters.lineDashPattern != params.lineDashPattern) {
            
        }
        if (m_parameters.nodeLineStyle != params.nodeLineStyle) {
            
        }
        if (m_parameters.fillColor != params.fillColor) {
            m_renderManager->setFaceColor(params.fillColor);
        }
        if (m_parameters.fillType != params.fillType) {
            
        }
        if (m_parameters.borderColor != params.borderColor) {
            
        }
        if (m_parameters.showBorder != params.showBorder) {
            
        }
        if (m_parameters.material!= params.material) {
            m_renderManager->setMaterial(params.material);
        }
    }
    
    if (m_nodeManager) {
        if (m_parameters.showPoints != params.showPoints) {
            m_nodeManager->setVertexVisible(params.showPoints);
        }
        if (m_parameters.showEdges != params.showEdges) {
            m_nodeManager->setEdgeVisible(params.showEdges);
        }
        if (m_parameters.showFaces != params.showFaces) {
            m_nodeManager->setFaceVisible(params.showFaces);
        }
        // 细分等级变化需要重新计算
        if (m_parameters.subdivisionLevel != params.subdivisionLevel) {
            m_nodeManager->updateGeometries();
        }
    }

    m_parameters = params;
    m_parametersChanged = true;
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
