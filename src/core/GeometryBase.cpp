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
#include <osg/ValueObject>
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
            mm_node()->updateGeometries();
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
// 序列化/反序列化实现
// ============================================================================

QString Geo3D::serialize() const
{
    // 序列化格式：几何体类型|参数数据|控制点数据
    QString result;
    result += QString::number(static_cast<int>(m_geoType)) + "|";
    result += QString::fromStdString(m_parameters.toString()) + "|";
    result += QString::fromStdString(mm_controlPoint()->serializeControlPoints());
    return result;
}

bool Geo3D::deserialize(const QString& data)
{
    // 解析序列化数据
    QStringList parts = data.split("|");
    if (parts.size() < 2) {
        LOG_ERROR("反序列化数据格式错误", "几何体");
        return false;
    }
    
    // 恢复几何体类型
    bool ok;
    int geoTypeInt = parts[0].toInt(&ok);
    if (!ok) {
        LOG_ERROR("反序列化几何体类型失败", "几何体");
        return false;
    }
    m_geoType = static_cast<GeoType3D>(geoTypeInt);
    
    // 恢复参数数据
    if (!m_parameters.fromString(parts[1].toStdString())) {
        LOG_ERROR("反序列化参数数据失败", "几何体");
        return false;
    }
    
    // 恢复控制点数据（如果存在）- 新格式支持
    if (parts.size() >= 3) {
        QString controlPointData = parts[2];
        if (!mm_controlPoint()->deserializeControlPoints(controlPointData.toStdString())) {
            LOG_WARNING("反序列化控制点数据失败，使用默认控制点", "几何体");
            // 不返回false，继续使用默认控制点
        } else {
            LOG_INFO("成功反序列化控制点数据", "几何体");
        }
    } else {
        LOG_INFO("旧格式数据，使用默认控制点", "几何体");
    }
    
    // 重新构建几何体（基于恢复的控制点和参数）
    if (mm_state()->isStateComplete()) {
        mm_node()->updateGeometries();
        LOG_INFO("基于反序列化数据重新构建几何体", "几何体");
    }
    
    return true;
}

void Geo3D::restoreFromFileNode(osg::ref_ptr<osg::Node> node)
{
    if (!node.valid()) {
        LOG_ERROR("传入的节点为空，无法恢复对象状态", "文件加载");
        return;
    }
    
    LOG_INFO("开始从文件节点恢复对象状态", "文件加载");
    
    // 0. 首先从节点恢复对象内部数据（参数+控制点） 
    osg::UserDataContainer* userData = node->getUserDataContainer();
    if (userData) {
        osg::Object* geoDataObj = userData->getUserObject("GeoData");
        osg::StringValueObject* geoDataValue = dynamic_cast<osg::StringValueObject*>(geoDataObj);
        if (geoDataValue) {
            QString internalData = QString::fromStdString(geoDataValue->getValue());
            LOG_INFO(QString("读取到内部数据: '%1'").arg(internalData), "文件加载");
            
            // 分割内部数据：参数|控制点
            QStringList parts = internalData.split("|");
            if (parts.size() >= 2) {
                // 恢复参数数据
                if (!m_parameters.fromString(parts[0].toStdString())) {
                    LOG_WARNING("恢复参数数据失败，使用默认参数", "文件加载");
                } else {
                    LOG_INFO("步骤0a: 参数数据恢复成功", "文件加载");
                }
                
                // 恢复控制点数据
                QString controlPointData = parts.mid(1).join("|"); // 重新组合控制点部分
                if (!mm_controlPoint()->deserializeControlPoints(controlPointData.toStdString())) {
                    LOG_WARNING("恢复控制点数据失败，使用默认控制点", "文件加载");
                } else {
                    LOG_INFO("步骤0b: 控制点数据恢复成功", "文件加载");
                }
            } else {
                LOG_WARNING("内部数据格式不正确，使用默认值", "文件加载");
            }
        } else {
            LOG_WARNING("节点没有内部数据，使用默认值", "文件加载");
        }
    } else {
        LOG_WARNING("节点没有用户数据容器，使用默认值", "文件加载");
    }
    assert(this);
    // 1. 设置OSG节点（这会解析节点结构并分配各个几何体组件）
    mm_node()->setOSGNode(node);
    LOG_INFO("步骤1: OSG节点设置完成", "文件加载");
    
    // 2. 重新初始化渲染状态（解决材质悬空问题）
    assert(this->mm_node());
    if (mm_render()) {
        mm_render()->reinitializeRenderStates();
        LOG_INFO("步骤2: 渲染状态重新初始化完成", "文件加载");
    }
    
    if (mm_render()) {
        mm_render()->updateRenderingParameters(m_parameters);
        LOG_INFO("步骤3: 渲染参数应用完成", "文件加载");
    }
    
    // 直接构建完成
    mm_state()->setStateComplete();
    LOG_INFO("文件节点恢复完成，对象状态已完全恢复", "文件加载");
}

