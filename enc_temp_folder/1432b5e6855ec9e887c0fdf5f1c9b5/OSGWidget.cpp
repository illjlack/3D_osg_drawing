#include "OSGWidget.h"
#include "../core/Common3D.h"
#include "../core/GeometryBase.h"
#include "../core/world/CoordinateSystem3D.h"
#include "../util/LogManager.h"
#include "../util/GeometryFactory.h"

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osg/ComputeBoundsVisitor>
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
#include <QDateTime>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>

// ========================================= OSGWidget 窗口控制 - 事件处理控制中心 =========================================

OSGWidget::OSGWidget(QWidget* parent)
    : osgQOpenGLWidget(parent)
    , m_sceneManager(std::make_unique<SceneManager3D>())
    , m_cameraController(std::make_unique<CameraController>())
    , m_lastDrawMode(DrawPoint3D)
    , m_lastMouseWorldPos(0.0)
    , m_shouldPassMouseToOSG(true)
    , m_mousePosCacheValid(false)
    , m_updateTimer(new QTimer(this))
    , m_contextMenuGeo(nullptr)
    , m_contextMenuPointIndex(-1)
{
    // 设置窗口属性 - 确保能接收键盘和鼠标事件
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setFocus();
    
    // 初始化按键状态数组
    memset(m_keyPressed, false, sizeof(m_keyPressed));
    
    // 连接渲染循环
    connect(m_updateTimer, &QTimer::timeout, this, [this]() {
        update();
    });
    
    // 连接初始化完成信号
    connect(this, &osgQOpenGLWidget::initialized, this, &OSGWidget::initializeScene);
    
    // 启动渲染循环 (60 FPS)
    m_updateTimer->start(16);
    
    // 发送初始相机速度
    emit cameraSpeedChanged(m_initialSpeed);
    
    LOG_INFO("OSGWidget窗口控制初始化完成 - 统一窗口控制", "窗口控制");
}

OSGWidget::~OSGWidget()
{
    if (m_updateTimer)
    {
        m_updateTimer->stop();
    }
    
    LOG_INFO("OSGWidget窗口控制销毁", "窗口控制");
}

// ========================================= 初始化与配置 =========================================

void OSGWidget::initializeScene()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) {
        LOG_ERROR("无法获取OSG查看器", "窗口控制");
        return;
    }

    // 初始化场景管理器
    if (!m_sceneManager->initializeScene(viewer)) {
        LOG_ERROR("场景管理器初始化失败", "窗口控制");
        return;
    }
    
    // 设置相机控制器
    m_cameraController->setViewer(viewer);
    
    // 设置基本的相机参数
    setupCamera();
    
    // 配置事件处理
    setupEventHandlers();
    
    LOG_SUCCESS("OSGWidget场景初始化完成", "窗口控制");
}

void OSGWidget::setupCamera()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    // 设置清除颜色（浅灰色背景）
    osg::Camera* camera = viewer->getCamera();
    if (camera) {
        camera->setClearColor(osg::Vec4(0.9f, 0.9f, 0.9f, 1.0f));
        
        // 设置视口
        camera->setViewport(0, 0, width(), height());
        
        // 设置投影矩阵（透视投影）
        double aspectRatio = static_cast<double>(width()) / static_cast<double>(height());
        camera->setProjectionMatrixAsPerspective(45.0, aspectRatio, 0.001, 100000.0);
        
        // 设置视图矩阵
        camera->setViewMatrixAsLookAt(
            osg::Vec3d(10, 10, 10),  // 眼睛位置
            osg::Vec3d(0, 0, 0),     // 观察点
            osg::Vec3d(0, 0, 1)      // 上方向
        );
        
        LOG_INFO(QString("相机设置: 视口(%1x%2), 投影角度45度, 位置(10,10,10)").arg(width()).arg(height()), "相机");
    }
    
    // 同时设置相机控制器
    if (m_cameraController) {
        osg::Vec3d center(0, 0, 0);
        osg::Vec3d eye(10, 10, 10);
        m_cameraController->setPosition(eye, center, osg::Vec3d(0, 0, 1));
    }
}

