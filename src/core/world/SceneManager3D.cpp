#include "SceneManager3D.h"
#include "../GeometryBase.h"
#include "../../util/LogManager.h"
#include "../../util/GeometryFactory.h"

#include <osgViewer/Viewer>
#include <osg/Material>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/PolygonMode>
#include <osg/StateSet>
#include <osg/LightSource>
#include <osg/Light>
#include <osg/BlendFunc>
#include <osg/Multisample>
#include <osg/ComputeBoundsVisitor>
#include <osg/GL>
#include <algorithm>

// ========================================= 构造函数和析构函数 =========================================

SceneManager3D::SceneManager3D()
    : m_rootNode(new osg::Group)
    , m_sceneNode(new osg::Group)
    , m_geometryNode(new osg::Group)
    , m_lightNode(new osg::Group)
    , m_pickingIndicatorNode(new osg::Group)
    , m_skyboxNode(new osg::Group)
    , m_selectedGeometry(nullptr)
    , m_isDrawing(false)
    , m_currentDrawingGeometry(nullptr)
    , m_isDraggingControlPoint(false)
    , m_draggingGeometry(nullptr)
    , m_draggingControlPointIndex(-1)
    , m_geometryPickingSystem(nullptr)
    , m_skybox(std::make_unique<Skybox>())
    , m_skyboxEnabled(true)
    , m_coordinateSystemRenderer(std::make_unique<CoordinateSystemRenderer>())
    , m_coordinateSystemEnabled(true)
{
    LOG_INFO("场景管理器初始化", "场景管理器");
}

SceneManager3D::~SceneManager3D()
{
    // 清理几何拾取系统
    if (m_geometryPickingSystem) {
        m_geometryPickingSystem->shutdown();
        m_geometryPickingSystem = nullptr;
    }
    
    // 清理几何体
    m_geometries.clear();
    m_selectedGeometries.clear();
    
    LOG_INFO("场景管理器析构", "场景管理器");
}

// ========================================= 场景初始化 =========================================

bool SceneManager3D::initializeScene(osgViewer::Viewer* viewer)
{
    if (!viewer) {
        LOG_ERROR("查看器为空", "场景管理器");
        return false;
    }
    
    setupSceneGraph();
    setupRenderingStates();
    setupLighting();
    setupPickingSystem(viewer);
    setupSkybox();
    setupCoordinateSystem();
    
    // 设置查看器的场景数据
    viewer->setSceneData(m_rootNode.get());
    
    LOG_SUCCESS("场景初始化完成", "场景管理器");
    return true;
}

void SceneManager3D::setupSceneGraph()
{
    // 设置场景图层次结构
    m_rootNode->addChild(m_sceneNode.get());
    m_sceneNode->addChild(m_geometryNode.get());
    m_sceneNode->addChild(m_lightNode.get());
    m_sceneNode->addChild(m_pickingIndicatorNode.get());
    m_sceneNode->addChild(m_skyboxNode.get());
    
    // 设置节点名称
    m_rootNode->setName("3D_SCENE_ROOT");
    m_sceneNode->setName("3D_SCENE_NODE");
    m_geometryNode->setName("3D_GEOMETRY_NODE");
    m_lightNode->setName("3D_LIGHT_NODE");
    m_pickingIndicatorNode->setName("3D_PICKING_INDICATOR_NODE");
    m_skyboxNode->setName("3D_SKYBOX_NODE");
    
    LOG_INFO("场景图层次结构设置完成", "场景管理器");
}

void SceneManager3D::setupRenderingStates()
{
    osg::StateSet* rootStateSet = m_rootNode->getOrCreateStateSet();
    
    // 启用深度测试
    rootStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    
    // 启用混合
    rootStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    rootStateSet->setAttributeAndModes(blendFunc, osg::StateAttribute::ON);
    
    // 抗锯齿设置
    rootStateSet->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    rootStateSet->setMode(GL_POINT_SMOOTH, osg::StateAttribute::ON);
    rootStateSet->setMode(GL_POLYGON_SMOOTH, osg::StateAttribute::ON);
    // rootStateSet->setMode(GL_MULTISAMPLE, osg::StateAttribute::ON);  // 注释掉，避免GL常量问题
    
    // 高质量混合函数设置
    osg::ref_ptr<osg::BlendFunc> alphaBlend = new osg::BlendFunc;
    alphaBlend->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    rootStateSet->setAttributeAndModes(alphaBlend, osg::StateAttribute::ON);
    
    LOG_INFO("渲染状态设置完成", "场景管理器");
}

