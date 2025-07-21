#pragma once
#include <QWidget>
#include <QGroupBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <vector>
#include "../core/GeometryBase.h"

class PropertyEditor3D : public QWidget
{
    Q_OBJECT
public:
    explicit PropertyEditor3D(QWidget* parent = nullptr);
    void setGeo(Geo3D* geo);
    void setSelectedGeos(const std::vector<Geo3D*>& geos);
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
    void onShowPointsChanged();
    void onShowEdgesChanged();
    void onShowFacesChanged();
private:
    void setupUI();
    void createPointGroup();
    void createLineGroup();
    void createSurfaceGroup();
    void createMaterialGroup();
    void createVolumeGroup();
    void createDisplayGroup();
    void updatePointUI();
    void updateLineUI();
    void updateSurfaceUI();
    void updateMaterialUI();
    void updateVolumeUI();
    void updateDisplayUI();
    QPushButton* createColorButton(const QColor& color);
    void updateColorButton(QPushButton* button, const QColor& color);
    Geo3D* m_currentGeo;
    std::vector<Geo3D*> m_selectedGeos;
    bool m_updating;
    QGroupBox* m_pointGroup;
    QComboBox* m_pointShapeCombo;
    QDoubleSpinBox* m_pointSizeSpin;
    QPushButton* m_pointColorButton;
    QGroupBox* m_lineGroup;
    QComboBox* m_lineStyleCombo;
    QDoubleSpinBox* m_lineWidthSpin;
    QPushButton* m_lineColorButton;
    QDoubleSpinBox* m_lineDashPatternSpin;
    QComboBox* m_nodeLineStyleCombo;
    QGroupBox* m_surfaceGroup;
    QComboBox* m_fillTypeCombo;
    QPushButton* m_fillColorButton;
    QPushButton* m_borderColorButton;
    QCheckBox* m_showBorderCheck;
    QGroupBox* m_materialGroup;
    QComboBox* m_materialTypeCombo;
    QSlider* m_shininessSlider;
    QSlider* m_transparencySlider;
    QGroupBox* m_volumeGroup;
    QComboBox* m_subdivisionLevelCombo;
    QGroupBox* m_displayGroup;
    QCheckBox* m_showPointsCheck;
    QCheckBox* m_showEdgesCheck;
    QCheckBox* m_showFacesCheck;
}; 