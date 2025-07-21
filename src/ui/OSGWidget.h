#pragma once
#pragma execution_character_set("utf-8")

#include "../core/Common3D.h"
#include "../core/world/Skybox.h"
#include "../core/world/CoordinateSystem3D.h"
#include "../core/world/CoordinateSystemRenderer.h"
#include "../core/camera/CameraController.h"
#include "../core/picking/RayPickingSystem.h"
#include <osgViewer/Viewer>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
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
#include <osg/TextureCubeMap>
#include <osg/Texture2D>
#include <osg/Image>
#include <osgQOpenGL/osgQOpenGLWidget>
#include <QTimer>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDateTime>

// 前向声明
class Geo3D;
class SimplePickingIndicatorManager;

// OSG视图组件
class OSGWidget : public osgQOpenGLWidget
{
    Q_OBJECT

public:
    OSGWidget(QWidget* parent = nullptr);
    virtual ~OSGWidget();
    
    // 场景管理
    void initializeScene();
    void resetCamera();
    void fitAll();
    void setViewDirection(const glm::vec3& direction, const glm::vec3& up = glm::vec3(0, 0, 1));
    
    // 显示模式
    void setWireframeMode(bool wireframe);
    void setShadedMode(bool shaded);
    void setPointMode(bool point);
    
    // 几何对象管理
    void addGeo(Geo3D* geo);
    void removeGeo(Geo3D* geo);
    void removeAllGeos();
    void selectGeo(Geo3D* geo);
    void deselectAll();
    Geo3D* getSelectedGeo() const;
    const std::vector<osg::ref_ptr<Geo3D>>& getAllGeos() const;
    
    // 多选功能
    void addToSelection(Geo3D* geo);
    void removeFromSelection(Geo3D* geo);
    void clearSelection();
    const std::vector<Geo3D*>& getSelectedGeos() const;
    bool isSelected(Geo3D* geo) const;
    int getSelectionCount() const;
    
    // 拖动控制点功能
    void startDraggingControlPoint(Geo3D* geo, int controlPointIndex);
    void stopDraggingControlPoint();
    bool isDraggingControlPoint() const { return m_isDraggingControlPoint; }
    Geo3D* getDraggingGeo() const { return m_draggingGeo; }
    int getDraggingControlPointIndex() const { return m_draggingControlPointIndex; }
    
    // 高亮管理
    void updateSelectionHighlight();
    void highlightSelectedObjects();
    
    // 拾取
    // PickResult3D pick(int x, int y);
    
    // 高级拾取系统
    void enableAdvancedPicking(bool enabled);
    bool isAdvancedPickingEnabled() const;
    void setPickingRadius(int radius);
    void setPickingFrequency(double frequency);
    
    // 拾取系统配置
    void setPickingConfig(const PickConfig& config);
    QString getPickingSystemInfo() const;
    
    // 确保所有几何对象都在拾取系统中
    void ensureAllGeosInPickingSystem();
    
    // 获取拾取系统状态信息
    QString getPickingSystemStatus() const;
    
    // 绘制状态查询
    bool isDrawing() const { return m_isDrawing; }
    
    // 绘制模式管理
    void setDrawMode(DrawMode3D mode);
    
    // 坐标转换
    glm::vec3 screenToWorld(int x, int y, float depth = 0.0f);
    glm::vec2 worldToScreen(const glm::vec3& worldPos);
    
    // 天空盒管理
    void enableSkybox(bool enabled);
    bool isSkyboxEnabled() const;
    void setSkyboxGradient(const osg::Vec4& topColor, const osg::Vec4& bottomColor);
    void setSkyboxSolidColor(const osg::Vec4& color);
    void setSkyboxCubeMap(const std::string& positiveX, const std::string& negativeX,
                         const std::string& positiveY, const std::string& negativeY,
                         const std::string& positiveZ, const std::string& negativeZ);
    void refreshSkybox();
    
    // 坐标系管理
    void enableCoordinateSystem(bool enabled);
    bool isCoordinateSystemEnabled() const;
    void refreshCoordinateSystem();
    
    // 摄像机控制器接口（委托给CameraController）
    CameraController* getCameraController() const { return m_cameraController.get(); }
    
    // 相机操控器管理
    void setManipulatorType(ManipulatorType type);
    ManipulatorType getManipulatorType() const;
    void switchToNextManipulator();
    void switchToPreviousManipulator();
    
    // 摄像机移动控制（委托给CameraController）
    void setCameraMoveSpeed(double speed);
    double getCameraMoveSpeed() const;
    void setWheelMoveSensitivity(double sensitivity);
    double getWheelMoveSensitivity() const;
    
