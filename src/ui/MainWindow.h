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
#include <QStackedWidget>

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

#include "OSGWidget.h"
#include "CoordinateSystemDialog.h"

#include "LogOutputWidget.h"
#include "StatusBar3D.h"
#include "../util/GeoOsgbIO.h"
#include <QDateTime>
#include "PropertyEditor3D.h"
#include "ToolPanel3D.h"
#include "PropertyEditor3D.h"

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
    
    // 拾取系统相关
    void onPickingSystemSettings();
    
    // 实用工具相关
    void onClearScene();
    void onExportImage();
    void onDisplaySettings();
    
    void onHelpAbout();
    
    void onDrawModeChanged(DrawMode3D mode);
    void onGeoSelected(Geo3D* geo);
    void onGeoParametersChanged();
    void onSimplePickingResult(const PickResult& result);

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
    
    // 状态栏
    StatusBar3D* m_statusBar3D;
    
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
