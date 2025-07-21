#include "OSGWidget.h"
#include "../core/Common3D.h"
#include "../core/GeometryBase.h"
#include "../core/picking/OSGIndexPickingSystem.h"

#include "../util/LogManager.h"
#include <osgViewer/Viewer>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/TerrainManipulator>
#include <osg/Node>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/PolygonMode>
#include <osg/StateSet>
#include <osg/LightSource>
#include <osg/Light>
#include <osg/PositionAttitudeTransform>
#include <osg/Math>
#include <osg/BlendFunc>
#include <osg/Multisample>
#include <osgDB/Registry>
#include <osgQOpenGL/osgQOpenGLWidget>
#include <QTimer>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QApplication>
#include <QDebug>
#include <algorithm>
#include <QFont>
#include <QColor>
#include <cmath>
#include <QDateTime> // Added for cache
#include <QProcessEnvironment>
#include <thread>

// ========================================= OSGWidget 实现 =========================================
OSGWidget::OSGWidget(QWidget* parent)
    : osgQOpenGLWidget(parent)
    , m_rootNode(new osg::Group)
    , m_sceneNode(new osg::Group)
    , m_geoNode(new osg::Group)
    , m_lightNode(new osg::Group)
    , m_pickingIndicatorNode(new osg::Group)
    , m_skyboxNode(new osg::Group)
    , m_cameraController(std::make_unique<CameraController>())
    , m_currentDrawingGeo(nullptr)
    , m_selectedGeo(nullptr)
    , m_isDrawing(false)
    , m_lastMouseWorldPos(0.0f)
    , m_advancedPickingEnabled(false)
    , m_skybox(std::make_unique<Skybox>())
    , m_skyboxEnabled(true)
    , m_coordinateSystemRenderer(std::make_unique<CoordinateSystemRenderer>())
    , m_coordinateSystemEnabled(true)
    , m_scaleBarEnabled(true)
    , m_scaleBarPosition(10, 10)
    , m_scaleBarSize(200, 60)
    , m_updateTimer(new QTimer(this))
    , m_cachedScaleValue(0.0)
    , m_mousePosCacheValid(false)
    , m_multiSelectMode(false)
    , m_isDraggingControlPoint(false)
    , m_draggingGeo(nullptr)
    , m_draggingControlPointIndex(-1)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setFocus(); // 确保获得焦点
    
    // 连接信号槽
    connect(m_updateTimer, &QTimer::timeout, this, [this]() {
        update();
    });
    
    int* a = new int[8];
    assert(a);

    assert(m_cameraController.get());

    assert(qobject_cast<QObject*>(m_cameraController.get()));

    // 连接CameraController的信号
    connect(m_cameraController.get(), &CameraController::cameraMoveSpeedChanged, 
            this, &OSGWidget::cameraMoveSpeedChanged);
    connect(m_cameraController.get(), &CameraController::wheelMoveSensitivityChanged, 
            this, &OSGWidget::wheelMoveSensitivityChanged);
    connect(m_cameraController.get(), &CameraController::accelerationRateChanged, 
            this, &OSGWidget::accelerationRateChanged);
    connect(m_cameraController.get(), &CameraController::maxAccelerationSpeedChanged, 
            this, &OSGWidget::maxAccelerationSpeedChanged);
    connect(m_cameraController.get(), &CameraController::manipulatorTypeChanged, 
            this, &OSGWidget::manipulatorTypeChanged);
    
    // 连接初始化完成信号
    connect(this, &osgQOpenGLWidget::initialized, this, &OSGWidget::initializeScene);
    
    // 设置渲染循环
    m_updateTimer->start(16);
    
    LOG_INFO("OSGWidget初始化完成", "系统");
    // 渲染循环已启动（移除调试日志）
}

OSGWidget::~OSGWidget()
{
    if (m_updateTimer)
    {
        m_updateTimer->stop();
    }
}

void OSGWidget::initializeScene()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;

    // 单线程模式（单线程模式下，更方便调试）
    //viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    
    // 设置场景图
    m_rootNode->addChild(m_sceneNode);
    m_rootNode->addChild(m_lightNode);
    m_rootNode->addChild(m_pickingIndicatorNode);
    m_rootNode->addChild(m_skyboxNode);
    m_sceneNode->addChild(m_geoNode);
    
    // 为根节点设置抗锯齿
    osg::StateSet* rootStateSet = m_rootNode->getOrCreateStateSet();
    rootStateSet->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    rootStateSet->setMode(GL_POINT_SMOOTH, osg::StateAttribute::ON);
    rootStateSet->setMode(GL_MULTISAMPLE, osg::StateAttribute::ON);
    
    // 设置混合函数以支持线抗锯齿
    osg::ref_ptr<osg::BlendFunc> rootBlendFunc = new osg::BlendFunc();
    rootBlendFunc->setSource(GL_SRC_ALPHA);
    rootBlendFunc->setDestination(GL_ONE_MINUS_SRC_ALPHA);
    rootStateSet->setAttributeAndModes(rootBlendFunc.get(), osg::StateAttribute::ON);
    
    // 设置多重采样抗锯齿（如果硬件支持）
    osg::ref_ptr<osg::Multisample> rootMultisample = new osg::Multisample();
    rootMultisample->setCoverage(0.5f);
    rootStateSet->setAttributeAndModes(rootMultisample.get(), osg::StateAttribute::ON);
    
    // 设置线宽和点大小
    osg::ref_ptr<osg::LineWidth> rootLineWidth = new osg::LineWidth();
    rootLineWidth->setWidth(1.0f);  // 设置更细的线宽以更好地展示抗锯齿
    rootStateSet->setAttributeAndModes(rootLineWidth.get(), osg::StateAttribute::ON);
    
    osg::ref_ptr<osg::Point> rootPointSize = new osg::Point();
    rootPointSize->setSize(3.0f);  // 设置更小的点大小
    rootStateSet->setAttributeAndModes(rootPointSize.get(), osg::StateAttribute::ON);
    
    // 启用混合以支持抗锯齿
    rootStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    
    viewer->setSceneData(m_rootNode);
    
    // 设置摄像机控制器
    m_cameraController->setViewer(viewer);
    
    setupCamera();
    setupLighting();
    setupEventHandlers();
    setupPickingSystem();
    setupSkybox();
    setupCoordinateSystem();
}

void OSGWidget::setupCamera()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    osg::Camera* camera = viewer->getCamera();
    
    // 设置基本的渲染状态
    osg::StateSet* stateSet = camera->getOrCreateStateSet();
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    
    // 设置背景色
    camera->setClearColor(osg::Vec4(0.2f, 0.2f, 0.2f, 1.0f));
    
    // 设置初始视点
    resetCamera();
}

void OSGWidget::setupLighting()
{
    // 创建光源
    osg::ref_ptr<osg::Light> light = new osg::Light();
    light->setLightNum(0);
    light->setPosition(osg::Vec4(10.0f, 10.0f, 10.0f, 1.0f));
    light->setDirection(osg::Vec3(-1.0f, -1.0f, -1.0f));
    light->setAmbient(osg::Vec4(0.3f, 0.3f, 0.3f, 1.0f));
    light->setDiffuse(osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f));
    light->setSpecular(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    
    osg::ref_ptr<osg::LightSource> lightSource = new osg::LightSource();
    lightSource->setLight(light.get());
    lightSource->setLocalStateSetModes(osg::StateAttribute::ON);
    
    m_lightNode->addChild(lightSource.get());
    
    // 设置全局环境光
    osg::StateSet* stateSet = m_rootNode->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHT0, osg::StateAttribute::ON);
}

