// #include "InteractionModeManager.h"
// #include "../OSGWidget.h"
// #include "../../util/LogManager.h"

// InteractionModeManager::InteractionModeManager(OSGWidget* widget, QObject* parent)
//     : QObject(parent)
//     , m_widget(widget)
//     , m_currentModeType(InteractionModeType::Selection)
//     , m_currentMode(nullptr)
//     , m_cameraOverrideEnabled(false)
//     , m_isInitialized(false)
// {
//     initializeModes();
    
//     // 默认激活选择模式
//     switchToMode(InteractionModeType::Selection);
    
//     LOG_INFO("交互模式管理器初始化完成", "交互管理");
// }

// InteractionModeManager::~InteractionModeManager()
// {
//     deactivateCurrentMode();
//     LOG_INFO("交互模式管理器销毁", "交互管理");
// }

// void InteractionModeManager::initializeModes()
// {
//     // 创建基础交互模式
//     createMode(InteractionModeType::Camera);
//     createMode(InteractionModeType::Selection);
//     createMode(InteractionModeType::ViewOnly);
//     createMode(InteractionModeType::Measurement);
    
//     // 绘制模式将在需要时动态创建
    
//     m_isInitialized = true;
//     LOG_INFO("交互模式初始化完成", "交互管理");
// }

// void InteractionModeManager::createMode(InteractionModeType modeType)
// {
//     std::unique_ptr<InputInteractionMode> mode;
    
//     switch (modeType) {
//         case InteractionModeType::Camera:
//             mode = std::make_unique<CameraInteractionMode>(m_widget);
//             break;
//         case InteractionModeType::Selection:
//             mode = std::make_unique<SelectionInteractionMode>(m_widget);
//             break;
//         case InteractionModeType::ViewOnly:
//             mode = std::make_unique<ViewOnlyInteractionMode>(m_widget);
//             break;
//         case InteractionModeType::Measurement:
//             mode = std::make_unique<MeasurementInteractionMode>(m_widget);
//             break;
//         case InteractionModeType::Drawing:
//             // 绘制模式需要特殊处理，不在这里创建
//             LOG_WARNING("绘制模式需要通过 createDrawingMode 创建", "交互管理");
//             return;
//     }
    
//     if (mode) {
//         m_modes[modeType] = std::move(mode);
//         LOG_INFO(QString("创建交互模式: %1").arg(static_cast<int>(modeType)), "交互管理");
//     }
// }

// void InteractionModeManager::createDrawingMode(DrawMode3D drawMode)
// {
//     if (m_drawingModes.find(drawMode) == m_drawingModes.end()) {
//         auto mode = std::make_unique<DrawingInteractionMode>(m_widget, drawMode);
//         m_drawingModes[drawMode] = std::move(mode);
//         LOG_INFO(QString("创建绘制交互模式: %1").arg(static_cast<int>(drawMode)), "交互管理");
//     }
// }

// void InteractionModeManager::switchToMode(InteractionModeType modeType)
// {
//     if (modeType == m_currentModeType && m_currentMode) {
//         LOG_DEBUG("已经处于目标模式，跳过切换", "交互管理");
//         return;
//     }
    
//     // 确保模式存在
//     if (m_modes.find(modeType) == m_modes.end()) {
//         createMode(modeType);
//     }
    
//     auto it = m_modes.find(modeType);
//     if (it != m_modes.end()) {
//         activateMode(it->second.get(), modeType);
//         emit modeChanged(modeType, getCurrentModeName());
//         LOG_INFO(QString("切换到交互模式: %1").arg(getCurrentModeName()), "交互管理");
//     } else {
//         LOG_ERROR(QString("无法找到交互模式: %1").arg(static_cast<int>(modeType)), "交互管理");
//     }
// }

// void InteractionModeManager::switchToDrawingMode(DrawMode3D drawMode)
// {
//     // 确保绘制模式存在
//     createDrawingMode(drawMode);
    
//     auto it = m_drawingModes.find(drawMode);
//     if (it != m_drawingModes.end()) {
//         activateMode(it->second.get(), InteractionModeType::Drawing);
//         emit modeChanged(InteractionModeType::Drawing, getCurrentModeName());
//         emit drawingModeChanged(drawMode);
//         LOG_INFO(QString("切换到绘制模式: %1").arg(getCurrentModeName()), "交互管理");
//     } else {
//         LOG_ERROR(QString("无法找到绘制模式: %1").arg(static_cast<int>(drawMode)), "交互管理");
//     }
// }

// void InteractionModeManager::activateMode(InputInteractionMode* mode, InteractionModeType modeType)
// {
//     if (mode == m_currentMode) return;
    
//     deactivateCurrentMode();
    
//     m_currentMode = mode;
//     m_currentModeType = modeType;
    
//     if (m_currentMode) {
//         m_currentMode->activate();
//     }
// }

