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
    
    // 绘制辅助函数
    void updateCurrentDrawing(const glm::dvec3& worldPos);
    void completeCurrentDrawing();
    void cancelCurrentDrawing();
    void onSimplePickingResult(const PickResult& result);
    
    // 事件传递控制
    void setMousePassToOSG(bool shouldPass);
    
    // 坐标转换辅助函数
    glm::dvec3 screenToWorld(int x, int y, double depth = 0.0);

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
}; 




