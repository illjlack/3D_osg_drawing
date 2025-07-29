#include "InputInteractionMode.h"
#include "../OSGWidget.h"
#include "../../util/LogManager.h"
#include "../../util/GeometryFactory.h"
#include "../../core/picking/PickingIndicator.h"

//=============================================================================
// CameraInteractionMode - 相机控制模式
//=============================================================================

CameraInteractionMode::CameraInteractionMode(OSGWidget* widget)
    : InputInteractionMode(widget)
{
}

void CameraInteractionMode::onMousePress(QMouseEvent* event)
{
    // 相机控制模式下，所有鼠标按键都用于相机操作
    m_isDragging = true;
    m_dragButton = event->button();
    
    // 直接传递给OSG处理
    // m_widget->osgQOpenGLWidget::mousePressEvent(event);
    
    LOG_DEBUG(QString("相机控制: 开始拖拽，按键=%1").arg(event->button()), "相机交互");
}

void CameraInteractionMode::onMouseMove(QMouseEvent* event)
{
    if (m_isDragging) {
        // 根据按键类型执行不同的相机操作
        switch (m_dragButton) {
            case Qt::LeftButton:
                // 旋转
                LOG_DEBUG("相机旋转", "相机交互");
                break;
            case Qt::MiddleButton:
                // 平移
                LOG_DEBUG("相机平移", "相机交互");
                break;
            case Qt::RightButton:
                // 缩放
                LOG_DEBUG("相机缩放", "相机交互");
                break;
            default:
                break;
        }
        
        // m_widget->osgQOpenGLWidget::mouseMoveEvent(event);
    }
}

void CameraInteractionMode::onMouseRelease(QMouseEvent* event)
{
    if (event->button() == m_dragButton) {
        m_isDragging = false;
        m_dragButton = Qt::NoButton;
        
        LOG_DEBUG("相机控制: 结束拖拽", "相机交互");
    }
    
    // m_widget->osgQOpenGLWidget::mouseReleaseEvent(event);
}

void CameraInteractionMode::onMouseDoubleClick(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 双击设置相机旋转中心
        // m_widget->updateWorldPosition(event->x(), event->y());
        // 设置旋转中心到拾取点
        LOG_INFO("设置相机旋转中心", "相机交互");
    }
}

void CameraInteractionMode::onWheel(QWheelEvent* event)
{
    // 滚轮缩放
    // m_widget->osgQOpenGLWidget::wheelEvent(event);
    LOG_DEBUG("相机滚轮缩放", "相机交互");
}

void CameraInteractionMode::onKeyPress(QKeyEvent* event)
{
    // 键盘相机控制 (WASD, 方向键等)
    switch (event->key()) {
        case Qt::Key_W:
        case Qt::Key_S:
        case Qt::Key_A:
        case Qt::Key_D:
        case Qt::Key_Q:
        case Qt::Key_E:
            // m_widget->m_cameraController->setKeyPressed(event->key(), true);
            LOG_DEBUG(QString("相机键盘控制: %1").arg(event->key()), "相机交互");
            break;
        default:
            break;
    }
}

void CameraInteractionMode::onKeyRelease(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_W:
        case Qt::Key_S:
        case Qt::Key_A:
        case Qt::Key_D:
        case Qt::Key_Q:
        case Qt::Key_E:
            // m_widget->m_cameraController->setKeyPressed(event->key(), false);
            break;
        default:
            break;
    }
}

//=============================================================================
// SelectionInteractionMode - 选择交互模式
//=============================================================================

SelectionInteractionMode::SelectionInteractionMode(OSGWidget* widget)
    : InputInteractionMode(widget)
{
}

void SelectionInteractionMode::activate()
{
    LOG_INFO("激活选择交互模式", "选择交互");
    // 可以在这里设置选择模式的特殊状态
}

