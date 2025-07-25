#include "OSGWidget.h"
#include "../core/Common3D.h"
#include "../core/GeometryBase.h"
#include "../core/picking/PickingIndicator.h"

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
#include <osg/ComputeBoundsVisitor>
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
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include "ImportInfoDialog.h"
#include "../util/GeometryFactory.h"

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
    , m_lastMouseWorldPos(0.0)

    , m_skybox(std::make_unique<Skybox>())
    , m_skyboxEnabled(true)
    , m_coordinateSystemRenderer(std::make_unique<CoordinateSystemRenderer>())
    , m_coordinateSystemEnabled(true)
    , m_updateTimer(new QTimer(this))
    , m_mousePosCacheValid(false)
    , m_multiSelectMode(false)
    , m_isDraggingControlPoint(false)
    , m_draggingGeo(nullptr)
    , m_draggingControlPointIndex(-1)
    , m_geometryPickingSystem(nullptr)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setFocus(); // 确保获得焦点
    
    // 连接信号槽
    connect(m_updateTimer, &QTimer::timeout, this, [this]() {
        update(); });
    
    // 连接初始化完成信号
    connect(this, &osgQOpenGLWidget::initialized, this, &OSGWidget::initializeScene);
    
    // 设置渲染循环
    m_updateTimer->start(16);
    
    LOG_INFO("OSGWidget初始化完成", "系统");
}

// ========================================= 右键菜单功能实现 =========================================

void OSGWidget::contextMenuEvent(QContextMenuEvent* event)
{
    if (!event) return;
    
    // ========================================= 自定义条件判断 =========================================
    

     if (GlobalDrawMode3D != DrawSelect3D) {
         LOG_INFO("非选择模式，跳过右键菜单", "右键菜单");
         return; // 不显示右键菜单
     }   

    //if (m_isDrawing) {
    //    LOG_INFO("正在绘制中，跳过右键菜单", "右键菜单");
    //    return; // 不显示右键菜单
    //}
    

    
    m_lastContextMenuPos = event->pos();
    
    // 使用拾取系统检测右键点击位置的对象
    PickResult pickResult = performSimplePicking(event->x(), height() - event->y());
    
    QMenu contextMenu(this);
    
    if (pickResult.hasResult && pickResult.geometry)
    {
        // 点击到了对象
        m_contextMenuGeo = pickResult.geometry;
        m_contextMenuPointIndex = pickResult.primitiveIndex;
        
        // 对象相关菜单
        QAction* deleteAction = contextMenu.addAction("删除选中对象");
        deleteAction->setIcon(QIcon(":/icons/delete.png"));
        connect(deleteAction, &QAction::triggered, this, &OSGWidget::onDeleteSelectedObjects);
        
        contextMenu.addSeparator();
        
        if (pickResult.featureType == PickFeatureType::VERTEX && pickResult.primitiveIndex >= 0)
        {
            // 点击到了顶点
            QAction* movePointAction = contextMenu.addAction("移动点到坐标...");
            movePointAction->setIcon(QIcon(":/icons/move.png"));
            connect(movePointAction, &QAction::triggered, this, &OSGWidget::onMovePointToCoordinate);
        }
        
        QAction* centerObjectAction = contextMenu.addAction("将对象居中显示");
        centerObjectAction->setIcon(QIcon(":/icons/center.png"));
        connect(centerObjectAction, &QAction::triggered, this, &OSGWidget::onCenterObjectToView);
    }
    else
    {
        // 点击到了空白区域
        m_contextMenuGeo = nullptr;
        m_contextMenuPointIndex = -1;
        
        // 通用菜单
        if (!m_selectedGeos.empty())
        {
            QAction* deleteSelectedAction = contextMenu.addAction(QString("删除选中对象 (%1个)").arg(m_selectedGeos.size()));
            deleteSelectedAction->setIcon(QIcon(":/icons/delete.png"));
            connect(deleteSelectedAction, &QAction::triggered, this, &OSGWidget::onDeleteSelectedObjects);
            
            contextMenu.addSeparator();
        }
    }
    
    // 视图相关菜单
    QMenu* viewMenu = contextMenu.addMenu("视图");
    
    QAction* setCameraAction = viewMenu->addAction("设置眼点坐标...");
    setCameraAction->setIcon(QIcon(":/icons/camera.png"));
    connect(setCameraAction, &QAction::triggered, this, &OSGWidget::onSetEyePosition);
    
    QAction* resetCameraAction = viewMenu->addAction("重置相机");
    resetCameraAction->setIcon(QIcon(":/icons/reset_camera.png"));
    connect(resetCameraAction, &QAction::triggered, this, &OSGWidget::onResetCamera);
    
    QAction* fitAllAction = viewMenu->addAction("适应窗口");
    fitAllAction->setIcon(QIcon(":/icons/fit_all.png"));
    connect(fitAllAction, &QAction::triggered, this, &OSGWidget::onFitAll);
    
    // 坐标系设置菜单
    contextMenu.addSeparator();
    QAction* coordSystemAction = contextMenu.addAction("坐标系统设置...");
    coordSystemAction->setIcon(QIcon(":/icons/coordinates.png"));
    connect(coordSystemAction, &QAction::triggered, [this]() {
        // 发射信号给主窗口处理
        emit coordinateSystemSettingsRequested();
    });
    
    // 显示菜单
    if (!contextMenu.actions().isEmpty())
    {
        contextMenu.exec(event->globalPos());
    }
    
    LOG_INFO("显示右键菜单", "右键菜单");
}