void OSGWidget::setupEventHandlers()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    // 添加事件处理器
    viewer->addEventHandler(new osgViewer::StatsHandler());
    viewer->addEventHandler(new osgViewer::WindowSizeHandler());
    viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
}

void OSGWidget::setupPickingSystem()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    if (!OSGIndexPickingSystemManager::getInstance().initialize(
        viewer->getCamera(), m_geoNode.get()))
    {
        LOG_ERROR("Failed to initialize simplified picking system", "拾取");
        return;
    }
    
    // 设置拾取配置
    OSGIndexPickConfig config;
    config.pickingRadius = 15;       // 拾取半径
    config.snapThreshold = 0.2f;     // 捕捉阈值（适中的值）
    config.enableSnapping = true;    // 启用捕捉，但会优先使用拾取坐标
    config.enableIndicator = true;   // 确保指示器启用
    config.enableHighlight = true;   // 确保高亮启用
    config.indicatorSize = 0.3f;     // 设置指示器大小
    config.pickingFrequency = 60.0;  // 拾取频率
    OSGIndexPickingSystemManager::getInstance().setConfig(config);
    
    // 设置拾取回调
    OSGIndexPickingSystemManager::getInstance().setPickingCallback(
        [this](const OSGIndexPickResult& result) {
            emit simplePickingResult(result);
            
            // 如果有拾取结果，立即更新状态栏坐标（覆盖普通坐标）
            if (result.hasResult)
            {
                m_lastMouseWorldPos = result.worldPosition;
                emit mousePositionChanged(result.worldPosition);
            }
        });
    
    // 添加事件处理器
    osgGA::GUIEventHandler* eventHandler = OSGIndexPickingSystemManager::getInstance().getEventHandler();
    if (eventHandler) {
        viewer->addEventHandler(eventHandler);
    }
    
    // 将指示器根节点添加到场景图
    osg::Group* indicatorRoot = OSGIndexPickingSystemManager::getInstance().getIndicatorRoot();
    if (indicatorRoot) {
        m_pickingIndicatorNode->addChild(indicatorRoot);
        LOG_INFO("Added simplified picking indicator root to scene graph", "拾取");
    }
    
    m_advancedPickingEnabled = true;
    
    LOG_SUCCESS("Simplified picking system initialized successfully", "拾取");
}

void OSGWidget::resetCamera()
{
    if (!m_cameraController) return;
    
    // 只考虑实际的几何对象，不考虑坐标系统
    osg::BoundingSphere bs = m_geoNode->getBound();
    
    if (bs.valid() && bs.radius() > 0)
    {
        // 如果有几何对象，使用几何对象的包围盒
        osg::Vec3d center = bs.center();
        double radius = bs.radius();
        double distance = radius * 2.0; // 距离为半径的2倍
        
        osg::Vec3d eye = center + osg::Vec3d(distance, distance, distance);
        m_cameraController->setPosition(eye, center, osg::Vec3d(0, 0, 1));
    }
    else
    {
        // 如果没有几何对象，使用默认位置
        osg::Vec3d center(0, 0, 0);
        osg::Vec3d eye(10, 10, 10);
        m_cameraController->setPosition(eye, center, osg::Vec3d(0, 0, 1));
    }
}

void OSGWidget::fitAll()
{
    if (!m_cameraController || !m_geoNode.valid()) return;
    
    // 只考虑实际的几何对象，不考虑坐标系统
    osg::BoundingSphere bs = m_geoNode->getBound();
    
    if (bs.valid() && bs.radius() > 0)
    {
        // 如果有几何对象，使用几何对象的包围盒
        osg::Vec3d center = bs.center();
        double radius = bs.radius();
        double distance = radius * 2.5; // 距离为半径的2.5倍，留一些边距
        
        osg::Vec3d eye = center + osg::Vec3d(distance, distance, distance);
        m_cameraController->setPosition(eye, center, osg::Vec3d(0, 0, 1));
    }
    else
    {
        // 如果没有几何对象，使用默认位置
        osg::Vec3d center(0, 0, 0);
        osg::Vec3d eye(10, 10, 10);
        m_cameraController->setPosition(eye, center, osg::Vec3d(0, 0, 1));
    }
}

void OSGWidget::setViewDirection(const glm::vec3& direction, const glm::vec3& up)
{
    if (!m_cameraController) return;
    
    // 只考虑实际的几何对象，不考虑坐标系统
    osg::BoundingSphere bs = m_geoNode->getBound();
    
    osg::Vec3d center;
    double distance;
    
    if (bs.valid() && bs.radius() > 0)
    {
        // 如果有几何对象，使用几何对象的包围盒
        center = bs.center();
        distance = bs.radius() * 3.0;
    }
    else
    {
        // 如果没有几何对象，使用默认值
        center = osg::Vec3d(0, 0, 0);
        distance = 10.0;
    }
    
    osg::Vec3d eye = center - osg::Vec3d(direction.x, direction.y, direction.z) * distance;
    m_cameraController->setPosition(eye, center, osg::Vec3d(up.x, up.y, up.z));
}

void OSGWidget::setWireframeMode(bool wireframe)
{
    osg::StateSet* stateSet = m_geoNode->getOrCreateStateSet();
    if (wireframe)
    {
        osg::PolygonMode* polyMode = new osg::PolygonMode();
        polyMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
        stateSet->setAttributeAndModes(polyMode, osg::StateAttribute::ON);
    }
    else
    {
        stateSet->removeAttribute(osg::StateAttribute::POLYGONMODE);
    }
}

void OSGWidget::setShadedMode(bool shaded)
{
    osg::StateSet* stateSet = m_geoNode->getOrCreateStateSet();
    if (shaded)
    {
        stateSet->removeAttribute(osg::StateAttribute::POLYGONMODE);
    }
}

void OSGWidget::setPointMode(bool point)
{
    osg::StateSet* stateSet = m_geoNode->getOrCreateStateSet();
    if (point)
    {
        osg::PolygonMode* polyMode = new osg::PolygonMode();
        polyMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::POINT);
        stateSet->setAttributeAndModes(polyMode, osg::StateAttribute::ON);
    }
    else
    {
        stateSet->removeAttribute(osg::StateAttribute::POLYGONMODE);
    }
}

void OSGWidget::addGeo(Geo3D* geo)
{
    if (geo && m_geoNode.valid())
    {
        osg::ref_ptr<Geo3D> geoRef(geo);
        m_geoList.push_back(geoRef);
        
        // 获取几何体的OSG节点
        osg::ref_ptr<osg::Group> geoOSGNode = geo->mm_node()->getOSGNode();
        if (geoOSGNode.valid()) {
            m_geoNode->addChild(geoOSGNode.get());
            qDebug() << "Added geometry to scene:" << geo->getGeoType() << "children:" << geoOSGNode->getNumChildren();
        } else {
            qDebug() << "Warning: Geometry has no valid OSG node:" << geo->getGeoType();
        }
        
        // 连接几何对象的信号
        // connect(geo, &Geo3D::drawingCompleted, this, &OSGWidget::onGeoDrawingCompleted);
        // connect(geo, &Geo3D::geometryUpdated, this, &OSGWidget::onGeoGeometryUpdated);
        // connect(geo, &Geo3D::parametersChanged, this, &OSGWidget::onGeoParametersChanged);
        
        // 添加到拾取系统 - 对于从文件IO读入的几何对象，强制添加到拾取系统
        if (m_advancedPickingEnabled)
        {
            // 检查几何对象是否已经完成绘制
            if (geo->mm_state()->isStateComplete())
            {
                OSGIndexPickingSystemManager::getInstance().addGeometry(geo);
                // 添加完成的几何体到拾取系统（移除调试日志）
            }
            else
            {
                // 对于未完成的几何对象（如从文件读入的），也强制添加到拾取系统
                // 这样可以确保文件IO读入的几何对象能够被拾取
                OSGIndexPickingSystemManager::getInstance().addGeometry(geo);
                // 添加未完成的几何体到拾取系统（移除调试日志）
            }
        }
        else
        {
            // 高级拾取已禁用（移除调试日志）
        }
    }
}