void SelectionInteractionMode::deactivate()
{
    if (m_isDraggingControlPoint) {
        stopControlPointDrag();
    }
    LOG_INFO("停用选择交互模式", "选择交互");
}

void SelectionInteractionMode::onMousePress(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (event->modifiers() & Qt::ShiftModifier) {
            handleMultiSelection(event);
        } else {
            handleSingleSelection(event);
        }
    } else if (event->button() == Qt::RightButton) {
        // 右键用于相机平移（转换为中键）
        QMouseEvent modifiedEvent(
            event->type(),
            event->pos(),
            event->globalPos(),
            Qt::MiddleButton,
            (event->buttons() & ~Qt::RightButton) | Qt::MiddleButton,
            event->modifiers()
        );
        // m_widget->osgQOpenGLWidget::mousePressEvent(&modifiedEvent);
        LOG_DEBUG("选择模式: 右键平移", "选择交互");
    } else if (event->button() == Qt::MiddleButton) {
        // 中键显示右键菜单
        // m_widget->showContextMenu(event->pos());
        LOG_DEBUG("选择模式: 显示右键菜单", "选择交互");
    }
}

void SelectionInteractionMode::handleSingleSelection(QMouseEvent* event)
{
    // PickResult pickResult = m_widget->performSimplePicking(event->x(), m_widget->height() - event->y());
    
    // 模拟拾取结果
    LOG_INFO("执行单选拾取", "选择交互");
    
    /*
    if (pickResult.hasResult && pickResult.geometry) {
        // 清除之前的选择
        m_widget->clearSelection();
        
        // 添加新选择
        m_widget->addToSelection(pickResult.geometry);
        
        // 检查是否需要开始控制点拖拽
        if (pickResult.featureType == PickFeatureType::VERTEX && 
            pickResult.osgGeometry && 
            pickResult.osgGeometry->getNodeMask() == NODE_MASK_CONTROL_POINTS && 
            pickResult.primitiveIndex >= 0) {
            startControlPointDrag(pickResult.geometry, pickResult.primitiveIndex);
        }
    } else {
        m_widget->clearSelection();
    }
    */
}

void SelectionInteractionMode::handleMultiSelection(QMouseEvent* event)
{
    // PickResult pickResult = m_widget->performSimplePicking(event->x(), m_widget->height() - event->y());
    
    LOG_INFO("执行多选拾取", "选择交互");
    
    /*
    if (pickResult.hasResult && pickResult.geometry) {
        if (m_widget->isSelected(pickResult.geometry)) {
            m_widget->removeFromSelection(pickResult.geometry);
        } else {
            m_widget->addToSelection(pickResult.geometry);
        }
    }
    */
}

void SelectionInteractionMode::startControlPointDrag(osg::ref_ptr<Geo3D> geo, int pointIndex)
{
    m_isDraggingControlPoint = true;
    m_draggingGeo = geo;
    m_draggingControlPointIndex = pointIndex;
    // m_dragStartPosition = m_widget->m_lastMouseWorldPos;
    
    LOG_INFO(QString("开始拖拽控制点: 几何体类型=%1, 点索引=%2")
        .arg(geo ? geo->getGeoType() : -1)
        .arg(pointIndex), "选择交互");
}

void SelectionInteractionMode::stopControlPointDrag()
{
    if (m_isDraggingControlPoint) {
        LOG_INFO("停止拖拽控制点", "选择交互");
    }
    
    m_isDraggingControlPoint = false;
    m_draggingGeo = nullptr;
    m_draggingControlPointIndex = -1;
}