void OSGWidget::onDeleteSelectedObjects()
{
    if (m_selectedGeos.empty() && !m_contextMenuGeo)
    {
        LOG_WARNING("没有选中的对象可删除", "右键菜单");
        return;
    }
    
    int objectCount = 0;
    
    if (m_contextMenuGeo)
    {
        // 删除右键点击的对象
        removeGeo(m_contextMenuGeo);
        objectCount = 1;
        LOG_INFO("删除右键点击的对象", "右键菜单");
    }
    
    if (!m_selectedGeos.empty())
    {
        // 删除所有选中的对象
        std::vector<osg::ref_ptr<Geo3D>> objectsToDelete = m_selectedGeos; // 复制列表避免迭代时修改
        
        for (osg::ref_ptr<Geo3D> geo : objectsToDelete)
        {
            if (geo)
            {
                removeGeo(geo);
                objectCount++;
            }
        }
        
        clearSelection();
        LOG_INFO(QString("删除 %1 个选中对象").arg(objectsToDelete.size()), "右键菜单");
    }
    
    // 重置上下文菜单状态
    m_contextMenuGeo = nullptr;
    m_contextMenuPointIndex = -1;
    
    QMessageBox::information(this, "删除完成", QString("已删除 %1 个对象").arg(objectCount));
}

void OSGWidget::onSetEyePosition()
{
    if (!m_cameraController)
    {
        LOG_ERROR("摄像机控制器不可用", "右键菜单");
        return;
    }
    
    // 获取当前相机位置
    osg::Vec3 currentEye, currentCenter, currentUp;
    m_cameraController->getViewMatrixAsLookAt(currentEye, currentCenter, currentUp);
    
    bool ok;
    
    // 输入新的眼点坐标
    QString eyeText = QInputDialog::getText(this, "设置眼点坐标", 
        QString("请输入眼点坐标 (x,y,z):\n当前位置: (%1, %2, %3)")
            .arg(currentEye.x(), 0, 'f', 3)
            .arg(currentEye.y(), 0, 'f', 3)
            .arg(currentEye.z(), 0, 'f', 3),
        QLineEdit::Normal,
        QString("%1,%2,%3").arg(currentEye.x(), 0, 'f', 3).arg(currentEye.y(), 0, 'f', 3).arg(currentEye.z(), 0, 'f', 3),
        &ok);
    
    if (!ok || eyeText.isEmpty()) return;
    
    // 解析输入的坐标
    QStringList coords = eyeText.split(',');
    if (coords.size() != 3)
    {
        QMessageBox::warning(this, "输入错误", "请输入有效的坐标格式: x,y,z");
        return;
    }
    
    bool xOk, yOk, zOk;
    double x = coords[0].trimmed().toDouble(&xOk);
    double y = coords[1].trimmed().toDouble(&yOk);
    double z = coords[2].trimmed().toDouble(&zOk);
    
    if (!xOk || !yOk || !zOk)
    {
        QMessageBox::warning(this, "输入错误", "请输入有效的数值坐标");
        return;
    }
    
    // 输入目标点坐标
    QString targetText = QInputDialog::getText(this, "设置目标点坐标", 
        QString("请输入目标点坐标 (x,y,z):\n当前目标: (%1, %2, %3)")
            .arg(currentCenter.x(), 0, 'f', 3)
            .arg(currentCenter.y(), 0, 'f', 3)
            .arg(currentCenter.z(), 0, 'f', 3),
        QLineEdit::Normal,
        QString("%1,%2,%3").arg(currentCenter.x(), 0, 'f', 3).arg(currentCenter.y(), 0, 'f', 3).arg(currentCenter.z(), 0, 'f', 3),
        &ok);
    
    if (!ok || targetText.isEmpty()) return;
    
    // 解析目标点坐标
    QStringList targetCoords = targetText.split(',');
    if (targetCoords.size() != 3)
    {
        QMessageBox::warning(this, "输入错误", "请输入有效的目标点坐标格式: x,y,z");
        return;
    }
    
    bool txOk, tyOk, tzOk;
    double tx = targetCoords[0].trimmed().toDouble(&txOk);
    double ty = targetCoords[1].trimmed().toDouble(&tyOk);
    double tz = targetCoords[2].trimmed().toDouble(&tzOk);
    
    if (!txOk || !tyOk || !tzOk)
    {
        QMessageBox::warning(this, "输入错误", "请输入有效的目标点数值坐标");
        return;
    }
    
    // 设置相机位置
    setCameraPosition(glm::dvec3(x, y, z), glm::dvec3(tx, ty, tz));
    
    LOG_INFO(QString("设置眼点坐标: (%1, %2, %3) -> (%4, %5, %6)")
        .arg(x, 0, 'f', 3).arg(y, 0, 'f', 3).arg(z, 0, 'f', 3)
        .arg(tx, 0, 'f', 3).arg(ty, 0, 'f', 3).arg(tz, 0, 'f', 3), "右键菜单");
}

