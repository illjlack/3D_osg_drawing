#pragma once
#pragma execution_character_set("utf-8")

#include <QStatusBar>
#include <QLabel>
#include <QWidget>
#include <QHBoxLayout>
#include <QTimer>
#include <glm/glm.hpp>

// 前向声明
class OSGWidget;

// 3D状态栏类
class StatusBar3D : public QWidget
{
    Q_OBJECT

public:
    explicit StatusBar3D(QWidget* parent = nullptr);
    ~StatusBar3D();

    // 设置OSG Widget引用
    void setOSGWidget(OSGWidget* osgWidget);

    // 更新屏幕坐标显示
    void updateScreenCoordinates(int x, int y);
    
    // 更新空间坐标显示
    void updateWorldCoordinates(const glm::vec3& pos);
    
    // 更新模式显示
    void updateMode(const QString& mode);
    
    // 更新对象数量
    void updateObjectCount(int count);
    
    // 更新坐标范围
    void updateCoordinateRange(const QString& range);
    
    // 更新摄像机速度
    void updateCameraSpeed(double speed);
    
    // 更新投影模式
    void updateProjectionMode(const QString& mode);
    
    // 更新操作器类型
    void updateManipulatorType(const QString& type);
    
    // 更新FPS
    void updateFPS(double fps);
    
    // 更新内存使用
    void updateMemoryUsage(double mb);
    
    // 显示临时消息（不会覆盖固定内容）
    void showTemporaryMessage(const QString& message, int duration = 3000);

private slots:
    // 定时更新FPS和内存使用
    void updatePerformanceInfo();

private:
    void setupUI();
    void setupConnections();
    void updateLayout();

private:
    // OSG Widget引用
    OSGWidget* m_osgWidget;
    
    // 固定显示标签
    QLabel* m_screenCoordLabel;      // 屏幕坐标
    QLabel* m_worldCoordLabel;       // 空间坐标
    QLabel* m_modeLabel;             // 当前模式
    QLabel* m_objectCountLabel;      // 对象数量
    QLabel* m_coordinateRangeLabel;  // 坐标范围
    QLabel* m_cameraSpeedLabel;      // 摄像机速度
    QLabel* m_projectionModeLabel;   // 投影模式
    QLabel* m_manipulatorLabel;      // 操作器类型
    QLabel* m_fpsLabel;              // FPS
    QLabel* m_memoryLabel;           // 内存使用
    
    // 临时消息标签（右侧）
    QLabel* m_temporaryMessageLabel;
    
    // 性能更新定时器
    QTimer* m_performanceTimer;
    
    // 布局
    QHBoxLayout* m_mainLayout;
    
    // 分隔符
    QFrame* m_separator1;
    QFrame* m_separator2;
    QFrame* m_separator3;
    QFrame* m_separator4;
    QFrame* m_separator5;
    QFrame* m_separator6;
    QFrame* m_separator7;
    QFrame* m_separator8;
    QFrame* m_separator9;
    QFrame* m_separator10;
    
    // 临时消息定时器
    QTimer* m_messageTimer;
}; 