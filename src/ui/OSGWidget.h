#pragma once
#pragma execution_character_set("utf-8")

#include "../core/Common3D.h"
#include "../core/camera/CameraController.h"
#include "../core/world/SceneManager3D.h"
#include <osgQOpenGL/osgQOpenGLWidget>
#include <osg/ref_ptr>
#include <QTimer>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDateTime>
#include <memory>
#include "../core/GeometryBase.h"

class SceneManager3D;

class OSGWidget : public osgQOpenGLWidget
{
    Q_OBJECT

public:
    OSGWidget(QWidget* parent = nullptr);
    virtual ~OSGWidget();
    
    // 初始化
    void initializeScene();
    
    // 管理器访问
    SceneManager3D* getSceneManager() const { return m_sceneManager.get(); }
    CameraController* getCameraController() const { return m_cameraController.get(); }

signals:
    void geoSelected(osg::ref_ptr<Geo3D> geo);
    void mousePositionChanged(const glm::dvec3& worldPos);
    void screenPositionChanged(int x, int y);
    void simplePickingResult(const PickResult& result);
    void coordinateSystemSettingsRequested();
    void drawModeChanged(DrawMode3D mode);
    void cameraSpeedChanged(double speed);

protected:
    // 事件处理
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void wheelEvent(QWheelEvent* event) override;
    virtual void keyPressEvent(QKeyEvent* event) override;
    virtual void keyReleaseEvent(QKeyEvent* event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
    virtual void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    // 右键菜单槽函数
    void onDeleteSelectedObjects();
    void onSetCameraPosition();
    void onMovePointToCoordinate();
    void onSetEyePosition();
    void onResetCamera();
    void onFitAll();
    void onCenterObjectToView();

private:
    // 初始化和设置
    void setupCamera();
    void setupEventHandlers();
    
    // 鼠标事件处理函数
    void handleMousePress(QMouseEvent* event);
    void handleMouseMove(QMouseEvent* event);
    void handleMouseRelease(QMouseEvent* event);
    
    // 键盘事件处理函数
    void handleKeyPress(QKeyEvent* event);
    void handleKeyRelease(QKeyEvent* event);
    
    // 键盘移动处理
    void processKeyboardMovement();
    double calculateMoveSpeed(int keyCode);
    
    // 绘制辅助函数
    glm::dvec3 screenToWorld(int x, int y, double depth = 0.0);
    void completeCurrentDrawing();
    void cancelCurrentDrawing();
    void onSimplePickingResult(const PickResult& result);
    
    // 事件传递控制
    void setMousePassToOSG(bool shouldPass);
    
private:
    // 核心系统
    std::unique_ptr<SceneManager3D> m_sceneManager;
    std::unique_ptr<CameraController> m_cameraController;
    
    // 交互状态
    DrawMode3D m_lastDrawMode;
    glm::dvec3 m_lastMouseWorldPos;
    bool m_shouldPassMouseToOSG;  // 是否传递鼠标事件给OSG
    
    // 鼠标缓存
    QPoint m_lastMouseScreenPos;
    glm::dvec3 m_cachedMouseWorldPos;
    bool m_mousePosCacheValid;
    QDateTime m_lastMouseCalculation;
    static const int MOUSE_CACHE_DURATION = 16;
    
    // 右键菜单
    QPoint m_lastContextMenuPos;
    osg::ref_ptr<Geo3D> m_contextMenuGeo;
    int m_contextMenuPointIndex;
    
    // 渲染循环
    QTimer* m_updateTimer;
    
    // 键盘移动加速控制变量
    double m_initialSpeed = 0.1;        // 起始速度
    double m_acceleration = 0.01;        // 加速度
    int m_speedCounter = 0;          // 速度计数器
    int m_maxCount = 10000;              // 最大计数
    
    // 按键状态跟踪
    bool m_keyPressed[512];              // 按键状态数组，512足够覆盖所有按键
}; 




