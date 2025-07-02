#pragma once

#include <QtWidgets/QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QApplication>

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
// OSG 3.6.5兼容性: Geode被弃用，推荐使用Group+Drawable
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
// 使用osgQOpenGL代替osgQt
#include <osgQOpenGL/osgQOpenGLWidget>
#include <QTimer>
#include <QOpenGLWidget>

#include "Common3D.h"

// 前向声明
class Geo3D;
class OSGWidget;
class PropertyEditor3D;
class ToolPanel3D;

// 主窗口类
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onFileNew();
    void onFileOpen();
    void onFileSave();
    void onFileSaveAs();
    void onFileExit();
    
    void onEditUndo();
    void onEditRedo();
    void onEditCopy();
    void onEditPaste();
    void onEditDelete();
    void onEditSelectAll();
    
    void onViewResetCamera();
    void onViewFitAll();
    void onViewTop();
    void onViewFront();
    void onViewRight();
    void onViewIsometric();
    void onViewWireframe();
    void onViewShaded();
    void onViewShadedWireframe();
    
    void onHelpAbout();
    
    void onDrawModeChanged(DrawMode3D mode);
    void onGeoSelected(Geo3D* geo);
    void onGeoParametersChanged();

private:
    void setupUI();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void createDockWidgets();
    void connectSignals();
    
    void updateStatusBar(const QString& message);
    void updateDrawModeUI();

private:
    // UI组件
    OSGWidget* m_osgWidget;
    PropertyEditor3D* m_propertyEditor;
    ToolPanel3D* m_toolPanel;
    
    // 菜单
    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_viewMenu;
    QMenu* m_helpMenu;
    
    // 工具栏
    QToolBar* m_mainToolBar;
    QToolBar* m_drawingToolBar;
    QToolBar* m_viewToolBar;
    
    // 停靠窗口
    QDockWidget* m_propertyDock;
    QDockWidget* m_toolDock;
    
    // 状态栏标签
    QLabel* m_positionLabel;
    QLabel* m_modeLabel;
    QLabel* m_objectCountLabel;
    
    QString m_currentFilePath;
    bool m_modified;
};

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
    
    // 坐标转换
    glm::vec3 screenToWorld(int x, int y, float depth = 0.0f);
    glm::vec2 worldToScreen(const glm::vec3& worldPos);

signals:
    void geoSelected(Geo3D* geo);
    void mousePositionChanged(const glm::vec3& worldPos);
    void drawingProgress(const QString& message);

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
    
    void handleDrawingInput(QMouseEvent* event);
    void updateCurrentDrawing(const glm::vec3& worldPos);
    void completeCurrentDrawing();
    void cancelCurrentDrawing();

private:
    // OSG场景图相关成员
    osg::ref_ptr<osg::Group> m_rootNode;
    osg::ref_ptr<osg::Group> m_sceneNode;
    osg::ref_ptr<osg::Group> m_geoNode;
    osg::ref_ptr<osg::Group> m_lightNode;
    
    // 相机操控器
    osg::ref_ptr<osgGA::TrackballManipulator> m_trackballManipulator;
    
    // 当前绘制状态
    Geo3D* m_currentDrawingGeo;
    std::vector<Geo3D*> m_geoList;
    Geo3D* m_selectedGeo;
    
    // 交互状态
    bool m_isDrawing;
    glm::vec3 m_lastMouseWorldPos;
    
    QTimer* m_updateTimer;
};

// 属性编辑器
class PropertyEditor3D : public QWidget
{
    Q_OBJECT

public:
    explicit PropertyEditor3D(QWidget* parent = nullptr);
    
    void setGeo(Geo3D* geo);
    void updateFromGeo();
    void updateGlobalSettings();

signals:
    void parametersChanged();

private slots:
    void onPointShapeChanged();
    void onPointSizeChanged();
    void onPointColorChanged();
    
    void onLineStyleChanged();
    void onLineWidthChanged();
    void onLineColorChanged();
    void onLineDashPatternChanged();
    void onNodeLineStyleChanged();
    
    void onFillTypeChanged();
    void onFillColorChanged();
    void onBorderColorChanged();
    void onShowBorderChanged();
    
    void onMaterialTypeChanged();
    void onShininessChanged();
    void onTransparencyChanged();
    void onSubdivisionLevelChanged();

private:
    void setupUI();
    void createPointGroup();
    void createLineGroup();
    void createSurfaceGroup();
    void createMaterialGroup();
    void createVolumeGroup();
    
    void updatePointUI();
    void updateLineUI();
    void updateSurfaceUI();
    void updateMaterialUI();
    void updateVolumeUI();
    
    QPushButton* createColorButton(const QColor& color);
    void updateColorButton(QPushButton* button, const QColor& color);

private:
    Geo3D* m_currentGeo;
    bool m_updating;
    
    // 点属性控件
    QGroupBox* m_pointGroup;
    QComboBox* m_pointShapeCombo;
    QDoubleSpinBox* m_pointSizeSpin;
    QPushButton* m_pointColorButton;
    
    // 线属性控件
    QGroupBox* m_lineGroup;
    QComboBox* m_lineStyleCombo;
    QDoubleSpinBox* m_lineWidthSpin;
    QPushButton* m_lineColorButton;
    QDoubleSpinBox* m_lineDashPatternSpin;
    QComboBox* m_nodeLineStyleCombo;
    
    // 面属性控件
    QGroupBox* m_surfaceGroup;
    QComboBox* m_fillTypeCombo;
    QPushButton* m_fillColorButton;
    QPushButton* m_borderColorButton;
    QCheckBox* m_showBorderCheck;
    
    // 材质属性控件
    QGroupBox* m_materialGroup;
    QComboBox* m_materialTypeCombo;
    QSlider* m_shininessSlider;
    QSlider* m_transparencySlider;
    
    // 体属性控件
    QGroupBox* m_volumeGroup;
    QComboBox* m_subdivisionLevelCombo;
};

// 工具面板
class ToolPanel3D : public QWidget
{
    Q_OBJECT

public:
    explicit ToolPanel3D(QWidget* parent = nullptr);
    
    void updateDrawMode(DrawMode3D mode);

signals:
    void drawModeChanged(DrawMode3D mode);

private slots:
    void onDrawModeButtonClicked();

private:
    void setupUI();
    void createDrawingGroup();
    void createViewGroup();
    void createUtilityGroup();

private:
    DrawMode3D m_currentMode;
    
    // 绘制工具按钮
    QGroupBox* m_drawingGroup;
    QPushButton* m_selectButton;
    QPushButton* m_pointButton;
    QPushButton* m_lineButton;
    QPushButton* m_arcButton;
    QPushButton* m_bezierButton;
    QPushButton* m_triangleButton;
    QPushButton* m_quadButton;
    QPushButton* m_polygonButton;
    QPushButton* m_boxButton;
    QPushButton* m_cubeButton;
    QPushButton* m_cylinderButton;
    QPushButton* m_coneButton;
    QPushButton* m_sphereButton;
    QPushButton* m_torusButton;
    
    // 视图和实用工具组
    QGroupBox* m_viewGroup;
    QGroupBox* m_utilityGroup;
    
    std::vector<QPushButton*> m_drawButtons;
};