void OSGWidget::setupEventHandlers()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    // 保留OSG的相机操控器，用于相机控制
    if (m_cameraController) {
        m_cameraController->setViewer(viewer);
    }
    
    LOG_INFO("启用混合事件控制：鼠标→OSG，键盘→自定义", "窗口控制");
}

// ========================================= 渲染控制 =========================================

void OSGWidget::paintEvent(QPaintEvent* event)
{
    // 调用基类的绘制
    osgQOpenGLWidget::paintEvent(event);
    
    // 确保OSG渲染循环运行
    osgViewer::Viewer* viewer = getOsgViewer();
    if (viewer) {
        viewer->frame();
    }
}

void OSGWidget::resizeEvent(QResizeEvent* event)
{
    // 调用基类的大小调整
    osgQOpenGLWidget::resizeEvent(event);
    
    // 更新相机视口和投影矩阵
    osgViewer::Viewer* viewer = getOsgViewer();
    if (viewer) {
        osg::Camera* camera = viewer->getCamera();
        if (camera) {
            // 更新视口
            camera->setViewport(0, 0, width(), height());
            
            // 更新投影矩阵
            double aspectRatio = static_cast<double>(width()) / static_cast<double>(height());
            camera->setProjectionMatrixAsPerspective(45.0, aspectRatio, 0.001, 10000.0);
            
            LOG_INFO(QString("窗口大小改变，更新相机视口: %1x%2").arg(width()).arg(height()), "相机");
        }
    }
    
    // 使缓存无效
    m_mousePosCacheValid = false;
}

// ========================================= 鼠标事件控制 - 处理后传给OSG =========================================

void OSGWidget::mousePressEvent(QMouseEvent* event)
{
    // 1. 首先进行自定义处理
    handleMousePress(event);
    
    // 2. 根据状态决定是否传递给OSG进行相机控制
    if (m_shouldPassMouseToOSG) {
        osgQOpenGLWidget::mousePressEvent(event);
    }
}

void OSGWidget::mouseMoveEvent(QMouseEvent* event)
{
    // 1. 首先进行自定义处理
    handleMouseMove(event);
    
    // 2. 根据状态决定是否传递给OSG进行相机控制
    if (m_shouldPassMouseToOSG) {
        osgQOpenGLWidget::mouseMoveEvent(event);
    }
}

void OSGWidget::mouseReleaseEvent(QMouseEvent* event)
{
    // 1. 首先进行自定义处理
    handleMouseRelease(event);
    
    // 2. 根据状态决定是否传递给OSG进行相机控制
    if (m_shouldPassMouseToOSG) {
        osgQOpenGLWidget::mouseReleaseEvent(event);
    }
}

void OSGWidget::wheelEvent(QWheelEvent* event)
{
    // 使缓存无效
    m_mousePosCacheValid = false;
    
    // 直接传递给OSG处理缩放
    osgQOpenGLWidget::wheelEvent(event);
}

void OSGWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!m_sceneManager) return;
    
    if (event->button() == Qt::LeftButton && GlobalDrawMode3D != DrawSelect3D)
    {
        // 双击完成绘制
        if (m_sceneManager->isDrawing())
        {
            completeCurrentDrawing();
        }
    }
    
    // 双击事件不传递给OSG，避免干扰
}

// ========================================= 鼠标事件处理实现 =========================================

