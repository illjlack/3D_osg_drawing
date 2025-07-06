#pragma once

#include "../core/Common3D.h"
#include "../core/picking/PickingSystem.h"
#include "../core/world/Skybox.h"
#include "../core/world/CoordinateSystem3D.h"
#include "../core/world/CoordinateSystemRenderer.h"
#include "../core/camera/CameraController.h"
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
#include <osg/TextureCubeMap>
#include <osg/Texture2D>
#include <osg/Image>
#include <osgQOpenGL/osgQOpenGLWidget>
#include <QTimer>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QKeyEvent>

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
    
    // 拾取
    PickResult3D pick(int x, int y);
    
    // 高级拾取系统
    void enableAdvancedPicking(bool enabled);
    bool isAdvancedPickingEnabled() const;
    void setPickingRadius(int radius);
    void setPickingFrequency(double frequency);
    
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
    
    // 摄像机移动控制
    void moveCameraUp();
    void moveCameraDown();
    void moveCameraLeft();
    void moveCameraRight();
    void moveCameraForward();
    void moveCameraBackward();
    void setCameraMoveSpeed(double speed);
    double getCameraMoveSpeed() const { return m_cameraMoveSpeed; }
    void setWheelMoveSensitivity(double sensitivity);
    double getWheelMoveSensitivity() const { return m_wheelMoveSensitivity; }
    
    // 加速度移动控制
    void setAccelerationRate(double rate);
    double getAccelerationRate() const { return m_accelerationRate; }
    void setMaxAccelerationSpeed(double speed);
    double getMaxAccelerationSpeed() const { return m_maxAccelerationSpeed; }
    void resetAllAcceleration();
    
    // 投影模式控制
    void setProjectionMode(ProjectionMode mode);
    ProjectionMode getProjectionMode() const;
    void setPerspectiveFOV(double fov);
    void setOrthographicSize(double left, double right, double bottom, double top);
    void setOrthographicNearFar(double near, double far);
    
    // 比例尺相关
    void enableScaleBar(bool enabled);
    bool isScaleBarEnabled() const { return m_scaleBarEnabled; }
    void setScaleBarPosition(const QPoint& position);
    void setScaleBarSize(int width, int height);

signals:
    void geoSelected(Geo3D* geo);
    void mousePositionChanged(const glm::vec3& worldPos);
    void advancedPickingResult(const PickingResult& result);
    void cameraMoveSpeedChanged(double speed);
    void wheelMoveSensitivityChanged(double sensitivity);
    void accelerationRateChanged(double rate);
    void maxAccelerationSpeedChanged(double speed);

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
    
    // 摄像机移动相关
    void updateCameraPosition();
    
    // 比例尺相关
    void drawScaleBar();
    double calculateScaleValue();
    QString formatScaleText(double worldUnits);
    
    // 拾取系统回调
    void onPickingResult(const PickingResult& result);

private:
    // OSG场景图相关成员
    osg::ref_ptr<osg::Group> m_rootNode;
    osg::ref_ptr<osg::Group> m_sceneNode;
    osg::ref_ptr<osg::Group> m_geoNode;
    osg::ref_ptr<osg::Group> m_lightNode;
    osg::ref_ptr<osg::Group> m_pickingIndicatorNode;  // 拾取指示器节点
    osg::ref_ptr<osg::Group> m_skyboxNode;            // 天空盒节点
    
    // 相机操控器
    osg::ref_ptr<osgGA::TrackballManipulator> m_trackballManipulator;
    
    // 摄像机控制器
    std::unique_ptr<CameraController> m_cameraController;
    
    // 摄像机移动控制
    double m_cameraMoveSpeed;
    double m_wheelMoveSensitivity; // 滚轮移动灵敏度
    bool m_cameraMoveKeys[6]; // up, down, left, right, forward, backward
    
    // 加速度移动相关
    double m_accelerationRate;
    double m_maxAccelerationSpeed;
    double m_accelerationSpeeds[6]; // 各方向的当前加速度
    
    // 当前绘制状态
    osg::ref_ptr<Geo3D> m_currentDrawingGeo;
    std::vector<osg::ref_ptr<Geo3D>> m_geoList;
    osg::ref_ptr<Geo3D> m_selectedGeo;
    
    // 交互状态
    bool m_isDrawing;
    glm::vec3 m_lastMouseWorldPos;
    
    // 拾取系统
    osg::ref_ptr<PickingEventHandler> m_pickingEventHandler;
    osg::ref_ptr<SimplePickingIndicatorManager> m_pickingIndicatorManager;
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
    
    QTimer* m_updateTimer;
}; 