void OSGWidget::removeGeo(Geo3D* geo)
{
    if (geo && m_geoNode.valid())
    {
        auto it = std::find_if(m_geoList.begin(), m_geoList.end(), 
            [geo](const osg::ref_ptr<Geo3D>& ref) { return ref.get() == geo; });
        if (it != m_geoList.end())
        {
            // 断开信号连接
            // disconnect(geo, &Geo3D::drawingCompleted, this, &OSGWidget::onGeoDrawingCompleted);
            // disconnect(geo, &Geo3D::geometryUpdated, this, &OSGWidget::onGeoGeometryUpdated);
            // disconnect(geo, &Geo3D::parametersChanged, this, &OSGWidget::onGeoParametersChanged);
            
            m_geoNode->removeChild(geo->mm_node()->getOSGNode().get());
            m_geoList.erase(it);
            
            // 从OSG索引拾取系统移除
            if (m_advancedPickingEnabled)
            {
                OSGIndexPickingSystemManager::getInstance().removeGeometry(geo);
            }
        }
    }
}

void OSGWidget::removeAllGeos()
{
    if (m_geoNode.valid())
    {
        m_geoNode->removeChildren(0, m_geoNode->getNumChildren());
        m_geoList.clear();
        m_selectedGeo = nullptr;
        m_currentDrawingGeo = nullptr;
        
        // 清除拾取系统中的所有几何对象
        if (m_advancedPickingEnabled)
        {
            OSGIndexPickingSystemManager::getInstance().clearAllGeometries();
            // 清除所有几何体（移除调试日志）
        }
    }
}

void OSGWidget::selectGeo(Geo3D* geo)
{
    if (m_selectedGeo)
    {
        m_selectedGeo->mm_state()->clearStateSelected();
    }
    
    m_selectedGeo = geo;
    if (m_selectedGeo)
    {
        m_selectedGeo->mm_state()->setStateSelected();
    }
    
    emit geoSelected(geo);
}

void OSGWidget::deselectAll()
{
    selectGeo(nullptr);
}

// 多选功能实现
void OSGWidget::addToSelection(Geo3D* geo)
{
    if (!geo) return;
    
    LOG_INFO(QString("尝试添加到选择: 对象类型=%1").arg(geo->getGeoType()), "选择");
    
    // 检查是否已经在选中列表中
    auto it = std::find(m_selectedGeos.begin(), m_selectedGeos.end(), geo);
    if (it == m_selectedGeos.end())
    {
        LOG_INFO(QString("对象不在选择列表中，开始添加"), "选择");
        
        m_selectedGeos.push_back(geo);
        geo->mm_state()->setStateSelected(); // 这会自动显示包围盒
        
        // 显示控制点高亮
        if (m_advancedPickingEnabled)
        {
            OSGIndexPickingSystemManager::getInstance().showSelectionHighlight(geo);
        }
        
        // 发送选择信号
        emit geoSelected(geo);
        
        LOG_INFO(QString("添加到选择: 对象类型=%1, 总选择数=%2")
            .arg(geo->getGeoType())
            .arg(m_selectedGeos.size()), "选择");
    }
    else
    {
        LOG_INFO(QString("对象已在选择列表中，跳过"), "选择");
    }
}

void OSGWidget::removeFromSelection(Geo3D* geo)
{
    if (!geo) return;
    
    auto it = std::find(m_selectedGeos.begin(), m_selectedGeos.end(), geo);
    if (it != m_selectedGeos.end())
    {
        m_selectedGeos.erase(it);
        geo->mm_state()->clearStateSelected(); // 这会自动隐藏包围盒
        
        // 清除控制点高亮
        if (m_advancedPickingEnabled)
        {
            OSGIndexPickingSystemManager::getInstance().hideSelectionHighlight();
        }
        
        // 如果移除的是当前选中的对象，清空当前选中
        if (m_selectedGeo.get() == geo)
        {
            m_selectedGeo = nullptr;
        }
        
        // 发送选择信号
        emit geoSelected(nullptr);
        
        LOG_INFO(QString("从选择中移除: 对象类型=%1, 剩余选择数=%2")
            .arg(geo->getGeoType())
            .arg(m_selectedGeos.size()), "选择");
    }
}

void OSGWidget::clearSelection()
{
    // 清除所有选中对象的状态
    for (auto* geo : m_selectedGeos)
    {
        if (geo)
        {
            geo->mm_state()->clearStateSelected(); // 这会自动隐藏包围盒
        }
    }
    
    // 清除控制点高亮
    if (m_advancedPickingEnabled)
    {
        OSGIndexPickingSystemManager::getInstance().hideSelectionHighlight();
    }
    
    m_selectedGeos.clear();
    m_selectedGeo = nullptr;
    
    // 发送选择信号
    emit geoSelected(nullptr);
    
    LOG_INFO("清除所有选择", "选择");
}

const std::vector<Geo3D*>& OSGWidget::getSelectedGeos() const
{
    return m_selectedGeos;
}

bool OSGWidget::isSelected(Geo3D* geo) const
{
    if (!geo) return false;
    return std::find(m_selectedGeos.begin(), m_selectedGeos.end(), geo) != m_selectedGeos.end();
}

int OSGWidget::getSelectionCount() const
{
    return static_cast<int>(m_selectedGeos.size());
}

// 拖动控制点功能实现
void OSGWidget::startDraggingControlPoint(Geo3D* geo, int controlPointIndex)
{
    if (!geo || controlPointIndex < 0) return;
    
    m_isDraggingControlPoint = true;
    m_draggingGeo = geo;
    m_draggingControlPointIndex = controlPointIndex;
    m_dragStartPosition = m_lastMouseWorldPos;
    
            // 开始拖动控制点（移除调试日志）
}

void OSGWidget::stopDraggingControlPoint()
{
    if (m_isDraggingControlPoint)
    {
        // 停止拖动控制点（移除调试日志）
    }
    
    m_isDraggingControlPoint = false;
    m_draggingGeo = nullptr;
    m_draggingControlPointIndex = -1;
}

// 高亮管理实现
void OSGWidget::updateSelectionHighlight()
{
    // 暂时禁用高亮更新，避免循环
    LOG_INFO("updateSelectionHighlight被调用，但暂时禁用", "选择");
    return;
    
    // 清除之前的高亮
    if (m_advancedPickingEnabled)
    {
        // 暂时不调用高亮系统，因为SimplifiedPickingSystemManager没有直接的高亮清除方法
        // 高亮会在下一次拾取时自动更新
    }
    
    // 为选中的对象创建高亮和显示包围盒
    highlightSelectedObjects();
}

