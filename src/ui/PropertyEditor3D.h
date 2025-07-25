#pragma once
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QGroupBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>
#include <QSpinBox>
#include <vector>
#include "../core/GeometryBase.h"

class PropertyEditor3D : public QWidget
{
    Q_OBJECT
public:
    explicit PropertyEditor3D(QWidget* parent = nullptr);
    void setGeo(osg::ref_ptr<Geo3D> geo);
    void setSelectedGeos(const std::vector<osg::ref_ptr<Geo3D>>& geos);
    void updateFromGeo();
    void updateGlobalSettings();
    
signals:
    void geometryRecalculationRequired();  // 需要重新计算几何体的参数变化
    void renderingParametersChanged();     // 只需要更新渲染的参数变化

private slots:
    // 需要重新计算几何体的参数
    void onPointShapeChanged();
    void onSubdivisionLevelChanged();
    
    // 只需要更新渲染的参数
    void onPointSizeChanged();
    void onPointColorChanged();
    void onLineWidthChanged();
    void onLineColorChanged();
    void onLineStyleChanged();
    void onLineDashPatternChanged();
    void onFillColorChanged();
    void onShowPointsChanged();
    void onShowEdgesChanged();
    void onShowFacesChanged();

private:
    void setupUI();
    void setupStyles();
    void createPointSection();
    void createLineSection();
    void createSurfaceSection();
    void createDisplaySection();
    void updatePointUI();
    void updateLineUI();
    void updateSurfaceUI();
    void updateDisplayUI();
    
    QPushButton* createColorButton(const QColor& color);
    void updateColorButton(QPushButton* button, const QColor& color);
    QWidget* createCollapsibleSection(const QString& title, const QString& emoji, QWidget* content);
    
    osg::ref_ptr<Geo3D> m_currentGeo;
    std::vector<osg::ref_ptr<Geo3D>> m_selectedGeos;
    bool m_updating;
    
    // 点属性控件
    QGroupBox* m_pointGroup;
    QComboBox* m_pointShapeCombo;      // 需要重新计算
    QDoubleSpinBox* m_pointSizeSpin;   // 只需渲染更新
    QPushButton* m_pointColorButton;   // 只需渲染更新
    
    // 线属性控件
    QGroupBox* m_lineGroup;
    QComboBox* m_lineStyleCombo;       // 只需渲染更新
    QDoubleSpinBox* m_lineWidthSpin;   // 只需渲染更新
    QPushButton* m_lineColorButton;    // 只需渲染更新
    QDoubleSpinBox* m_lineDashPatternSpin; // 只需渲染更新（自定义虚线）
    
    // 面属性控件
    QGroupBox* m_surfaceGroup;
    QPushButton* m_fillColorButton;    // 只需渲染更新（包含透明度）
    
    // 高级设置控件
    QGroupBox* m_advancedGroup;
    QComboBox* m_subdivisionLevelCombo;  // 需要重新计算
    
    // 显示控制控件
    QGroupBox* m_displayGroup;
    QCheckBox* m_showPointsCheck;      // 只需渲染更新
    QCheckBox* m_showEdgesCheck;       // 只需渲染更新
    QCheckBox* m_showFacesCheck;       // 只需渲染更新
}; 