void OSGWidget::handleMousePress(QMouseEvent* event)
{
    if (!m_sceneManager || !m_cameraController) return;
    
    // 获取鼠标世界坐标
    glm::dvec3 worldPos = screenToWorld(event->x(), event->y(), 0.0);
    m_lastMouseWorldPos = worldPos;
    
    // 发送屏幕位置信号
    emit screenPositionChanged(event->x(), event->y());
    
    if (event->button() == Qt::LeftButton)
    {
        if (GlobalDrawMode3D == DrawSelect3D)
        {
            // 选择模式：执行拾取
            PickResult result = m_sceneManager->performPicking(event->x(), event->y());
            onSimplePickingResult(result);
            // 选择模式保持传递给OSG，除非开始拖动控制点
        }
        else
        {
            // 绘制模式：开始或继续绘制
            if (!m_sceneManager->isDrawing())
            {
                m_sceneManager->startDrawing(GlobalDrawMode3D);
                LOG_INFO(QString("开始绘制: %1").arg(drawMode3DToString(GlobalDrawMode3D)), "窗口控制");
            }
            
            // 左键点击添加控制点
            if (m_sceneManager->getCurrentDrawingGeometry())
            {
                auto geo = m_sceneManager->getCurrentDrawingGeometry();
                auto controlPointManager = geo->mm_controlPoint();
                if (controlPointManager)
                {
                    Point3D point(worldPos.x, worldPos.y, worldPos.z);
                    if (controlPointManager->addControlPoint(point))
                    {
                        LOG_INFO(QString("添加控制点: (%1,%2,%3)")
                            .arg(worldPos.x, 0, 'f', 3)
                            .arg(worldPos.y, 0, 'f', 3)
                            .arg(worldPos.z, 0, 'f', 3), "窗口控制");
                        
                        // 检查是否可以自动完成绘制
                        if (geo->mm_state() && geo->mm_state()->isStateComplete())
                        {
                            completeCurrentDrawing();
                        }
                    }
                    else
                    {
                        LOG_INFO(QString("添加控制点失败: (%1,%2,%3)")
                            .arg(worldPos.x, 0, 'f', 3)
                            .arg(worldPos.y, 0, 'f', 3)
                            .arg(worldPos.z, 0, 'f', 3), "窗口控制");

                        //非法就取消绘制
                        if (geo->mm_state() && geo->mm_state()->isStateInvalid())
                        {
                            // 如果添加失败，则取消绘制
                            m_sceneManager->cancelDrawing();
                        }
                    }
                    
                }
            }
        }
    }
    else if (event->button() == Qt::MiddleButton)
    {
        // 中键：开始拖动操作
        m_cameraController->startPan(event->x(), event->y());
        // 中键操作时确保传递给OSG进行相机控制
        setMousePassToOSG(true);
    }
    
    LOG_INFO(QString("鼠标按下处理: 按键=%1, 世界坐标=(%2,%3,%4), 传递状态=%5")
        .arg(event->button())
        .arg(worldPos.x, 0, 'f', 3)
        .arg(worldPos.y, 0, 'f', 3)
        .arg(worldPos.z, 0, 'f', 3)
        .arg(m_shouldPassMouseToOSG ? "传递" : "不传递"), "窗口控制");
}

void OSGWidget::handleMouseMove(QMouseEvent* event)
{
    if (!m_sceneManager || !m_cameraController) return;
    
    // 获取鼠标世界坐标
    glm::dvec3 worldPos = screenToWorld(event->x(), event->y(), 0.0);
    m_lastMouseWorldPos = worldPos;
    
    // 发送位置更新信号
    emit mousePositionChanged(worldPos);
    emit screenPositionChanged(event->x(), event->y());
    
    // 始终执行拾取，不管什么模式
    PickResult result = m_sceneManager->performPicking(event->x(), event->y());
    emit simplePickingResult(result);
    
    // 处理按键状态下的操作
    if (event->buttons() & Qt::LeftButton)
    {
        if (GlobalDrawMode3D != DrawSelect3D && m_sceneManager->isDrawing())
        {
            // 绘制模式：更新绘制预览（不添加控制点）
            m_sceneManager->updateDrawingPreview(worldPos);
        }
        else if (m_sceneManager->isDraggingControlPoint())
        {
            // 拖动控制点
            m_sceneManager->updateDraggingControlPoint(worldPos);
        }
    }
    else
    {
        // 鼠标移动时（没有按键）：更新绘制预览
        if (GlobalDrawMode3D != DrawSelect3D && m_sceneManager->isDrawing())
        {
            m_sceneManager->updateDrawingPreview(worldPos);
        }
    }
}

void OSGWidget::handleMouseRelease(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (m_sceneManager && m_sceneManager->isDraggingControlPoint())
        {
            m_sceneManager->stopDraggingControlPoint();
            setMousePassToOSG(true);  // 停止拖动控制点时恢复传递
            LOG_INFO("停止拖动控制点", "窗口控制");
        }
    }
}