void OSGWidget::highlightSelectedObjects()
{
    if (!m_advancedPickingEnabled || m_selectedGeos.empty())
        return;
    
    // 为每个选中的对象创建高亮和显示包围盒
    for (auto* geo : m_selectedGeos)
    {
        if (geo)
        {
            // 注意：不要在这里再次调用setStateSelected，因为addToSelection已经调用了
            // 这里只添加额外的高亮效果
            // 暂时使用简化的方式
        }
    }
}

// 高级拾取系统接口
void OSGWidget::enableAdvancedPicking(bool enabled)
{
    m_advancedPickingEnabled = enabled;
}

bool OSGWidget::isAdvancedPickingEnabled() const
{
    return m_advancedPickingEnabled;
}

void OSGWidget::setPickingRadius(int radius)
{
    OSGIndexPickConfig config = OSGIndexPickingSystemManager::getInstance().getConfig();
    config.pickingRadius = radius;
    OSGIndexPickingSystemManager::getInstance().setConfig(config);
}

void OSGWidget::setPickingFrequency(double frequency)
{
    OSGIndexPickConfig config = OSGIndexPickingSystemManager::getInstance().getConfig();
    config.pickingFrequency = frequency;
    OSGIndexPickingSystemManager::getInstance().setConfig(config);
}

void OSGWidget::setPickingConfig(const OSGIndexPickConfig& config)
{
    OSGIndexPickingSystemManager::getInstance().setConfig(config);
    
    LOG_INFO(QString("Updated picking config - Radius: %1, Threshold: %2")
        .arg(config.pickingRadius)
        .arg(config.snapThreshold), "拾取");
}

QString OSGWidget::getPickingSystemInfo() const
{
    return OSGIndexPickingSystemManager::getInstance().getSystemInfo();
}

void OSGWidget::ensureAllGeosInPickingSystem()
{
    if (!m_advancedPickingEnabled) return;
    
    // 确保所有几何对象都在拾取系统中（移除频繁的日志）
    
            for (const auto& geoRef : m_geoList)
        {
            if (geoRef)
            {
                // 使用updateGeometry方法，它会自动检查几何对象是否已经在拾取系统中
                OSGIndexPickingSystemManager::getInstance().updateGeometry(geoRef.get());
            }
        }
    
    // 所有几何对象已确保在拾取系统中（移除频繁的日志）
}

QString OSGWidget::getPickingSystemStatus() const
{
    if (!m_advancedPickingEnabled) {
        return "拾取系统已禁用";
    }
    
    QString status = QString("拾取系统状态:\n");
    status += QString("- 几何对象总数: %1\n").arg(m_geoList.size());
    status += QString("- 拾取系统信息: %1").arg(getPickingSystemInfo());
    
    return status;
}

void OSGWidget::onSimplePickingResult(const OSGIndexPickResult& result)
{
    // 发射OSG索引拾取结果信号
    emit simplePickingResult(result);
    
    // 如果有拾取结果，立即更新状态栏坐标（覆盖普通坐标）
    if (result.hasResult) {
        m_lastMouseWorldPos = result.worldPosition;
        emit mousePositionChanged(result.worldPosition);
    }
}



// PickResult3D OSGWidget::pick(int x, int y)
// {
//     PickResult3D result;
    
//     osgViewer::Viewer* viewer = getOsgViewer();
//     if (!viewer) return result;
    
//     osg::Camera* camera = viewer->getCamera();
    
//     // 创建射线
//     osg::Vec3f nearPoint, farPoint;
//     if (camera->getViewport())
//     {
//         osg::Matrix VPW = camera->getViewMatrix() * 
//                          camera->getProjectionMatrix() * 
//                          camera->getViewport()->computeWindowMatrix();
//         osg::Matrix invVPW;
//         invVPW.invert(VPW);
        
//         nearPoint = osg::Vec3f(x, height() - y, 0.0f) * invVPW;
//         farPoint = osg::Vec3f(x, height() - y, 1.0f) * invVPW;
//     }
    
//     // 计算射线方向和起点
//     glm::vec3 rayOrigin(nearPoint.x(), nearPoint.y(), nearPoint.z());
//     glm::vec3 rayDirection = glm::normalize(glm::vec3(farPoint.x() - nearPoint.x(), 
//                                                       farPoint.y() - nearPoint.y(), 
//                                                       farPoint.z() - nearPoint.z()));
    
//     Ray3D ray(rayOrigin, rayDirection);
    
//     // 添加调试信息
//     LOG_DEBUG(QString("射线拾取: 屏幕坐标(%1,%2), 射线起点(%3,%4,%5), 方向(%6,%7,%8)")
//         .arg(x).arg(y)
//         .arg(rayOrigin.x, 0, 'f', 3).arg(rayOrigin.y, 0, 'f', 3).arg(rayOrigin.z, 0, 'f', 3)
//         .arg(rayDirection.x, 0, 'f', 3).arg(rayDirection.y, 0, 'f', 3).arg(rayDirection.z, 0, 'f', 3), "拾取");
    
//     LOG_DEBUG(QString("几何体数量: %1").arg(m_geoList.size()), "拾取");
    
//     // 测试所有几何对象，使用KDTree支持的快速查询
//     float minDistance = FLT_MAX;
//     for (const osg::ref_ptr<Geo3D>& geo : m_geoList)
//     {
//         if (!geo) continue;
        
//         LOG_DEBUG(QString("测试几何体: 类型=%1, 状态=%2")
//             .arg(geo->getGeoType())
//             .arg(geo->isStateComplete() ? "完成" : "未完成"), "拾取");
        
//         PickResult3D geoResult;
//         // 优先使用KDTree支持的hitTest，如果失败则使用传统方法
//         if (geo->hitTestWithKdTree(ray, geoResult) || geo->hitTestVisible(ray, geoResult))
//         {
//             LOG_DEBUG(QString("几何体命中: 类型=%1, 距离=%2")
//                 .arg(geo->getGeoType())
//                 .arg(geoResult.distance, 0, 'f', 3), "拾取");
            
//             if (geoResult.distance < minDistance)
//             {
//                 minDistance = geoResult.distance;
//                 result = geoResult;
//             }
//         }
//     }
    
//     if (result.hit) {
//         LOG_DEBUG(QString("射线拾取成功: 距离=%1").arg(result.distance, 0, 'f', 3), "拾取");
//     } else {
//         LOG_DEBUG("射线拾取失败: 没有命中任何几何体", "拾取");
//     }
    
//     return result;
// }



glm::vec3 OSGWidget::screenToWorld(int x, int y, float depth)
{
    if (!m_cameraController) return glm::vec3(0, 0, 0);
    
    // 检查鼠标位置缓存
    QPoint currentPos(x, y);
    if (m_mousePosCacheValid && 
        m_lastMouseScreenPos == currentPos &&
        m_lastMouseCalculation.isValid() &&
        m_lastMouseCalculation.msecsTo(QDateTime::currentDateTime()) < MOUSE_CACHE_DURATION)
    {
        return m_cachedMouseWorldPos;
    }
    
    osg::Vec3d worldPoint = m_cameraController->screenToWorld(x, y, depth, width(), height());
    glm::vec3 result(worldPoint.x(), worldPoint.y(), worldPoint.z());
    
    // 更新缓存
    m_lastMouseScreenPos = currentPos;
    m_cachedMouseWorldPos = result;
    m_mousePosCacheValid = true;
    m_lastMouseCalculation = QDateTime::currentDateTime();
    
    return result;
}

glm::vec2 OSGWidget::worldToScreen(const glm::vec3& worldPos)
{
    if (!m_cameraController) return glm::vec2(0, 0);
    
    osg::Vec2d screenPoint = m_cameraController->worldToScreen(osg::Vec3d(worldPos.x, worldPos.y, worldPos.z), width(), height());
    return glm::vec2(screenPoint.x(), screenPoint.y());
}