    // 加速度移动控制（委托给CameraController）
    void setAccelerationRate(double rate);
    double getAccelerationRate() const;
    void setMaxAccelerationSpeed(double speed);
    double getMaxAccelerationSpeed() const;
    void resetAllAcceleration();
    
    // 投影模式控制（委托给CameraController）
    void setProjectionMode(ProjectionMode mode);
    ProjectionMode getProjectionMode() const;
    void setFOV(double fov);
    void setNearFar(double near, double far);
    void setViewSize(double left, double right, double bottom, double top);
    
    // 比例尺相关
    void enableScaleBar(bool enabled);
    bool isScaleBarEnabled() const { return m_scaleBarEnabled; }
    void setScaleBarPosition(const QPoint& position);
    void setScaleBarSize(int width, int height);

signals:
    void geoSelected(Geo3D* geo);
    void mousePositionChanged(const glm::vec3& worldPos);
    void screenPositionChanged(int x, int y);
    void simplePickingResult(const PickResult& result);
    void cameraMoveSpeedChanged(double speed);
    void wheelMoveSensitivityChanged(double speed);
    void accelerationRateChanged(double rate);
    void maxAccelerationSpeedChanged(double speed);
    void manipulatorTypeChanged(ManipulatorType type);

protected:
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void wheelEvent(QWheelEvent* event) override;
    virtual void keyPressEvent(QKeyEvent* event) override;
    virtual void keyReleaseEvent(QKeyEvent* event) override;

private:
    void setupCamera();
    void setupLighting();
    void setupEventHandlers();
    void setupPickingSystem();
    void setupSkybox();
    void setupCoordinateSystem();
    
    void handleDrawingInput(QMouseEvent* event);
    void updateCurrentDrawing(const glm::vec3& worldPos);
    void completeCurrentDrawing();
    void cancelCurrentDrawing();
    
    // 比例尺相关
    void drawScaleBar();
    double calculateScaleValue();
    QString formatScaleText(double worldUnits);
    
    // 拾取系统回调
    void onSimplePickingResult(const PickResult& result);

private:
    // OSG场景图相关成员
    osg::ref_ptr<osg::Group> m_rootNode;
    osg::ref_ptr<osg::Group> m_sceneNode;
    osg::ref_ptr<osg::Group> m_geoNode;
    osg::ref_ptr<osg::Group> m_lightNode;
    osg::ref_ptr<osg::Group> m_pickingIndicatorNode;  // 拾取指示器节点
    osg::ref_ptr<osg::Group> m_skyboxNode;            // 天空盒节点
    
    // 摄像机控制器
    std::unique_ptr<CameraController> m_cameraController;
    
    // 当前绘制状态
    osg::ref_ptr<Geo3D> m_currentDrawingGeo;
    std::vector<osg::ref_ptr<Geo3D>> m_geoList;
    osg::ref_ptr<Geo3D> m_selectedGeo;
    
    // 多选功能
    std::vector<Geo3D*> m_selectedGeos;  // 选中的几何对象列表
    bool m_multiSelectMode;               // 是否处于多选模式
    
    // 拖动控制点功能
    bool m_isDraggingControlPoint;
    Geo3D* m_draggingGeo;
    int m_draggingControlPointIndex;
    glm::vec3 m_dragStartPosition;
    
    // 交互状态
    bool m_isDrawing;
    glm::vec3 m_lastMouseWorldPos;
    
    // 拾取系统
    bool m_advancedPickingEnabled;
    
    // 天空盒
    std::unique_ptr<Skybox> m_skybox;
    bool m_skyboxEnabled;
    
    // 坐标系
    std::unique_ptr<CoordinateSystemRenderer> m_coordinateSystemRenderer;
    bool m_coordinateSystemEnabled;
    
    // 比例尺相关
    bool m_scaleBarEnabled;
    QPoint m_scaleBarPosition;
    QSize m_scaleBarSize;
    
    // 比例尺缓存 - 避免每帧重新计算
    double m_cachedScaleValue;
    QDateTime m_lastScaleCalculation;
    static const int SCALE_CACHE_DURATION = 100; // 100ms缓存时间
    
    // 鼠标位置缓存 - 避免频繁的坐标转换
    QPoint m_lastMouseScreenPos;
    glm::vec3 m_cachedMouseWorldPos;
    bool m_mousePosCacheValid;
    QDateTime m_lastMouseCalculation;
    static const int MOUSE_CACHE_DURATION = 16; // 16ms缓存时间（约60FPS）
    
    QTimer* m_updateTimer;

private slots:
    // 几何对象信号响应
    void onGeoDrawingCompleted(Geo3D* geo);
    void onGeoGeometryUpdated(Geo3D* geo);
    void onGeoParametersChanged(Geo3D* geo);
}; 