void OSGWidget::onMovePointToCoordinate()
{
    if (!m_contextMenuGeo || m_contextMenuPointIndex < 0)
    {
        LOG_WARNING("没有选中的有效控制点", "右键菜单");
        return;
    }
    
    bool ok;
    QString coordText = QInputDialog::getText(this, "移动点到坐标", 
        QString("请输入新的坐标 (x,y,z):"),
        QLineEdit::Normal,
        QString("0,0,0"),
        &ok);
    
    if (!ok || coordText.isEmpty()) return;
    
    // 解析输入的坐标
    QStringList coords = coordText.split(',');
    if (coords.size() != 3)
    {
        QMessageBox::warning(this, "输入错误", "请输入有效的坐标格式: x,y,z");
        return;
    }
    
    bool xOk, yOk, zOk;
    double x = coords[0].trimmed().toDouble(&xOk);
    double y = coords[1].trimmed().toDouble(&yOk);
    double z = coords[2].trimmed().toDouble(&zOk);
    
    if (!xOk || !yOk || !zOk)
    {
        QMessageBox::warning(this, "输入错误", "请输入有效的数值坐标");
        return;
    }
    
    // 移动点到新坐标
    movePointToCoordinate(m_contextMenuGeo, m_contextMenuPointIndex, glm::dvec3(x, y, z));
    
    LOG_INFO(QString("移动控制点到新坐标: (%1, %2, %3)")
        .arg(x, 0, 'f', 3).arg(y, 0, 'f', 3).arg(z, 0, 'f', 3), "右键菜单");
}

void OSGWidget::onCenterObjectToView()
{
    if (!m_contextMenuGeo)
    {
        LOG_WARNING("没有选中的对象", "右键菜单");
        return;
    }
    
    // 计算对象的包围盒
    osg::ComputeBoundsVisitor visitor;
    m_contextMenuGeo->mm_node()->getOSGNode()->accept(visitor);
    osg::BoundingBox boundingBox = visitor.getBoundingBox();
    
    if (!boundingBox.valid())
    {
        LOG_WARNING("对象包围盒无效", "右键菜单");
        return;
    }
    
    // 设置相机查看整个对象
    osg::Vec3 center = boundingBox.center();
    double radius = boundingBox.radius();
    double distance = radius * 2.5; // 距离为半径的2.5倍
    
    osg::Vec3 eye = center + osg::Vec3(distance, distance, distance);
    setCameraPosition(glm::dvec3(eye.x(), eye.y(), eye.z()), glm::dvec3(center.x(), center.y(), center.z()));
    
    LOG_INFO("将对象居中显示", "右键菜单");
}

void OSGWidget::onResetCamera()
{
    resetCamera();
    LOG_INFO("重置相机", "右键菜单");
}

void OSGWidget::onFitAll()
{
    fitAll();
    LOG_INFO("适应窗口", "右键菜单");
}

// ========================================= 右键菜单相关公共方法实现 =========================================

void OSGWidget::deleteSelectedObjects()
{
    onDeleteSelectedObjects();
}

void OSGWidget::setCameraPosition(const glm::dvec3& position, const glm::dvec3& target)
{
    if (!m_cameraController)
    {
        LOG_ERROR("摄像机控制器不可用", "相机");
        return;
    }
    
    osg::Vec3d eye(position.x, position.y, position.z);
    osg::Vec3d center(target.x, target.y, target.z);
    osg::Vec3d up(0, 0, 1); // 默认向上方向
    
    m_cameraController->setPosition(eye, center, up);
    
    LOG_INFO(QString("设置相机位置: 眼点(%1,%2,%3) 目标(%4,%5,%6)")
        .arg(position.x, 0, 'f', 3).arg(position.y, 0, 'f', 3).arg(position.z, 0, 'f', 3)
        .arg(target.x, 0, 'f', 3).arg(target.y, 0, 'f', 3).arg(target.z, 0, 'f', 3), "相机");
}

void OSGWidget::movePointToCoordinate(osg::ref_ptr<Geo3D> geo, int pointIndex, const glm::dvec3& newPosition)
{
    if (!geo || pointIndex < 0)
    {
        LOG_ERROR("无效的几何体或点索引", "点移动");
        return;
    }
    
    // 获取控制点管理器
    auto controlPointManager = geo->mm_controlPoint();
    if (!controlPointManager)
    {
        LOG_ERROR("几何体没有控制点管理器", "点移动");
        return;
    }
    
    // 更新控制点位置
    Point3D newPoint(newPosition.x, newPosition.y, newPosition.z);
    controlPointManager->setControlPoint(pointIndex, newPoint);
}

