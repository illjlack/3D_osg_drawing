#pragma once
#pragma execution_character_set("utf-8")

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
#include "../core/picking/PickingIntegration.h"
#include "OSGWidget.h"
#include "CoordinateSystemDialog.h"
#include "LogOutputWidget.h"
#include "../util/GeoOsgbIO.h"
#include <QDateTime>

// 前向声明
class Geo3D;
class PropertyEditor3D;
class ToolPanel3D;
class SimplePickingIndicatorManager;

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
    
    // 投影模式相关
    void onProjectionModeChanged();
    void onPerspectiveFOVChanged();
    void onOrthographicSizeChanged();
    
    // 相机操控器相关
    void onManipulatorTypeChanged();
    
    // 天空盒相关
    void onViewSkybox();
    void onToolPanelSkyboxEnabled(bool enabled);
    void onSkyboxGradient();
    void onSkyboxSolid();
    void onSkyboxCustom();
    
    // 坐标系统相关
    void onCoordinateSystemSettings();
    
    // 实用工具相关
    void onClearScene();
    void onExportImage();
    void onDisplaySettings();
    
    void onHelpAbout();
    
    void onDrawModeChanged(DrawMode3D mode);
    void onGeoSelected(Geo3D* geo);
    void onGeoParametersChanged();
    void onAdvancedPickingResult(const PickingResult& result);

private:
    void setupUI();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void createDockWidgets();
    void connectSignals();
    
    void updateStatusBar(const QString& message);
    void updateDrawModeUI();
    void updateCoordinateRangeLabel();
    void updateObjectCount();

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
    QDockWidget* m_logDock;
    
    // 状态栏标签
    QLabel* m_positionLabel;
    QLabel* m_modeLabel;
    QLabel* m_objectCountLabel;
    QLabel* m_coordinateRangeLabel;
    
    // 日志输出栏
    LogOutputWidget* m_logOutputWidget;
    
    // 投影模式相关UI控件
    QComboBox* m_projectionModeCombo;
    QDoubleSpinBox* m_perspectiveFOVSpinBox;
    QDoubleSpinBox* m_orthographicSizeSpinBox;
    
    // 相机操控器相关UI控件
    QComboBox* m_manipulatorCombo;
    
    QString m_currentFilePath;
    bool m_modified;
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
    
    // 天空盒相关信号
    void skyboxEnabled(bool enabled);
    void skyboxGradientRequested();
    void skyboxSolidRequested();
    void skyboxCustomRequested();
    
    // 视图工具相关信号
    void resetViewRequested();
    void fitViewRequested();
    void topViewRequested();
    void frontViewRequested();
    void rightViewRequested();
    void isometricViewRequested();
    
    // 实用工具相关信号
    void clearSceneRequested();
    void exportImageRequested();
    void coordinateSystemRequested();
    void displaySettingsRequested();

private slots:
    void onDrawModeButtonClicked();
    
    // 天空盒相关槽函数
    void onSkyboxEnabledChanged(bool enabled);
    void onSkyboxGradientClicked();
    void onSkyboxSolidClicked();
    void onSkyboxCustomClicked();
    
    // 视图工具相关槽函数
    void onResetViewClicked();
    void onFitViewClicked();
    void onTopViewClicked();
    void onFrontViewClicked();
    void onRightViewClicked();
    void onIsometricViewClicked();
    
    // 实用工具相关槽函数
    void onClearSceneClicked();
    void onExportImageClicked();
    void onCoordinateSystemClicked();
    void onDisplaySettingsClicked();

private:
    void setupUI();
    void createDrawingGroup();
    void createViewGroup();
    void createUtilityGroup();
    void createSkyboxGroup();

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
    
    // 视图工具按钮
    QGroupBox* m_viewGroup;
    QPushButton* m_resetViewButton;
    QPushButton* m_fitViewButton;
    QPushButton* m_topViewButton;
    QPushButton* m_frontViewButton;
    QPushButton* m_rightViewButton;
    QPushButton* m_isometricViewButton;
    
    // 实用工具按钮
    QGroupBox* m_utilityGroup;
    QPushButton* m_clearSceneButton;
    QPushButton* m_exportImageButton;
    QPushButton* m_coordinateSystemButton;
    QPushButton* m_displaySettingsButton;
    
    // 天空盒控件
    QGroupBox* m_skyboxGroup;
    QCheckBox* m_skyboxEnabledCheck;
    QPushButton* m_skyboxGradientButton;
    QPushButton* m_skyboxSolidButton;
    QPushButton* m_skyboxCustomButton;
    
    std::vector<QPushButton*> m_drawButtons;
};