void SelectionInteractionMode::onMouseMove(QMouseEvent* event)
{
    // 更新世界坐标
    // m_widget->updateWorldPosition(event->x(), event->y());
    
    if (m_isDraggingControlPoint && m_draggingGeo && m_draggingControlPointIndex >= 0) {
        // 更新控制点位置
        // Point3D newPoint(m_widget->m_lastMouseWorldPos.x, m_widget->m_lastMouseWorldPos.y, m_widget->m_lastMouseWorldPos.z);
        // m_draggingGeo->mm_controlPoint()->setControlPoint(m_draggingControlPointIndex, newPoint);
        
        LOG_DEBUG("拖拽控制点中", "选择交互");
    } else if (event->buttons() & Qt::RightButton) {
        // 右键平移
        QMouseEvent modifiedEvent(
            event->type(),
            event->pos(),
            event->globalPos(),
            Qt::MiddleButton,
            (event->buttons() & ~Qt::RightButton) | Qt::MiddleButton,
            event->modifiers()
        );
        // m_widget->osgQOpenGLWidget::mouseMoveEvent(&modifiedEvent);
    }
}

void SelectionInteractionMode::onMouseRelease(QMouseEvent* event)
{
    if (m_isDraggingControlPoint && event->button() == Qt::LeftButton) {
        stopControlPointDrag();
    } else if (event->button() == Qt::RightButton) {
        // 右键释放
        QMouseEvent modifiedEvent(
            event->type(),
            event->pos(),
            event->globalPos(),
            Qt::MiddleButton,
            (event->buttons() & ~Qt::RightButton) | Qt::MiddleButton,
            event->modifiers()
        );
        // m_widget->osgQOpenGLWidget::mouseReleaseEvent(&modifiedEvent);
    }
}

void SelectionInteractionMode::onMouseDoubleClick(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 双击设置相机旋转中心
        // m_widget->updateWorldPosition(event->x(), event->y());
        // m_widget->m_cameraController->setRotationCenter(...)
        
        LOG_INFO("双击设置相机旋转中心", "选择交互");
    }
}

void SelectionInteractionMode::onWheel(QWheelEvent* event)
{
    // 滚轮缩放
    // m_widget->osgQOpenGLWidget::wheelEvent(event);
}

void SelectionInteractionMode::onKeyPress(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_Delete:
            // 删除选中对象
            // m_widget->deleteSelectedObjects();
            LOG_INFO("删除选中对象", "选择交互");
            break;
        case Qt::Key_W:
        case Qt::Key_S:
        case Qt::Key_A:
        case Qt::Key_D:
        case Qt::Key_Q:
        case Qt::Key_E:
            // 相机控制
            // m_widget->m_cameraController->setKeyPressed(event->key(), true);
            break;
        default:
            break;
    }
}

void SelectionInteractionMode::onKeyRelease(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_W:
        case Qt::Key_S:
        case Qt::Key_A:
        case Qt::Key_D:
        case Qt::Key_Q:
        case Qt::Key_E:
            // m_widget->m_cameraController->setKeyPressed(event->key(), false);
            break;
        default:
            break;
    }
}

//=============================================================================
// DrawingInteractionMode - 绘制交互模式
//=============================================================================

DrawingInteractionMode::DrawingInteractionMode(OSGWidget* widget, DrawMode3D drawMode)
    : InputInteractionMode(widget), m_drawMode(drawMode)
{
}

void DrawingInteractionMode::activate()
{
    LOG_INFO(QString("激活绘制交互模式: %1").arg(getModeName()), "绘制交互");
    // 清除选择
    // m_widget->clearSelection();
}

void DrawingInteractionMode::deactivate()
{
    if (m_isDrawing) {
        cancelDrawing();
    }
    LOG_INFO("停用绘制交互模式", "绘制交互");
}

QString DrawingInteractionMode::getModeName() const
{
    switch (m_drawMode) {
        case DrawLine3D: return "直线绘制模式";
        case DrawCircle3D: return "圆形绘制模式";
        case DrawPolygon3D: return "多边形绘制模式";
        case DrawRect3D: return "矩形绘制模式";
        default: return "绘制模式";
    }
}

