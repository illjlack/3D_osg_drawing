#include "ImportInfoDialog.h"
#include "../util/LogManager.h"
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QTabWidget>
#include <QScrollArea>
#include <QSplitter>
#include <osg/ComputeBoundsVisitor>
#include <cmath>

ImportInfoDialog::ImportInfoDialog(osg::ref_ptr<Geo3D> importedGeo, QWidget* parent)
    : QDialog(parent)
    , m_geometry(importedGeo)
    , m_updating(false)
{
    if (!m_geometry.valid()) {
        LOG_ERROR("导入几何体为空", "导入对话框");
        reject();
        return;
    }
    
    setWindowTitle("导入对象信息设置");
    setModal(true);
    resize(600, 500);
    
    // 居中显示
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    // 计算原始包围盒
    osg::ComputeBoundsVisitor visitor;
    m_geometry->mm_node()->getOSGNode()->accept(visitor);
    m_originalBoundingBox = visitor.getBoundingBox();
    
    setupUI();
    updateBoundingBoxInfo();
    updatePreview();
    
    LOG_INFO("导入信息对话框已打开", "导入对话框");
}

void ImportInfoDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // 创建选项卡
    QTabWidget* tabWidget = new QTabWidget();
    
    // 信息页面
    QWidget* infoPage = new QWidget();
    QVBoxLayout* infoLayout = new QVBoxLayout(infoPage);
    
    // 包围盒信息组
    m_boundingBoxGroup = new QGroupBox("包围盒信息");
    QFormLayout* boundingBoxLayout = new QFormLayout(m_boundingBoxGroup);
    
    m_minPointLabel = new QLabel();
    m_maxPointLabel = new QLabel();
    m_centerLabel = new QLabel();
    m_sizeLabel = new QLabel();
    m_volumeLabel = new QLabel();
    
    boundingBoxLayout->addRow("最小点:", m_minPointLabel);
    boundingBoxLayout->addRow("最大点:", m_maxPointLabel);
    boundingBoxLayout->addRow("中心点:", m_centerLabel);
    boundingBoxLayout->addRow("尺寸:", m_sizeLabel);
    boundingBoxLayout->addRow("体积:", m_volumeLabel);
    
    infoLayout->addWidget(m_boundingBoxGroup);
    
    // 预览信息组
    m_previewGroup = new QGroupBox("变换后预览");
    QFormLayout* previewLayout = new QFormLayout(m_previewGroup);
    
    m_transformedCenterLabel = new QLabel();
    m_transformedSizeLabel = new QLabel();
    
    previewLayout->addRow("变换后中心:", m_transformedCenterLabel);
    previewLayout->addRow("变换后尺寸:", m_transformedSizeLabel);
    
    infoLayout->addWidget(m_previewGroup);
    infoLayout->addStretch();
    
    // 变换页面
    QWidget* transformPage = new QWidget();
    QVBoxLayout* transformLayout = new QVBoxLayout(transformPage);
    
    // 偏移设置组
    m_offsetGroup = new QGroupBox("位置偏移");
    QFormLayout* offsetLayout = new QFormLayout(m_offsetGroup);
    
    m_offsetXSpin = new QDoubleSpinBox();
    m_offsetXSpin->setRange(-1e6, 1e6);
    m_offsetXSpin->setDecimals(3);
    m_offsetXSpin->setSingleStep(0.1);
    m_offsetXSpin->setSuffix(" m");
    connect(m_offsetXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ImportInfoDialog::onOffsetChanged);
    
    m_offsetYSpin = new QDoubleSpinBox();
    m_offsetYSpin->setRange(-1e6, 1e6);
    m_offsetYSpin->setDecimals(3);
    m_offsetYSpin->setSingleStep(0.1);
    m_offsetYSpin->setSuffix(" m");
    connect(m_offsetYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ImportInfoDialog::onOffsetChanged);
    
    m_offsetZSpin = new QDoubleSpinBox();
    m_offsetZSpin->setRange(-1e6, 1e6);
    m_offsetZSpin->setDecimals(3);
    m_offsetZSpin->setSingleStep(0.1);
    m_offsetZSpin->setSuffix(" m");
    connect(m_offsetZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ImportInfoDialog::onOffsetChanged);
    
    offsetLayout->addRow("X偏移:", m_offsetXSpin);
    offsetLayout->addRow("Y偏移:", m_offsetYSpin);
    offsetLayout->addRow("Z偏移:", m_offsetZSpin);
    
    // 旋转设置组
    m_rotationGroup = new QGroupBox("旋转角度");
    QFormLayout* rotationLayout = new QFormLayout(m_rotationGroup);
    
    m_rotationXSpin = new QDoubleSpinBox();
    m_rotationXSpin->setRange(-360, 360);
    m_rotationXSpin->setDecimals(1);
    m_rotationXSpin->setSingleStep(1.0);
    m_rotationXSpin->setSuffix("°");
    connect(m_rotationXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ImportInfoDialog::onOffsetChanged);
    
    m_rotationYSpin = new QDoubleSpinBox();
    m_rotationYSpin->setRange(-360, 360);
    m_rotationYSpin->setDecimals(1);
    m_rotationYSpin->setSingleStep(1.0);
    m_rotationYSpin->setSuffix("°");
    connect(m_rotationYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ImportInfoDialog::onOffsetChanged);
    
    m_rotationZSpin = new QDoubleSpinBox();
    m_rotationZSpin->setRange(-360, 360);
    m_rotationZSpin->setDecimals(1);
    m_rotationZSpin->setSingleStep(1.0);
    m_rotationZSpin->setSuffix("°");
    connect(m_rotationZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ImportInfoDialog::onOffsetChanged);
    
    rotationLayout->addRow("绕X轴:", m_rotationXSpin);
    rotationLayout->addRow("绕Y轴:", m_rotationYSpin);
    rotationLayout->addRow("绕Z轴:", m_rotationZSpin);
    
    // 缩放设置组
    m_scaleGroup = new QGroupBox("缩放比例");
    QFormLayout* scaleLayout = new QFormLayout(m_scaleGroup);
    
    m_uniformScaleCheck = new QCheckBox("等比缩放");
    m_uniformScaleCheck->setChecked(true);
    scaleLayout->addRow(m_uniformScaleCheck);
    
    m_scaleXSpin = new QDoubleSpinBox();
    m_scaleXSpin->setRange(0.001, 1000.0);
    m_scaleXSpin->setDecimals(3);
    m_scaleXSpin->setSingleStep(0.1);
    m_scaleXSpin->setValue(1.0);
    connect(m_scaleXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ImportInfoDialog::onOffsetChanged);
    
    m_scaleYSpin = new QDoubleSpinBox();
    m_scaleYSpin->setRange(0.001, 1000.0);
    m_scaleYSpin->setDecimals(3);
    m_scaleYSpin->setSingleStep(0.1);
    m_scaleYSpin->setValue(1.0);
    connect(m_scaleYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ImportInfoDialog::onOffsetChanged);
    
    m_scaleZSpin = new QDoubleSpinBox();
    m_scaleZSpin->setRange(0.001, 1000.0);
    m_scaleZSpin->setDecimals(3);
    m_scaleZSpin->setSingleStep(0.1);
    m_scaleZSpin->setValue(1.0);
    connect(m_scaleZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ImportInfoDialog::onOffsetChanged);
    
    // 连接等比缩放
    connect(m_uniformScaleCheck, &QCheckBox::toggled, [this](bool uniform) {
        if (uniform) {
            m_scaleYSpin->setValue(m_scaleXSpin->value());
            m_scaleZSpin->setValue(m_scaleXSpin->value());
        }
    });
    
    connect(m_scaleXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) {
                if (m_uniformScaleCheck->isChecked() && !m_updating) {
                    m_updating = true;
                    m_scaleYSpin->setValue(value);
                    m_scaleZSpin->setValue(value);
                    m_updating = false;
                }
            });
    
    scaleLayout->addRow("X缩放:", m_scaleXSpin);
    scaleLayout->addRow("Y缩放:", m_scaleYSpin);
    scaleLayout->addRow("Z缩放:", m_scaleZSpin);
    
    transformLayout->addWidget(m_offsetGroup);
    transformLayout->addWidget(m_rotationGroup);
    transformLayout->addWidget(m_scaleGroup);
    transformLayout->addStretch();
    
    // 添加选项卡
    tabWidget->addTab(infoPage, "对象信息");
    tabWidget->addTab(transformPage, "变换设置");
    
    mainLayout->addWidget(tabWidget);
    
    // 快捷操作按钮布局
    QHBoxLayout* quickButtonLayout = new QHBoxLayout();
    
    m_centerToOriginButton = new QPushButton("移至原点");
    m_centerToOriginButton->setToolTip("将对象中心移动到坐标原点");
    connect(m_centerToOriginButton, &QPushButton::clicked, this, &ImportInfoDialog::onCenterToOriginClicked);
    
    m_resetButton = new QPushButton("重置变换");
    m_resetButton->setToolTip("重置所有变换参数");
    connect(m_resetButton, &QPushButton::clicked, this, &ImportInfoDialog::onResetClicked);
    
    quickButtonLayout->addWidget(m_centerToOriginButton);
    quickButtonLayout->addWidget(m_resetButton);
    quickButtonLayout->addStretch();
    
    mainLayout->addLayout(quickButtonLayout);
    
    // 自动应用选项
    m_autoApplyCheck = new QCheckBox("实时预览变换效果");
    m_autoApplyCheck->setChecked(false);
    mainLayout->addWidget(m_autoApplyCheck);
    
    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_applyButton = new QPushButton("应用变换");
    m_applyButton->setDefault(true);
    connect(m_applyButton, &QPushButton::clicked, this, &ImportInfoDialog::onApplyClicked);
    
    m_cancelButton = new QPushButton("取消");
    connect(m_cancelButton, &QPushButton::clicked, this, &ImportInfoDialog::onCancelClicked);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
}

