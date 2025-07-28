#include "OSGWidget.h"
#include "../core/Common3D.h"
#include "../core/GeometryBase.h"
#include "../core/picking/PickingIndicator.h"
#include "../util/LogManager.h"
#include "../util/GeometryFactory.h"
#include "../util/VertexShapeUtils.h"

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>
#include <osg/Node>
#include <osg/Group>
#include <osg/Material>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/PolygonMode>
#include <osg/StateSet>
#include <osg/LightSource>
#include <osg/Light>
#include <osg/Math>
#include <osg/BlendFunc>
#include <osg/Multisample>
#include <osg/ComputeBoundsVisitor>
#include <osgQOpenGL/osgQOpenGLWidget>
#include <QTimer>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QApplication>
#include <QDateTime>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>

OSGWidget::OSGWidget(QWidget* parent)
    : osgQOpenGLWidget(parent)
    , m_rootNode(new osg::Group)
    , m_sceneNode(new osg::Group)
    , m_geoNode(new osg::Group)
    , m_lightNode(new osg::Group)
    , m_pickingIndicatorNode(new osg::Group)
    , m_cameraController(std::make_unique<CameraController>())
    , m_currentDrawingGeo(nullptr)
    , m_selectedGeo(nullptr)
    , m_isDrawing(false)
    , m_lastMouseWorldPos(0.0)
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
    setFocus();
    
    connect(m_updateTimer, &QTimer::timeout, this, [this]() { update(); });
    connect(this, &osgQOpenGLWidget::initialized, this, &OSGWidget::initializeScene);
    
    // 设置全局相机控制器，以便VertexShapeUtils可以获取真实的相机方向
    VertexShapeUtils::setCameraController(m_cameraController.get());
    
    m_updateTimer->start(16);
    LOG_INFO("OSGWidget初始化完成", "系统");
}

OSGWidget::~OSGWidget()
{
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    
    if (m_geometryPickingSystem) {
        m_geometryPickingSystem->shutdown();
        m_geometryPickingSystem = nullptr;
    }
    
    // 清理全局相机控制器引用
    VertexShapeUtils::setCameraController(nullptr);
}

void OSGWidget::showContextMenu(const QPoint& pos)
{
    if (!pos.isNull()) {
        m_lastContextMenuPos = pos;
    }
    
    // 使用屏幕坐标进行拾取
    PickResult pickResult = performSimplePicking(pos.x(), height() - pos.y());
    
    QMenu contextMenu(this);
    
    if (pickResult.hasResult && pickResult.geometry) {
        m_contextMenuGeo = pickResult.geometry;
        m_contextMenuPointIndex = pickResult.primitiveIndex;
        
        QAction* deleteAction = contextMenu.addAction("删除选中对象");
        connect(deleteAction, &QAction::triggered, this, &OSGWidget::onDeleteSelectedObjects);
        
        contextMenu.addSeparator();
        
        if (pickResult.featureType == PickFeatureType::VERTEX && pickResult.primitiveIndex >= 0) {
            QAction* movePointAction = contextMenu.addAction("移动点到坐标...");
            connect(movePointAction, &QAction::triggered, this, &OSGWidget::onMovePointToCoordinate);
        }
        
        QAction* centerObjectAction = contextMenu.addAction("将对象居中显示");
        connect(centerObjectAction, &QAction::triggered, this, &OSGWidget::onCenterObjectToView);
    } else {
        m_contextMenuGeo = nullptr;
        m_contextMenuPointIndex = -1;
        
        if (!m_selectedGeos.empty()) {
            QAction* deleteSelectedAction = contextMenu.addAction(QString("删除选中对象 (%1个)").arg(m_selectedGeos.size()));
            connect(deleteSelectedAction, &QAction::triggered, this, &OSGWidget::onDeleteSelectedObjects);
            contextMenu.addSeparator();
        }
    }
    
    QMenu* viewMenu = contextMenu.addMenu("视图");
    QAction* setCameraAction = viewMenu->addAction("设置眼点坐标...");
    connect(setCameraAction, &QAction::triggered, this, &OSGWidget::onSetEyePosition);
    
    QAction* resetCameraAction = viewMenu->addAction("重置相机");
    connect(resetCameraAction, &QAction::triggered, this, &OSGWidget::onResetCamera);
    
    QAction* fitAllAction = viewMenu->addAction("适应窗口");
    connect(fitAllAction, &QAction::triggered, this, &OSGWidget::onFitAll);
    
    if (!contextMenu.actions().isEmpty()) {
        // 将局部坐标转换为全局坐标
        QPoint globalPos = mapToGlobal(pos);
        contextMenu.exec(globalPos);
    }
    
    LOG_INFO("显示右键菜单", "右键菜单");
}

void OSGWidget::onDeleteSelectedObjects()
{
    if (m_selectedGeos.empty() && !m_contextMenuGeo) {
        LOG_WARNING("没有选中的对象可删除", "右键菜单");
        return;
    }
    
    int objectCount = 0;
    
    if (m_contextMenuGeo) {
        removeGeo(m_contextMenuGeo);
        objectCount = 1;
        LOG_INFO("删除右键点击的对象", "右键菜单");
    }
    
    if (!m_selectedGeos.empty()) {
        std::vector<osg::ref_ptr<Geo3D>> objectsToDelete = m_selectedGeos;
        
        for (osg::ref_ptr<Geo3D> geo : objectsToDelete) {
            if (geo) {
                removeGeo(geo);
                objectCount++;
            }
        }
        
        clearSelection();
        LOG_INFO(QString("删除 %1 个选中对象").arg(objectsToDelete.size()), "右键菜单");
    }
    
    m_contextMenuGeo = nullptr;
    m_contextMenuPointIndex = -1;
    
    QMessageBox::information(this, "删除完成", QString("已删除 %1 个对象").arg(objectCount));
}