void DrawingInteractionMode::onMousePress(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (!m_isDrawing) {
            startDrawing();
        }
        
        if (m_currentDrawingGeo) {
            // 添加控制点
            // m_widget->updateWorldPosition(event->x(), event->y());
            // auto controlPointManager = m_currentDrawingGeo->mm_controlPoint();
            // bool success = controlPointManager->addControlPoint(Point3D(m_widget->m_lastMouseWorldPos));
            
            LOG_INFO("添加绘制控制点", "绘制交互");
            
            // 检查是否绘制完成
            // if (success && m_currentDrawingGeo->mm_state()->isStateComplete()) {
            //     completeDrawing();
            // }
        }
    } else if (event->button() == Qt::RightButton) {
        if (m_currentDrawingGeo) {
            // 进入下一阶段或完成绘制
            // auto controlPointManager = m_currentDrawingGeo->mm_controlPoint();
            // if (!controlPointManager->nextStage()) {
            //     completeDrawing();
            // }
            
            LOG_INFO("绘制进入下一阶段或完成", "绘制交互");
        }
    }
}

void DrawingInteractionMode::startDrawing()
{
    // osg::ref_ptr<Geo3D> newGeo = GeometryFactory::createGeometry(m_drawMode);
    // if (newGeo) {
    //     m_currentDrawingGeo = newGeo;
    //     m_isDrawing = true;
    //     m_widget->addGeo(newGeo);
    //     
    //     LOG_INFO("开始绘制新几何体", "绘制交互");
    // }
    
    m_isDrawing = true;
    LOG_INFO("开始绘制", "绘制交互");
}

void DrawingInteractionMode::onMouseMove(QMouseEvent* event)
{
    // m_widget->updateWorldPosition(event->x(), event->y());
    
    if (m_isDrawing && m_currentDrawingGeo) {
        // 更新临时点
        // updateTemporaryPoint(m_widget->m_lastMouseWorldPos);
        LOG_DEBUG("更新绘制临时点", "绘制交互");
    }
}

void DrawingInteractionMode::updateTemporaryPoint(const glm::dvec3& worldPos)
{
    if (m_currentDrawingGeo) {
        // auto controlPointManager = m_currentDrawingGeo->mm_controlPoint();
        // if (controlPointManager) {
        //     controlPointManager->setTempPoint(Point3D(worldPos));
        // }
    }
}

void DrawingInteractionMode::completeDrawing()
{
    if (m_currentDrawingGeo) {
        // m_currentDrawingGeo->mm_state()->setStateComplete();
        LOG_SUCCESS("绘制完成", "绘制交互");
    }
    
    m_currentDrawingGeo = nullptr;
    m_isDrawing = false;
}

void DrawingInteractionMode::cancelDrawing()
{
    if (m_currentDrawingGeo) {
        // m_widget->removeGeo(m_currentDrawingGeo.get());
        LOG_WARNING("取消绘制", "绘制交互");
    }
    
    m_currentDrawingGeo = nullptr;
    m_isDrawing = false;
}

void DrawingInteractionMode::onMouseRelease(QMouseEvent* event)
{
    // 在绘制模式下，鼠标释放通常不需要特殊处理
}

void DrawingInteractionMode::onMouseDoubleClick(QMouseEvent* event)
{
    // 双击可能用于快速完成绘制
    if (m_isDrawing && event->button() == Qt::LeftButton) {
        completeDrawing();
        LOG_INFO("双击完成绘制", "绘制交互");
    }
}

void DrawingInteractionMode::onWheel(QWheelEvent* event)
{
    // 绘制模式下允许滚轮缩放
    // m_widget->osgQOpenGLWidget::wheelEvent(event);
}

