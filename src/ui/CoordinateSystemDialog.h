#pragma once
#pragma execution_character_set("utf-8")

#include "../core/world/CoordinateSystem3D.h"
#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QLineEdit>

class CoordinateSystemDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CoordinateSystemDialog(QWidget* parent = nullptr);
    ~CoordinateSystemDialog() = default;

private slots:
    void onPresetRangeChanged(int index);
    void onRangeLimitToggled(bool enabled);
    void onSkyboxBindingToggled(bool enabled);
    void onCoordinateRangeChanged();
    void onSkyboxRangeChanged();
    void onCoordinateSystemTypeChanged(int index);
    void onAxisVisibleToggled(bool enabled);
    void onGridVisibleToggled(bool enabled);
    void onGridPlaneToggled(bool enabled);
    void onScaleUnitChanged(int index);
    void onCustomUnitNameChanged(const QString& text);
    void onScaleIntervalChanged(double value);
    void onAxisLengthChanged(double value);
    void onAxisThicknessChanged(double value);
    void onGridSpacingChanged(double value);
    void onGridThicknessChanged(double value);
    void onFontSizeChanged(int index);
    void onCustomFontSizeChanged(double value);
    void onApplyClicked();
    void onResetClicked();

private:
    void setupUI();
    void createPresetGroup();
    void createCoordinateGroup();
    void createSkyboxGroup();
    void createCoordinateSystemGroup();
    void createOptionsGroup();
    void createButtons();
    
    void updateFromCoordinateSystem();
    void updateCoordinateRangeUI();
    void updateSkyboxRangeUI();
    void updateCoordinateSystemUI();
    void updatePresetCombo();

private:
    // 选项卡控件
    QTabWidget* m_tabWidget;
    
    // 预设范围组
    QGroupBox* m_presetGroup;
    QComboBox* m_presetCombo;
    QLabel* m_presetInfoLabel;
    
    // 坐标范围组
    QGroupBox* m_coordinateGroup;
    QDoubleSpinBox* m_minXSpin;
    QDoubleSpinBox* m_maxXSpin;
    QDoubleSpinBox* m_minYSpin;
    QDoubleSpinBox* m_maxYSpin;
    QDoubleSpinBox* m_minZSpin;
    QDoubleSpinBox* m_maxZSpin;
    QLabel* m_coordinateInfoLabel;
    
    // 天空盒范围组
    QGroupBox* m_skyboxGroup;
    QDoubleSpinBox* m_skyboxMinXSpin;
    QDoubleSpinBox* m_skyboxMaxXSpin;
    QDoubleSpinBox* m_skyboxMinYSpin;
    QDoubleSpinBox* m_skyboxMaxYSpin;
    QDoubleSpinBox* m_skyboxMinZSpin;
    QDoubleSpinBox* m_skyboxMaxZSpin;
    QLabel* m_skyboxInfoLabel;
    
    // 坐标系组
    QGroupBox* m_coordinateSystemGroup;
    QComboBox* m_coordSystemTypeCombo;
    QCheckBox* m_axisXCheck;
    QCheckBox* m_axisYCheck;
    QCheckBox* m_axisZCheck;
    QCheckBox* m_gridVisibleCheck;
    QCheckBox* m_gridXYCheck;
    QCheckBox* m_gridYZCheck;
    QCheckBox* m_gridXZCheck;
    QComboBox* m_scaleUnitCombo;
    QLineEdit* m_customUnitEdit;
    QDoubleSpinBox* m_scaleIntervalSpin;
    QDoubleSpinBox* m_axisLengthSpin;
    QDoubleSpinBox* m_axisThicknessSpin;
    QDoubleSpinBox* m_gridSpacingSpin;
    QDoubleSpinBox* m_gridThicknessSpin;
    QComboBox* m_fontSizeCombo;
    QDoubleSpinBox* m_customFontSizeSpin;
    QLabel* m_coordinateSystemInfoLabel;
    
    // 选项组
    QGroupBox* m_optionsGroup;
    QCheckBox* m_rangeLimitCheck;
    QCheckBox* m_skyboxBindingCheck;
    
    // 按钮
    QPushButton* m_applyButton;
    QPushButton* m_resetButton;
    QPushButton* m_cancelButton;
    
    // 坐标系统实例
    CoordinateSystem3D* m_coordSystem;
    
    // 更新标志
    bool m_updating;
}; 