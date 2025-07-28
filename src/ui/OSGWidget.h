#pragma once
#pragma execution_character_set("utf-8")

#include "../core/Common3D.h"
#include "../core/GeometryBase.h"  // 包含Geo3D的完整定义，Qt MOC需要
#include "../core/camera/CameraController.h"
#include "../core/picking/PickingIndicator.h"
#include "../core/picking/GeometryPickingSystem.h"
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
    void setViewDirection(const glm::dvec3& direction, const glm::dvec3& up = glm::dvec3(0, 0, 1));
    
    // 显示模式
    void setWireframeMode(bool wireframe);
    void setShadedMode(bool shaded);
    void setPointMode(bool point);
    
    // 几何对象管理
    void addGeo(osg::ref_ptr<Geo3D> geo);
    void removeGeo(osg::ref_ptr<Geo3D> geo);
    void removeAllGeos();
    void selectGeo(osg::ref_ptr<Geo3D> geo);
    void deselectAll();
    osg::ref_ptr<Geo3D> getSelectedGeo() const;
    const std::vector<osg::ref_ptr<Geo3D>>& getAllGeos() const;
    
    // 多选功能
    void addToSelection(osg::ref_ptr<Geo3D> geo);
    void removeFromSelection(osg::ref_ptr<Geo3D> geo);
    void clearSelection();
    const std::vector<osg::ref_ptr<Geo3D>>& getSelectedGeos() const;
    bool isSelected(osg::ref_ptr<Geo3D> geo) const;
    int getSelectionCount() const;
    
    // 拖动控制点功能
    void startDraggingControlPoint(osg::ref_ptr<Geo3D> geo, int controlPointIndex);
    void stopDraggingControlPoint();
    bool isDraggingControlPoint() const { return m_isDraggingControlPoint; }
    osg::ref_ptr<Geo3D> getDraggingGeo() const { return m_draggingGeo; }
    int getDraggingControlPointIndex() const { return m_draggingControlPointIndex; }
    
    // 高亮管理
    void updateSelectionHighlight();
    void highlightSelectedObjects();
    
    // 简化的拾取功能
    PickResult performSimplePicking(int mouseX, int mouseY);
    
    // 绘制状态查询
    bool isDrawing() const { return m_isDrawing; }
    
    // 绘制模式管理
    void setDrawMode(DrawMode3D mode);
    
    // 坐标转换（委托给CameraController）
    glm::dvec3 screenToWorld(int x, int y, double depth = 0.0);
    glm::dvec2 worldToScreen(const glm::dvec3& worldPos);
    
    // 摄像机控制器接口（直接访问，不再提供委托方法）
    CameraController* getCameraController() const { return m_cameraController.get(); }
    
    // 右键菜单功能
    void deleteSelectedObjects();
    void setCameraPosition(const glm::dvec3& position, const glm::dvec3& target = glm::dvec3(0,0,0));
    void movePointToCoordinate(osg::ref_ptr<Geo3D> geo, int pointIndex, const glm::dvec3& newPosition);

signals:
    void geoSelected(osg::ref_ptr<Geo3D> geo);
    void mousePositionChanged(const glm::dvec3& worldPos);
    void screenPositionChanged(int x, int y);
    void simplePickingResult(const PickResult& result);

protected:
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void wheelEvent(QWheelEvent* event) override;
    virtual void keyPressEvent(QKeyEvent* event) override;
    virtual void keyReleaseEvent(QKeyEvent* event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

private slots:
    // 右键菜单槽函数
    void onDeleteSelectedObjects();
    void onSetCameraPosition();
    void onMovePointToCoordinate();
    void onSetEyePosition();
    void onResetCamera();
    void onFitAll();
    void onCenterObjectToView();
    
    // 几何体状态管理槽函数
    void onGeoStateInvalidated();

private:
    void setupCamera();
    void setupLighting();
    void setupEventHandlers();
    void setupPickingSystem();
    
    // 配置相机操作器
    void configureCameraManipulator();
    
    // 右键菜单功能
    void showContextMenu(const QPoint& pos);
    
    // 房屋几何体特殊渲染设置
    bool isHouseGeometry(osg::ref_ptr<Geo3D> geo);
    void setupHouseRenderingState(osg::Node* node);
    
    void updateCurrentDrawing(const glm::dvec3& worldPos);
    void completeCurrentDrawing();
    void cancelCurrentDrawing();
    
    // 拾取系统回调
    void onSimplePickingResult(const PickResult& result);

private:
    // OSG场景图相关成员
    osg::ref_ptr<osg::Group> m_rootNode;
    osg::ref_ptr<osg::Group> m_sceneNode;
    osg::ref_ptr<osg::Group> m_geoNode;
    osg::ref_ptr<osg::Group> m_lightNode;
    osg::ref_ptr<osg::Group> m_pickingIndicatorNode;  // 拾取指示器节点
    
    // 摄像机控制器
    std::unique_ptr<CameraController> m_cameraController;
    
    // 当前绘制状态
    osg::ref_ptr<Geo3D> m_currentDrawingGeo;
    std::vector<osg::ref_ptr<Geo3D>> m_geoList;
    osg::ref_ptr<Geo3D> m_selectedGeo;
    
    // 多选功能
    std::vector<osg::ref_ptr<Geo3D>> m_selectedGeos;  // 选中的几何对象列表
    bool m_multiSelectMode;               // 是否处于多选模式
    
    // 拖动控制点功能
    bool m_isDraggingControlPoint;
    osg::ref_ptr<Geo3D> m_draggingGeo;
    int m_draggingControlPointIndex;
    glm::dvec3 m_dragStartPosition;
    
    // 交互状态
    bool m_isDrawing;
    glm::dvec3 m_lastMouseWorldPos;
    
    // 拾取指示器
    osg::ref_ptr<PickingIndicator> m_pickingIndicator;
    
    // 几何拾取系统
    osg::ref_ptr<GeometryPickingSystem> m_geometryPickingSystem;
    
    // 鼠标位置缓存 - 避免频繁的坐标转换
    QPoint m_lastMouseScreenPos;
    glm::dvec3 m_cachedMouseWorldPos;
    bool m_mousePosCacheValid;
    QDateTime m_lastMouseCalculation;
    static const int MOUSE_CACHE_DURATION = 16;
    
    // 右键菜单相关
    QPoint m_lastContextMenuPos;
    osg::ref_ptr<Geo3D> m_contextMenuGeo;
    int m_contextMenuPointIndex; // 16ms缓存时间（约60FPS）
    
    QTimer* m_updateTimer;

private slots:
}; 