void ImportInfoDialog::updateBoundingBoxInfo()
{
    if (!m_originalBoundingBox.valid()) {
        LOG_WARNING("包围盒无效", "导入对话框");
        return;
    }
    
    osg::Vec3 minPoint = m_originalBoundingBox._min;
    osg::Vec3 maxPoint = m_originalBoundingBox._max;
    osg::Vec3 center = m_originalBoundingBox.center();
    osg::Vec3 size = maxPoint - minPoint;
    double volume = size.x() * size.y() * size.z();
    
    m_minPointLabel->setText(QString("(%1, %2, %3)")
        .arg(minPoint.x(), 0, 'f', 3)
        .arg(minPoint.y(), 0, 'f', 3)
        .arg(minPoint.z(), 0, 'f', 3));
    
    m_maxPointLabel->setText(QString("(%1, %2, %3)")
        .arg(maxPoint.x(), 0, 'f', 3)
        .arg(maxPoint.y(), 0, 'f', 3)
        .arg(maxPoint.z(), 0, 'f', 3));
    
    m_centerLabel->setText(QString("(%1, %2, %3)")
        .arg(center.x(), 0, 'f', 3)
        .arg(center.y(), 0, 'f', 3)
        .arg(center.z(), 0, 'f', 3));
    
    m_sizeLabel->setText(QString("(%1 × %2 × %3)")
        .arg(size.x(), 0, 'f', 3)
        .arg(size.y(), 0, 'f', 3)
        .arg(size.z(), 0, 'f', 3));
    
    m_volumeLabel->setText(QString("%1 m³").arg(volume, 0, 'f', 3));
}

