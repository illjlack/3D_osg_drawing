// #pragma once

// #include <memory>
// #include <unordered_map>
// #include <QObject>
// #include "InputInteractionMode.h"
// #include "../../core/Common3D.h"

// class OSGWidget;

// /**
//  * 交互模式类型枚举
//  */
// enum class InteractionModeType {
//     Camera,         // 相机控制模式
//     Selection,      // 选择模式
//     Drawing,        // 绘制模式
//     ViewOnly,       // 仅查看模式
//     Measurement     // 测量模式
// };

// /**
//  * 交互模式管理器
//  * 负责管理和切换不同的交互模式
//  */
// class InteractionModeManager : public QObject {
//     Q_OBJECT

// public:
//     explicit InteractionModeManager(OSGWidget* widget, QObject* parent = nullptr);
//     ~InteractionModeManager();

//     // 模式切换
//     void switchToMode(InteractionModeType modeType);
//     void switchToDrawingMode(DrawMode3D drawMode);
    
//     // 获取当前模式
//     InteractionModeType getCurrentModeType() const { return m_currentModeType; }
//     InputInteractionMode* getCurrentMode() const { return m_currentMode; }
//     QString getCurrentModeName() const;
    
//     // 事件分发
//     void handleMousePress(QMouseEvent* event);
//     void handleMouseMove(QMouseEvent* event);
//     void handleMouseRelease(QMouseEvent* event);
//     void handleMouseDoubleClick(QMouseEvent* event);
//     void handleWheel(QWheelEvent* event);
//     void handleKeyPress(QKeyEvent* event);
//     void handleKeyRelease(QKeyEvent* event);
    
//     // 模式状态查询
//     bool isInDrawingMode() const;
//     bool isInSelectionMode() const;
//     bool isInCameraMode() const;
    
//     // 特殊功能
//     void enableCameraOverride(bool enabled) { m_cameraOverrideEnabled = enabled; }
//     bool isCameraOverrideEnabled() const { return m_cameraOverrideEnabled; }

// signals:
//     void modeChanged(InteractionModeType newMode, const QString& modeName);
//     void drawingModeChanged(DrawMode3D drawMode);

// private slots:
//     void onModeActivated();
//     void onModeDeactivated();

// private:
//     OSGWidget* m_widget;
    
//     // 当前模式
//     InteractionModeType m_currentModeType;
//     InputInteractionMode* m_currentMode;
    
//     // 模式实例管理
//     std::unordered_map<InteractionModeType, std::unique_ptr<InputInteractionMode>> m_modes;
//     std::unordered_map<DrawMode3D, std::unique_ptr<DrawingInteractionMode>> m_drawingModes;
    
//     // 特殊状态
//     bool m_cameraOverrideEnabled;  // 相机控制覆盖（Ctrl键）
//     bool m_isInitialized;
    
//     // 初始化
//     void initializeModes();
//     void createMode(InteractionModeType modeType);
//     void createDrawingMode(DrawMode3D drawMode);
    
//     // 模式切换内部逻辑
//     void activateMode(InputInteractionMode* mode, InteractionModeType modeType);
//     void deactivateCurrentMode();
    
//     // 事件预处理
//     bool handleSpecialKeys(QKeyEvent* event);
//     bool shouldUseCameraMode(QMouseEvent* event) const;
// }; 