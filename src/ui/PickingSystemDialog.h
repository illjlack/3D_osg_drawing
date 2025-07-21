#pragma once
#pragma execution_character_set("utf-8")

#include "../core/picking/RayPickingSystem.h"
#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QSlider>

class PickingSystemDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PickingSystemDialog(QWidget* parent = nullptr);
    ~PickingSystemDialog() = default;

private slots:
    void onPresetConfigChanged(int index);
    void onPickRadiusChanged(double value);
    void onVertexPickRadiusChanged(double value);
    void onEdgePickRadiusChanged(double value);
    void onSnapThresholdChanged(double value);
    void onSnappingToggled(bool enabled);
    void onIndicatorToggled(bool enabled);
    void onHighlightToggled(bool enabled);
    void onPickVertexFirstToggled(bool enabled);
    void onPickEdgeSecondToggled(bool enabled);
    void onPickFaceLastToggled(bool enabled);
    void onIndicatorSizeChanged(double value);
    void onApplyClicked();
    void onResetClicked();

private:
    void setupUI();
    void createPresetGroup();
    void createToleranceGroup();
    void createPriorityGroup();
    void createIndicatorGroup();
    void createSnappingGroup();
    void createButtons();
    
    void updateFromPickingSystem();
    void updateToleranceUI();
    void updatePriorityUI();
    void updateIndicatorUI();
    void updateSnappingUI();
    void updatePresetCombo();

private:
    // 选项卡控件
    QTabWidget* m_tabWidget;
    
    // 预设配置组
    QGroupBox* m_presetGroup;
    QComboBox* m_presetCombo;
    QLabel* m_presetInfoLabel;
    
    // 容差设置组
    QGroupBox* m_toleranceGroup;
    QDoubleSpinBox* m_pickRadiusSpin;
    QDoubleSpinBox* m_vertexPickRadiusSpin;
    QDoubleSpinBox* m_edgePickRadiusSpin;
    QLabel* m_toleranceInfoLabel;
    
    // 拾取优先级组
    QGroupBox* m_priorityGroup;
    QCheckBox* m_pickVertexFirstCheck;
    QCheckBox* m_pickEdgeSecondCheck;
    QCheckBox* m_pickFaceLastCheck;
    QLabel* m_priorityInfoLabel;
    
    // 指示器设置组
    QGroupBox* m_indicatorGroup;
    QCheckBox* m_enableIndicatorCheck;
    QDoubleSpinBox* m_indicatorSizeSpin;
    QLabel* m_indicatorInfoLabel;
    
    // 捕捉设置组
    QGroupBox* m_snappingGroup;
    QCheckBox* m_enableSnappingCheck;
    QDoubleSpinBox* m_snapThresholdSpin;
    QLabel* m_snappingInfoLabel;
    
    // 高级设置组
    QGroupBox* m_advancedGroup;
    QCheckBox* m_enableHighlightCheck;
    QLabel* m_advancedInfoLabel;
    
    // 按钮
    QPushButton* m_applyButton;
    QPushButton* m_resetButton;
    QPushButton* m_cancelButton;
    
    // 拾取系统管理器引用
    PickingSystemManager* m_pickingManager;
    
    // 更新标志
    bool m_updating;
}; 