void DrawingInteractionMode::onKeyPress(QKeyEvent* event)
{
    if (m_isDrawing && m_currentDrawingGeo) {
        switch (event->key()) {
            case Qt::Key_Escape:
                // 撤销上一个点或取消绘制
                // auto controlPointManager = m_currentDrawingGeo->mm_controlPoint();
                // bool hasPoints = controlPointManager->undoLastControlPoint();
                // if (!hasPoints) {
                //     cancelDrawing();
                // }
                LOG_INFO("撤销绘制点或取消绘制", "绘制交互");
                break;
            case Qt::Key_Return:
            case Qt::Key_Enter:
                completeDrawing();
                break;
            default:
                break;
        }
    }
    
    // 相机控制键
    switch (event->key()) {
        case Qt::Key_W:
        case Qt::Key_S:
        case Qt::Key_A:
        case Qt::Key_D:
        case Qt::Key_Q:
        case Qt::Key_E:
            // m_widget->m_cameraController->setKeyPressed(event->key(), true);
            break;
        default:
            break;
    }
}

void DrawingInteractionMode::onKeyRelease(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_W:
        case Qt::Key_S:
        case Qt::Key_A:
        case Qt::Key_D:
        case Qt::Key_Q:
        case Qt::Key_E:
            // m_widget->m_cameraController->setKeyPressed(event->key(), false);
            break;
        default:
            break;
    }
}

//=============================================================================
// ViewOnlyInteractionMode - 仅查看模式
//=============================================================================

ViewOnlyInteractionMode::ViewOnlyInteractionMode(OSGWidget* widget)
    : InputInteractionMode(widget)
{
}

void ViewOnlyInteractionMode::onMousePress(QMouseEvent* event)
{
    // 只允许相机操作
    // m_widget->osgQOpenGLWidget::mousePressEvent(event);
    LOG_DEBUG("查看模式: 相机操作", "查看交互");
}

void ViewOnlyInteractionMode::onMouseMove(QMouseEvent* event)
{
    // m_widget->osgQOpenGLWidget::mouseMoveEvent(event);
}

void ViewOnlyInteractionMode::onMouseRelease(QMouseEvent* event)
{
    // m_widget->osgQOpenGLWidget::mouseReleaseEvent(event);
}

void ViewOnlyInteractionMode::onMouseDoubleClick(QMouseEvent* event)
{
    // 双击设置旋转中心
    if (event->button() == Qt::LeftButton) {
        // m_widget->updateWorldPosition(event->x(), event->y());
        // 设置旋转中心
        LOG_INFO("查看模式: 设置旋转中心", "查看交互");
    }
}

void ViewOnlyInteractionMode::onWheel(QWheelEvent* event)
{
    // m_widget->osgQOpenGLWidget::wheelEvent(event);
}

void ViewOnlyInteractionMode::onKeyPress(QKeyEvent* event)
{
    // 只允许相机控制键
    switch (event->key()) {
        case Qt::Key_W:
        case Qt::Key_S:
        case Qt::Key_A:
        case Qt::Key_D:
        case Qt::Key_Q:
        case Qt::Key_E:
            // m_widget->m_cameraController->setKeyPressed(event->key(), true);
            break;
        default:
            break;
    }
}

void ViewOnlyInteractionMode::onKeyRelease(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_W:
        case Qt::Key_S:
        case Qt::Key_A:
        case Qt::Key_D:
        case Qt::Key_Q:
        case Qt::Key_E:
            // m_widget->m_cameraController->setKeyPressed(event->key(), false);
            break;
        default:
            break;
    }
}

//=============================================================================
// MeasurementInteractionMode - 测量模式
//=============================================================================

MeasurementInteractionMode::MeasurementInteractionMode(OSGWidget* widget)
    : InputInteractionMode(widget)
{
}

void MeasurementInteractionMode::activate()
{
    LOG_INFO("激活测量交互模式", "测量交互");
    m_measurementPoints.clear();
    m_isMeasuring = false;
}

void MeasurementInteractionMode::deactivate()
{
    if (m_isMeasuring) {
        cancelMeasurement();
    }
    LOG_INFO("停用测量交互模式", "测量交互");
}

