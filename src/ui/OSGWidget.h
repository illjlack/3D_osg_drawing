#pragma once

#include "../core/Common3D.h"
#include "../core/picking/PickingSystem.h"
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
    
    // 坐标转换
    glm::vec3 screenToWorld(int x, int y, float depth = 0.0f);
    glm::vec2 worldToScreen(const glm::vec3& worldPos);

signals:
    void geoSelected(Geo3D* geo);
    void mousePositionChanged(const glm::vec3& worldPos);
    void drawingProgress(const QString& message);
    void advancedPickingResult(const PickingResult& result);

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
    
    void handleDrawingInput(QMouseEvent* event);
    void updateCurrentDrawing(const glm::vec3& worldPos);
    void completeCurrentDrawing();
    void cancelCurrentDrawing();
    
    // 拾取系统回调
    void onPickingResult(const PickingResult& result);

private:
    // OSG场景图相关成员
    osg::ref_ptr<osg::Group> m_rootNode;
    osg::ref_ptr<osg::Group> m_sceneNode;
    osg::ref_ptr<osg::Group> m_geoNode;
    osg::ref_ptr<osg::Group> m_lightNode;
    osg::ref_ptr<osg::Group> m_pickingIndicatorNode;  // 拾取指示器节点
    
    // 相机操控器
    osg::ref_ptr<osgGA::TrackballManipulator> m_trackballManipulator;
    
    // 当前绘制状态
    Geo3D* m_currentDrawingGeo;
    std::vector<Geo3D*> m_geoList;
    Geo3D* m_selectedGeo;
    
    // 交互状态
    bool m_isDrawing;
    glm::vec3 m_lastMouseWorldPos;
    
    // 拾取系统
    osg::ref_ptr<PickingEventHandler> m_pickingEventHandler;
    osg::ref_ptr<SimplePickingIndicatorManager> m_pickingIndicatorManager;
    bool m_advancedPickingEnabled;
    
    QTimer* m_updateTimer;
}; 