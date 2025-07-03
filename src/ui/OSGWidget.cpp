#include "OSGWidget.h"
#include "../core/Common3D.h"
#include "../core/GeometryBase.h"
#include "../core/picking/PickingSystem.h"
#include "../core/picking/PickingIntegration.h"
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
#include <osgQOpenGL/osgQOpenGLWidget>
#include <QTimer>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QWheelEvent>
#include <algorithm>

// ========================================= OSGWidget 实现 =========================================
OSGWidget::OSGWidget(QWidget* parent)
    : osgQOpenGLWidget(parent)
    , m_rootNode(new osg::Group)
    , m_sceneNode(new osg::Group)
    , m_geoNode(new osg::Group)
    , m_lightNode(new osg::Group)
    , m_pickingIndicatorNode(new osg::Group)
    , m_trackballManipulator(new osgGA::TrackballManipulator)
    , m_currentDrawingGeo(nullptr)
    , m_selectedGeo(nullptr)
    , m_isDrawing(false)
    , m_lastMouseWorldPos(0.0f)
    , m_advancedPickingEnabled(false)
    , m_updateTimer(new QTimer(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    // 连接信号槽
    connect(m_updateTimer, &QTimer::timeout, this, [this]() {
        update();
    });
    
    // 连接初始化完成信号
    connect(this, &osgQOpenGLWidget::initialized, this, &OSGWidget::initializeScene);
    
    // 设置渲染循环
    m_updateTimer->start(16); // 约60FPS
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
    
    // 设置场景图
    m_rootNode->addChild(m_sceneNode);
    m_rootNode->addChild(m_lightNode);
    m_rootNode->addChild(m_pickingIndicatorNode);
    m_sceneNode->addChild(m_geoNode);
    
    viewer->setSceneData(m_rootNode);
    
    // 设置线程模型
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded); //调试
    //viewer->setThreadingModel(osgViewer::Viewer::AutomaticSelection);
    
    // 设置相机操控器
    viewer->setCameraManipulator(m_trackballManipulator);
    
    setupCamera();
    setupLighting();
    setupEventHandlers();
    setupPickingSystem();
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
        Log3D << "Failed to initialize picking system";
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
    
    Log3D << "Picking system initialized successfully";
}

void OSGWidget::resetCamera()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    if (viewer->getCameraManipulator())
    {
        viewer->getCameraManipulator()->setHomePosition(
            osg::Vec3d(10, 10, 10),  // eye
            osg::Vec3d(0, 0, 0),     // center
            osg::Vec3d(0, 0, 1)      // up
        );
        viewer->getCameraManipulator()->home(0.0);
    }
}

void OSGWidget::fitAll()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer || !m_geoNode.valid()) return;
    
    if (viewer->getCameraManipulator())
    {
        osg::BoundingSphere bs = m_geoNode->getBound();
        if (bs.valid())
        {
            viewer->getCameraManipulator()->setHomePosition(
                bs.center() + osg::Vec3d(bs.radius() * 2, bs.radius() * 2, bs.radius() * 2),
                bs.center(),
                osg::Vec3d(0, 0, 1)
            );
            viewer->getCameraManipulator()->home(0.0);
        }
    }
}

void OSGWidget::setViewDirection(const glm::vec3& direction, const glm::vec3& up)
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    if (viewer->getCameraManipulator())
    {
        osg::BoundingSphere bs = m_geoNode->getBound();
        osg::Vec3d center = bs.valid() ? osg::Vec3d(bs.center()) : osg::Vec3d(0, 0, 0);
        float distance = bs.valid() ? bs.radius() * 3.0f : 10.0f;
        
        osg::Vec3d eye = center - osg::Vec3d(direction.x, direction.y, direction.z) * distance;
        
        viewer->getCameraManipulator()->setHomePosition(
            eye,
            center,
            osg::Vec3d(up.x, up.y, up.z)
        );
        viewer->getCameraManipulator()->home(0.0);
    }
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
        m_geoList.push_back(geo);
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
        auto it = std::find(m_geoList.begin(), m_geoList.end(), geo);
        if (it != m_geoList.end())
        {
            m_geoList.erase(it);
            m_geoNode->removeChild(geo->getOSGNode().get());
            
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
    for (Geo3D* geo : m_geoList)
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
    osg::Vec3f worldPoint;
    
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return glm::vec3(0, 0, 0);
    
    osg::Camera* camera = viewer->getCamera();
    if (camera->getViewport())
    {
        osg::Matrix VPW = camera->getViewMatrix() * 
                         camera->getProjectionMatrix() * 
                         camera->getViewport()->computeWindowMatrix();
        osg::Matrix invVPW;
        invVPW.invert(VPW);
        
        worldPoint = osg::Vec3f(x, height() - y, depth) * invVPW;
    }
    
    return glm::vec3(worldPoint.x(), worldPoint.y(), worldPoint.z());
}

