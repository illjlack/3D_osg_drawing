#pragma once
#pragma execution_character_set("utf-8")

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <osg/BoundingBox>
#include <osg/MatrixTransform>
#include "../core/GeometryBase.h"

class ImportInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportInfoDialog(Geo3D* importedGeo, QWidget* parent = nullptr);
    ~ImportInfoDialog() = default;

    // 获取偏移矩阵
    osg::Matrix getOffsetMatrix() const;
    
    // 获取是否应用偏移
    bool shouldApplyOffset() const;

private slots:
    void onApplyClicked();
    void onCancelClicked();
    void onResetClicked();
    void onCenterToOriginClicked();
    void onOffsetChanged();
    void updatePreview();

private:
    void setupUI();
    void updateBoundingBoxInfo();
    void calculateCurrentMatrix();

private:
    Geo3D* m_geometry;
    
    // 包围盒信息显示
    QGroupBox* m_boundingBoxGroup;
    QLabel* m_minPointLabel;
    QLabel* m_maxPointLabel;
    QLabel* m_centerLabel;
    QLabel* m_sizeLabel;
    QLabel* m_volumeLabel;
    
    // 偏移设置
    QGroupBox* m_offsetGroup;
    QDoubleSpinBox* m_offsetXSpin;
    QDoubleSpinBox* m_offsetYSpin;
    QDoubleSpinBox* m_offsetZSpin;
    
    // 旋转设置
    QGroupBox* m_rotationGroup;
    QDoubleSpinBox* m_rotationXSpin;
    QDoubleSpinBox* m_rotationYSpin;
    QDoubleSpinBox* m_rotationZSpin;
    
    // 缩放设置
    QGroupBox* m_scaleGroup;
    QDoubleSpinBox* m_scaleXSpin;
    QDoubleSpinBox* m_scaleYSpin;
    QDoubleSpinBox* m_scaleZSpin;
    QCheckBox* m_uniformScaleCheck;
    
    // 预览信息
    QGroupBox* m_previewGroup;
    QLabel* m_transformedCenterLabel;
    QLabel* m_transformedSizeLabel;
    
    // 按钮
    QPushButton* m_applyButton;
    QPushButton* m_resetButton;
    QPushButton* m_centerToOriginButton;
    QPushButton* m_cancelButton;
    
    // 快捷操作
    QCheckBox* m_autoApplyCheck;
    
    // 原始包围盒
    osg::BoundingBox m_originalBoundingBox;
    bool m_updating;
}; 