// OSGWidget 事件处理
void OSGWidget::paintEvent(QPaintEvent* event)
{
    // osgQOpenGLWidget已经处理了渲染，这里不需要手动调用frame()
    QOpenGLWidget::paintEvent(event);
    
    // 更新相机位置（基于时间的移动）- 只在有移动键按下时更新
    if (m_cameraController && m_cameraController->isMoving()) {
        m_cameraController->updateCameraPosition();
    }
    
    // 绘制比例尺 - 只在启用时绘制
    if (m_scaleBarEnabled)
    {
        drawScaleBar();
    }
}

void OSGWidget::resizeEvent(QResizeEvent* event)
{
    osgQOpenGLWidget::resizeEvent(event);
    
    // 更新摄像机控制器的投影矩阵
    if (m_cameraController)
    {
        m_cameraController->updateProjectionMatrix(width(), height());
    }
}

void OSGWidget::mousePressEvent(QMouseEvent* event)
{
    // 处理绘制输入
    handleDrawingInput(event);
    
    // 处理拖动控制点 - 使用拾取系统
    if (GlobalDrawMode3D == DrawSelect3D && event->button() == Qt::LeftButton)
    {
        // 使用拾取系统进行拾取
        OSGIndexPickResult pickResult = OSGIndexPickingSystemManager::getInstance().pick(event->x(), event->y());
        
        if (pickResult.hasResult && pickResult.geometry)
        {
            // 检查是否为顶点拾取（控制点）
            if (pickResult.featureType == PickFeatureType::VERTEX && pickResult.vertexIndex >= 0)
            {
                // 检查该几何体是否在选中列表中
                auto it = std::find(m_selectedGeos.begin(), m_selectedGeos.end(), pickResult.geometry);
                if (it != m_selectedGeos.end())
                {
                    startDraggingControlPoint(pickResult.geometry, pickResult.vertexIndex);
                    event->accept();
                    return;
                }
            }
        }
    }
    
    osgQOpenGLWidget::mousePressEvent(event);
}

void OSGWidget::mouseMoveEvent(QMouseEvent* event)
{
    // 鼠标移动事件处理（移除频繁的调试日志）
    if (GlobalDrawMode3D == DrawSelect3D || (QApplication::keyboardModifiers() & Qt::ControlModifier))
    {
    }
    
    osgQOpenGLWidget::mouseMoveEvent(event);
    
    // 发送屏幕坐标信号
    emit screenPositionChanged(event->x(), event->y());
    
    // 节流机制：每16ms（约60FPS）更新一次鼠标世界坐标
    static QDateTime lastMouseUpdate;
    QDateTime currentTime = QDateTime::currentDateTime();
    if (!lastMouseUpdate.isValid() || lastMouseUpdate.msecsTo(currentTime) >= MOUSE_CACHE_DURATION)
    {
        if (m_advancedPickingEnabled)
        {
            // 使用拾取系统获取世界坐标
            OSGIndexPickResult pickResult = OSGIndexPickingSystemManager::getInstance().pick(event->x(), event->y());
            
            if (pickResult.hasResult)
            {
                // 使用拾取结果的世界坐标覆盖普通坐标
                m_lastMouseWorldPos = pickResult.worldPosition;
                emit mousePositionChanged(pickResult.worldPosition);
            }
            else
            {
                // 如果没有拾取到对象，使用屏幕坐标转换
                glm::vec3 worldPos = screenToWorld(event->x(), event->y(), 0.5f);
                
                // 应用坐标范围限制
                CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
                glm::vec3 clampedPos = coordSystem->clampPointToSkybox(worldPos);
                
                m_lastMouseWorldPos = clampedPos;
                emit mousePositionChanged(clampedPos);
            }
        }
        else
        {
            // 非拾取模式：使用传统的屏幕坐标转换
            glm::vec3 worldPos = screenToWorld(event->x(), event->y(), 0.5f);
            
            // 应用坐标范围限制
            CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
            glm::vec3 clampedPos = coordSystem->clampPointToSkybox(worldPos);
            
            m_lastMouseWorldPos = clampedPos;
            emit mousePositionChanged(clampedPos);
        }
        
        lastMouseUpdate = currentTime;
    }
    
    // 处理拖动控制点
    if (m_isDraggingControlPoint && m_draggingGeo && m_draggingControlPointIndex >= 0)
    {
        // 计算拖动偏移
        glm::vec3 dragOffset = m_lastMouseWorldPos - m_dragStartPosition;
        
        // 更新控制点位置
        const auto& controlPoints = m_draggingGeo->mm_controlPoint()->getControlPoints();
        if (m_draggingControlPointIndex < static_cast<int>(controlPoints.size()))
        {
            // 更新控制点
            Point3D newPoint = controlPoints[m_draggingControlPointIndex];
            newPoint.position += dragOffset;
            m_draggingGeo->mm_controlPoint()->setControlPoint(m_draggingControlPointIndex, newPoint);
            
            // 更新拖动起始位置
            m_dragStartPosition = m_lastMouseWorldPos;
            
            // 拖动控制点更新（移除频繁的调试日志）
        }
        else
        {
            // 包围盒控制点功能已移除，直接使用OSG的包围盒
            m_dragStartPosition = m_lastMouseWorldPos;
            
            // 拖动包围盒控制点功能已移除
        }
    }
    
    // 处理绘制预览 - 使用拾取系统获取更精确的世界坐标
    if (m_isDrawing && m_currentDrawingGeo)
    {
        // 使用拾取系统获取世界坐标
        OSGIndexPickResult pickResult = OSGIndexPickingSystemManager::getInstance().pick(event->x(), event->y());
        glm::vec3 drawingWorldPos;
        
        if (pickResult.hasResult)
        {
            // 使用拾取结果的世界坐标覆盖普通坐标
            drawingWorldPos = pickResult.worldPosition;
            m_lastMouseWorldPos = pickResult.worldPosition;
            // 更新状态栏坐标
            emit mousePositionChanged(pickResult.worldPosition);
        }
        else
        {
            // 如果没有拾取到对象，使用缓存的位置
            drawingWorldPos = m_lastMouseWorldPos;
        }
        
        updateCurrentDrawing(drawingWorldPos);
    }
}

void OSGWidget::mouseReleaseEvent(QMouseEvent* event)
{
    // 处理拖动控制点结束
    if (m_isDraggingControlPoint)
    {
        stopDraggingControlPoint();
        event->accept();
        return;
    }
    
    osgQOpenGLWidget::mouseReleaseEvent(event);
}

void OSGWidget::wheelEvent(QWheelEvent* event)
{
    // 检查是否按住Ctrl键进行快速移动
    if (event->modifiers() & Qt::ControlModifier)
    {
        // Ctrl + 滚轮 = 快速前后移动
        int delta = event->angleDelta().y();
        
        // Ctrl+滚轮缩放（移除调试日志）
        
        if (m_cameraController)
        {
            m_cameraController->handleWheelZoom(delta);
        }
        
        event->accept();
    }
    else
    {
        // 正常的滚轮缩放（OSG默认行为）
        int delta = event->angleDelta().y();
        // 滚轮缩放（移除调试日志）
        
        osgQOpenGLWidget::wheelEvent(event);
    }
}