void MeasurementInteractionMode::onMousePress(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // m_widget->updateWorldPosition(event->x(), event->y());
        // addMeasurementPoint(m_widget->m_lastMouseWorldPos);
        
        LOG_INFO("添加测量点", "测量交互");
    } else if (event->button() == Qt::RightButton) {
        if (m_isMeasuring) {
            completeMeasurement();
        }
    }
}

void MeasurementInteractionMode::addMeasurementPoint(const glm::dvec3& point)
{
    m_measurementPoints.push_back(point);
    m_isMeasuring = true;
    
    // 创建测量可视化
    // 创建测量标注
    
    LOG_INFO(QString("添加测量点: (%1, %2, %3), 总点数: %4")
        .arg(point.x, 0, 'f', 3)
        .arg(point.y, 0, 'f', 3)
        .arg(point.z, 0, 'f', 3)
        .arg(m_measurementPoints.size()), "测量交互");
}

void MeasurementInteractionMode::completeMeasurement()
{
    if (m_measurementPoints.size() >= 2) {
        // 计算距离或面积
        double totalDistance = 0.0;
        for (size_t i = 1; i < m_measurementPoints.size(); ++i) {
            glm::dvec3 delta = m_measurementPoints[i] - m_measurementPoints[i-1];
            double segmentLength = glm::length(delta);
            totalDistance += segmentLength;
        }
        
        LOG_SUCCESS(QString("测量完成: 总长度 = %1").arg(totalDistance, 0, 'f', 3), "测量交互");
    }
    
    m_measurementPoints.clear();
    m_isMeasuring = false;
}

void MeasurementInteractionMode::cancelMeasurement()
{
    m_measurementPoints.clear();
    m_isMeasuring = false;
    
    // 清除可视化
    LOG_WARNING("取消测量", "测量交互");
}

void MeasurementInteractionMode::onMouseMove(QMouseEvent* event)
{
    if (m_isMeasuring && !m_measurementPoints.empty()) {
        // m_widget->updateWorldPosition(event->x(), event->y());
        // updateTemporaryLine(m_widget->m_lastMouseWorldPos);
    }
}

void MeasurementInteractionMode::updateTemporaryLine(const glm::dvec3& currentPos)
{
    if (!m_measurementPoints.empty()) {
        // 显示从最后一个点到当前鼠标位置的临时线
        glm::dvec3 lastPoint = m_measurementPoints.back();
        double distance = glm::length(currentPos - lastPoint);
        
        LOG_DEBUG(QString("临时测量线长度: %1").arg(distance, 0, 'f', 3), "测量交互");
    }
}

void MeasurementInteractionMode::onMouseRelease(QMouseEvent* event)
{
    // 测量模式下鼠标释放不需要特殊处理
}

void MeasurementInteractionMode::onMouseDoubleClick(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_isMeasuring) {
        completeMeasurement();
        LOG_INFO("双击完成测量", "测量交互");
    }
}

void MeasurementInteractionMode::onWheel(QWheelEvent* event)
{
    // 测量模式下允许滚轮缩放
    // m_widget->osgQOpenGLWidget::wheelEvent(event);
}

void MeasurementInteractionMode::onKeyPress(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_Escape:
            if (m_isMeasuring) {
                cancelMeasurement();
            }
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (m_isMeasuring) {
                completeMeasurement();
            }
            break;
        case Qt::Key_W:
        case Qt::Key_S:
        case Qt::Key_A:
        case Qt::Key_D:
        case Qt::Key_Q:
        case Qt::Key_E:
            // 相机控制
            // m_widget->m_cameraController->setKeyPressed(event->key(), true);
            break;
        default:
            break;
    }
}

void MeasurementInteractionMode::onKeyRelease(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_W:
        case Qt::Key_S:
        case Qt::Key_A:
        case Qt::Key_D:
        case Qt::Key_Q:
        case Qt::Key_E:
            // m_widget->m_cameraController->setKeyPressed(event->key(), false);
            break;
        default:
            break;
    }
} 