void OSGWidget::onSetEyePosition()
{
    if (!m_cameraController) {
        LOG_ERROR("摄像机控制器不可用", "右键菜单");
        return;
    }
    
    osg::Vec3 currentEye, currentCenter, currentUp;
    m_cameraController->getViewMatrixAsLookAt(currentEye, currentCenter, currentUp);
    
    bool ok;
    QString eyeText = QInputDialog::getText(this, "设置眼点坐标", 
        QString("请输入眼点坐标 (x,y,z):\n当前位置: (%1, %2, %3)")
            .arg(currentEye.x(), 0, 'f', 3)
            .arg(currentEye.y(), 0, 'f', 3)
            .arg(currentEye.z(), 0, 'f', 3),
        QLineEdit::Normal,
        QString("%1,%2,%3").arg(currentEye.x(), 0, 'f', 3).arg(currentEye.y(), 0, 'f', 3).arg(currentEye.z(), 0, 'f', 3),
        &ok);
    
    if (!ok || eyeText.isEmpty()) return;
    
    QStringList coords = eyeText.split(',');
    if (coords.size() != 3) {
        QMessageBox::warning(this, "输入错误", "请输入有效的坐标格式: x,y,z");
        return;
    }
    
    bool xOk, yOk, zOk;
    double x = coords[0].trimmed().toDouble(&xOk);
    double y = coords[1].trimmed().toDouble(&yOk);
    double z = coords[2].trimmed().toDouble(&zOk);
    
    if (!xOk || !yOk || !zOk) {
        QMessageBox::warning(this, "输入错误", "请输入有效的数值坐标");
        return;
    }
    
    QString targetText = QInputDialog::getText(this, "设置目标点坐标", 
        QString("请输入目标点坐标 (x,y,z):\n当前目标: (%1, %2, %3)")
            .arg(currentCenter.x(), 0, 'f', 3)
            .arg(currentCenter.y(), 0, 'f', 3)
            .arg(currentCenter.z(), 0, 'f', 3),
        QLineEdit::Normal,
        QString("%1,%2,%3").arg(currentCenter.x(), 0, 'f', 3).arg(currentCenter.y(), 0, 'f', 3).arg(currentCenter.z(), 0, 'f', 3),
        &ok);
    
    if (!ok || targetText.isEmpty()) return;
    
    QStringList targetCoords = targetText.split(',');
    if (targetCoords.size() != 3) {
        QMessageBox::warning(this, "输入错误", "请输入有效的目标点坐标格式: x,y,z");
        return;
    }
    
    bool txOk, tyOk, tzOk;
    double tx = targetCoords[0].trimmed().toDouble(&txOk);
    double ty = targetCoords[1].trimmed().toDouble(&tyOk);
    double tz = targetCoords[2].trimmed().toDouble(&tzOk);
    
    if (!txOk || !tyOk || !tzOk) {
        QMessageBox::warning(this, "输入错误", "请输入有效的目标点数值坐标");
        return;
    }
    
    setCameraPosition(glm::dvec3(x, y, z), glm::dvec3(tx, ty, tz));
    
    LOG_INFO(QString("设置眼点坐标: (%1, %2, %3) -> (%4, %5, %6)")
        .arg(x, 0, 'f', 3).arg(y, 0, 'f', 3).arg(z, 0, 'f', 3)
        .arg(tx, 0, 'f', 3).arg(ty, 0, 'f', 3).arg(tz, 0, 'f', 3), "右键菜单");
}

