#pragma once
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QPushButton>
#include <QComboBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
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
    void pickingSystemRequested();
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
    void onPickingSystemClicked();
    void onDisplaySettingsClicked();
    void onDrawingModeChanged(int index);
    void onViewToggleClicked();
    void onUtilityToggleClicked();
    void onSkyboxToggleClicked();

private:
    void setupUI();
    void setupStyles();
    
    // 创建各个可折叠模块
    void createCollapsibleDrawingSection(QVBoxLayout* parentLayout);
    void createCollapsibleViewSection(QVBoxLayout* parentLayout);
    void createCollapsibleUtilitySection(QVBoxLayout* parentLayout);
    void createCollapsibleSkyboxSection(QVBoxLayout* parentLayout);
    void createBasicToolsPage();
    void createGeometryPage();
    void createBuildingPage();
    
    // 辅助方法
    QPushButton* createStyledButton(const QString& emoji, const QString& text, const QString& tooltip, DrawMode3D mode);
    QPushButton* createActionButton(const QString& emoji, const QString& text, const QString& tooltip);

private:
    DrawMode3D m_currentMode;
    
    // 绘制工具相关
    QComboBox* m_drawingModeCombo;
    QStackedWidget* m_drawingStackedWidget;
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
    QPushButton* m_prismButton;
    QPushButton* m_hemisphereButton;
    QPushButton* m_ellipsoidButton;
    QPushButton* m_gableHouseButton;
    QPushButton* m_spireHouseButton;
    QPushButton* m_domeHouseButton;
    QPushButton* m_flatHouseButton;
    QPushButton* m_lHouseButton;
    
    // 视图工具相关
    QPushButton* m_viewToggleButton;
    QWidget* m_viewContentWidget;
    QPushButton* m_resetViewButton;
    QPushButton* m_fitViewButton;
    QPushButton* m_topViewButton;
    QPushButton* m_frontViewButton;
    QPushButton* m_rightViewButton;
    QPushButton* m_isometricViewButton;
    
    // 实用工具相关
    QPushButton* m_utilityToggleButton;
    QWidget* m_utilityContentWidget;
    QPushButton* m_clearSceneButton;
    QPushButton* m_exportImageButton;
    QPushButton* m_coordinateSystemButton;
    QPushButton* m_pickingSystemButton;
    QPushButton* m_displaySettingsButton;
    
    // 天空盒相关
    QPushButton* m_skyboxToggleButton;
    QWidget* m_skyboxContentWidget;
    QCheckBox* m_skyboxEnabledCheck;
    QPushButton* m_skyboxGradientButton;
    QPushButton* m_skyboxSolidButton;
    QPushButton* m_skyboxCustomButton;
}; 