void OSGWidget::handleDrawingInput(QMouseEvent* event)
{
    // 右键取消绘制
    if (event->button() == Qt::RightButton && m_isDrawing)
    {
        cancelCurrentDrawing();
        return;
    }
    
    // 只处理左键事件
    if (event->button() != Qt::LeftButton)
        return;
    
    if (GlobalDrawMode3D == DrawSelect3D)
    {
        // 选择模式：使用拾取系统进行选择
        OSGIndexPickResult pickResult = OSGIndexPickingSystemManager::getInstance().pick(event->x(), event->y());
        
        // 检查是否按下了Ctrl键（多选模式）
        bool isCtrlPressed = (QApplication::keyboardModifiers() & Qt::ControlModifier);
        
        // 选择模式点击（移除频繁的日志）
        
        if (pickResult.hasResult && pickResult.geometry)
        {
            Geo3D* pickedGeo = pickResult.geometry;
            
            if (isCtrlPressed)
            {
                // Ctrl+点击：多选模式
                if (isSelected(pickedGeo))
                {
                    // 如果已经选中，则从选中列表中移除
                    removeFromSelection(pickedGeo);
                }
                else
                {
                    // 如果未选中，则添加到选中列表
                    addToSelection(pickedGeo);
                }
            }
            else
            {
                // 普通点击：单选模式
                clearSelection();
                addToSelection(pickedGeo);
            }
        }
        else
        {
            // 没有点击到对象，清除选择
            if (!isCtrlPressed)
            {
                clearSelection();
            }
        }
    }
    else
    {
        // 绘制模式：使用拾取系统获取世界坐标
        OSGIndexPickResult pickResult = OSGIndexPickingSystemManager::getInstance().pick(event->x(), event->y());
        glm::vec3 worldPos;
        
        if (pickResult.hasResult)
        {
            // 使用拾取结果的世界坐标覆盖普通坐标
            worldPos = pickResult.worldPosition;
            m_lastMouseWorldPos = pickResult.worldPosition;
            // 在绘制模式下也更新状态栏坐标
            emit mousePositionChanged(pickResult.worldPosition);
        }
        else
        {
            // 如果没有拾取到对象，使用屏幕坐标转换
            worldPos = screenToWorld(event->x(), event->y(), 0.5f);
            // 应用坐标范围限制
            CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
            glm::vec3 clampedPos = coordSystem->clampPointToSkybox(worldPos);
            worldPos = clampedPos;
            m_lastMouseWorldPos = clampedPos;
            // 更新状态栏坐标
            emit mousePositionChanged(clampedPos);
        }
        
        if (!m_isDrawing)
        {
            // 开始新的绘制
            Geo3D* newGeo = createGeo3D(GlobalDrawMode3D);
            if (newGeo)
            {
                m_currentDrawingGeo = newGeo;
                m_isDrawing = true;
                addGeo(newGeo);
                LOG_INFO("开始绘制...", "绘制");
            }
        }
        
        if (m_currentDrawingGeo)
        {
            // 应用天空盒范围限制，确保不会超出天空盒
            CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
            glm::vec3 clampedPos = coordSystem->clampPointToSkybox(worldPos);
            
            m_currentDrawingGeo->mousePressEvent(event, clampedPos);
            
            // 检查是否完成绘制
            if (m_currentDrawingGeo->mm_state()->isStateComplete())
            {
                completeCurrentDrawing();
            }
        }
    }
}