void OSGWidget::onMovePointToCoordinate()
{
    if (!m_contextMenuGeo || m_contextMenuPointIndex < 0) {
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
    
    QStringList coords = coordText.split(',');
    if (coords.size() != 3) {
        QMessageBox::warning(this, "输入错误", "请输入有效的坐标格式: x,y,z");
        return;
    }
    
    bool xOk, yOk, zOk;
    double x = coords[0].trimmed().toDouble(&xOk);
    double y = coords[1].trimmed().toDouble(&yOk);
    double z = coords[2].trimmed().toDouble(&zOk);
    
    if (!xOk || !yOk || !zOk) {
        QMessageBox::warning(this, "输入错误", "请输入有效的数值坐标");
        return;
    }
    
    movePointToCoordinate(m_contextMenuGeo, m_contextMenuPointIndex, glm::dvec3(x, y, z));
    
    LOG_INFO(QString("移动控制点到新坐标: (%1, %2, %3)")
        .arg(x, 0, 'f', 3).arg(y, 0, 'f', 3).arg(z, 0, 'f', 3), "右键菜单");
}

void OSGWidget::onCenterObjectToView()
{
    if (!m_contextMenuGeo) {
        LOG_WARNING("没有选中的对象", "右键菜单");
        return;
    }
    
    osg::ComputeBoundsVisitor visitor;
    m_contextMenuGeo->mm_node()->getOSGNode()->accept(visitor);
    osg::BoundingBox boundingBox = visitor.getBoundingBox();
    
    if (!boundingBox.valid()) {
        LOG_WARNING("对象包围盒无效", "右键菜单");
        return;
    }
    
    osg::Vec3 center = boundingBox.center();
    double radius = boundingBox.radius();
    double distance = radius * 2.5;
    
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

void OSGWidget::deleteSelectedObjects()
{
    onDeleteSelectedObjects();
}

void OSGWidget::setCameraPosition(const glm::dvec3& position, const glm::dvec3& target)
{
    if (!m_cameraController) {
        LOG_ERROR("摄像机控制器不可用", "相机");
        return;
    }
    
    osg::Vec3d eye(position.x, position.y, position.z);
    osg::Vec3d center(target.x, target.y, target.z);
    osg::Vec3d up(0, 0, 1);
    
    m_cameraController->setPosition(eye, center, up);
    
    LOG_INFO(QString("设置相机位置: 眼点(%1,%2,%3) 目标(%4,%5,%6)")
        .arg(position.x, 0, 'f', 3).arg(position.y, 0, 'f', 3).arg(position.z, 0, 'f', 3)
        .arg(target.x, 0, 'f', 3).arg(target.y, 0, 'f', 3).arg(target.z, 0, 'f', 3), "相机");
}

void OSGWidget::movePointToCoordinate(osg::ref_ptr<Geo3D> geo, int pointIndex, const glm::dvec3& newPosition)
{
    if (!geo || pointIndex < 0) {
        LOG_ERROR("无效的几何体或点索引", "点移动");
        return;
    }
    
    auto controlPointManager = geo->mm_controlPoint();
    if (!controlPointManager) {
        LOG_ERROR("几何体没有控制点管理器", "点移动");
        return;
    }
    
    Point3D newPoint(newPosition.x, newPosition.y, newPosition.z);
    controlPointManager->setControlPoint(pointIndex, newPoint);
}

void OSGWidget::initializeScene()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    m_rootNode->addChild(m_sceneNode);
    m_rootNode->addChild(m_lightNode);
    m_rootNode->addChild(m_pickingIndicatorNode);
    m_sceneNode->addChild(m_geoNode);
    
    m_pickingIndicatorNode->setNodeMask(NODE_MASK_PICKING_INDICATOR);
    
    // 简化全局渲染设置，只保留最基础的功能，避免干扰osgb文件的原始渲染状态
    osg::StateSet* rootStateSet = m_rootNode->getOrCreateStateSet();
    
    // 必要的基础设置（不使用OVERRIDE，避免覆盖模型原始状态）
    rootStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    rootStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    rootStateSet->setMode(GL_MULTISAMPLE, osg::StateAttribute::ON);
    
    // 全局透明度混合设置（不使用OVERRIDE）
    osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc();
    blendFunc->setSource(osg::BlendFunc::SRC_ALPHA);
    blendFunc->setDestination(osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    rootStateSet->setAttributeAndModes(blendFunc.get(), osg::StateAttribute::ON);
    
    // 移除所有几何体节点的全局渲染设置，让每个几何体或文件保持原始状态
    // (不再设置geoStateSet的任何强制属性)
    
    viewer->setSceneData(m_rootNode);
    
    m_cameraController->setViewer(viewer);
    
    setupCamera();
    setupLighting();
    setupEventHandlers();
    setupPickingSystem();
    
    // 配置TrackballManipulator的鼠标按键绑定
    configureCameraManipulator();
}

void OSGWidget::configureCameraManipulator()
{
    if (!m_cameraController) return;
    
    osgGA::TrackballManipulator* trackball = m_cameraController->getTrackballManipulator();
    if (trackball) {
        // OSG TrackballManipulator默认行为：
        // 左键：旋转
        // 中键：平移  
        // 右键：缩放
        // 
        // 我们通过事件转换实现的自定义映射：
        // 左键：旋转
        // 右键：平移（转换为中键事件）
        // 中键：缩放
        LOG_INFO("TrackballManipulator配置：左键旋转，右键平移，中键缩放", "相机");
    }
}

void OSGWidget::setupCamera()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    osg::Camera* camera = viewer->getCamera();
    
    osg::StateSet* stateSet = camera->getOrCreateStateSet();
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    
    camera->setClearColor(osg::Vec4(0.4, 0.6, 0.9, 1.0));
    
    resetCamera();
}

void OSGWidget::setupLighting()
{
    // 主太阳光 - 增强亮度和温暖色调
    osg::ref_ptr<osg::Light> sunLight = new osg::Light();
    sunLight->setLightNum(0);
    sunLight->setPosition(osg::Vec4(0.0, 0.0, 800.0, 1.0));
    sunLight->setDirection(osg::Vec3(0.0, 0.0, -1.0));
    sunLight->setAmbient(osg::Vec4(0.7, 0.7, 0.8, 1.0));    // 增强环境光
    sunLight->setDiffuse(osg::Vec4(1.2, 1.1, 1.0, 1.0));    // 增强漫反射，略偏暖
    sunLight->setSpecular(osg::Vec4(1.0, 1.0, 1.0, 1.0));
    
    osg::ref_ptr<osg::LightSource> sunLightSource = new osg::LightSource();
    sunLightSource->setLight(sunLight.get());
    sunLightSource->setLocalStateSetModes(osg::StateAttribute::ON);
    
    // 天空散射光 - 大幅增强亮度
    osg::ref_ptr<osg::Light> skyLight = new osg::Light();
    skyLight->setLightNum(1);
    skyLight->setPosition(osg::Vec4(-300.0, -300.0, 500.0, 1.0));
    skyLight->setDirection(osg::Vec3(0.6, 0.6, -0.8));
    skyLight->setAmbient(osg::Vec4(0.5, 0.6, 0.7, 1.0));    // 大幅增强环境光
    skyLight->setDiffuse(osg::Vec4(0.8, 0.9, 1.0, 1.0));    // 大幅增强漫反射
    skyLight->setSpecular(osg::Vec4(0.4, 0.5, 0.6, 1.0));   // 增强镜面反射
    
    osg::ref_ptr<osg::LightSource> skyLightSource = new osg::LightSource();
    skyLightSource->setLight(skyLight.get());
    skyLightSource->setLocalStateSetModes(osg::StateAttribute::ON);
    
    // 添加第三个补光源 - 从另一侧提供补光
    osg::ref_ptr<osg::Light> fillLight = new osg::Light();
    fillLight->setLightNum(2);
    fillLight->setPosition(osg::Vec4(400.0, 400.0, 300.0, 1.0));
    fillLight->setDirection(osg::Vec3(-0.5, -0.5, -0.6));
    fillLight->setAmbient(osg::Vec4(0.3, 0.3, 0.4, 1.0));   // 柔和的环境光
    fillLight->setDiffuse(osg::Vec4(0.6, 0.7, 0.8, 1.0));   // 补充漫反射
    fillLight->setSpecular(osg::Vec4(0.2, 0.2, 0.3, 1.0));
    
    osg::ref_ptr<osg::LightSource> fillLightSource = new osg::LightSource();
    fillLightSource->setLight(fillLight.get());
    fillLightSource->setLocalStateSetModes(osg::StateAttribute::ON);
    
    m_lightNode->addChild(sunLightSource.get());
    m_lightNode->addChild(skyLightSource.get());
    m_lightNode->addChild(fillLightSource.get());
    
    osg::StateSet* stateSet = m_rootNode->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHT0, osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHT1, osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHT2, osg::StateAttribute::ON);   // 启用第三个光源
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    
    LOG_INFO("增强光照系统设置完成", "光照");
}