// ========================================= 键盘事件控制 - 完全自定义处理，不传给OSG =========================================

void OSGWidget::keyPressEvent(QKeyEvent* event)
{
    // 完全自定义处理，不传递给OSG
    handleKeyPress(event);
    
    // 不调用基类的keyPressEvent，实现完全自定义控制
}

void OSGWidget::keyReleaseEvent(QKeyEvent* event)
{
    // 完全自定义处理，不传递给OSG
    handleKeyRelease(event);
    
    // 不调用基类的keyReleaseEvent，实现完全自定义控制
}

// ========================================= 键盘事件处理实现 =========================================

void OSGWidget::handleKeyPress(QKeyEvent* event)
{
    if (!m_sceneManager && !m_cameraController) return;
    
    bool handled = false;
    
    // 相机移动控制键 - WSAD + Up/Down
    if (m_cameraController) {
        double moveDistance = m_initialSpeed;
        
        if (event->key() == Qt::Key_W || event->key() == Qt::Key_S
            || event->key() == Qt::Key_A || event->key() == Qt::Key_D
            || event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
        {
            // 标记按键被按下
            m_keyPressed[event->key()] = true;
            
            if(m_speedCounter++ > m_maxCount) m_speedCounter = m_maxCount;
            moveDistance = m_initialSpeed + m_acceleration * m_speedCounter;
            
            // 通知状态栏当前相机速度
            emit cameraSpeedChanged(moveDistance);
        }

        switch (event->key()) {
        case Qt::Key_W:
            // 前进 (沿视线方向)
            m_cameraController->moveForward(moveDistance);
            handled = true;
            break;
        case Qt::Key_S:
            // 后退 (沿视线方向)
            m_cameraController->moveBackward(moveDistance);
            handled = true;
            break;
        case Qt::Key_A:
            // 左移 (垂直于视线方向)
            if (!(event->modifiers() & Qt::ControlModifier)) { 
                m_cameraController->moveLeft(moveDistance);
                handled = true;
            }
            break;
        case Qt::Key_D:
            // 右移 (垂直于视线方向)
            m_cameraController->moveRight(moveDistance);
            handled = true;
            break;
        case Qt::Key_Up:
            // 上升 (垂直向上)
            m_cameraController->moveUp(moveDistance);
            handled = true;
            break;
        case Qt::Key_Down:
            // 下降 (垂直向下)
            m_cameraController->moveDown(moveDistance);
            handled = true;
            break;
        case Qt::Key_R:
            // 重置相机
            m_cameraController->resetCamera();
            LOG_INFO("键盘控制: 重置相机", "窗口控制");
            handled = true;
            break;
        case Qt::Key_F:
            // 适应全部
            m_cameraController->fitAll();
            LOG_INFO("键盘控制: 适应全部", "窗口控制");
            handled = true;
            break;
        case Qt::Key_Home:
            // 返回初始位置
            onResetCamera();
            LOG_INFO("键盘控制: 返回初始位置", "窗口控制");
            handled = true;
            break;
        }
    }
    
    // 绘制和选择控制键
    if (!handled && m_sceneManager) {
        switch (event->key())
        {
        case Qt::Key_Escape:
            // 取消当前绘制或选择
            if (m_sceneManager->isDrawing())
            {
                cancelCurrentDrawing();
                LOG_INFO("键盘控制: 取消绘制", "窗口控制");
            }
            else if (m_sceneManager->isDraggingControlPoint())
            {
                m_sceneManager->stopDraggingControlPoint();
                setMousePassToOSG(true);  // 停止拖动时恢复传递
                LOG_INFO("键盘控制: 停止拖动控制点", "窗口控制");
            }
            else if (!m_sceneManager->getSelectedGeometries().empty())
            {
                m_sceneManager->clearSelection();
                emit geoSelected(osg::ref_ptr<Geo3D>());
                setMousePassToOSG(true); 
                LOG_INFO("键盘控制: 清空选择", "窗口控制");
            }
            handled = true;
            break;
            
        case Qt::Key_Enter:
        case Qt::Key_Return:
            // 完成当前绘制
            if (m_sceneManager->isDrawing())
            {
                completeCurrentDrawing();
                LOG_INFO("键盘控制: 完成绘制", "窗口控制");
            }
            handled = true;
            break;
            
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            // 删除选中的对象
            onDeleteSelectedObjects();
            LOG_INFO("键盘控制: 删除选中对象", "窗口控制");
            handled = true;
            break;
            
        case Qt::Key_Tab:
            // 切换到上一个绘制模式
            if (GlobalDrawMode3D == DrawSelect3D && m_lastDrawMode != DrawSelect3D)
            {
                GlobalDrawMode3D = m_lastDrawMode;
                emit drawModeChanged(GlobalDrawMode3D);
                LOG_INFO(QString("键盘控制: 切换到绘制模式 %1").arg(drawMode3DToString(GlobalDrawMode3D)), "窗口控制");
            }
            handled = true;
            break;
            
        case Qt::Key_Space:
            // 空格键切换选择模式
            if (GlobalDrawMode3D != DrawSelect3D)
            {
                m_lastDrawMode = GlobalDrawMode3D;
                GlobalDrawMode3D = DrawSelect3D;
                emit drawModeChanged(GlobalDrawMode3D);
                LOG_INFO("键盘控制: 切换到选择模式", "窗口控制");
            }
            handled = true;
            break;
        }
    }
    
    // 通用快捷键
    if (!handled) {
        switch (event->key()) {
        case Qt::Key_A:
            if (event->modifiers() & Qt::ControlModifier) {
                // Ctrl+A: 全选
                if (m_sceneManager) {
                    const auto& allGeos = m_sceneManager->getAllGeometries();
                    for (const auto& geo : allGeos) {
                        m_sceneManager->addToSelection(geo);
                    }
                    LOG_INFO(QString("键盘控制: 全选 %1 个对象").arg(allGeos.size()), "窗口控制");
                }
                handled = true;
            }
            break;
            
        case Qt::Key_S:
            if (event->modifiers() & Qt::ControlModifier) {
                // Ctrl+S: 保存（预留）
                LOG_INFO("键盘控制: 保存快捷键（功能待实现）", "窗口控制");
                handled = true;
            }
            break;
        }
    }
    
    if (!handled) {
        LOG_INFO(QString("键盘控制: 未处理的按键 %1").arg(event->key()), "窗口控制");
    }
}

void OSGWidget::handleKeyRelease(QKeyEvent* event)
{
    // 取消按键标记
    m_keyPressed[event->key()] = false;
    
    // 检查是否还有移动键被按下
    bool hasMovementKeys = m_keyPressed[Qt::Key_W] || m_keyPressed[Qt::Key_S] ||
                          m_keyPressed[Qt::Key_A] || m_keyPressed[Qt::Key_D] ||
                          m_keyPressed[Qt::Key_Up] || m_keyPressed[Qt::Key_Down];
    
    // 如果没有移动键被按下，则重置速度计数器
    if (!hasMovementKeys)
    {
        m_speedCounter = 0;
        // 通知状态栏速度重置
        emit cameraSpeedChanged(m_initialSpeed);
    }
}

// ========================================= 右键菜单控制 =========================================

void OSGWidget::contextMenuEvent(QContextMenuEvent* event)
{
    if (!m_sceneManager) return;
    
    // 记录右键菜单位置
    m_lastContextMenuPos = event->pos();
    
    // 执行拾取
    PickResult result = m_sceneManager->performPicking(event->x(), event->y());
    m_contextMenuGeo = result.geometry;
    m_contextMenuPointIndex = result.primitiveIndex;
    
    // 创建右键菜单
    QMenu contextMenu(this);
    
    if (result.geometry)
    {
        // 有选中的几何体
        QAction* deleteAction = contextMenu.addAction("删除对象");
        connect(deleteAction, &QAction::triggered, this, &OSGWidget::onDeleteSelectedObjects);
        
        QAction* movePointAction = contextMenu.addAction("移动点到坐标");
        connect(movePointAction, &QAction::triggered, this, &OSGWidget::onMovePointToCoordinate);
        
        contextMenu.addSeparator();
    }
    
    // 相机操作
    QAction* setCameraAction = contextMenu.addAction("设置相机位置");
    connect(setCameraAction, &QAction::triggered, this, &OSGWidget::onSetCameraPosition);
    
    QAction* setEyeAction = contextMenu.addAction("设置视点位置");
    connect(setEyeAction, &QAction::triggered, this, &OSGWidget::onSetEyePosition);
    
    contextMenu.addSeparator();
    
    QAction* resetCameraAction = contextMenu.addAction("重置相机");
    connect(resetCameraAction, &QAction::triggered, this, &OSGWidget::onResetCamera);
    
    QAction* fitAllAction = contextMenu.addAction("适应全部");
    connect(fitAllAction, &QAction::triggered, this, &OSGWidget::onFitAll);
    
    QAction* centerViewAction = contextMenu.addAction("居中显示");
    connect(centerViewAction, &QAction::triggered, this, &OSGWidget::onCenterObjectToView);
    
    // 显示菜单
    contextMenu.exec(event->globalPos());
    
    LOG_INFO("显示右键菜单", "窗口控制");
}

// ========================================= 辅助处理函数 =========================================

void OSGWidget::setMousePassToOSG(bool shouldPass)
{
    if (m_shouldPassMouseToOSG != shouldPass) {
        m_shouldPassMouseToOSG = shouldPass;
        LOG_INFO(QString("设置鼠标事件传递状态: %1").arg(shouldPass ? "传递给OSG" : "不传递"), "窗口控制");
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
    
    // 委托给相机控制器
    osg::Vec3d worldPoint = m_cameraController->screenToWorld(x, y, depth, width(), height());
    glm::dvec3 result(worldPoint.x(), worldPoint.y(), worldPoint.z());
    
    // 更新缓存
    m_lastMouseScreenPos = currentPos;
    m_cachedMouseWorldPos = result;
    m_mousePosCacheValid = true;
    m_lastMouseCalculation = QDateTime::currentDateTime();
    
    return result;
}

void OSGWidget::completeCurrentDrawing()
{
    if (!m_sceneManager || !m_sceneManager->isDrawing()) return;
    
    osg::ref_ptr<Geo3D> completedGeo = m_sceneManager->completeDrawing();
    setMousePassToOSG(true);  // 完成绘制时恢复传递
    
    if (completedGeo.valid())
    {
        LOG_INFO(QString("完成绘制: %1").arg(geoType3DToString(completedGeo->getGeoType())), "窗口控制");
        emit geoSelected(completedGeo);
    }
}

void OSGWidget::cancelCurrentDrawing()
{
    if (!m_sceneManager || !m_sceneManager->isDrawing()) return;
    
    m_sceneManager->cancelDrawing();
    setMousePassToOSG(true);  // 取消绘制时恢复传递
    LOG_INFO("取消当前绘制", "窗口控制");
}

void OSGWidget::onSimplePickingResult(const PickResult& result)
{
    emit simplePickingResult(result);
    
    if (result.hasResult && result.geometry)
    {
        // 检查是否点击了控制点
        if (result.featureType == PickFeatureType::VERTEX && result.primitiveIndex >= 0)
        {
            // 开始拖动控制点
            m_sceneManager->startDraggingControlPoint(result.geometry, result.primitiveIndex);
            setMousePassToOSG(false);  // 拖动控制点时不传递鼠标事件
            LOG_INFO(QString("开始拖动控制点: 几何体=%1, 索引=%2")
                .arg(geoType3DToString(result.geometry->getGeoType()))
                .arg(result.primitiveIndex), "窗口控制");
        }
        else
        {
            // 选择几何体
            bool isCtrlPressed = QApplication::keyboardModifiers() & Qt::ControlModifier;
            
            if (isCtrlPressed)
            {
                // Ctrl+点击：多选模式
                if (m_sceneManager->isSelected(result.geometry))
                {
                    m_sceneManager->removeFromSelection(result.geometry);
                }
                else
                {
                    m_sceneManager->addToSelection(result.geometry);
                }
            }
            else
            {
                // 普通点击：单选模式
                m_sceneManager->setSelectedGeometry(result.geometry);
            }
            
            emit geoSelected(result.geometry);
        }
        
        LOG_INFO(QString("拾取几何体: %1 at (%2,%3,%4)")
            .arg(geoType3DToString(result.geometry->getGeoType()))
            .arg(result.worldPosition.x, 0, 'f', 3)
            .arg(result.worldPosition.y, 0, 'f', 3)
            .arg(result.worldPosition.z, 0, 'f', 3), "窗口控制");
    }
    else
    {
        // 点击空白区域
        bool isCtrlPressed = QApplication::keyboardModifiers() & Qt::ControlModifier;
        if (!isCtrlPressed)
        {
            // 没有按Ctrl，清空选择
            m_sceneManager->clearSelection();
            emit geoSelected(osg::ref_ptr<Geo3D>());
            
            // 如果当前没有在绘制或拖动，确保恢复鼠标事件传递
            if (!m_sceneManager->isDrawing() && !m_sceneManager->isDraggingControlPoint())
            {
                setMousePassToOSG(true);
            }
        }
    }
}

// ========================================= 右键菜单槽函数 =========================================

void OSGWidget::onDeleteSelectedObjects()
{
    if (!m_sceneManager) return;
    
    const auto& selectedGeos = m_sceneManager->getSelectedGeometries();
    if (selectedGeos.empty()) return;
    
    int reply = QMessageBox::question(this, "删除确认", 
        QString("确定要删除 %1 个选中的对象吗？").arg(selectedGeos.size()),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes)
    {
        for (auto& geo : selectedGeos)
        {
            m_sceneManager->removeGeometry(geo);
            LOG_INFO(QString("删除几何体: %1").arg(geoType3DToString(geo->getGeoType())), "窗口控制");
        }
        
        m_sceneManager->clearSelection();
        emit geoSelected(osg::ref_ptr<Geo3D>());
    }
}

void OSGWidget::onSetCameraPosition()
{
    if (!m_cameraController) return;
    
    // 获取当前鼠标世界坐标作为目标点
    glm::dvec3 targetPos = screenToWorld(m_lastContextMenuPos.x(), m_lastContextMenuPos.y(), 0.0);
    
    bool ok;
    QString posStr = QInputDialog::getText(this, "设置相机位置", 
        "输入相机位置 (x,y,z):", QLineEdit::Normal,
        QString("10,10,10"), &ok);
    
    if (ok && !posStr.isEmpty())
    {
        QStringList coords = posStr.split(',');
        if (coords.size() == 3)
        {
            bool okX, okY, okZ;
            double x = coords[0].trimmed().toDouble(&okX);
            double y = coords[1].trimmed().toDouble(&okY);
            double z = coords[2].trimmed().toDouble(&okZ);
            
            if (okX && okY && okZ)
            {
                osg::Vec3d eye(x, y, z);
                osg::Vec3d center(targetPos.x, targetPos.y, targetPos.z);
                osg::Vec3d up(0, 0, 1);
                
                m_cameraController->setPosition(eye, center, up);
                LOG_INFO(QString("设置相机位置: (%1,%2,%3) -> (%4,%5,%6)")
                    .arg(x).arg(y).arg(z)
                    .arg(targetPos.x).arg(targetPos.y).arg(targetPos.z), "窗口控制");
            }
        }
    }
}

void OSGWidget::onMovePointToCoordinate()
{
    if (!m_contextMenuGeo || m_contextMenuPointIndex < 0) return;
    
    bool ok;
    QString posStr = QInputDialog::getText(this, "移动点到坐标", 
        "输入新坐标 (x,y,z):", QLineEdit::Normal, 
        QString("0,0,0"), &ok);
    
    if (ok && !posStr.isEmpty())
    {
        QStringList coords = posStr.split(',');
        if (coords.size() == 3)
        {
            bool okX, okY, okZ;
            double x = coords[0].trimmed().toDouble(&okX);
            double y = coords[1].trimmed().toDouble(&okY);
            double z = coords[2].trimmed().toDouble(&okZ);
            
            if (okX && okY && okZ)
            {
                auto controlPointManager = m_contextMenuGeo->mm_controlPoint();
                if (controlPointManager)
                {
                    Point3D newPoint(x, y, z);
                    controlPointManager->setControlPoint(m_contextMenuPointIndex, newPoint);
                    LOG_INFO(QString("移动控制点: %1[%2] -> (%3,%4,%5)")
                        .arg(geoType3DToString(m_contextMenuGeo->getGeoType()))
                        .arg(m_contextMenuPointIndex)
                        .arg(x).arg(y).arg(z), "窗口控制");
                }
            }
        }
    }
}

void OSGWidget::onSetEyePosition()
{
    if (!m_cameraController) return;
    
    // 获取当前鼠标世界坐标作为目标点
    glm::dvec3 targetPos = screenToWorld(m_lastContextMenuPos.x(), m_lastContextMenuPos.y(), 0.0);
    
    osg::Vec3d eye(targetPos.x, targetPos.y, targetPos.z + 10.0);
    osg::Vec3d center(targetPos.x, targetPos.y, targetPos.z);
    osg::Vec3d up(0, 0, 1);
    
    m_cameraController->setPosition(eye, center, up);
    LOG_INFO(QString("设置视点到: (%1,%2,%3)")
        .arg(targetPos.x).arg(targetPos.y).arg(targetPos.z + 10.0), "窗口控制");
}

void OSGWidget::onResetCamera()
{
    if (!m_cameraController) return;
    
    osg::Vec3d center(0, 0, 0);
    osg::Vec3d eye(10, 10, 10);
    osg::Vec3d up(0, 0, 1);
    
    m_cameraController->setPosition(eye, center, up);
    LOG_INFO("重置相机到默认位置", "窗口控制");
}

void OSGWidget::onFitAll()
{
    if (!m_cameraController || !m_sceneManager) return;
    
    // 计算所有几何体的包围盒
    const auto& geometries = m_sceneManager->getAllGeometries();
    if (geometries.empty())
    {
        onResetCamera();
        return;
    }
    
    osg::BoundingSphere combinedBounds;
    for (const auto& geo : geometries)
    {
        if (geo && geo->mm_node() && geo->mm_node()->getOSGNode())
        {
            osg::BoundingSphere geoBounds = geo->mm_node()->getOSGNode()->getBound();
            if (geoBounds.valid())
            {
                combinedBounds.expandBy(geoBounds);
            }
        }
    }
    
    if (combinedBounds.valid() && combinedBounds.radius() > 0)
    {
        osg::Vec3d center = combinedBounds.center();
        double radius = combinedBounds.radius();
        double distance = radius * 2.5; // 留一些边距
        
        osg::Vec3d eye = center + osg::Vec3d(distance, distance, distance);
        m_cameraController->setPosition(eye, center, osg::Vec3d(0, 0, 1));
        
        LOG_INFO(QString("适应全部: 中心(%1,%2,%3) 半径=%4")
            .arg(center.x()).arg(center.y()).arg(center.z()).arg(radius), "窗口控制");
    }
    else
    {
        onResetCamera();
    }
}

void OSGWidget::onCenterObjectToView()
{
    if (!m_cameraController || !m_contextMenuGeo) return;
    
    if (m_contextMenuGeo->mm_node() && m_contextMenuGeo->mm_node()->getOSGNode())
    {
        osg::BoundingSphere bounds = m_contextMenuGeo->mm_node()->getOSGNode()->getBound();
        if (bounds.valid())
        {
            osg::Vec3d center = bounds.center();
            double radius = bounds.radius();
            double distance = radius * 3.0;
            
            osg::Vec3d eye = center + osg::Vec3d(distance, distance, distance);
            m_cameraController->setPosition(eye, center, osg::Vec3d(0, 0, 1));
            
            LOG_INFO(QString("居中显示对象: %1")
                .arg(geoType3DToString(m_contextMenuGeo->getGeoType())), "窗口控制");
        }
    }
} 