void ImportInfoDialog::onOffsetChanged()
{
    if (m_updating) return;
    updatePreview();
}

void ImportInfoDialog::updatePreview()
{
    osg::Matrix matrix = getOffsetMatrix();
    
    // 计算变换后的中心点
    osg::Vec3 originalCenter = m_originalBoundingBox.center();
    osg::Vec3 transformedCenter = originalCenter * matrix;
    
    // 计算变换后的尺寸（只考虑缩放）
    osg::Vec3 originalSize = m_originalBoundingBox._max - m_originalBoundingBox._min;
    osg::Vec3 transformedSize(
        originalSize.x() * m_scaleXSpin->value(),
        originalSize.y() * m_scaleYSpin->value(),
        originalSize.z() * m_scaleZSpin->value()
    );
    
    m_transformedCenterLabel->setText(QString("(%1, %2, %3)")
        .arg(transformedCenter.x(), 0, 'f', 3)
        .arg(transformedCenter.y(), 0, 'f', 3)
        .arg(transformedCenter.z(), 0, 'f', 3));
    
    m_transformedSizeLabel->setText(QString("(%1 × %2 × %3)")
        .arg(transformedSize.x(), 0, 'f', 3)
        .arg(transformedSize.y(), 0, 'f', 3)
        .arg(transformedSize.z(), 0, 'f', 3));
}