void OSGWidget::setupEventHandlers()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    viewer->addEventHandler(new osgViewer::StatsHandler());
    viewer->addEventHandler(new osgViewer::WindowSizeHandler());
    viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
}

void OSGWidget::setupPickingSystem()
{
    LOG_INFO("开始设置拾取系统", "拾取");
    
    m_pickingIndicator = new PickingIndicator;
    if (!m_pickingIndicator->initialize()) {
        LOG_ERROR("拾取指示器初始化失败", "拾取");
        return;
    }
    
    osg::Group* indicatorRoot = m_pickingIndicator->getIndicatorRoot();
    if (indicatorRoot) {
        m_pickingIndicatorNode->addChild(indicatorRoot);
        LOG_INFO("拾取指示器已添加到场景", "拾取");
    }
    
    m_geometryPickingSystem = new GeometryPickingSystem();
    
    PickConfig pickConfig;
    pickConfig.cylinderRadius = m_pickingIndicator->getConfig().pickingPixelRadius;
    m_geometryPickingSystem->setConfig(pickConfig);
    
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) {
        LOG_WARNING("OSG查看器为空，延迟初始化几何拾取系统", "拾取");
        QTimer::singleShot(100, this, [this]() {
            if (getOsgViewer() && getOsgViewer()->getCamera()) {
                if (m_geometryPickingSystem && !m_geometryPickingSystem->isInitialized()) {
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
        QTimer::singleShot(100, this, [this]() {
            if (getOsgViewer() && getOsgViewer()->getCamera()) {
                if (m_geometryPickingSystem && !m_geometryPickingSystem->isInitialized()) {
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

bool OSGWidget::isHouseGeometry(osg::ref_ptr<Geo3D> geo)
{
    if (!geo) return false;
    
    // 检查几何体类型是否为房屋类型
    GeoType3D geoType = geo->getGeoType();
    return geoType == Geo_FlatHouse3D ||
           geoType == Geo_DomeHouse3D ||
           geoType == Geo_SpireHouse3D ||
           geoType == Geo_GableHouse3D ||
           geoType == Geo_LHouse3D;
}

void OSGWidget::setupHouseRenderingState(osg::Node* node)
{
    if (!node) return;
    
    osg::StateSet* stateSet = node->getOrCreateStateSet();
    
    // 关闭深度测试，让房屋显示在最上方
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    
    // 设置渲染顺序为最高优先级（后渲染，显示在上方）
    stateSet->setRenderBinDetails(1000, "RenderBin");
    
    // 可选：关闭深度写入，避免遮挡其他透明物体
    stateSet->setMode(GL_DEPTH_WRITEMASK, osg::StateAttribute::OFF);
    
    // 确保混合功能开启，支持透明效果
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    
    LOG_INFO("房屋几何体渲染状态设置完成：关闭深度测试，设置最高渲染优先级", "渲染");
}

void OSGWidget::resetCamera()
{
    if (!m_cameraController) return;
    
    osg::BoundingSphere bs = m_geoNode->getBound();
    
    if (bs.valid() && bs.radius() > 0)
    {
        osg::Vec3d center = bs.center();
        double radius = bs.radius();
        double distance = radius * 2.0;
        
        osg::Vec3d eye = center + osg::Vec3d(distance, distance, distance);
        m_cameraController->setPosition(eye, center, osg::Vec3d(0, 0, 1));
    }
    else
    {
        osg::Vec3d center(0, 0, 0);
        osg::Vec3d eye(10, 10, 10);
        m_cameraController->setPosition(eye, center, osg::Vec3d(0, 0, 1));
    }
}

void OSGWidget::fitAll()
{
    if (!m_cameraController || !m_geoNode.valid()) return;
    
    osg::BoundingSphere bs = m_geoNode->getBound();
    
    if (bs.valid() && bs.radius() > 0)
    {
        osg::Vec3d center = bs.center();
        double radius = bs.radius();
        double distance = radius * 2.5;
        
        osg::Vec3d eye = center + osg::Vec3d(distance, distance, distance);
        m_cameraController->setPosition(eye, center, osg::Vec3d(0, 0, 1));
    }
    else
    {
        osg::Vec3d center(0, 0, 0);
        osg::Vec3d eye(10, 10, 10);
        m_cameraController->setPosition(eye, center, osg::Vec3d(0, 0, 1));
    }
}

void OSGWidget::setViewDirection(const glm::dvec3& direction, const glm::dvec3& up)
{
    if (!m_cameraController) return;
    
    osg::BoundingSphere bs = m_geoNode->getBound();
    
    osg::Vec3d center;
    double distance;
    
    if (bs.valid() && bs.radius() > 0)
    {
        center = bs.center();
        distance = bs.radius() * 3.0;
    }
    else
    {
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
        
        osg::ref_ptr<osg::Group> geoOSGNode = geo->mm_node()->getOSGNode();
        if (geoOSGNode.valid()) {
            m_geoNode->addChild(geoOSGNode.get());
            
            // 检查是否为房屋几何体，如果是则设置特殊渲染状态
            if (isHouseGeometry(geo)) {
                setupHouseRenderingState(geoOSGNode.get());
                LOG_INFO(QString("为房屋几何体设置关闭深度测试: %1").arg(geo->getGeoType()), "OSGWidget");
            }
        } else {
        }
        
        // 连接几何体状态无效信号
        if (geo->mm_state()) {
            connect(geo->mm_state(), &GeoStateManager::stateInvalidated, 
                    this, &OSGWidget::onGeoStateInvalidated);
        }
    }
}

void OSGWidget::onGeoStateInvalidated()
{
    // 获取发送信号的GeoStateManager
    GeoStateManager* stateManager = qobject_cast<GeoStateManager*>(sender());
    if (!stateManager) {
        LOG_WARNING("无法获取状态管理器", "几何体状态");
        return;
    }
    
    // 查找对应的几何体并移除
    for (auto it = m_geoList.begin(); it != m_geoList.end(); ++it) {
        osg::ref_ptr<Geo3D> geo = *it;
        if (geo && geo->mm_state() == stateManager) {
            LOG_INFO(QString("移除状态无效的几何体: %1").arg(geo->getGeoType()), "几何体状态");
            
            // 从OSG场景图中移除
            if (geo->mm_node() && geo->mm_node()->getOSGNode()) {
                m_geoNode->removeChild(geo->mm_node()->getOSGNode().get());
            }
            
            // 从选择列表中移除
            removeFromSelection(geo);
            
            // 如果是当前选中的几何体，清除选择
            if (m_selectedGeo == geo) {
                m_selectedGeo = nullptr;
            }
            
            // 如果是当前绘制的几何体，停止绘制
            if (m_currentDrawingGeo == geo) {
                m_currentDrawingGeo = nullptr;
                m_isDrawing = false;
            }
            
            // 从列表中移除
            m_geoList.erase(it);
            
            LOG_SUCCESS("已自动移除状态无效的几何体", "几何体状态");
            break;
        }
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
            // 断开状态信号连接
            if (geo->mm_state()) {
                disconnect(geo->mm_state(), &GeoStateManager::stateInvalidated, 
                          this, &OSGWidget::onGeoStateInvalidated);
            }
            
            m_geoNode->removeChild(geo->mm_node()->getOSGNode().get());
            m_geoList.erase(it);
        }
    }
}

void OSGWidget::removeAllGeos()
{
    if (m_geoNode.valid())
    {
        // 断开所有几何体的状态信号连接
        for (auto& geo : m_geoList) {
            if (geo && geo->mm_state()) {
                disconnect(geo->mm_state(), &GeoStateManager::stateInvalidated, 
                          this, &OSGWidget::onGeoStateInvalidated);
            }
        }
        
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

void OSGWidget::addToSelection(osg::ref_ptr<Geo3D> geo)
{
    if (!geo) return;
    
    LOG_INFO(QString("尝试添加到选择: 对象类型=%1").arg(geo->getGeoType()), "选择");
    
    auto it = std::find(m_selectedGeos.begin(), m_selectedGeos.end(), geo);
    if (it == m_selectedGeos.end())
    {
        LOG_INFO(QString("对象不在选择列表中，开始添加"), "选择");
        
        m_selectedGeos.push_back(geo);
        geo->mm_state()->setStateSelected();
        
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
        geo->mm_state()->clearStateSelected();
        
        if (m_selectedGeo.get() == geo)
        {
            m_selectedGeo = nullptr;
        }
        
        emit geoSelected(nullptr);
        
        LOG_INFO(QString("从选择中移除: 对象类型=%1, 剩余选择数=%2")
            .arg(geo->getGeoType())
            .arg(m_selectedGeos.size()), "选择");
    }
}

void OSGWidget::clearSelection()
{
    for (auto& geo : m_selectedGeos)
    {
        if (geo)
        {
            geo->mm_state()->clearStateSelected();
        }
    }
    
    m_selectedGeos.clear();
    m_selectedGeo = nullptr;
    
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
    }
    
    m_isDraggingControlPoint = false;
    m_draggingGeo = nullptr;
    m_draggingControlPointIndex = -1;
}

void OSGWidget::updateSelectionHighlight()
{
    LOG_INFO("updateSelectionHighlight被调用，但暂时禁用", "选择");
    return;
    
}

void OSGWidget::highlightSelectedObjects()
{
    
}

void OSGWidget::onSimplePickingResult(const PickResult& result)
{
    emit simplePickingResult(result);
    
    if (result.hasResult) {
        m_lastMouseWorldPos = result.worldPosition;
        emit mousePositionChanged(result.worldPosition);
    }
}



glm::dvec3 OSGWidget::screenToWorld(int x, int y, double depth)
{
    if (!m_cameraController) return glm::dvec3(0, 0, 0);
    
    QPoint currentPos(x, y);
    if (m_mousePosCacheValid && 
        m_lastMouseScreenPos == currentPos &&
        m_lastMouseCalculation.isValid() &&
        m_lastMouseCalculation.msecsTo(QDateTime::currentDateTime()) < MOUSE_CACHE_DURATION)
    {
        return m_cachedMouseWorldPos;
    }
    
    osg::Vec3d worldPoint = m_cameraController->screenToWorld(x, y, depth, width(), height());
    glm::dvec3 result(worldPoint.x(), worldPoint.y(), worldPoint.z());
    
    m_lastMouseScreenPos = currentPos;
    m_cachedMouseWorldPos = result;
    m_mousePosCacheValid = true;
    m_lastMouseCalculation = QDateTime::currentDateTime();
    
    return result;
}

glm::dvec2 OSGWidget::worldToScreen(const glm::dvec3& worldPos)
{
    if (!m_cameraController) return glm::dvec2(0, 0);
    
    osg::Vec2d screenPoint = m_cameraController->worldToScreen(osg::Vec3d(worldPos.x, worldPos.y, worldPos.z), width(), height());
    return glm::dvec2(screenPoint.x(), screenPoint.y());
}

void OSGWidget::paintEvent(QPaintEvent* event)
{
    QOpenGLWidget::paintEvent(event);
    
    if (m_cameraController && m_cameraController->isMoving()) {
        m_cameraController->updateCameraPosition();
    }
}

void OSGWidget::resizeEvent(QResizeEvent* event)
{
    osgQOpenGLWidget::resizeEvent(event);
    
    if (m_cameraController)
    {
        m_cameraController->updateProjectionMatrix(width(), height());
    }
}

void OSGWidget::mousePressEvent(QMouseEvent* event)
{
    // 中键点击显示菜单（无论什么模式）
    if (event->button() == Qt::MiddleButton) {
        showContextMenu(event->pos());
        event->accept();
        return;
    }

    // 绘制模式下的右键处理
    if (event->button() == Qt::RightButton) {
        // 如果按住Ctrl，转换为中键传递给OSG进行相机控制
        if (event->modifiers() & Qt::ControlModifier) {
            QMouseEvent* newEvent = new QMouseEvent(
                event->type(),
                event->pos(),
                event->globalPos(),
                Qt::MiddleButton,  // 转换为中键
                event->buttons() & ~Qt::RightButton | Qt::MiddleButton,  // 更新按键状态
                event->modifiers()
            );
            osgQOpenGLWidget::mousePressEvent(newEvent);
            delete newEvent;
            return;
        }
        
        // 否则处理绘制逻辑：进入下一阶段
        if (m_currentDrawingGeo) {
            auto controlPointManager = m_currentDrawingGeo->mm_controlPoint();
            if (controlPointManager) {
                bool hasNextStage = controlPointManager->nextStage();
                if (!hasNextStage) {
                    completeCurrentDrawing();
                } else {
                    LOG_INFO("进入下一阶段", "绘制");
                }
            } else {
                completeCurrentDrawing();
            }
        } else {
            completeCurrentDrawing();
        }
        event->accept();
        return;
    }

    // 选择模式下，左键和右键都传递给OSG进行相机控制
    if (GlobalDrawMode3D == DrawSelect3D) {
        // 先处理拾取和选择逻辑（仅左键且不按Ctrl）
        if (event->button() == Qt::LeftButton && !(event->modifiers() & Qt::ControlModifier)) {
            PickResult pickResult = performSimplePicking(event->x(), height() - event->y());
            glm::dvec3 worldPos;
            
            if (pickResult.hasResult) { 
                worldPos = pickResult.worldPosition; 
            } else { 
                worldPos = screenToWorld(event->x(), event->y(), 0.5);
            }

            emit mousePositionChanged(worldPos);

            if (pickResult.hasResult && pickResult.geometry) {
                osg::ref_ptr<Geo3D> pickedGeo = pickResult.geometry;

                if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
                    if (isSelected(pickedGeo)) {
                        removeFromSelection(pickedGeo);
                    } else {
                        addToSelection(pickedGeo);
                    }
                } else {
                    clearSelection();
                    addToSelection(pickedGeo);
                }
            } else {
                clearSelection();
            }

            // 检查控制点拖拽
            if (pickResult.hasResult && pickResult.geometry && pickResult.osgGeometry) {
                if (pickResult.featureType == PickFeatureType::VERTEX && 
                    pickResult.osgGeometry->getNodeMask() == NODE_MASK_CONTROL_POINTS && 
                    pickResult.primitiveIndex >= 0) {
                    auto it = std::find(m_selectedGeos.begin(), m_selectedGeos.end(), pickResult.geometry);
                    if (it != m_selectedGeos.end()) {
                        startDraggingControlPoint(pickResult.geometry, pickResult.primitiveIndex);
                        event->accept();
                        return;
                    }
                }
            }
            
            // 不按Ctrl的左键选择完成后不传递给OSG
            event->accept();
            return;
        }
        
        // 按住Ctrl或其他按键时，传递给OSG进行相机控制
        if (event->modifiers() & Qt::ControlModifier) {
            // 如果是右键，转换为中键事件来实现右键平移
            if (event->button() == Qt::RightButton) {
                QMouseEvent* newEvent = new QMouseEvent(
                    event->type(),
                    event->pos(),
                    event->globalPos(),
                    Qt::MiddleButton,  // 转换为中键
                    event->buttons() & ~Qt::RightButton | Qt::MiddleButton,  // 更新按键状态
                    event->modifiers()
                );
                osgQOpenGLWidget::mousePressEvent(newEvent);
                delete newEvent;
            } else {
                osgQOpenGLWidget::mousePressEvent(event);
            }
            return;
        }
        
        // 其他情况（如右键不按Ctrl）接受事件但不传递给OSG
        event->accept();
        return;
    }

    // 绘制模式
    if (event->button() == Qt::LeftButton) {
        // 如果按住Ctrl，传递给OSG进行相机控制
        if (event->modifiers() & Qt::ControlModifier) {
            osgQOpenGLWidget::mousePressEvent(event);
            return;
        }
        
        // 否则处理绘制逻辑
        PickResult pickResult = performSimplePicking(event->x(), height() - event->y());
        glm::dvec3 worldPos;
        
        if (pickResult.hasResult) { 
            worldPos = pickResult.worldPosition; 
        } else { 
            worldPos = screenToWorld(event->x(), event->y(), 0.5);
        }

        emit mousePositionChanged(worldPos);

        if (!m_isDrawing) {
            osg::ref_ptr<Geo3D> newGeo = GeometryFactory::createGeometry(GlobalDrawMode3D);
            if (newGeo) {
                m_currentDrawingGeo = newGeo;
                m_isDrawing = true;
                addGeo(newGeo);
                LOG_INFO("开始绘制...", "绘制");
            }
        }

        if (m_currentDrawingGeo) {
            auto controlPointManager = m_currentDrawingGeo->mm_controlPoint();
            if (controlPointManager) {
                bool success = controlPointManager->addControlPoint(Point3D(worldPos));
                if (success) {
                    LOG_INFO("添加控制点成功", "绘制");
                    
                    if (m_currentDrawingGeo->mm_state()->isStateComplete()) {
                        completeCurrentDrawing();
                    }
                }
            }
        }
        
        event->accept();
        return;
    }
    
    // 其他情况传递给OSG
    osgQOpenGLWidget::mousePressEvent(event);
}

void OSGWidget::mouseMoveEvent(QMouseEvent* event)
{
    // 选择模式下，只有按住Ctrl时才传递给OSG进行相机控制
    if (GlobalDrawMode3D == DrawSelect3D) {
        if (event->modifiers() & Qt::ControlModifier) {
            // 如果是右键拖动，转换为中键拖动来实现平移
            if (event->buttons() & Qt::RightButton) {
                QMouseEvent* newEvent = new QMouseEvent(
                    event->type(),
                    event->pos(),
                    event->globalPos(),
                    Qt::MiddleButton,  // 转换为中键
                    event->buttons() & ~Qt::RightButton | Qt::MiddleButton,  // 更新按键状态
                    event->modifiers()
                );
                osgQOpenGLWidget::mouseMoveEvent(newEvent);
                delete newEvent;
            } else {
                osgQOpenGLWidget::mouseMoveEvent(event);
            }
        }
    } else {
        // 绘制模式下，只有按住Ctrl时才传递给OSG进行相机控制
        if (event->modifiers() & Qt::ControlModifier) {
            // 如果是右键拖动，转换为中键拖动来实现平移
            if (event->buttons() & Qt::RightButton) {
                QMouseEvent* newEvent = new QMouseEvent(
                    event->type(),
                    event->pos(),
                    event->globalPos(),
                    Qt::MiddleButton,  // 转换为中键
                    event->buttons() & ~Qt::RightButton | Qt::MiddleButton,  // 更新按键状态
                    event->modifiers()
                );
                osgQOpenGLWidget::mouseMoveEvent(newEvent);
                delete newEvent;
            } else {
                osgQOpenGLWidget::mouseMoveEvent(event);
            }
        }
    }
    
    emit screenPositionChanged(event->x(), event->y());
    
    static QDateTime lastMouseUpdate;
    QDateTime currentTime = QDateTime::currentDateTime();
    if (!lastMouseUpdate.isValid() || lastMouseUpdate.msecsTo(currentTime) >= MOUSE_CACHE_DURATION) {
        PickResult pickResult = performSimplePicking(event->x(), height() - event->y());
        
        if (pickResult.hasResult) {
            m_lastMouseWorldPos = pickResult.worldPosition;
            emit mousePositionChanged(pickResult.worldPosition);
            emit simplePickingResult(pickResult);
        } else {
            glm::dvec3 worldPos = screenToWorld(event->x(), event->y(), 0.5);
            
            m_lastMouseWorldPos = worldPos;
            emit mousePositionChanged(worldPos);
        }
        
        lastMouseUpdate = currentTime;
    }
    
    if (m_isDraggingControlPoint && m_draggingGeo && m_draggingControlPointIndex >= 0) {
        Point3D newPoint(m_lastMouseWorldPos.x, m_lastMouseWorldPos.y, m_lastMouseWorldPos.z);
        m_draggingGeo->mm_controlPoint()->setControlPoint(m_draggingControlPointIndex, newPoint);
    }
    
    if (m_isDrawing && m_currentDrawingGeo) {
        PickResult pickResult = performSimplePicking(event->x(), height() - event->y());
        glm::dvec3 drawingWorldPos;
        
        if (pickResult.hasResult) {
            drawingWorldPos = pickResult.worldPosition;
            m_lastMouseWorldPos = pickResult.worldPosition;
            emit mousePositionChanged(pickResult.worldPosition);
        } else {
            drawingWorldPos = m_lastMouseWorldPos;
        }
        
        updateCurrentDrawing(drawingWorldPos);
    }
}

void OSGWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_isDraggingControlPoint)
    {
        stopDraggingControlPoint();
        event->accept();
        return;
    }
    
    // 选择模式下，只有按住Ctrl时才传递给OSG
    if (GlobalDrawMode3D == DrawSelect3D) {
        if (event->modifiers() & Qt::ControlModifier) {
            // 如果是右键释放，转换为中键释放事件
            if (event->button() == Qt::RightButton) {
                QMouseEvent* newEvent = new QMouseEvent(
                    event->type(),
                    event->pos(),
                    event->globalPos(),
                    Qt::MiddleButton,  // 转换为中键
                    event->buttons() & ~Qt::RightButton | Qt::MiddleButton,  // 更新按键状态
                    event->modifiers()
                );
                osgQOpenGLWidget::mouseReleaseEvent(newEvent);
                delete newEvent;
            } else {
                osgQOpenGLWidget::mouseReleaseEvent(event);
            }
        } else {
            event->accept();
        }
        return;
    }
    
    // 绘制模式下，只有按住Ctrl时才传递给OSG
    if (event->modifiers() & Qt::ControlModifier) {
        // 如果是右键释放，转换为中键释放事件
        if (event->button() == Qt::RightButton) {
            QMouseEvent* newEvent = new QMouseEvent(
                event->type(),
                event->pos(),
                event->globalPos(),
                Qt::MiddleButton,  // 转换为中键
                event->buttons() & ~Qt::RightButton | Qt::MiddleButton,  // 更新按键状态
                event->modifiers()
            );
            osgQOpenGLWidget::mouseReleaseEvent(newEvent);
            delete newEvent;
        } else {
            osgQOpenGLWidget::mouseReleaseEvent(event);
        }
        return;
    }
    
    // 其他情况接受事件但不传递给OSG
    event->accept();
}

void OSGWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (GlobalDrawMode3D != DrawSelect3D || event->button() != Qt::LeftButton) {
        osgQOpenGLWidget::mouseDoubleClickEvent(event);
        return;
    }
    
    PickResult pickResult = performSimplePicking(event->x(), height() - event->y());
    glm::dvec3 worldPos;
    
    if (pickResult.hasResult) {
        worldPos = pickResult.worldPosition;
        LOG_INFO(QString("双击拾取到几何对象，位置: (%1, %2, %3)")
                 .arg(worldPos.x, 0, 'f', 2)
                 .arg(worldPos.y, 0, 'f', 2)
                 .arg(worldPos.z, 0, 'f', 2), "相机");
    } else {
        worldPos = screenToWorld(event->x(), event->y(), 0.5);
        
        LOG_INFO(QString("双击空白区域，转换世界坐标: (%1, %2, %3)")
                 .arg(worldPos.x, 0, 'f', 2)
                 .arg(worldPos.y, 0, 'f', 2)
                 .arg(worldPos.z, 0, 'f', 2), "相机");
    }
    
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
    if (event->modifiers() & Qt::ControlModifier)
    {
        int delta = event->angleDelta().y();
        
        if (m_cameraController)
        {
            m_cameraController->handleWheelZoom(delta);
        }
        
        event->accept();
    }
    else
    {
        int delta = event->angleDelta().y();
        
        osgQOpenGLWidget::wheelEvent(event);
    }
}

void OSGWidget::updateCurrentDrawing(const glm::dvec3& worldPos)
{
    if (m_currentDrawingGeo)
    {
        auto controlPointManager = m_currentDrawingGeo->mm_controlPoint();
        if (controlPointManager)
        {
            controlPointManager->setTempPoint(Point3D(worldPos));
        }
    }
}

void OSGWidget::completeCurrentDrawing()
{
    if (m_currentDrawingGeo)
    {
        m_currentDrawingGeo->mm_state()->setStateComplete();
        m_currentDrawingGeo = nullptr;
        m_isDrawing = false;
        
        LOG_SUCCESS("绘制完成", "绘制");
        
        setFocus();
    }
}

void OSGWidget::cancelCurrentDrawing()
{
    if (m_currentDrawingGeo)
    {
        removeGeo(m_currentDrawingGeo.get());
        
        m_currentDrawingGeo = nullptr;
        m_isDrawing = false;
        
        LOG_WARNING("取消绘制", "绘制");
        
        setFocus();
    }
}

void OSGWidget::setDrawMode(DrawMode3D mode)
{
    if (m_isDrawing)
    {
        cancelCurrentDrawing();
    }
    
    if (mode == DrawSelect3D)
    {
        deselectAll();
    }
    
    GlobalDrawMode3D = mode;
    
    if (mode == DrawSelect3D)
    {
        LOG_INFO("切换到选择模式", "模式");
    }
    else
    {
        LOG_INFO(tr("切换到绘制模式: %1").arg(drawMode3DToString(mode)), "模式");
    }
}

void OSGWidget::keyPressEvent(QKeyEvent* event)
{
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
            m_cameraController->setKeyPressed(event->key(), true);
            break;
        default:
            osgQOpenGLWidget::keyPressEvent(event);
            
            if (m_isDrawing && m_currentDrawingGeo)
            {
                auto controlPointManager = m_currentDrawingGeo->mm_controlPoint();
                if (controlPointManager)
                {
                    if (event->key() == Qt::Key_Escape)
                    {
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
                }
            }
            
            if (GlobalDrawMode3D == DrawSelect3D && event->key() == Qt::Key_Delete)
            {
                if (m_selectedGeo)
                {
                    removeGeo(m_selectedGeo.get());
                    
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
            m_cameraController->setKeyPressed(event->key(), false);
            break;
        default:
            osgQOpenGLWidget::keyReleaseEvent(event);
            
        }
    
    event->accept();
}

osg::ref_ptr<Geo3D> OSGWidget::getSelectedGeo() const
{
    return m_selectedGeo.get();
}

const std::vector<osg::ref_ptr<Geo3D>>& OSGWidget::getAllGeos() const
{
    return m_geoList;
}

PickResult OSGWidget::performSimplePicking(int mouseX, int mouseY)
{
    if (!m_geometryPickingSystem || !m_geometryPickingSystem->isInitialized()) {
        LOG_WARNING("几何拾取系统未初始化，返回空拾取结果", "拾取");
        return PickResult();
    }
    
    PickResult result = m_geometryPickingSystem->pickGeometry(mouseX, mouseY);
    
    if (result.hasResult && m_pickingIndicator) {
        m_pickingIndicator->showIndicator(result.worldPosition, result.featureType, result.surfaceNormal);
    } else if (m_pickingIndicator) {
        m_pickingIndicator->hideIndicator();
    }
    
    return result;
}

void OSGWidget::onSetCameraPosition()
{
    if (!m_cameraController) {
        LOG_ERROR("相机控制器未初始化", "右键菜单");
        return;
    }
    
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
    
    m_cameraController->setPosition(newEye, osg::Vec3d(currentCenter.x(), currentCenter.y(), currentCenter.z()), 
                                   osg::Vec3d(currentUp.x(), currentUp.y(), currentUp.z()));
    
    LOG_SUCCESS(QString("相机位置已设置为 (%1, %2, %3)")
        .arg(newEye.x(), 0, 'f', 3)
        .arg(newEye.y(), 0, 'f', 3)
        .arg(newEye.z(), 0, 'f', 3), "右键菜单");
}