OSGWidget::~OSGWidget()
{
    if (m_updateTimer)
    {
        m_updateTimer->stop();
    }
    
    // 清理几何拾取系统
    if (m_geometryPickingSystem) {
        m_geometryPickingSystem->shutdown();
        m_geometryPickingSystem = nullptr;
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
    
    // 设置各个节点的掩码
    m_pickingIndicatorNode->setNodeMask(NODE_MASK_PICKING_INDICATOR); // 拾取指示器，避免被拾取
    m_skyboxNode->setNodeMask(NODE_MASK_SKYBOX);                     // 天空盒节点掩码
    
    // ========================================= 改进的抗锯齿设置 =========================================
    osg::StateSet* rootStateSet = m_rootNode->getOrCreateStateSet();
    
    // 1. 基础抗锯齿设置
    rootStateSet->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    rootStateSet->setMode(GL_POINT_SMOOTH, osg::StateAttribute::ON);
    rootStateSet->setMode(GL_POLYGON_SMOOTH, osg::StateAttribute::ON);
    rootStateSet->setMode(GL_MULTISAMPLE, osg::StateAttribute::ON);
    
    // 2. 高质量混合函数设置
    osg::ref_ptr<osg::BlendFunc> rootBlendFunc = new osg::BlendFunc();
    rootBlendFunc->setSource(osg::BlendFunc::SRC_ALPHA);
    rootBlendFunc->setDestination(osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    rootStateSet->setAttributeAndModes(rootBlendFunc.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    
    // 3. 改进的多重采样抗锯齿设置
    osg::ref_ptr<osg::Multisample> rootMultisample = new osg::Multisample();
    rootMultisample->setHint(osg::Multisample::NICEST);  // 使用最高质量
    rootMultisample->setCoverage(1.0);  // 提高覆盖率到1.0
    rootMultisample->setInvert(false);
    rootStateSet->setAttributeAndModes(rootMultisample.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    
    // 4. 优化线宽和点大小以获得更好的抗锯齿效果
    osg::ref_ptr<osg::LineWidth> rootLineWidth = new osg::LineWidth();
    rootLineWidth->setWidth(1.5);  // 稍微增加线宽以改善抗锯齿效果
    rootStateSet->setAttributeAndModes(rootLineWidth.get(), osg::StateAttribute::ON);
    
    osg::ref_ptr<osg::Point> rootPointSize = new osg::Point();
    rootPointSize->setSize(4.0);  // 稍微增加点大小
    rootPointSize->setMinSize(2.0);  // 设置最小点大小
    rootPointSize->setMaxSize(8.0);  // 设置最大点大小
    rootPointSize->setDistanceAttenuation(osg::Vec3(1.0, 0.0, 0.0));  // 距离衰减
    rootStateSet->setAttributeAndModes(rootPointSize.get(), osg::StateAttribute::ON);
    
    // 5. 启用混合和深度测试
    rootStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    rootStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    
    // 6. 设置更高质量的渲染提示
    rootStateSet->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
    
    // 7. 针对几何体节点的特殊抗锯齿设置
    osg::StateSet* geoStateSet = m_geoNode->getOrCreateStateSet();
    geoStateSet->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    geoStateSet->setMode(GL_POINT_SMOOTH, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    geoStateSet->setMode(GL_POLYGON_SMOOTH, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    
    // 为几何体设置更精细的线宽
    osg::ref_ptr<osg::LineWidth> geoLineWidth = new osg::LineWidth();
    geoLineWidth->setWidth(2.0);  // 几何体使用稍粗的线条
    geoStateSet->setAttributeAndModes(geoLineWidth.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    
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
    camera->setClearColor(osg::Vec4(0.2, 0.2, 0.2, 1.0));
    
    // 设置初始视点
    resetCamera();
}

void OSGWidget::setupLighting()
{
    // 创建光源
    osg::ref_ptr<osg::Light> light = new osg::Light();
    light->setLightNum(0);
    light->setPosition(osg::Vec4(10.0, 10.0, 10.0, 1.0));
    light->setDirection(osg::Vec3(-1.0, -1.0, -1.0));
    light->setAmbient(osg::Vec4(0.3, 0.3, 0.3, 1.0));
    light->setDiffuse(osg::Vec4(0.8, 0.8, 0.8, 1.0));
    light->setSpecular(osg::Vec4(1.0, 1.0, 1.0, 1.0));
    
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
    LOG_INFO("开始设置拾取系统", "拾取");
    
    // 创建拾取指示器
    m_pickingIndicator = new PickingIndicator;
    if (!m_pickingIndicator->initialize()) {
        LOG_ERROR("拾取指示器初始化失败", "拾取");
        return;
    }
    
    // 添加指示器到场景
    osg::Group* indicatorRoot = m_pickingIndicator->getIndicatorRoot();
    if (indicatorRoot) {
        m_pickingIndicatorNode->addChild(indicatorRoot);
        LOG_INFO("拾取指示器已添加到场景", "拾取");
    }
    
    // 创建几何拾取系统
    m_geometryPickingSystem = new GeometryPickingSystem();
    
    // 配置拾取系统，使其与拾取指示器使用相同的像素半径
    PickConfig pickConfig;
    pickConfig.cylinderRadius = m_pickingIndicator->getConfig().pickingPixelRadius;  // 保持一致
    m_geometryPickingSystem->setConfig(pickConfig);
    
    // 获取相机和场景根节点来初始化拾取系统
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) {
        LOG_WARNING("OSG查看器为空，延迟初始化几何拾取系统", "拾取");
        // 使用定时器延迟重试
        QTimer::singleShot(100, this, [this]() {
            if (getOsgViewer() && getOsgViewer()->getCamera()) {
                if (m_geometryPickingSystem && !m_geometryPickingSystem->isInitialized()) {
                    // 再次配置拾取系统
                    PickConfig pickConfig;
                    pickConfig.cylinderRadius = m_pickingIndicator->getConfig().pickingPixelRadius;
                    m_geometryPickingSystem->setConfig(pickConfig);
                    
                    if (m_geometryPickingSystem->initialize(getOsgViewer()->getCamera(), m_sceneNode.get())) {
                        LOG_SUCCESS("几何拾取系统延迟初始化完成", "拾取");
                    } else {
                        LOG_ERROR("几何拾取系统延迟初始化失败", "拾取");
                    }
                }
            }
        });
        return;
    }
    
    if (!viewer->getCamera()) {
        LOG_WARNING("OSG相机为空，延迟初始化几何拾取系统", "拾取");
        // 使用定时器延迟重试
        QTimer::singleShot(100, this, [this]() {
            if (getOsgViewer() && getOsgViewer()->getCamera()) {
                if (m_geometryPickingSystem && !m_geometryPickingSystem->isInitialized()) {
                    // 再次配置拾取系统
                    PickConfig pickConfig;
                    pickConfig.cylinderRadius = m_pickingIndicator->getConfig().pickingPixelRadius;
                    m_geometryPickingSystem->setConfig(pickConfig);
                    
                    if (m_geometryPickingSystem->initialize(getOsgViewer()->getCamera(), m_sceneNode.get())) {
                        LOG_SUCCESS("几何拾取系统延迟初始化完成", "拾取");
                    } else {
                        LOG_ERROR("几何拾取系统延迟初始化失败", "拾取");
                    }
                }
            }
        });
        return;
    }
    
    LOG_INFO("OSG查看器和相机已准备就绪，初始化几何拾取系统", "拾取");
    if (!m_geometryPickingSystem->initialize(viewer->getCamera(), m_sceneNode.get())) {
        LOG_ERROR("几何拾取系统初始化失败", "拾取");
        m_geometryPickingSystem = nullptr;
    } else {
        LOG_SUCCESS("几何拾取系统初始化完成", "拾取");
    }
    
    LOG_SUCCESS("拾取指示器设置完成", "拾取");
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

void OSGWidget::setViewDirection(const glm::dvec3& direction, const glm::dvec3& up)
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

void OSGWidget::addGeo(osg::ref_ptr<Geo3D> geo)
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
        
        // 几何体已添加到场景，无需额外的拾取系统注册
    }
}

void OSGWidget::removeGeo(osg::ref_ptr<Geo3D> geo)
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

void OSGWidget::selectGeo(osg::ref_ptr<Geo3D> geo)
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
void OSGWidget::addToSelection(osg::ref_ptr<Geo3D> geo)
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

void OSGWidget::removeFromSelection(osg::ref_ptr<Geo3D> geo)
{
    if (!geo) return;
    
    auto it = std::find(m_selectedGeos.begin(), m_selectedGeos.end(), geo);
    if (it != m_selectedGeos.end())
    {
        m_selectedGeos.erase(it);
        geo->mm_state()->clearStateSelected(); // 这会自动隐藏包围盒
        
        // 清除控制点高亮
        // if (m_advancedPickingEnabled)
        // {
        //     PickingSystemManager::getInstance().hideHighlight();
        // }
        
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
    for (auto& geo : m_selectedGeos)
    {
        if (geo)
        {
            geo->mm_state()->clearStateSelected(); // 这会自动隐藏包围盒
        }
    }
    
    // 清除控制点高亮
    // if (m_advancedPickingEnabled)
    // {
    //     PickingSystemManager::getInstance().hideHighlight();
    // }
    
    m_selectedGeos.clear();
    m_selectedGeo = nullptr;
    
    // 发送选择信号
    emit geoSelected(nullptr);
    
    LOG_INFO("清除所有选择", "选择");
}

const std::vector<osg::ref_ptr<Geo3D>>& OSGWidget::getSelectedGeos() const
{
    return m_selectedGeos;
}

bool OSGWidget::isSelected(osg::ref_ptr<Geo3D> geo) const
{
    if (!geo) return false;
    return std::find(m_selectedGeos.begin(), m_selectedGeos.end(), geo) != m_selectedGeos.end();
}

int OSGWidget::getSelectionCount() const
{
    return static_cast<int>(m_selectedGeos.size());
}

// 拖动控制点功能实现
void OSGWidget::startDraggingControlPoint(osg::ref_ptr<Geo3D> geo, int controlPointIndex)
{
    if (!geo || controlPointIndex < 0) return;
    
    m_isDraggingControlPoint = true;
    m_draggingGeo = geo;
    m_draggingControlPointIndex = controlPointIndex;
    m_dragStartPosition = m_lastMouseWorldPos;
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
    // if (m_advancedPickingEnabled)
    // {
    //     // 暂时不调用高亮系统，因为SimplifiedPickingSystemManager没有直接的高亮清除方法
    //     // 高亮会在下一次拾取时自动更新
    // }
    
    // 为选中的对象创建高亮和显示包围盒
    // highlightSelectedObjects();
}

void OSGWidget::highlightSelectedObjects()
{
    // if (!m_advancedPickingEnabled || m_selectedGeos.empty())
    //     return;
    
    // 为每个选中的对象创建高亮和显示包围盒
    // for (auto& geo : m_selectedGeos)
    // {
    //     if (geo)
    //     {
    //         // 注意：不要在这里再次调用setStateSelected，因为addToSelection已经调用了
    //         // 这里只添加额外的高亮效果
    //         // 暂时使用简化的方式
    //     }
    // }
}

void OSGWidget::onSimplePickingResult(const PickResult& result)
{
    // 发射OSG索引拾取结果信号
    emit simplePickingResult(result);
    
    // 如果有拾取结果，立即更新状态栏坐标（覆盖普通坐标）
    if (result.hasResult) {
        m_lastMouseWorldPos = result.worldPosition;
        emit mousePositionChanged(result.worldPosition);
    }
}



glm::dvec3 OSGWidget::screenToWorld(int x, int y, double depth)
{
    if (!m_cameraController) return glm::dvec3(0, 0, 0);
    
    // 检查鼠标位置缓存
    QPoint currentPos(x, y);
    if (m_mousePosCacheValid && 
        m_lastMouseScreenPos == currentPos &&
        m_lastMouseCalculation.isValid() &&
        m_lastMouseCalculation.msecsTo(QDateTime::currentDateTime()) < MOUSE_CACHE_DURATION)
    {
        return m_cachedMouseWorldPos;
    }
    
    // 直接委托给CameraController
    osg::Vec3d worldPoint = m_cameraController->screenToWorld(x, y, depth, width(), height());
    glm::dvec3 result(worldPoint.x(), worldPoint.y(), worldPoint.z());
    
    // 更新缓存
    m_lastMouseScreenPos = currentPos;
    m_cachedMouseWorldPos = result;
    m_mousePosCacheValid = true;
    m_lastMouseCalculation = QDateTime::currentDateTime();
    
    return result;
}

glm::dvec2 OSGWidget::worldToScreen(const glm::dvec3& worldPos)
{
    if (!m_cameraController) return glm::dvec2(0, 0);
    
    // 直接委托给CameraController
    osg::Vec2d screenPoint = m_cameraController->worldToScreen(osg::Vec3d(worldPos.x, worldPos.y, worldPos.z), width(), height());
    return glm::dvec2(screenPoint.x(), screenPoint.y());
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
    // 右键进入下一阶段或完成绘制
    if (event->button() == Qt::RightButton && m_isDrawing)
    {
        if (m_currentDrawingGeo)
        {
            auto controlPointManager = m_currentDrawingGeo->mm_controlPoint();
            if (controlPointManager)
            {
                // 尝试进入下一阶段
                bool hasNextStage = controlPointManager->nextStage();
                if (!hasNextStage)
                {
                    // 没有下一阶段，完成绘制
                    completeCurrentDrawing();
                }
                else
                {
                    LOG_INFO("进入下一阶段", "绘制");
                }
            }
            else
            {
                // 没有控制点管理器，直接完成绘制
                completeCurrentDrawing();
            }
        }
        else
        {
            // 没有当前绘制对象，直接完成绘制
            completeCurrentDrawing();
        }
        return;
    }

    // 只处理左键事件
    if (event->button() != Qt::LeftButton)
        return;

    PickResult pickResult = performSimplePicking(event->x(), height() - event->y());
    glm::dvec3 worldPos;
    if (pickResult.hasResult) 
    { 
        worldPos = pickResult.worldPosition; 
    }
    else 
    { 
        worldPos= screenToWorld(event->x(), event->y(), 0.5);
        // 坐标限制在坐标系内
        CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
        glm::dvec3 clampedPos = coordSystem->clampPointToSkybox(worldPos);
        worldPos = clampedPos;
    }

    emit mousePositionChanged(worldPos);

    if (GlobalDrawMode3D == DrawSelect3D)
    {
        if (pickResult.hasResult && pickResult.geometry)
        {
            osg::ref_ptr<Geo3D> pickedGeo = pickResult.geometry;

            if (QApplication::keyboardModifiers() & Qt::ControlModifier)
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
            clearSelection();
        }

        if (pickResult.hasResult && pickResult.geometry && pickResult.osgGeometry)
        {
            // 检查是否为顶点拾取（控制点）
            if (pickResult.featureType == PickFeatureType::VERTEX && pickResult.osgGeometry->getNodeMask() == NODE_MASK_CONTROL_POINTS && pickResult.primitiveIndex >= 0)
            {
                // 检查该几何体是否在选中列表中
                auto it = std::find(m_selectedGeos.begin(), m_selectedGeos.end(), pickResult.geometry);
                if (it != m_selectedGeos.end())
                {
                    startDraggingControlPoint(pickResult.geometry, pickResult.primitiveIndex);
                    event->accept();
                    return;
                }
            }
        }
    }
    else
    {
        if (!m_isDrawing)
        {
            // 开始新的绘制
            osg::ref_ptr<Geo3D> newGeo = GeometryFactory::createGeometry(GlobalDrawMode3D);
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
            // 直接调用控制点管理器而不是几何体的事件处理方法
            auto controlPointManager = m_currentDrawingGeo->mm_controlPoint();
            if (controlPointManager)
            {
                // 左键添加控制点
                if (event->button() == Qt::LeftButton)
                {
                    bool success = controlPointManager->addControlPoint(Point3D(worldPos));
                    if (success)
                    {
                        LOG_INFO("添加控制点成功", "绘制");
                        
                        // 检查是否完成绘制
                        if (m_currentDrawingGeo->mm_state()->isStateComplete())
                        {
                            completeCurrentDrawing();
                        }
                    }
                }
                // 右键进入下一阶段或完成绘制（某些几何体需要）
                else if (event->button() == Qt::RightButton && !m_isDrawing)
                {
                    // 这里右键已经在函数开头处理了，用于进入下一阶段或完成绘制
                    // 具体逻辑已经在函数开头实现
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
        // 使用简化拾取获取世界坐标
        PickResult pickResult = performSimplePicking(event->x(), height() - event->y());
        
        if (pickResult.hasResult)
        {
            // 使用拾取结果的世界坐标
            m_lastMouseWorldPos = pickResult.worldPosition;
            emit mousePositionChanged(pickResult.worldPosition);
            emit simplePickingResult(pickResult);
        }
        else
        {
            // 如果没有拾取到对象，使用屏幕坐标转换
            glm::dvec3 worldPos = screenToWorld(event->x(), event->y(), 0.5);
            
            // 应用坐标范围限制
            CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
            glm::dvec3 clampedPos = coordSystem->clampPointToSkybox(worldPos);
            
            m_lastMouseWorldPos = clampedPos;
            emit mousePositionChanged(clampedPos);
        }
        
        lastMouseUpdate = currentTime;
    }
    
    // 处理拖动控制点
    if (m_isDraggingControlPoint && m_draggingGeo && m_draggingControlPointIndex >= 0)
    {
        // 直接使用当前鼠标世界坐标作为新的控制点位置
        Point3D newPoint(m_lastMouseWorldPos.x, m_lastMouseWorldPos.y, m_lastMouseWorldPos.z);
        m_draggingGeo->mm_controlPoint()->setControlPoint(m_draggingControlPointIndex, newPoint);
        
        // 拖动控制点更新（移除频繁的调试日志）
    }
    
    // 处理绘制预览 - 使用拾取系统获取更精确的世界坐标
    if (m_isDrawing && m_currentDrawingGeo)
    {
        // 使用简化拾取获取世界坐标
        PickResult pickResult = performSimplePicking(event->x(), height() - event->y());
        glm::dvec3 drawingWorldPos;
        
        if (pickResult.hasResult)
        {
            // 使用拾取结果的世界坐标
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

void OSGWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    // 只在选择模式和左键双击时处理
    if (GlobalDrawMode3D != DrawSelect3D || event->button() != Qt::LeftButton) {
        osgQOpenGLWidget::mouseDoubleClickEvent(event);
        return;
    }
    
    // 执行拾取操作获取双击位置的世界坐标
    PickResult pickResult = performSimplePicking(event->x(), height() - event->y());
    glm::dvec3 worldPos;
    
    if (pickResult.hasResult) {
        // 如果拾取到了几何对象，使用拾取到的世界坐标
        worldPos = pickResult.worldPosition;
        LOG_INFO(QString("双击拾取到几何对象，位置: (%1, %2, %3)")
                 .arg(worldPos.x, 0, 'f', 2)
                 .arg(worldPos.y, 0, 'f', 2)
                 .arg(worldPos.z, 0, 'f', 2), "相机");
    } else {
        // 如果没有拾取到几何对象，使用屏幕坐标转换为世界坐标
        worldPos = screenToWorld(event->x(), event->y(), 0.5);
        
        // 坐标限制在坐标系内
        CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
        glm::dvec3 clampedPos = coordSystem->clampPointToSkybox(worldPos);
        worldPos = clampedPos;
        
        LOG_INFO(QString("双击空白区域，转换世界坐标: (%1, %2, %3)")
                 .arg(worldPos.x, 0, 'f', 2)
                 .arg(worldPos.y, 0, 'f', 2)
                 .arg(worldPos.z, 0, 'f', 2), "相机");
    }
    
    // 设置相机旋转中心
    if (m_cameraController) {
        osg::Vec3d newCenter(worldPos.x, worldPos.y, worldPos.z);
        m_cameraController->setRotationCenter(newCenter);
        
        LOG_SUCCESS(QString("相机旋转中心已设置为: (%1, %2, %3)")
                   .arg(worldPos.x, 0, 'f', 2)
                   .arg(worldPos.y, 0, 'f', 2)
                   .arg(worldPos.z, 0, 'f', 2), "相机");
    }
    
    event->accept();
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

void OSGWidget::updateCurrentDrawing(const glm::dvec3& worldPos)
{
    if (m_currentDrawingGeo)
    {
        // 应用天空盒范围限制，确保不会超出天空盒
        CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
        glm::dvec3 clampedPos = coordSystem->clampPointToSkybox(worldPos);
        
        // 直接调用控制点管理器设置临时点
        auto controlPointManager = m_currentDrawingGeo->mm_controlPoint();
        if (controlPointManager)
        {
            controlPointManager->setTempPoint(Point3D(clampedPos));
        }
        
        // QMouseEvent moveEvent(QEvent::MouseMove, QPoint(0, 0), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        // m_currentDrawingGeo->mouseMoveEvent(&moveEvent, clampedPos);  // 移除原来的调用
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
        m_skybox->setCenter(osg::Vec3(0.0, 0.0, 0.0));
        
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
            coordSystemNode->setNodeMask(NODE_MASK_COORDINATE_SYSTEM); // 设置坐标系统节点掩码
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



// ========================================= 摄像机控制器委托方法已移除 =========================================
// 现在直接通过getCameraController()访问相机控制器的功能



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
                auto controlPointManager = m_currentDrawingGeo->mm_controlPoint();
                if (controlPointManager)
                {
                    if (event->key() == Qt::Key_Escape)
                    {
                        // ESC键撤销上一个控制点，如果没有控制点则取消绘制
                        bool hasControlPoints = controlPointManager->undoLastControlPoint();
                        if (!hasControlPoints)
                        {
                            cancelCurrentDrawing();
                        }
                        LOG_INFO("撤销上一个控制点", "绘制");
                    }
                    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
                    {
                        completeCurrentDrawing();
                    }
                    else
                    {
                        // 其他按键处理 - 移除了keyPressEvent调用，因为不再提供此接口
                        // m_currentDrawingGeo->keyPressEvent(event);
                    }
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
                // 移除了keyReleaseEvent调用，因为不再提供此接口
                // m_currentDrawingGeo->keyReleaseEvent(event);
            }
            break;
    }
    
    event->accept();
}

// ========================================= 投影模式相关方法已移至CameraController =========================================
// 现在直接通过getCameraController()访问投影模式功能

// ========================================= 几何对象查询方法 =========================================

osg::ref_ptr<Geo3D> OSGWidget::getSelectedGeo() const
{
    return m_selectedGeo.get();
}

const std::vector<osg::ref_ptr<Geo3D>>& OSGWidget::getAllGeos() const
{
    return m_geoList;
}

// ========================================= 拾取系统实现 =========================================

PickResult OSGWidget::performSimplePicking(int mouseX, int mouseY)
{
    // 检查几何拾取系统是否已初始化
    if (!m_geometryPickingSystem || !m_geometryPickingSystem->isInitialized()) {
        LOG_WARNING("几何拾取系统未初始化，返回空拾取结果", "拾取");
        return PickResult();
    }
    
    // 使用几何拾取系统进行拾取
    PickResult result = m_geometryPickingSystem->pickGeometry(mouseX, mouseY);
    
    // 如果有拾取结果且有拾取指示器，更新指示器位置
    if (result.hasResult && m_pickingIndicator) {
        m_pickingIndicator->showIndicator(result.worldPosition, result.featureType, result.surfaceNormal);
    } else if (m_pickingIndicator) {
        m_pickingIndicator->hideIndicator();
    }
    
    return result;
}

// ========================================= 右键菜单槽函数实现 =========================================

void OSGWidget::onSetCameraPosition()
{
    if (!m_cameraController) {
        LOG_ERROR("相机控制器未初始化", "右键菜单");
        return;
    }
    
    // 获取当前相机位置
    osg::Vec3 currentEye, currentCenter, currentUp;
    m_cameraController->getViewMatrixAsLookAt(currentEye, currentCenter, currentUp);
    
    bool ok;
    QString posText = QInputDialog::getText(this, "设置相机位置", 
        QString("请输入相机位置 (x,y,z):\n当前位置: (%1, %2, %3)")
            .arg(currentEye.x(), 0, 'f', 3)
            .arg(currentEye.y(), 0, 'f', 3)
            .arg(currentEye.z(), 0, 'f', 3),
        QLineEdit::Normal, 
        QString("%1,%2,%3")
            .arg(currentEye.x(), 0, 'f', 3)
            .arg(currentEye.y(), 0, 'f', 3)
            .arg(currentEye.z(), 0, 'f', 3), 
        &ok);
    
    if (!ok || posText.isEmpty()) {
        return;
    }
    
    // 解析坐标
    QStringList coords = posText.split(',');
    if (coords.size() != 3) {
        LOG_ERROR("坐标格式错误，请使用 x,y,z 格式", "右键菜单");
        return;
    }
    
    bool parseOk = true;
    osg::Vec3d newEye(
        coords[0].trimmed().toDouble(&parseOk),
        coords[1].trimmed().toDouble(&parseOk),
        coords[2].trimmed().toDouble(&parseOk)
    );
    
    if (!parseOk) {
        LOG_ERROR("坐标解析失败，请输入有效的数字", "右键菜单");
        return;
    }
    
    // 设置新的相机位置
    m_cameraController->setPosition(newEye, osg::Vec3d(currentCenter.x(), currentCenter.y(), currentCenter.z()), 
                                   osg::Vec3d(currentUp.x(), currentUp.y(), currentUp.z()));
    
    LOG_SUCCESS(QString("相机位置已设置为 (%1, %2, %3)")
        .arg(newEye.x(), 0, 'f', 3)
        .arg(newEye.y(), 0, 'f', 3)
        .arg(newEye.z(), 0, 'f', 3), "右键菜单");
}