void SceneManager3D::setupLighting()
{
    // 创建光源
    osg::ref_ptr<osg::LightSource> lightSource = new osg::LightSource;
    osg::ref_ptr<osg::Light> light = new osg::Light;
    
    // 设置光源属性
    light->setLightNum(0);
    light->setPosition(osg::Vec4(50.0f, 50.0f, 50.0f, 1.0f)); // 点光源
    light->setAmbient(osg::Vec4(0.3f, 0.3f, 0.3f, 1.0f));
    light->setDiffuse(osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f));
    light->setSpecular(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    
    lightSource->setLight(light.get());
    lightSource->setLocalStateSetModes(osg::StateAttribute::ON);
    
    // 启用光照
    osg::StateSet* lightStateSet = lightSource->getOrCreateStateSet();
    lightStateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    lightStateSet->setMode(GL_LIGHT0, osg::StateAttribute::ON);
    
    m_lightNode->addChild(lightSource.get());
    
    LOG_INFO("光照系统设置完成", "场景管理器");
}

void SceneManager3D::setupPickingSystem(osgViewer::Viewer* viewer)
{
    if (!viewer) return;
    
    // 创建拾取指示器
    m_pickingIndicator = new PickingIndicator();
    m_pickingIndicator->initialize();
    osg::Group* indicatorRoot = m_pickingIndicator->getIndicatorRoot();
    if (indicatorRoot) {
        m_pickingIndicatorNode->addChild(indicatorRoot);
    }
    
    // 创建几何拾取系统
    m_geometryPickingSystem = new GeometryPickingSystem();
    osg::Camera* camera = viewer->getCamera();
    if (camera) {
        m_geometryPickingSystem->initialize(camera, m_geometryNode.get());
    }
    
    LOG_INFO("拾取系统设置完成", "场景管理器");
}

void SceneManager3D::setupSkybox()
{
    if (m_skybox && m_skyboxEnabled) {
        osg::Node* skyboxNode = m_skybox->getSkyboxNode();
        if (skyboxNode) {
            m_skyboxNode->addChild(skyboxNode);
        }
        
        LOG_INFO("天空盒设置完成", "场景管理器");
    }
}

void SceneManager3D::setupCoordinateSystem()
{
    if (m_coordinateSystemRenderer && m_coordinateSystemEnabled) {
        osg::Node* coordNode = m_coordinateSystemRenderer->getCoordinateSystemNode();
        if (coordNode) {
            m_sceneNode->addChild(coordNode);
        }
        
        LOG_INFO("坐标系设置完成", "场景管理器");
    }
}

// ========================================= 几何体管理 =========================================

void SceneManager3D::addGeometry(Geo3D::Ptr geo)
{
    if (!geo) return;
    
    // 添加到几何体列表
    m_geometries.push_back(geo);
    
    // 添加到场景图
    if (geo->mm_node()) {
        osg::Node* geoOSGNode = geo->mm_node()->getOSGNode();
        if (geoOSGNode) {
            m_geometryNode->addChild(geoOSGNode);
            LOG_INFO(QString("添加几何体到场景: %1").arg(geoType3DToString(geo->getGeoType())), "场景管理器");
        } else {
            LOG_WARNING(QString("几何体没有有效的OSG节点: %1").arg(geoType3DToString(geo->getGeoType())), "场景管理器");
        }
    }
}

void SceneManager3D::removeGeometry(Geo3D::Ptr geo)
{
    if (!geo) return;
    
    // 从几何体列表中移除
    auto it = std::find_if(m_geometries.begin(), m_geometries.end(),
        [geo](const osg::ref_ptr<Geo3D>& ptr) { return ptr.get() == geo; });
    
    if (it != m_geometries.end()) {
        // 从场景图中移除
        if (geo->mm_node()) {
            osg::Node* geoOSGNode = geo->mm_node()->getOSGNode();
            if (geoOSGNode) {
                m_geometryNode->removeChild(geoOSGNode);
            }
        }
        
        // 从选择列表中移除
        removeFromSelection(geo);
        
        // 从几何体列表中移除
        m_geometries.erase(it);
        
        LOG_INFO(QString("从场景移除几何体: %1").arg(geoType3DToString(geo->getGeoType())), "场景管理器");
    }
}

