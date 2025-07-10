#include "OSGWidget.h"
#include "../core/Common3D.h"
#include "../core/GeometryBase.h"
#include "../core/picking/PickingSystem.h"
#include "../core/picking/PickingIntegration.h"
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
    , m_scaleBarPosition(20, 20)
    , m_scaleBarSize(150, 30)
    , m_updateTimer(new QTimer(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setFocus(); // 确保获得焦点
    
    // 连接信号槽
    connect(m_updateTimer, &QTimer::timeout, this, [this]() {
        update();
    });
    
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
    LOG_DEBUG("渲染循环已启动，帧率: 60fps", "系统");
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
    
    // 初始化OSG插件系统
    LOG_INFO("正在初始化OSG插件系统...", "系统");
    
    // 设置OSG插件路径
#ifdef OSG_PLUGIN_PATH
    QString pluginPath = QString::fromLocal8Bit(OSG_PLUGIN_PATH);
    LOG_INFO(QString("设置OSG插件路径: %1").arg(pluginPath), "系统");
    
    // 设置环境变量
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("OSG_PLUGIN_PATH", pluginPath);
    QProcessEnvironment::setSystemEnvironment(env);
    
    // 设置OSG插件路径
    osgDB::Registry* registry = osgDB::Registry::instance();
    if (registry) {
        registry->setLibraryFilePathList(pluginPath.toStdString());
        LOG_INFO("OSG插件路径已设置", "系统");
    }
#endif
    
    osgDB::Registry* registry = osgDB::Registry::instance();
    if (registry) {
        // 获取可用的插件信息
        LOG_INFO(QString("OSG插件系统初始化完成"), "系统");
        
        // 检查是否有osgb插件
        osgDB::ReaderWriter* osgbReader = registry->getReaderWriterForExtension("osgb");
        if (osgbReader) {
            LOG_INFO("找到OSGB文件读取插件", "系统");
        } else {
            LOG_WARNING("未找到OSGB文件读取插件", "系统");
        }
        
        // 检查其他常用格式
        QStringList commonFormats = {"osg", "osgb", "osgt", "ive", "obj", "3ds", "dae"};
        QStringList supportedFormats;
        for (const QString& format : commonFormats) {
            if (registry->getReaderWriterForExtension(format.toStdString())) {
                supportedFormats << format;
            }
        }
        LOG_INFO(QString("支持的常用格式: %1").arg(supportedFormats.join(", ")), "系统");
    } else {
        LOG_ERROR("OSG插件系统初始化失败", "系统");
    }
    
    // 设置场景图
    m_rootNode->addChild(m_sceneNode);
    m_rootNode->addChild(m_lightNode);
    m_rootNode->addChild(m_pickingIndicatorNode);
    m_rootNode->addChild(m_skyboxNode);
    m_sceneNode->addChild(m_geoNode);
    
    viewer->setSceneData(m_rootNode);
    
    // 设置线程模型
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    
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
    
    // 设置渲染状态
    osg::StateSet* stateSet = camera->getOrCreateStateSet();
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    
    // 启用反走样
    stateSet->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    stateSet->setMode(GL_POINT_SMOOTH, osg::StateAttribute::ON);
    
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
    
    // 初始化拾取系统
    if (!PickingSystemIntegration::initializePickingSystem(width(), height()))
    {
        LOG_ERROR("Failed to initialize picking system", "拾取");
        return;
    }
    
    // 设置主相机
    PickingSystemIntegration::setMainCamera(viewer->getCamera());
    
    // 创建指示器管理器
    m_pickingIndicatorManager = PickingSystemIntegration::getIndicatorManager();
    
    if (m_pickingIndicatorManager)
    {
        // 添加指示器和高亮节点到场景图
        m_pickingIndicatorNode->addChild(m_pickingIndicatorManager->getIndicatorRoot());
        m_pickingIndicatorNode->addChild(m_pickingIndicatorManager->getHighlightRoot());
    }
    
    // 添加拾取事件处理器
    PickingSystemIntegration::addPickingEventHandler(viewer, 
        [this](const PickingResult& result) {
            onPickingResult(result);
        });
    
    m_advancedPickingEnabled = true;
    
    LOG_SUCCESS("Picking system initialized successfully", "拾取");
}

void OSGWidget::resetCamera()
{
    if (!m_cameraController) return;
    
    // 获取坐标系统范围
    CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
    const CoordinateSystem3D::CoordinateRange& range = coordSystem->getCoordinateRange();
    
    // 修改：将摄像机位置设置为(1,0,0)附近
    glm::vec3 center = range.center();
    double maxRange = range.maxRange();
    
    // 计算摄像机位置：在(1,0,0)方向，距离为范围的较小比例
    double distance = maxRange * 0.3; // 减小距离，使缩放更小
    
    // 设置摄像机位置在(1,0,0)附近，稍微偏移以避免完全在轴上
    osg::Vec3d eye(center.x + distance, center.y + distance * 0.1, center.z + distance * 0.1);
    osg::Vec3d centerOsg(center.x, center.y, center.z);
    
    m_cameraController->setPosition(eye, centerOsg, osg::Vec3d(0, 0, 1));
}

void OSGWidget::fitAll()
{
    if (!m_cameraController || !m_geoNode.valid()) return;
    
    // 获取坐标系统范围
    CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
    const CoordinateSystem3D::CoordinateRange& range = coordSystem->getCoordinateRange();
    
    // 计算场景包围盒
    osg::BoundingSphere bs = m_geoNode->getBound();
    if (bs.valid())
    {
        // 结合坐标系统范围和场景包围盒
        glm::vec3 coordCenter = range.center();
        double coordRadius = range.maxRange() * 0.5;
        
        // 如果场景有对象，使用场景和坐标系统的并集
        if (bs.radius() > 0)
        {
            osg::Vec3d sceneCenter = bs.center();
            double sceneRadius = bs.radius();
            
            // 计算并集中心
            osg::Vec3d combinedCenter(
                (sceneCenter.x() + coordCenter.x) * 0.5,
                (sceneCenter.y() + coordCenter.y) * 0.5,
                (sceneCenter.z() + coordCenter.z) * 0.5
            );
            
            // 计算并集半径
            double combinedRadius = std::max(sceneRadius, coordRadius) * 1.2; // 20%边距
            
            osg::Vec3d eye = combinedCenter + osg::Vec3d(combinedRadius * 2, combinedRadius * 2, combinedRadius * 2);
            m_cameraController->setPosition(eye, combinedCenter, osg::Vec3d(0, 0, 1));
        }
        else
        {
            // 如果场景为空，使用坐标系统范围
            osg::Vec3d center(coordCenter.x, coordCenter.y, coordCenter.z);
            double distance = coordRadius * 2.0;
            
            osg::Vec3d eye = center + osg::Vec3d(distance, distance, distance);
            m_cameraController->setPosition(eye, center, osg::Vec3d(0, 0, 1));
        }
    }
}

void OSGWidget::setViewDirection(const glm::vec3& direction, const glm::vec3& up)
{
    if (!m_cameraController) return;
    
    osg::BoundingSphere bs = m_geoNode->getBound();
    osg::Vec3d center = bs.valid() ? osg::Vec3d(bs.center()) : osg::Vec3d(0, 0, 0);
    float distance = bs.valid() ? bs.radius() * 3.0f : 10.0f;
    
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
        m_geoNode->addChild(geo->getOSGNode().get());
        
        // 添加到拾取系统
        if (m_advancedPickingEnabled)
        {
            PickingSystemIntegration::addGeometry(geo);
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
            m_geoNode->removeChild(geo->getOSGNode().get());
            m_geoList.erase(it);
            
            // 从拾取系统移除
            if (m_advancedPickingEnabled)
            {
                PickingSystemIntegration::removeGeometry(geo);
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
    }
}

void OSGWidget::selectGeo(Geo3D* geo)
{
    if (m_selectedGeo)
    {
        m_selectedGeo->clearStateSelected();
    }
    
    m_selectedGeo = geo;
    if (m_selectedGeo)
    {
        m_selectedGeo->setStateSelected();
    }
    
    emit geoSelected(geo);
}

void OSGWidget::deselectAll()
{
    selectGeo(nullptr);
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
    if (m_pickingEventHandler)
    {
        m_pickingEventHandler->setPickingRadius(radius);
    }
}

void OSGWidget::setPickingFrequency(double frequency)
{
    if (m_pickingEventHandler)
    {
        m_pickingEventHandler->setPickingFrequency(frequency);
    }
}

void OSGWidget::onPickingResult(const PickingResult& result)
{
    if (m_pickingIndicatorManager)
    {
        m_pickingIndicatorManager->onPickingResult(result);
    }
    
    // 发射高级拾取结果信号
    emit advancedPickingResult(result);
}

PickResult3D OSGWidget::pick(int x, int y)
{
    PickResult3D result;
    
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return result;
    
    osg::Camera* camera = viewer->getCamera();
    
    // 创建射线
    osg::Vec3f nearPoint, farPoint;
    if (camera->getViewport())
    {
        osg::Matrix VPW = camera->getViewMatrix() * 
                         camera->getProjectionMatrix() * 
                         camera->getViewport()->computeWindowMatrix();
        osg::Matrix invVPW;
        invVPW.invert(VPW);
        
        nearPoint = osg::Vec3f(x, height() - y, 0.0f) * invVPW;
        farPoint = osg::Vec3f(x, height() - y, 1.0f) * invVPW;
    }
    
    Ray3D ray(glm::vec3(nearPoint.x(), nearPoint.y(), nearPoint.z()),
              glm::vec3(farPoint.x() - nearPoint.x(), farPoint.y() - nearPoint.y(), farPoint.z() - nearPoint.z()));
    
    // 测试所有几何对象
    float minDistance = FLT_MAX;
    for (const osg::ref_ptr<Geo3D>& geo : m_geoList)
    {
        PickResult3D geoResult;
        if (geo->hitTest(ray, geoResult))
        {
            if (geoResult.distance < minDistance)
            {
                minDistance = geoResult.distance;
                result = geoResult;
            }
        }
    }
    
    return result;
}

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
    // 获取当前键盘状态
    Qt::KeyboardModifiers currentModifiers = QApplication::keyboardModifiers();
    
    // 非绘制状态：根据模式决定处理方式
    if (GlobalDrawMode3D == DrawSelect3D || (currentModifiers & Qt::ControlModifier))
    {
        // 选择模式：正常传递给OSG窗口
        QString buttonName;
        switch (event->button()) {
            case Qt::LeftButton: buttonName = "左键"; break;
            case Qt::RightButton: buttonName = "右键"; break;
            case Qt::MiddleButton: buttonName = "中键"; break;
            default: buttonName = "未知"; break;
        }
        
        LOG_DEBUG(QString("鼠标按下: %1 位置(%2,%3)").arg(buttonName).arg(event->x()).arg(event->y()), "相机");
        
        osgQOpenGLWidget::mousePressEvent(event);
    }
    else
    {
        // 绘制模式：处理绘制输入
        handleDrawingInput(event);
        event->accept();
    }
}

void OSGWidget::mouseMoveEvent(QMouseEvent* event)
{
    // 如果是相机控制模式，添加日志
    if (GlobalDrawMode3D == DrawSelect3D || (QApplication::keyboardModifiers() & Qt::ControlModifier))
    {
        // 只在鼠标按下时输出移动日志，避免日志过多
        if (event->buttons() != Qt::NoButton)
        {
            static int logCounter = 0;
            if (++logCounter % 10 == 0) // 每10次移动输出一次日志
            {
                QString buttonName;
                if (event->buttons() & Qt::LeftButton) buttonName = "左键拖拽";
                else if (event->buttons() & Qt::RightButton) buttonName = "右键拖拽";
                else if (event->buttons() & Qt::MiddleButton) buttonName = "中键拖拽";
                else buttonName = "拖拽";
                
                LOG_DEBUG(QString("鼠标移动: %1 位置(%2,%3)").arg(buttonName).arg(event->x()).arg(event->y()), "相机");
            }
        }
    }
    
    osgQOpenGLWidget::mouseMoveEvent(event);
    
    // 节流机制：每16ms（约60FPS）更新一次鼠标世界坐标
    static QDateTime lastMouseUpdate;
    QDateTime currentTime = QDateTime::currentDateTime();
    if (!lastMouseUpdate.isValid() || lastMouseUpdate.msecsTo(currentTime) >= MOUSE_CACHE_DURATION)
    {
        // 更新鼠标世界坐标
        glm::vec3 worldPos = screenToWorld(event->x(), event->y(), 0.5f);
        
        // 应用坐标范围限制
        CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
        glm::vec3 clampedPos = coordSystem->clampPointToSkybox(worldPos);
        
        m_lastMouseWorldPos = clampedPos;
        emit mousePositionChanged(clampedPos);
        
        lastMouseUpdate = currentTime;
    }
    
    // 处理绘制预览 - 使用缓存的位置
    if (m_isDrawing && m_currentDrawingGeo)
    {
        updateCurrentDrawing(m_lastMouseWorldPos);
    }
}

void OSGWidget::mouseReleaseEvent(QMouseEvent* event)
{
    // 如果是相机控制模式，添加日志
    if (GlobalDrawMode3D == DrawSelect3D || (QApplication::keyboardModifiers() & Qt::ControlModifier))
    {
        QString buttonName;
        switch (event->button()) {
            case Qt::LeftButton: buttonName = "左键"; break;
            case Qt::RightButton: buttonName = "右键"; break;
            case Qt::MiddleButton: buttonName = "中键"; break;
            default: buttonName = "未知"; break;
        }
        
        LOG_DEBUG(QString("鼠标释放: %1 位置(%2,%3)").arg(buttonName).arg(event->x()).arg(event->y()), "相机");
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
        
        LOG_DEBUG(QString("Ctrl+滚轮缩放: delta=%1 位置(%2,%3)").arg(delta).arg(event->x()).arg(event->y()), "相机");
        
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
        LOG_DEBUG(QString("滚轮缩放: delta=%1 位置(%2,%3)").arg(delta).arg(event->x()).arg(event->y()), "相机");
        
        osgQOpenGLWidget::wheelEvent(event);
    }
}

void OSGWidget::handleDrawingInput(QMouseEvent* event)
{
    glm::vec3 worldPos = screenToWorld(event->x(), event->y(), 0.5f);
    
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
        // 选择模式：拾取对象
        PickResult3D pickResult = pick(event->x(), event->y());
        if (pickResult.hit)
        {
            selectGeo(static_cast<Geo3D*>(pickResult.userData));
        }
        else
        {
            deselectAll();
        }
    }
    else
    {
        // 绘制模式
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
            if (m_currentDrawingGeo->isStateComplete())
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
        // 完成绘制
        m_currentDrawingGeo->completeDrawing();
        
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
    // 添加测试日志，确认事件到达
    LOG_DEBUG(QString("OSGWidget keyPressEvent: key=%1, text='%2'").arg(event->key()).arg(event->text()), "相机");
    
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
            LOG_DEBUG(QString("OSGWidget收到键盘按下事件: key=%1").arg(event->key()), "相机");
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
            LOG_DEBUG(QString("OSGWidget收到键盘释放事件: key=%1").arg(event->key()), "相机");
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