void OSGWidget::updateCurrentDrawing(const glm::vec3& worldPos)
{
    if (m_currentDrawingGeo)
    {
        // 应用天空盒范围限制，确保不会超出天空盒
        CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
        glm::vec3 clampedPos = coordSystem->clampPointToSkybox(worldPos);
        
        QMouseEvent moveEvent(QEvent::MouseMove, QPoint(0, 0), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        m_currentDrawingGeo->mouseMoveEvent(&moveEvent, clampedPos);
    }
}

void OSGWidget::completeCurrentDrawing()
{
    if (m_currentDrawingGeo)
    {
        // 完成绘制 - 这会触发drawingCompleted信号
        m_currentDrawingGeo->mm_state()->setStateComplete();
        // 绘制完成后，对象保留在场景中，不需要删除
        m_currentDrawingGeo = nullptr;
        m_isDrawing = false;
        
        LOG_SUCCESS("绘制完成", "绘制");
        
        // 重新启用OSG窗口的事件处理
        setFocus();
    }
}

void OSGWidget::cancelCurrentDrawing()
{
    if (m_currentDrawingGeo)
    {
        // 从场景中移除（这会减少引用计数）
        removeGeo(m_currentDrawingGeo.get());
        
        // 重置绘制状态
        m_currentDrawingGeo = nullptr;
        m_isDrawing = false;
        
        LOG_WARNING("取消绘制", "绘制");
        
        // 重新启用OSG窗口的事件处理
        setFocus();
    }
}

void OSGWidget::setDrawMode(DrawMode3D mode)
{
    // 如果当前正在绘制，取消绘制
    if (m_isDrawing)
    {
        cancelCurrentDrawing();
    }
    
    // 如果切换到选择模式，取消选择
    if (mode == DrawSelect3D)
    {
        deselectAll();
    }
    
    // 更新全局绘制模式
    GlobalDrawMode3D = mode;
    
    // 发送状态更新信号
    if (mode == DrawSelect3D)
    {
        LOG_INFO("切换到选择模式", "模式");
    }
    else
    {
        LOG_INFO(tr("切换到绘制模式: %1").arg(drawMode3DToString(mode)), "模式");
    }
}

// ========================================= 天空盒相关方法 =========================================

void OSGWidget::setupSkybox()
{
    if (m_skybox && m_skyboxEnabled)
    {
        // 获取坐标系统范围并设置天空盒大小
        CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
        const CoordinateSystem3D::CoordinateRange& range = coordSystem->getSkyboxRange();
        
        // 根据坐标范围设置天空盒大小
        m_skybox->setSizeFromRange(range.minX, range.maxX, range.minY, range.maxY, range.minZ, range.maxZ);
        
        // 设置天空盒中心为坐标原点
        m_skybox->setCenter(osg::Vec3(0.0f, 0.0f, 0.0f));
        
        // 清除现有的天空盒
        m_skyboxNode->removeChildren(0, m_skyboxNode->getNumChildren());
        
        // 添加新的天空盒
        osg::ref_ptr<osg::Node> skyboxNode = m_skybox->getSkyboxNode();
        if (skyboxNode)
        {
            m_skyboxNode->addChild(skyboxNode);
        }
    }
}

void OSGWidget::enableSkybox(bool enabled)
{
    m_skyboxEnabled = enabled;
    
    if (enabled)
    {
        setupSkybox();
    }
    else
    {
        // 移除天空盒
        m_skyboxNode->removeChildren(0, m_skyboxNode->getNumChildren());
    }
}

bool OSGWidget::isSkyboxEnabled() const
{
    return m_skyboxEnabled;
}

void OSGWidget::setSkyboxGradient(const osg::Vec4& topColor, const osg::Vec4& bottomColor)
{
    if (m_skybox)
    {
        m_skybox->setGradientSkybox(topColor, bottomColor);
        if (m_skyboxEnabled)
        {
            setupSkybox();
        }
    }
}

void OSGWidget::setSkyboxSolidColor(const osg::Vec4& color)
{
    if (m_skybox)
    {
        m_skybox->setSolidColorSkybox(color);
        if (m_skyboxEnabled)
        {
            setupSkybox();
        }
    }
}

void OSGWidget::setSkyboxCubeMap(const std::string& positiveX, const std::string& negativeX,
                                const std::string& positiveY, const std::string& negativeY,
                                const std::string& positiveZ, const std::string& negativeZ)
{
    if (m_skybox)
    {
        m_skybox->setCubeMapTexture(positiveX, negativeX, positiveY, negativeY, positiveZ, negativeZ);
        if (m_skyboxEnabled)
        {
            setupSkybox();
        }
    }
}

void OSGWidget::refreshSkybox()
{
    if (m_skyboxEnabled)
    {
        setupSkybox();
    }
}

// ========================================= 坐标系相关方法 =========================================

void OSGWidget::setupCoordinateSystem()
{
    if (!m_coordinateSystemRenderer)
    {
        // 创建坐标系渲染器
        m_coordinateSystemRenderer = std::make_unique<CoordinateSystemRenderer>();
    }
    
    if (m_coordinateSystemEnabled)
    {
        // 清除现有的坐标系
        m_sceneNode->removeChild(m_coordinateSystemRenderer->getCoordinateSystemNode());
        
        // 添加坐标系到场景
        osg::ref_ptr<osg::Node> coordSystemNode = m_coordinateSystemRenderer->getCoordinateSystemNode();
        if (coordSystemNode)
        {
            m_sceneNode->addChild(coordSystemNode);
        }
    }
}

void OSGWidget::enableCoordinateSystem(bool enabled)
{
    m_coordinateSystemEnabled = enabled;
    
    if (enabled)
    {
        setupCoordinateSystem();
    }
    else
    {
        // 移除坐标系
        if (m_coordinateSystemRenderer)
        {
            m_sceneNode->removeChild(m_coordinateSystemRenderer->getCoordinateSystemNode());
        }
    }
}

bool OSGWidget::isCoordinateSystemEnabled() const
{
    return m_coordinateSystemEnabled;
}

void OSGWidget::refreshCoordinateSystem()
{
    if (m_coordinateSystemEnabled && m_coordinateSystemRenderer)
    {
        m_coordinateSystemRenderer->updateCoordinateSystem();
    }
}



// ========================================= 摄像机控制器委托方法 =========================================

void OSGWidget::setManipulatorType(ManipulatorType type)
{
    if (m_cameraController)
    {
        m_cameraController->setManipulatorType(type);
        
        // 清除比例尺缓存，因为相机操控器改变了
        m_lastScaleCalculation = QDateTime();
        
        // 清除鼠标位置缓存，因为相机改变了
        m_mousePosCacheValid = false;
        
        LOG_INFO(QString("切换相机操控器: %1").arg(static_cast<int>(type)), "相机");
    }
}

ManipulatorType OSGWidget::getManipulatorType() const
{
    if (m_cameraController)
    {
        return m_cameraController->getManipulatorType();
    }
    return ManipulatorType::Trackball;
}

void OSGWidget::switchToNextManipulator()
{
    m_cameraController->switchToNextManipulator();
}

void OSGWidget::switchToPreviousManipulator()
{
    m_cameraController->switchToPreviousManipulator();
}

void OSGWidget::setCameraMoveSpeed(double speed)
{
    m_cameraController->setCameraMoveSpeed(speed);
}

double OSGWidget::getCameraMoveSpeed() const
{
    if (m_cameraController)
    {
        return m_cameraController->getCameraMoveSpeed();
    }
    return 1.0;
}

void OSGWidget::setWheelMoveSensitivity(double sensitivity)
{
    m_cameraController->setWheelMoveSensitivity(sensitivity);
}

double OSGWidget::getWheelMoveSensitivity() const
{
    if (m_cameraController)
    {
        return m_cameraController->getWheelMoveSensitivity();
    }
    return 1.0;
}

void OSGWidget::setAccelerationRate(double rate)
{
    m_cameraController->setAccelerationRate(rate);
}

double OSGWidget::getAccelerationRate() const
{
    if (m_cameraController)
    {
        return m_cameraController->getAccelerationRate();
    }
    return 1.5;
}

void OSGWidget::setMaxAccelerationSpeed(double speed)
{
    m_cameraController->setMaxAccelerationSpeed(speed);
}

double OSGWidget::getMaxAccelerationSpeed() const
{
    if (m_cameraController)
    {
        return m_cameraController->getMaxAccelerationSpeed();
    }
    return 10.0;
}

void OSGWidget::resetAllAcceleration()
{
    m_cameraController->resetAllAcceleration();
}



void OSGWidget::keyPressEvent(QKeyEvent* event)
{
    // 处理摄像机移动键
    switch (event->key())
    {
        case Qt::Key_W:
        case Qt::Key_S:
        case Qt::Key_A:
        case Qt::Key_D:
        case Qt::Key_Q:
        case Qt::Key_E:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            // 委托给CameraController处理
            m_cameraController->setKeyPressed(event->key(), true);
            break;
        default:
            // 其他按键传递给基类
            osgQOpenGLWidget::keyPressEvent(event);
            
            // 处理绘制相关按键
            if (m_isDrawing && m_currentDrawingGeo)
            {
                m_currentDrawingGeo->keyPressEvent(event);
                
                if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
                {
                    completeCurrentDrawing();
                }
                else if (event->key() == Qt::Key_Escape)
                {
                    cancelCurrentDrawing();
                }
            }
            
            // 处理选择模式
            if (GlobalDrawMode3D == DrawSelect3D && event->key() == Qt::Key_Delete)
            {
                if (m_selectedGeo)
                {
                    // 从场景中移除（这会减少引用计数）
                    removeGeo(m_selectedGeo.get());
                    
                    // 重置选择状态
                    m_selectedGeo = nullptr;
                    
                    emit geoSelected(nullptr);
                }
            }
            break;
    }
    
    event->accept();
}

void OSGWidget::keyReleaseEvent(QKeyEvent* event)
{
    // 处理摄像机移动键释放
    switch (event->key())
    {
        case Qt::Key_W:
        case Qt::Key_S:
        case Qt::Key_A:
        case Qt::Key_D:
        case Qt::Key_Q:
        case Qt::Key_E:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            // 委托给CameraController处理
            m_cameraController->setKeyPressed(event->key(), false);
            break;
        default:
            // 其他按键传递给基类
            osgQOpenGLWidget::keyReleaseEvent(event);
            
            if (m_isDrawing && m_currentDrawingGeo)
            {
                m_currentDrawingGeo->keyReleaseEvent(event);
            }
            break;
    }
    
    event->accept();
}

// ========================================= 比例尺相关方法 =========================================

void OSGWidget::drawScaleBar()
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 获取当前相机信息
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    osg::Camera* camera = viewer->getCamera();
    if (!camera) return;
    
    // 计算比例尺
    double scaleValue = calculateScaleValue();
    QString scaleText = formatScaleText(scaleValue);
    
    // 设置绘制区域
    QRect scaleRect(m_scaleBarPosition.x(), m_scaleBarPosition.y(), 
                   m_scaleBarSize.width(), m_scaleBarSize.height());
    
    // 绘制背景
    painter.fillRect(scaleRect, QColor(0, 0, 0, 100));
    painter.setPen(QPen(QColor(255, 255, 255), 1));
    painter.drawRect(scaleRect);
    
    // 绘制比例尺线条
    int barWidth = m_scaleBarSize.width() - 20;
    int barHeight = 4;
    int barY = scaleRect.center().y() - barHeight / 2;
    
    // 绘制主线条
    painter.setPen(QPen(QColor(255, 255, 255), 2));
    painter.drawLine(scaleRect.left() + 10, barY, scaleRect.left() + 10 + barWidth, barY);
    
    // 绘制刻度线
    painter.setPen(QPen(QColor(255, 255, 255), 1));
    for (int i = 0; i <= 10; ++i)
    {
        int x = scaleRect.left() + 10 + (barWidth * i) / 10;
        int tickHeight = (i % 5 == 0) ? 8 : 4;
        painter.drawLine(x, barY - tickHeight, x, barY + tickHeight);
    }
    
    // 绘制文本
    painter.setPen(QColor(255, 255, 255));
    painter.setFont(QFont("Arial", 8));
    
    // 绘制比例尺值
    QRect textRect = scaleRect.adjusted(5, barY + 10, -5, -5);
    painter.drawText(textRect, Qt::AlignCenter, scaleText);
}