glm::vec2 OSGWidget::worldToScreen(const glm::vec3& worldPos)
{
    osg::Vec2f screenPoint;
    
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return glm::vec2(0, 0);
    
    osg::Camera* camera = viewer->getCamera();
    if (camera->getViewport())
    {
        osg::Matrix VPW = camera->getViewMatrix() * 
                         camera->getProjectionMatrix() * 
                         camera->getViewport()->computeWindowMatrix();
        
        osg::Vec3f world(worldPos.x, worldPos.y, worldPos.z);
        osg::Vec3f screen = world * VPW;
        screenPoint = osg::Vec2f(screen.x(), height() - screen.y());
    }
    
    return glm::vec2(screenPoint.x(), screenPoint.y());
}

// OSGWidget 事件处理
void OSGWidget::paintEvent(QPaintEvent* event)
{
    // osgQOpenGLWidget已经处理了渲染，这里不需要手动调用frame()
    QOpenGLWidget::paintEvent(event);
}

void OSGWidget::resizeEvent(QResizeEvent* event)
{
    osgQOpenGLWidget::resizeEvent(event);
    
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    osg::Camera* camera = viewer->getCamera();
    if (camera)
    {
        camera->setViewport(0, 0, width(), height());
        double aspectRatio = static_cast<double>(width()) / static_cast<double>(height());
        camera->setProjectionMatrixAsPerspective(45.0f, aspectRatio, 0.1f, 1000.0f);
    }
}

void OSGWidget::mousePressEvent(QMouseEvent* event)
{
    osgQOpenGLWidget::mousePressEvent(event);
    handleDrawingInput(event);
}

void OSGWidget::mouseMoveEvent(QMouseEvent* event)
{
    osgQOpenGLWidget::mouseMoveEvent(event);
    
    // 更新鼠标世界坐标
    glm::vec3 worldPos = screenToWorld(event->x(), event->y(), 0.5f);
    m_lastMouseWorldPos = worldPos;
    emit mousePositionChanged(worldPos);
    
    // 处理绘制预览
    if (m_isDrawing && m_currentDrawingGeo)
    {
        updateCurrentDrawing(worldPos);
    }
}

void OSGWidget::mouseReleaseEvent(QMouseEvent* event)
{
    osgQOpenGLWidget::mouseReleaseEvent(event);
}

void OSGWidget::wheelEvent(QWheelEvent* event)
{
    osgQOpenGLWidget::wheelEvent(event);
}

void OSGWidget::keyPressEvent(QKeyEvent* event)
{
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
            removeGeo(m_selectedGeo);
            delete m_selectedGeo;
            m_selectedGeo = nullptr;
            emit geoSelected(nullptr);
        }
    }
}

void OSGWidget::keyReleaseEvent(QKeyEvent* event)
{
    osgQOpenGLWidget::keyReleaseEvent(event);
    
    if (m_isDrawing && m_currentDrawingGeo)
    {
        m_currentDrawingGeo->keyReleaseEvent(event);
    }
}

void OSGWidget::handleDrawingInput(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return;
    
    glm::vec3 worldPos = screenToWorld(event->x(), event->y(), 0.5f);
    
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
            m_currentDrawingGeo = createGeo3D(GlobalDrawMode3D);
            if (m_currentDrawingGeo)
            {
                m_isDrawing = true;
                addGeo(m_currentDrawingGeo);
                emit drawingProgress("开始绘制...");
            }
        }
        
        if (m_currentDrawingGeo)
        {
            m_currentDrawingGeo->mousePressEvent(event, worldPos);
            
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
        QMouseEvent moveEvent(QEvent::MouseMove, QPoint(0, 0), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        m_currentDrawingGeo->mouseMoveEvent(&moveEvent, worldPos);
    }
}

void OSGWidget::completeCurrentDrawing()
{
    if (m_currentDrawingGeo)
    {
        m_currentDrawingGeo->completeDrawing();
        emit drawingProgress("绘制完成");
        m_currentDrawingGeo = nullptr;
        m_isDrawing = false;
    }
}

void OSGWidget::cancelCurrentDrawing()
{
    if (m_currentDrawingGeo)
    {
        removeGeo(m_currentDrawingGeo);
        delete m_currentDrawingGeo;
        m_currentDrawingGeo = nullptr;
        m_isDrawing = false;
        emit drawingProgress("取消绘制");
    }
}