void SceneManager3D::removeAllGeometries()
{
    // 清空选择
    clearSelection();
    
    // 从场景图中移除所有几何体
    m_geometryNode->removeChildren(0, m_geometryNode->getNumChildren());
    
    // 清空几何体列表
    m_geometries.clear();
    
    LOG_INFO("清空所有几何体", "场景管理器");
}

// ========================================= 选择管理 =========================================

void SceneManager3D::setSelectedGeometry(Geo3D::Ptr geo)
{
    // 清空当前选择
    clearSelection();
    
    if (geo) {
        m_selectedGeometry = geo;
        m_selectedGeometries.push_back(geo);
        
        // 设置几何体选中状态
        if (geo->mm_state()) {
            geo->mm_state()->setStateSelected();
        }
    }
    
    LOG_INFO(QString("设置选中几何体: %1").arg(geo ? geoType3DToString(geo->getGeoType()) : "无"), "场景管理器");
}

void SceneManager3D::addToSelection(Geo3D::Ptr geo)
{
    if (!geo || isSelected(geo)) return;
    
    m_selectedGeometries.push_back(geo);
    
    // 设置几何体选中状态
    if (geo->mm_state()) {
        geo->mm_state()->setStateSelected();
    }
    
    // 如果是第一个选中的几何体，也设置为主要选中几何体
    if (!m_selectedGeometry.valid()) {
        m_selectedGeometry = geo;
    }
    
    LOG_INFO(QString("添加到选择: 对象类型=%1, 总选择数=%2")
        .arg(geoType3DToString(geo->getGeoType()))
        .arg(m_selectedGeometries.size()), "场景管理器");
}

void SceneManager3D::removeFromSelection(Geo3D::Ptr geo)
{
    if (!geo) return;
    
    auto it = std::find(m_selectedGeometries.begin(), m_selectedGeometries.end(), geo);
    if (it != m_selectedGeometries.end()) {
        m_selectedGeometries.erase(it);
        
        // 取消几何体选中状态
        if (geo->mm_state()) {
            geo->mm_state()->clearStateSelected();
        }
        
        // 如果移除的是主要选中几何体，更新主要选中几何体
        if (m_selectedGeometry.get() == geo) {
            m_selectedGeometry = m_selectedGeometries.empty() ? nullptr : m_selectedGeometries[0];
        }
        
        LOG_INFO(QString("从选择中移除: 对象类型=%1, 剩余选择数=%2")
            .arg(geoType3DToString(geo->getGeoType()))
            .arg(m_selectedGeometries.size()), "场景管理器");
    }
}

void SceneManager3D::clearSelection()
{
    // 取消所有几何体的选中状态
    for (const auto& geo : m_selectedGeometries) {
        if (geo && geo->mm_state()) {
            geo->mm_state()->clearStateSelected();
        }
    }
    
    m_selectedGeometries.clear();
    m_selectedGeometry = nullptr;
    
    LOG_INFO("清空所有选择", "场景管理器");
}

bool SceneManager3D::isSelected(Geo3D::Ptr geo) const
{
    return std::find(m_selectedGeometries.begin(), m_selectedGeometries.end(), geo) != m_selectedGeometries.end();
}

// ========================================= 拾取系统 =========================================

PickResult SceneManager3D::performPicking(int mouseX, int mouseY)
{
    if (!m_geometryPickingSystem) {
        LOG_WARNING("拾取系统未初始化", "场景管理器");
        return PickResult();
    }
    
    return m_geometryPickingSystem->pickGeometry(mouseX, mouseY);
}

// ========================================= 显示模式 =========================================

