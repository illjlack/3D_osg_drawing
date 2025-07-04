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
#include <QApplication>
#include <algorithm>

// ========================================= OSGWidget 实现 =========================================
OSGWidget::OSGWidget(QWidget* parent)
    : osgQOpenGLWidget(parent)
    , m_rootNode(new osg::Group)
    , m_sceneNode(new osg::Group)
    , m_geoNode(new osg::Group)
    , m_lightNode(new osg::Group)
    , m_pickingIndicatorNode(new osg::Group)
    , m_skyboxNode(new osg::Group)
    , m_trackballManipulator(new osgGA::TrackballManipulator)
    , m_cameraMoveSpeed(100.0) // 默认移动速度
    , m_currentDrawingGeo(nullptr)
    , m_selectedGeo(nullptr)
    , m_isDrawing(false)
    , m_lastMouseWorldPos(0.0f)
    , m_advancedPickingEnabled(false)
    , m_skybox(std::make_unique<Skybox>())
    , m_skyboxEnabled(true)
    , m_coordinateSystemRenderer(std::make_unique<CoordinateSystemRenderer>())
    , m_coordinateSystemEnabled(true)
    , m_updateTimer(new QTimer(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    // 初始化摄像机移动键状态
    for (int i = 0; i < 6; ++i)
    {
        m_cameraMoveKeys[i] = false;
    }
    
    // 连接信号槽
    connect(m_updateTimer, &QTimer::timeout, this, [this]() {
        update();
        // 处理摄像机移动
        updateCameraPosition();
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
    m_rootNode->addChild(m_skyboxNode);
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
        
        viewer->getCameraManipulator()->setHomePosition(
            eye,        // eye
            centerOsg,  // center
            osg::Vec3d(0, 0, 1)  // up
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
                
                viewer->getCameraManipulator()->setHomePosition(
                    combinedCenter + osg::Vec3d(combinedRadius * 2, combinedRadius * 2, combinedRadius * 2),
                    combinedCenter,
                    osg::Vec3d(0, 0, 1)
                );
            }
            else
            {
                // 如果场景为空，使用坐标系统范围
                osg::Vec3d center(coordCenter.x, coordCenter.y, coordCenter.z);
                double distance = coordRadius * 2.0;
                
                viewer->getCameraManipulator()->setHomePosition(
                    center + osg::Vec3d(distance, distance, distance),
                    center,
                    osg::Vec3d(0, 0, 1)
                );
            }
            
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
        // todo: 应该可以设置：视场角、近平面、远平面参数
        camera->setProjectionMatrixAsPerspective(45.0f, aspectRatio, 0.1f, 1000.0f);
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
    osgQOpenGLWidget::mouseMoveEvent(event);
    
    // 更新鼠标世界坐标
    glm::vec3 worldPos = screenToWorld(event->x(), event->y(), 0.5f);
    
    // 应用坐标范围限制
    CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
    glm::vec3 clampedPos = coordSystem->clampPointToSkybox(worldPos);
    
    m_lastMouseWorldPos = clampedPos;
    emit mousePositionChanged(clampedPos);
    
    // 处理绘制预览
    if (m_isDrawing && m_currentDrawingGeo)
    {
        updateCurrentDrawing(clampedPos);
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
                emit drawingProgress("开始绘制...");
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
        
        emit drawingProgress("绘制完成");
        
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
        
        emit drawingProgress("取消绘制");
        
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
        emit drawingProgress("切换到选择模式");
    }
    else
    {
        emit drawingProgress(tr("切换到绘制模式: %1").arg(drawMode3DToString(mode)));
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

// ========================================= 摄像机移动控制 =========================================

void OSGWidget::moveCameraUp()
{
    m_cameraMoveKeys[0] = true;
}

void OSGWidget::moveCameraDown()
{
    m_cameraMoveKeys[1] = true;
}

void OSGWidget::moveCameraLeft()
{
    m_cameraMoveKeys[2] = true;
}

void OSGWidget::moveCameraRight()
{
    m_cameraMoveKeys[3] = true;
}

void OSGWidget::moveCameraForward()
{
    m_cameraMoveKeys[4] = true;
}

void OSGWidget::moveCameraBackward()
{
    m_cameraMoveKeys[5] = true;
}

void OSGWidget::setCameraMoveSpeed(double speed)
{
    m_cameraMoveSpeed = speed;
    emit cameraMoveSpeedChanged(speed);
}

void OSGWidget::performCameraMove(const osg::Vec3d& direction)
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer || !viewer->getCameraManipulator()) return;
    
    // 获取当前相机位置和朝向
    osg::Vec3d eye, center, up;
    viewer->getCameraManipulator()->getInverseMatrix().getLookAt(eye, center, up);
    
    // 计算移动距离
    double moveDistance = m_cameraMoveSpeed * 0.016; // 基于16ms帧时间
    
    // 更新相机位置
    osg::Vec3d newEye = eye + direction * moveDistance;
    osg::Vec3d newCenter = center + direction * moveDistance;
    
    // 设置新的相机位置
    viewer->getCameraManipulator()->setHomePosition(newEye, newCenter, up);
    viewer->getCameraManipulator()->home(0.0);
}

void OSGWidget::updateCameraPosition()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer || !viewer->getCameraManipulator()) return;
    
    // 获取当前相机矩阵
    osg::Matrix viewMatrix = viewer->getCameraManipulator()->getInverseMatrix();
    osg::Vec3d right, up, forward;
    
    // 从视图矩阵中提取方向向量
    right = osg::Vec3d(viewMatrix(0,0), viewMatrix(0,1), viewMatrix(0,2));
    up = osg::Vec3d(viewMatrix(1,0), viewMatrix(1,1), viewMatrix(1,2));
    forward = osg::Vec3d(viewMatrix(2,0), viewMatrix(2,1), viewMatrix(2,2));
    
    // 标准化方向向量
    right.normalize();
    up.normalize();
    forward.normalize();
    
    // 计算移动方向
    osg::Vec3d moveDirection(0, 0, 0);
    
    if (m_cameraMoveKeys[0]) // Up
        moveDirection += up;
    if (m_cameraMoveKeys[1]) // Down
        moveDirection -= up;
    if (m_cameraMoveKeys[2]) // Left
        moveDirection -= right;
    if (m_cameraMoveKeys[3]) // Right
        moveDirection += right;
    if (m_cameraMoveKeys[4]) // Forward
        moveDirection -= forward;
    if (m_cameraMoveKeys[5]) // Backward
        moveDirection += forward;
    
    // 如果有移动，执行移动
    if (moveDirection.length2() > 0)
    {
        moveDirection.normalize();
        performCameraMove(moveDirection);
    }
}

void OSGWidget::keyPressEvent(QKeyEvent* event)
{
    // 处理摄像机移动键
    switch (event->key())
    {
        case Qt::Key_W:
        case Qt::Key_Up:
            moveCameraUp();
            break;
        case Qt::Key_S:
        case Qt::Key_Down:
            moveCameraDown();
            break;
        case Qt::Key_A:
        case Qt::Key_Left:
            moveCameraLeft();
            break;
        case Qt::Key_D:
        case Qt::Key_Right:
            moveCameraRight();
            break;
        case Qt::Key_Q:
            moveCameraForward();
            break;
        case Qt::Key_E:
            moveCameraBackward();
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
            return;
    }
    
    event->accept();
}

void OSGWidget::keyReleaseEvent(QKeyEvent* event)
{
    // 处理摄像机移动键释放
    switch (event->key())
    {
        case Qt::Key_W:
        case Qt::Key_Up:
            m_cameraMoveKeys[0] = false;
            break;
        case Qt::Key_S:
        case Qt::Key_Down:
            m_cameraMoveKeys[1] = false;
            break;
        case Qt::Key_A:
        case Qt::Key_Left:
            m_cameraMoveKeys[2] = false;
            break;
        case Qt::Key_D:
        case Qt::Key_Right:
            m_cameraMoveKeys[3] = false;
            break;
        case Qt::Key_Q:
            m_cameraMoveKeys[4] = false;
            break;
        case Qt::Key_E:
            m_cameraMoveKeys[5] = false;
            break;
        default:
            // 其他按键传递给基类
            osgQOpenGLWidget::keyReleaseEvent(event);
            
            if (m_isDrawing && m_currentDrawingGeo)
            {
                m_currentDrawingGeo->keyReleaseEvent(event);
            }
            return;
    }
    
    event->accept();
}
