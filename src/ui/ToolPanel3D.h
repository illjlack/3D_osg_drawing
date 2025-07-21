#pragma once
#include <QWidget>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QStackedWidget>
#include "../core/Enums3D.h"
#include <QCheckBox>

class ToolPanel3D : public QWidget
{
    Q_OBJECT
public:
    explicit ToolPanel3D(QWidget* parent = nullptr);
    void updateDrawMode(DrawMode3D mode);
signals:
    void drawModeChanged(DrawMode3D mode);
    void skyboxEnabled(bool enabled);
    void skyboxGradientRequested();
    void skyboxSolidRequested();
    void skyboxCustomRequested();
    void resetViewRequested();
    void fitViewRequested();
    void topViewRequested();
    void frontViewRequested();
    void rightViewRequested();
    void isometricViewRequested();
    void clearSceneRequested();
    void exportImageRequested();
    void coordinateSystemRequested();
    void displaySettingsRequested();
public slots:
    void onDrawModeButtonClicked();
    void onSkyboxEnabledChanged(bool enabled);
    void onSkyboxGradientClicked();
    void onSkyboxSolidClicked();
    void onSkyboxCustomClicked();
    void onResetViewClicked();
    void onFitViewClicked();
    void onTopViewClicked();
    void onFrontViewClicked();
    void onRightViewClicked();
    void onIsometricViewClicked();
    void onClearSceneClicked();
    void onExportImageClicked();
    void onCoordinateSystemClicked();
    void onDisplaySettingsClicked();
    void onDrawingCategoryChanged(int index);
private:
    void setupUI();
    void createDrawingGroup();
    void createSelectPage();
    void createBasicGeometryPage();
    void createBuildingPage();
    void createViewGroup();
    void createUtilityGroup();
    void createSkyboxGroup();
    DrawMode3D m_currentMode;
    QGroupBox* m_drawingGroup;
    QComboBox* m_drawingCategoryCombo;
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
    QPushButton* m_gableHouseButton;
    QPushButton* m_spireHouseButton;
    QPushButton* m_domeHouseButton;
    QPushButton* m_flatHouseButton;
    QPushButton* m_lHouseButton;
    QPushButton* m_prismButton;
    QPushButton* m_hemisphereButton;
    QPushButton* m_ellipsoidButton;
    QStackedWidget* m_drawingStackedWidget;
    QGroupBox* m_viewGroup;
    QPushButton* m_resetViewButton;
    QPushButton* m_fitViewButton;
    QPushButton* m_topViewButton;
    QPushButton* m_frontViewButton;
    QPushButton* m_rightViewButton;
    QPushButton* m_isometricViewButton;
    QGroupBox* m_utilityGroup;
    QPushButton* m_clearSceneButton;
    QPushButton* m_exportImageButton;
    QPushButton* m_coordinateSystemButton;
    QPushButton* m_displaySettingsButton;
    QGroupBox* m_skyboxGroup;
    QCheckBox* m_skyboxEnabledCheck;
    QPushButton* m_skyboxGradientButton;
    QPushButton* m_skyboxSolidButton;
    QPushButton* m_skyboxCustomButton;
}; 