void SceneManager3D::setWireframeMode(bool wireframe)
{
    osg::StateSet* stateSet = m_geometryNode->getOrCreateStateSet();
    
    if (wireframe) {
        osg::ref_ptr<osg::PolygonMode> polygonMode = new osg::PolygonMode;
        polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
        stateSet->setAttributeAndModes(polygonMode, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    } else {
        stateSet->removeAttribute(osg::StateAttribute::POLYGONMODE);
    }
    
    LOG_INFO(QString("设置线框模式: %1").arg(wireframe ? "开启" : "关闭"), "场景管理器");
}

void SceneManager3D::setShadedMode(bool shaded)
{
    osg::StateSet* stateSet = m_geometryNode->getOrCreateStateSet();
    
    if (shaded) {
        osg::ref_ptr<osg::PolygonMode> polygonMode = new osg::PolygonMode;
        polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
        stateSet->setAttributeAndModes(polygonMode, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    } else {
        stateSet->removeAttribute(osg::StateAttribute::POLYGONMODE);
    }
    
    LOG_INFO(QString("设置着色模式: %1").arg(shaded ? "开启" : "关闭"), "场景管理器");
}

void SceneManager3D::setPointMode(bool point)
{
    osg::StateSet* stateSet = m_geometryNode->getOrCreateStateSet();
    
    if (point) {
        osg::ref_ptr<osg::PolygonMode> polygonMode = new osg::PolygonMode;
        polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::POINT);
        stateSet->setAttributeAndModes(polygonMode, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    } else {
        stateSet->removeAttribute(osg::StateAttribute::POLYGONMODE);
    }
    
    LOG_INFO(QString("设置点模式: %1").arg(point ? "开启" : "关闭"), "场景管理器");
}

// ========================================= 天空盒管理 =========================================

void SceneManager3D::enableSkybox(bool enabled)
{
    m_skyboxEnabled = enabled;
    
    if (m_skybox) {
        if (enabled) {
            setupSkybox();
        } else {
            m_skyboxNode->removeChildren(0, m_skyboxNode->getNumChildren());
        }
    }
    
    LOG_INFO(QString("天空盒: %1").arg(enabled ? "启用" : "禁用"), "场景管理器");
}

void SceneManager3D::setSkyboxGradient(const osg::Vec4& topColor, const osg::Vec4& bottomColor)
{
    if (m_skybox) {
        m_skybox->setGradientSkybox(topColor, bottomColor);
        refreshSkybox();
    }
}

void SceneManager3D::setSkyboxSolidColor(const osg::Vec4& color)
{
    if (m_skybox) {
        m_skybox->setSolidColorSkybox(color);
        refreshSkybox();
    }
}

void SceneManager3D::setSkyboxCubeMap(const std::string& positiveX, const std::string& negativeX,
                                     const std::string& positiveY, const std::string& negativeY,
                                     const std::string& positiveZ, const std::string& negativeZ)
{
    if (m_skybox) {
        m_skybox->setCubeMapTexture(positiveX, negativeX, positiveY, negativeY, positiveZ, negativeZ);
        refreshSkybox();
    }
}

void SceneManager3D::refreshSkybox()
{
    if (m_skybox && m_skyboxEnabled) {
        m_skyboxNode->removeChildren(0, m_skyboxNode->getNumChildren());
        
        osg::Node* skyboxNode = m_skybox->getSkyboxNode();
        if (skyboxNode) {
            m_skyboxNode->addChild(skyboxNode);
        }
        
        LOG_INFO("刷新天空盒", "场景管理器");
    }
}

// ========================================= 坐标系管理 =========================================

void SceneManager3D::enableCoordinateSystem(bool enabled)
{
    m_coordinateSystemEnabled = enabled;
    
    if (m_coordinateSystemRenderer) {
        if (enabled) {
            setupCoordinateSystem();
        } else {
            // 移除坐标系节点
            osg::Node* coordNode = m_coordinateSystemRenderer->getCoordinateSystemNode();
            if (coordNode) {
                m_sceneNode->removeChild(coordNode);
            }
        }
    }
    
    LOG_INFO(QString("坐标系: %1").arg(enabled ? "启用" : "禁用"), "场景管理器");
}

void SceneManager3D::refreshCoordinateSystem()
{
    if (m_coordinateSystemRenderer && m_coordinateSystemEnabled) {
        // 移除旧的坐标系节点
        osg::Node* oldCoordNode = m_coordinateSystemRenderer->getCoordinateSystemNode();
        if (oldCoordNode) {
            m_sceneNode->removeChild(oldCoordNode);
        }
        
        // 重新更新坐标系
        m_coordinateSystemRenderer->updateCoordinateSystem();
        osg::Node* newCoordNode = m_coordinateSystemRenderer->getCoordinateSystemNode();
        if (newCoordNode) {
            m_sceneNode->addChild(newCoordNode);
        }
        
        LOG_INFO("刷新坐标系", "场景管理器");
    }
}

// ========================================= 相机操作 =========================================

void SceneManager3D::resetCamera()
{
    LOG_INFO("重置相机（由相机控制器处理）", "场景管理器");
}

void SceneManager3D::fitAll()
{
    LOG_INFO("适应全部（由相机控制器处理）", "场景管理器");
}

void SceneManager3D::setViewDirection(const glm::dvec3& direction, const glm::dvec3& up)
{
    LOG_INFO("设置视图方向（由相机控制器处理）", "场景管理器");
}

// ========================================= 绘制管理 =========================================

Geo3D::Ptr SceneManager3D::startDrawing(DrawMode3D mode)
{
    if (m_isDrawing) {
        LOG_WARNING("已在绘制状态，取消当前绘制", "场景管理器");
        cancelDrawing();
    }
    
    // 创建新的几何体
    m_currentDrawingGeometry = GeometryFactory::createGeometry(mode);
    if (!m_currentDrawingGeometry.valid()) {
        LOG_ERROR(QString("无法创建几何体: %1").arg(drawMode3DToString(mode)), "场景管理器");
        return nullptr;
    }
    
    m_isDrawing = true;
    
    // 添加到场景（但还未完成）
    addGeometry(m_currentDrawingGeometry.get());
    
    LOG_INFO(QString("开始绘制: %1").arg(drawMode3DToString(mode)), "场景管理器");
    return m_currentDrawingGeometry.get();
}

Geo3D::Ptr SceneManager3D::completeDrawing()
{
    if (!m_isDrawing || !m_currentDrawingGeometry.valid()) {
        LOG_WARNING("没有正在进行的绘制", "场景管理器");
        return nullptr;
    }
    
    Geo3D::Ptr completedGeo = m_currentDrawingGeometry.get();
    
    // 设置几何体状态为完成
    if (completedGeo->mm_state()) {
        completedGeo->mm_state()->setStateComplete();
    }
    
    // 重置绘制状态
    m_isDrawing = false;
    m_currentDrawingGeometry = nullptr;
    
    LOG_INFO("完成绘制", "场景管理器");
    return completedGeo;
}

void SceneManager3D::cancelDrawing()
{
    if (!m_isDrawing || !m_currentDrawingGeometry.valid()) {
        LOG_WARNING("没有正在进行的绘制", "场景管理器");
        return;
    }
    
    // 从场景中移除未完成的几何体
    removeGeometry(m_currentDrawingGeometry.get());
    
    // 重置绘制状态
    m_isDrawing = false;
    m_currentDrawingGeometry = nullptr;
    
    LOG_INFO("取消绘制", "场景管理器");
}

void SceneManager3D::updateDrawingPreview(const glm::dvec3& worldPos)
{
    if (!m_isDrawing || !m_currentDrawingGeometry.valid()) {
        return;
    }
    
    // 更新绘制预览
    auto controlPointManager = m_currentDrawingGeometry->mm_controlPoint();
    if (controlPointManager) {
        controlPointManager->addControlPoint(Point3D(worldPos));
    }
}

// ========================================= 控制点拖动 =========================================

void SceneManager3D::startDraggingControlPoint(Geo3D::Ptr geo, int controlPointIndex)
{
    if (!geo || controlPointIndex < 0) {
        LOG_WARNING("无效的几何体或控制点索引", "场景管理器");
        return;
    }
    
    m_isDraggingControlPoint = true;
    m_draggingGeometry = geo;
    m_draggingControlPointIndex = controlPointIndex;
    
    LOG_INFO(QString("开始拖动控制点: 几何体=%1, 索引=%2")
        .arg(geoType3DToString(geo->getGeoType()))
        .arg(controlPointIndex), "场景管理器");
}

void SceneManager3D::stopDraggingControlPoint()
{
    m_isDraggingControlPoint = false;
    m_draggingGeometry = nullptr;
    m_draggingControlPointIndex = -1;
    
    LOG_INFO("停止拖动控制点", "场景管理器");
}

void SceneManager3D::updateDraggingControlPoint(const glm::dvec3& worldPos)
{
    if (!m_isDraggingControlPoint || !m_draggingGeometry) {
        return;
    }
    
    auto controlPointManager = m_draggingGeometry->mm_controlPoint();
    if (controlPointManager) {
        Point3D newPoint(worldPos.x, worldPos.y, worldPos.z);
        controlPointManager->setControlPoint(m_draggingControlPointIndex, newPoint);
    }
} 