// void InteractionModeManager::deactivateCurrentMode()
// {
//     if (m_currentMode) {
//         m_currentMode->deactivate();
//         LOG_DEBUG(QString("停用交互模式: %1").arg(getCurrentModeName()), "交互管理");
//     }
// }

// QString InteractionModeManager::getCurrentModeName() const
// {
//     if (m_currentMode) {
//         return m_currentMode->getModeName();
//     }
//     return "未知模式";
// }

// // 模式状态查询
// bool InteractionModeManager::isInDrawingMode() const
// {
//     return m_currentModeType == InteractionModeType::Drawing;
// }

// bool InteractionModeManager::isInSelectionMode() const
// {
//     return m_currentModeType == InteractionModeType::Selection;
// }

// bool InteractionModeManager::isInCameraMode() const
// {
//     return m_currentModeType == InteractionModeType::Camera;
// }

// // 事件分发
// void InteractionModeManager::handleMousePress(QMouseEvent* event)
// {
//     if (!m_currentMode) return;
    
//     // 检查是否需要相机控制覆盖
//     if (shouldUseCameraMode(event)) {
//         // 临时切换到相机模式处理事件
//         auto cameraMode = m_modes[InteractionModeType::Camera].get();
//         if (cameraMode) {
//             cameraMode->onMousePress(event);
//             return;
//         }
//     }
    
//     m_currentMode->onMousePress(event);
//     LOG_DEBUG("分发鼠标按下事件", "交互管理");
// }

// void InteractionModeManager::handleMouseMove(QMouseEvent* event)
// {
//     if (!m_currentMode) return;
    
//     // 检查相机控制覆盖
//     if (m_cameraOverrideEnabled && (event->modifiers() & Qt::ControlModifier)) {
//         auto cameraMode = m_modes[InteractionModeType::Camera].get();
//         if (cameraMode) {
//             cameraMode->onMouseMove(event);
//             return;
//         }
//     }
    
//     m_currentMode->onMouseMove(event);
// }

// void InteractionModeManager::handleMouseRelease(QMouseEvent* event)
// {
//     if (!m_currentMode) return;
    
//     // 检查相机控制覆盖
//     if (shouldUseCameraMode(event)) {
//         auto cameraMode = m_modes[InteractionModeType::Camera].get();
//         if (cameraMode) {
//             cameraMode->onMouseRelease(event);
//             return;
//         }
//     }
    
//     m_currentMode->onMouseRelease(event);
// }

// void InteractionModeManager::handleMouseDoubleClick(QMouseEvent* event)
// {
//     if (!m_currentMode) return;
    
//     m_currentMode->onMouseDoubleClick(event);
//     LOG_DEBUG("分发鼠标双击事件", "交互管理");
// }

// void InteractionModeManager::handleWheel(QWheelEvent* event)
// {
//     if (!m_currentMode) return;
    
//     m_currentMode->onWheel(event);
// }

// void InteractionModeManager::handleKeyPress(QKeyEvent* event)
// {
//     if (!m_currentMode) return;
    
//     // 处理特殊按键
//     if (handleSpecialKeys(event)) {
//         return;
//     }
    
//     // 检查相机控制键
//     if (event->modifiers() & Qt::ControlModifier) {
//         m_cameraOverrideEnabled = true;
//     }
    
//     m_currentMode->onKeyPress(event);
// }

// void InteractionModeManager::handleKeyRelease(QKeyEvent* event)
// {
//     if (!m_currentMode) return;
    
//     // 释放相机控制覆盖
//     if (!(event->modifiers() & Qt::ControlModifier)) {
//         m_cameraOverrideEnabled = false;
//     }
    
//     m_currentMode->onKeyRelease(event);
// }

// bool InteractionModeManager::handleSpecialKeys(QKeyEvent* event)
// {
//     switch (event->key()) {
//         case Qt::Key_F1:
//             // 快捷键切换到选择模式
//             switchToMode(InteractionModeType::Selection);
//             return true;
//         case Qt::Key_F2:
//             // 快捷键切换到相机模式
//             switchToMode(InteractionModeType::Camera);
//             return true;
//         case Qt::Key_F3:
//             // 快捷键切换到测量模式
//             switchToMode(InteractionModeType::Measurement);
//             return true;
//         case Qt::Key_F4:
//             // 快捷键切换到查看模式
//             switchToMode(InteractionModeType::ViewOnly);
//             return true;
//         default:
//             return false;
//     }
// }

// bool InteractionModeManager::shouldUseCameraMode(QMouseEvent* event) const
// {
//     // Ctrl + 任意鼠标按键都使用相机模式
//     return m_cameraOverrideEnabled && (event->modifiers() & Qt::ControlModifier);
// }

// void InteractionModeManager::onModeActivated()
// {
//     LOG_DEBUG("交互模式激活信号", "交互管理");
// }

// void InteractionModeManager::onModeDeactivated()
// {
//     LOG_DEBUG("交互模式停用信号", "交互管理");
// }

// #include "InteractionModeManager.moc" 