double OSGWidget::calculateScaleValue()
{
    if (!m_cameraController) return 1.0;
    
    // 检查缓存是否有效
    QDateTime currentTime = QDateTime::currentDateTime();
    if (m_lastScaleCalculation.isValid() && 
        m_lastScaleCalculation.msecsTo(currentTime) < SCALE_CACHE_DURATION)
    {
        return m_cachedScaleValue;
    }
    
    ProjectionMode mode = m_cameraController->getProjectionMode();
    
    if (mode == ProjectionMode::Orthographic)
    {
        // 正交投影模式：直接使用正交投影的大小计算比例尺
        double orthoWidth = m_cameraController->getRight() - m_cameraController->getLeft();
        double scaleBarPixels = m_scaleBarSize.width() - 20; // 减去边距
        double scaleBarWorldUnits = (orthoWidth * scaleBarPixels) / width();
        
        m_cachedScaleValue = scaleBarWorldUnits;
        m_lastScaleCalculation = currentTime;
        return scaleBarWorldUnits;
    }
    else
    {
        // 透视投影模式：使用原来的计算方法
        // 获取相机位置
        osg::Vec3d eye = m_cameraController->getEyePosition();
        osg::Vec3d center = m_cameraController->getCenterPosition();
        
        // 计算相机到中心的距离
        double distance = (eye - center).length();
        
        // 获取视口信息
        osgViewer::Viewer* viewer = getOsgViewer();
        if (!viewer || !viewer->getCamera()) return 1.0;
        
        osg::Viewport* viewport = viewer->getCamera()->getViewport();
        if (!viewport) return 1.0;
        
        // 计算屏幕像素对应的世界单位
        double screenHeight = viewport->height();
        double fov = m_cameraController->getFOV(); // 使用当前设置的FOV
        double worldHeight = 2.0 * distance * tan(osg::DegreesToRadians(fov / 2.0));
        double pixelsPerUnit = screenHeight / worldHeight;
        
        // 计算比例尺对应的世界单位
        double scaleBarPixels = m_scaleBarSize.width() - 20; // 减去边距
        double scaleBarWorldUnits = scaleBarPixels / pixelsPerUnit;
        
        m_cachedScaleValue = scaleBarWorldUnits;
        m_lastScaleCalculation = currentTime;
        return scaleBarWorldUnits;
    }
}

QString OSGWidget::formatScaleText(double worldUnits)
{
    QString unit = "m";
    double value = worldUnits;
    
    // 根据数值大小选择合适的单位
    if (value >= 1000.0)
    {
        value /= 1000.0;
        unit = "km";
    }
    else if (value < 1.0 && value >= 0.01)
    {
        value *= 100.0;
        unit = "cm";
    }
    else if (value < 0.01)
    {
        value *= 1000.0;
        unit = "mm";
    }
    
    // 格式化数值
    if (value >= 100.0)
    {
        return QString("%1 %2").arg(static_cast<int>(value)).arg(unit);
    }
    else if (value >= 10.0)
    {
        return QString("%1 %2").arg(value, 0, 'f', 1).arg(unit);
    }
    else
    {
        return QString("%1 %2").arg(value, 0, 'f', 2).arg(unit);
    }
}

void OSGWidget::enableScaleBar(bool enabled)
{
    m_scaleBarEnabled = enabled;
    update(); // 触发重绘
}

void OSGWidget::setScaleBarPosition(const QPoint& position)
{
    m_scaleBarPosition = position;
    update(); // 触发重绘
}

void OSGWidget::setScaleBarSize(int width, int height)
{
    m_scaleBarSize = QSize(width, height);
    update(); // 触发重绘
}

// ========================================= 投影模式相关方法 =========================================

void OSGWidget::setProjectionMode(ProjectionMode mode)
{
    if (m_cameraController)
    {
        m_cameraController->setProjectionMode(mode);
        
        // 如果切换到正交模式，自动调整正交投影的大小
        if (mode == ProjectionMode::Orthographic)
        {
            // 获取当前场景的包围盒来设置正交投影的大小
            CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
            const CoordinateSystem3D::CoordinateRange& range = coordSystem->getCoordinateRange();
            
            double maxRange = range.maxRange();
            double orthoSize = maxRange * 0.6; // 使用坐标范围的60%作为正交投影大小
            
            m_cameraController->setViewSize(-orthoSize, orthoSize, -orthoSize, orthoSize);
            m_cameraController->setNearFar(-maxRange, maxRange);
        }
        
        update(); // 触发重绘
    }
}

ProjectionMode OSGWidget::getProjectionMode() const
{
    if (m_cameraController)
    {
        return m_cameraController->getProjectionMode();
    }
    return ProjectionMode::Perspective;
}

void OSGWidget::setFOV(double fov)
{
    if (m_cameraController)
    {
        m_cameraController->setFOV(fov);
    }
}

void OSGWidget::setNearFar(double near_, double far_)
{
    if (m_cameraController)
    {
        m_cameraController->setNearFar(near_, far_);
    }
}

void OSGWidget::setViewSize(double left, double right, double bottom, double top)
{
    if (m_cameraController)
    {
        m_cameraController->setViewSize(left, right, bottom, top);
    }
}

// ========================================= 几何对象查询方法 =========================================

Geo3D* OSGWidget::getSelectedGeo() const
{
    return m_selectedGeo.get();
}

const std::vector<osg::ref_ptr<Geo3D>>& OSGWidget::getAllGeos() const
{
    return m_geoList;
}

// 几何对象信号响应槽函数
void OSGWidget::onGeoDrawingCompleted(Geo3D* geo)
{
    if (!geo || !m_advancedPickingEnabled)
        return;
    
            // 几何体绘制完成（移除调试日志）
    
    // 几何对象完成绘制后，更新OSG索引拾取系统
    // 注意：如果几何对象已经在拾取系统中，updateGeometry会更新它
    // 如果不在拾取系统中，addGeometry会添加它
    OSGIndexPickingSystemManager::getInstance().updateGeometry(geo);
    
            // 更新完成的几何体（移除频繁的日志）
}

void OSGWidget::onGeoGeometryUpdated(Geo3D* geo)
{
    if (!geo || !m_advancedPickingEnabled)
        return;
    
    // 几何对象更新时，更新OSG索引拾取系统中的Feature
    OSGIndexPickingSystemManager::getInstance().updateGeometry(geo);
    
            // 更新几何体（移除调试日志）
}

void OSGWidget::onGeoParametersChanged(Geo3D* geo)
{
    if (!geo || !m_advancedPickingEnabled)
        return;
    
    // 参数变化时，更新OSG索引拾取系统中的Feature
    OSGIndexPickingSystemManager::getInstance().updateGeometry(geo);
    
            // 更新几何体参数（移除调试日志）
}