void ImportInfoDialog::onCenterToOriginClicked()
{
    // 计算将中心移至原点的偏移
    osg::Vec3 center = m_originalBoundingBox.center();
    
    m_updating = true;
    m_offsetXSpin->setValue(-center.x());
    m_offsetYSpin->setValue(-center.y());
    m_offsetZSpin->setValue(-center.z());
    m_updating = false;
    
    updatePreview();
    LOG_INFO("已设置偏移使对象中心移至原点", "导入对话框");
}

void ImportInfoDialog::onResetClicked()
{
    m_updating = true;
    
    // 重置所有变换参数
    m_offsetXSpin->setValue(0.0);
    m_offsetYSpin->setValue(0.0);
    m_offsetZSpin->setValue(0.0);
    
    m_rotationXSpin->setValue(0.0);
    m_rotationYSpin->setValue(0.0);
    m_rotationZSpin->setValue(0.0);
    
    m_scaleXSpin->setValue(1.0);
    m_scaleYSpin->setValue(1.0);
    m_scaleZSpin->setValue(1.0);
    
    m_updating = false;
    
    updatePreview();
    LOG_INFO("已重置所有变换参数", "导入对话框");
}

void ImportInfoDialog::onApplyClicked()
{
    LOG_INFO("用户确认应用变换", "导入对话框");
    accept();
}

void ImportInfoDialog::onCancelClicked()
{
    LOG_INFO("用户取消导入设置", "导入对话框");
    reject();
}

osg::Matrix ImportInfoDialog::getOffsetMatrix() const
{
    // 创建变换矩阵：先缩放，再旋转，最后平移
    osg::Matrix matrix;
    
    // 缩放
    osg::Matrix scaleMatrix = osg::Matrix::scale(
        m_scaleXSpin->value(),
        m_scaleYSpin->value(),
        m_scaleZSpin->value()
    );
    
    // 旋转（按照 Z-Y-X 顺序）
    osg::Matrix rotationMatrix;
    rotationMatrix.makeRotate(
        osg::DegreesToRadians(m_rotationXSpin->value()), osg::Vec3(1, 0, 0),
        osg::DegreesToRadians(m_rotationYSpin->value()), osg::Vec3(0, 1, 0),
        osg::DegreesToRadians(m_rotationZSpin->value()), osg::Vec3(0, 0, 1)
    );
    
    // 平移
    osg::Matrix translationMatrix = osg::Matrix::translate(
        m_offsetXSpin->value(),
        m_offsetYSpin->value(),
        m_offsetZSpin->value()
    );
    
    // 组合变换：T * R * S
    matrix = scaleMatrix * rotationMatrix * translationMatrix;
    
    return matrix;
}

bool ImportInfoDialog::shouldApplyOffset() const
{
    // 检查是否有任何非默认的变换
    return (m_offsetXSpin->value() != 0.0 || m_offsetYSpin->value() != 0.0 || m_offsetZSpin->value() != 0.0 ||
            m_rotationXSpin->value() != 0.0 || m_rotationYSpin->value() != 0.0 || m_rotationZSpin->value() != 0.0 ||
            m_scaleXSpin->value() != 1.0 || m_scaleYSpin->value() != 1.0 || m_scaleZSpin->value() != 1.0);
}

