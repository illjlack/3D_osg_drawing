#include "PickingSystemDialog.h"
#include <QMessageBox>
#include <QApplication>
#include <QScreen>

PickingSystemDialog::PickingSystemDialog(QWidget* parent)
    : QDialog(parent)
    , m_pickingManager(&PickingSystemManager::getInstance())
    , m_updating(false)
{
    setWindowTitle("拾取系统设置");
    setModal(true);
    
    // 设置窗口大小
    resize(450, 350);
    
    // 居中显示
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    setupUI();
    updateFromPickingSystem();
}

void PickingSystemDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // 创建选项卡控件
    m_tabWidget = new QTabWidget(this);
    
    // 创建各个选项卡页面
    QWidget* basicTab = new QWidget();
    QWidget* advancedTab = new QWidget();
    
    // 设置基本配置选项卡
    QVBoxLayout* basicLayout = new QVBoxLayout(basicTab);
    createPresetGroup();
    createToleranceGroup();
    createPriorityGroup();
    basicLayout->addWidget(m_presetGroup);
    basicLayout->addWidget(m_toleranceGroup);
    basicLayout->addWidget(m_priorityGroup);
    basicLayout->addStretch();
    
    // 设置高级配置选项卡
    QVBoxLayout* advancedLayout = new QVBoxLayout(advancedTab);
    createIndicatorGroup();
    createSnappingGroup();
    advancedLayout->addWidget(m_indicatorGroup);
    advancedLayout->addWidget(m_snappingGroup);
    
    // 创建高级设置组
    m_advancedGroup = new QGroupBox("高级设置", this);
    QFormLayout* advancedFormLayout = new QFormLayout(m_advancedGroup);
    
    m_enableHighlightCheck = new QCheckBox("启用高亮显示");
    m_enableHighlightCheck->setToolTip("拾取时高亮显示几何体");
    connect(m_enableHighlightCheck, &QCheckBox::toggled, this, &PickingSystemDialog::onHighlightToggled);
    
    m_advancedInfoLabel = new QLabel();
    m_advancedInfoLabel->setWordWrap(true);
    m_advancedInfoLabel->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
    
    advancedFormLayout->addRow(m_enableHighlightCheck);
    advancedFormLayout->addRow("说明:", m_advancedInfoLabel);
    
    advancedLayout->addWidget(m_advancedGroup);
    advancedLayout->addStretch();
    
    // 添加选项卡
    m_tabWidget->addTab(basicTab, "基本设置");
    m_tabWidget->addTab(advancedTab, "高级设置");
    
    // 创建按钮
    createButtons();
    
    // 添加到主布局
    mainLayout->addWidget(m_tabWidget);
    
    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
}

void PickingSystemDialog::createPresetGroup()
{
    m_presetGroup = new QGroupBox("预设配置", this);
    QFormLayout* layout = new QFormLayout(m_presetGroup);
    
    m_presetCombo = new QComboBox();
    m_presetCombo->addItem("精确模式 (小容差)", 0);
    m_presetCombo->addItem("标准模式 (默认容差)", 1);
    m_presetCombo->addItem("宽松模式 (大容差)", 2);
    m_presetCombo->addItem("仅顶点模式", 3);
    m_presetCombo->addItem("仅面模式", 4);
    m_presetCombo->addItem("自定义配置", 5);
    
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PickingSystemDialog::onPresetConfigChanged);
    
    m_presetInfoLabel = new QLabel();
    m_presetInfoLabel->setWordWrap(true);
    m_presetInfoLabel->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
    
    layout->addRow("预设:", m_presetCombo);
    layout->addRow("说明:", m_presetInfoLabel);
}

void PickingSystemDialog::createToleranceGroup()
{
    m_toleranceGroup = new QGroupBox("拾取容差", this);
    QFormLayout* layout = new QFormLayout(m_toleranceGroup);
    
    m_pickRadiusSpin = new QDoubleSpinBox();
    m_pickRadiusSpin->setRange(1.0, 50.0);
    m_pickRadiusSpin->setDecimals(1);
    m_pickRadiusSpin->setSingleStep(1.0);
    m_pickRadiusSpin->setSuffix(" px");
    m_pickRadiusSpin->setToolTip("通用拾取半径（像素）");
    connect(m_pickRadiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PickingSystemDialog::onPickRadiusChanged);
    
    m_vertexPickRadiusSpin = new QDoubleSpinBox();
    m_vertexPickRadiusSpin->setRange(1.0, 50.0);
    m_vertexPickRadiusSpin->setDecimals(1);
    m_vertexPickRadiusSpin->setSingleStep(1.0);
    m_vertexPickRadiusSpin->setSuffix(" px");
    m_vertexPickRadiusSpin->setToolTip("顶点拾取半径（像素）");
    connect(m_vertexPickRadiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PickingSystemDialog::onVertexPickRadiusChanged);
    
    m_edgePickRadiusSpin = new QDoubleSpinBox();
    m_edgePickRadiusSpin->setRange(1.0, 20.0);
    m_edgePickRadiusSpin->setDecimals(1);
    m_edgePickRadiusSpin->setSingleStep(0.5);
    m_edgePickRadiusSpin->setSuffix(" px");
    m_edgePickRadiusSpin->setToolTip("边拾取半径（像素）");
    connect(m_edgePickRadiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PickingSystemDialog::onEdgePickRadiusChanged);
    
    m_toleranceInfoLabel = new QLabel();
    m_toleranceInfoLabel->setWordWrap(true);
    m_toleranceInfoLabel->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
    
    layout->addRow("通用半径:", m_pickRadiusSpin);
    layout->addRow("顶点半径:", m_vertexPickRadiusSpin);
    layout->addRow("边线半径:", m_edgePickRadiusSpin);
    layout->addRow("说明:", m_toleranceInfoLabel);
}

void PickingSystemDialog::createPriorityGroup()
{
    m_priorityGroup = new QGroupBox("拾取优先级", this);
    QVBoxLayout* mainLayout = new QVBoxLayout(m_priorityGroup);
    
    QHBoxLayout* checkLayout = new QHBoxLayout();
    
    m_pickVertexFirstCheck = new QCheckBox("优先拾取顶点");
    m_pickVertexFirstCheck->setToolTip("启用后，优先检测顶点拾取");
    connect(m_pickVertexFirstCheck, &QCheckBox::toggled, this, &PickingSystemDialog::onPickVertexFirstToggled);
    
    m_pickEdgeSecondCheck = new QCheckBox("其次拾取边线");
    m_pickEdgeSecondCheck->setToolTip("启用后，在顶点检测失败时检测边线拾取");
    connect(m_pickEdgeSecondCheck, &QCheckBox::toggled, this, &PickingSystemDialog::onPickEdgeSecondToggled);
    
    m_pickFaceLastCheck = new QCheckBox("最后拾取面");
    m_pickFaceLastCheck->setToolTip("启用后，在前两者检测失败时检测面拾取");
    connect(m_pickFaceLastCheck, &QCheckBox::toggled, this, &PickingSystemDialog::onPickFaceLastToggled);
    
    checkLayout->addWidget(m_pickVertexFirstCheck);
    checkLayout->addWidget(m_pickEdgeSecondCheck);
    checkLayout->addWidget(m_pickFaceLastCheck);
    checkLayout->addStretch();
    
    m_priorityInfoLabel = new QLabel();
    m_priorityInfoLabel->setWordWrap(true);
    m_priorityInfoLabel->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
    
    mainLayout->addLayout(checkLayout);
    mainLayout->addWidget(m_priorityInfoLabel);
}

void PickingSystemDialog::createIndicatorGroup()
{
    m_indicatorGroup = new QGroupBox("指示器设置", this);
    QFormLayout* layout = new QFormLayout(m_indicatorGroup);
    
    m_enableIndicatorCheck = new QCheckBox("显示拾取指示器");
    m_enableIndicatorCheck->setToolTip("拾取时显示几何形状指示器");
    connect(m_enableIndicatorCheck, &QCheckBox::toggled, this, &PickingSystemDialog::onIndicatorToggled);
    
    m_indicatorSizeSpin = new QDoubleSpinBox();
    m_indicatorSizeSpin->setRange(0.1, 2.0);
    m_indicatorSizeSpin->setDecimals(2);
    m_indicatorSizeSpin->setSingleStep(0.1);
    m_indicatorSizeSpin->setToolTip("指示器的基础大小（会根据视距自动缩放）");
    connect(m_indicatorSizeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PickingSystemDialog::onIndicatorSizeChanged);
    
    m_indicatorInfoLabel = new QLabel();
    m_indicatorInfoLabel->setWordWrap(true);
    m_indicatorInfoLabel->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
    
    layout->addRow(m_enableIndicatorCheck);
    layout->addRow("指示器大小:", m_indicatorSizeSpin);
    layout->addRow("说明:", m_indicatorInfoLabel);
}

void PickingSystemDialog::createSnappingGroup()
{
    m_snappingGroup = new QGroupBox("吸附设置", this);
    QFormLayout* layout = new QFormLayout(m_snappingGroup);
    
    m_enableSnappingCheck = new QCheckBox("启用吸附功能");
    m_enableSnappingCheck->setToolTip("启用后，鼠标将吸附到最近的几何特征");
    connect(m_enableSnappingCheck, &QCheckBox::toggled, this, &PickingSystemDialog::onSnappingToggled);
    
    m_snapThresholdSpin = new QDoubleSpinBox();
    m_snapThresholdSpin->setRange(0.01, 1.0);
    m_snapThresholdSpin->setDecimals(3);
    m_snapThresholdSpin->setSingleStep(0.01);
    m_snapThresholdSpin->setSuffix(" m");
    m_snapThresholdSpin->setToolTip("吸附阈值（世界坐标单位）");
    connect(m_snapThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PickingSystemDialog::onSnapThresholdChanged);
    
    m_snappingInfoLabel = new QLabel();
    m_snappingInfoLabel->setWordWrap(true);
    m_snappingInfoLabel->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
    
    layout->addRow(m_enableSnappingCheck);
    layout->addRow("吸附阈值:", m_snapThresholdSpin);
    layout->addRow("说明:", m_snappingInfoLabel);
}

void PickingSystemDialog::createButtons()
{
    m_applyButton = new QPushButton("应用");
    m_applyButton->setDefault(true);
    connect(m_applyButton, &QPushButton::clicked, this, &PickingSystemDialog::onApplyClicked);
    
    m_resetButton = new QPushButton("重置");
    connect(m_resetButton, &QPushButton::clicked, this, &PickingSystemDialog::onResetClicked);
    
    m_cancelButton = new QPushButton("取消");
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void PickingSystemDialog::updateFromPickingSystem()
{
    if (!m_pickingManager->isInitialized()) {
        return;
    }
    
    m_updating = true;
    
    updateToleranceUI();
    updatePriorityUI();
    updateIndicatorUI();
    updateSnappingUI();
    updatePresetCombo();
    
    m_updating = false;
}

void PickingSystemDialog::updateToleranceUI()
{
    const PickConfig& config = m_pickingManager->getConfig();
    
    m_pickRadiusSpin->setValue(config.pickRadius);
    m_vertexPickRadiusSpin->setValue(config.vertexPickRadius);
    m_edgePickRadiusSpin->setValue(config.edgePickRadius);
    
    QString info = QString("当前设置: 通用半径=%1px, 顶点半径=%2px, 边线半径=%3px")
        .arg(config.pickRadius, 0, 'f', 1)
        .arg(config.vertexPickRadius, 0, 'f', 1)
        .arg(config.edgePickRadius, 0, 'f', 1);
    m_toleranceInfoLabel->setText(info);
}

void PickingSystemDialog::updatePriorityUI()
{
    const PickConfig& config = m_pickingManager->getConfig();
    
    m_pickVertexFirstCheck->setChecked(config.pickVertexFirst);
    m_pickEdgeSecondCheck->setChecked(config.pickEdgeSecond);
    m_pickFaceLastCheck->setChecked(config.pickFaceLast);
    
    QStringList enabledTypes;
    if (config.pickVertexFirst) enabledTypes << "顶点";
    if (config.pickEdgeSecond) enabledTypes << "边线";
    if (config.pickFaceLast) enabledTypes << "面";
    
    QString info = QString("当前启用: %1").arg(enabledTypes.join(", "));
    if (enabledTypes.isEmpty()) info = "当前未启用任何拾取类型";
    m_priorityInfoLabel->setText(info);
}

void PickingSystemDialog::updateIndicatorUI()
{
    const PickConfig& config = m_pickingManager->getConfig();
    
    m_enableIndicatorCheck->setChecked(config.enableIndicator);
    m_indicatorSizeSpin->setValue(config.indicatorSize);
    m_indicatorSizeSpin->setEnabled(config.enableIndicator);
    
    QString info = config.enableIndicator ? 
        QString("指示器已启用，大小: %1").arg(config.indicatorSize, 0, 'f', 2) :
        "指示器已禁用";
    m_indicatorInfoLabel->setText(info);
}

void PickingSystemDialog::updateSnappingUI()
{
    const PickConfig& config = m_pickingManager->getConfig();
    
    m_enableSnappingCheck->setChecked(config.enableSnapping);
    m_snapThresholdSpin->setValue(config.snapThreshold);
    m_snapThresholdSpin->setEnabled(config.enableSnapping);
    
    QString info = config.enableSnapping ? 
        QString("吸附已启用，阈值: %1m").arg(config.snapThreshold, 0, 'f', 3) :
        "吸附已禁用";
    m_snappingInfoLabel->setText(info);
    
    // 更新高级设置
    m_enableHighlightCheck->setChecked(config.enableHighlight);
    QString advancedInfo = config.enableHighlight ? "高亮显示已启用" : "高亮显示已禁用";
    m_advancedInfoLabel->setText(advancedInfo);
}

void PickingSystemDialog::updatePresetCombo()
{
    const PickConfig& config = m_pickingManager->getConfig();
    
    // 根据当前配置判断预设类型
    int presetIndex = 5; // 默认自定义
    
    // 精确模式：小容差
    if (config.pickRadius <= 3.0 && config.vertexPickRadius <= 5.0 && config.edgePickRadius <= 2.0) {
        presetIndex = 0;
    }
    // 标准模式：默认容差
    else if (config.pickRadius == 5.0 && config.vertexPickRadius == 8.0 && config.edgePickRadius == 3.0) {
        presetIndex = 1;
    }
    // 宽松模式：大容差
    else if (config.pickRadius >= 8.0 && config.vertexPickRadius >= 12.0 && config.edgePickRadius >= 6.0) {
        presetIndex = 2;
    }
    // 仅顶点模式
    else if (config.pickVertexFirst && !config.pickEdgeSecond && !config.pickFaceLast) {
        presetIndex = 3;
    }
    // 仅面模式
    else if (!config.pickVertexFirst && !config.pickEdgeSecond && config.pickFaceLast) {
        presetIndex = 4;
    }
    
    m_presetCombo->setCurrentIndex(presetIndex);
    
    // 更新预设信息
    QString presetInfo;
    switch (presetIndex) {
        case 0: presetInfo = "精确拾取，适合精细操作"; break;
        case 1: presetInfo = "标准拾取，适合一般操作"; break;
        case 2: presetInfo = "宽松拾取，适合快速操作"; break;
        case 3: presetInfo = "仅拾取顶点，适合点编辑"; break;
        case 4: presetInfo = "仅拾取面，适合面操作"; break;
        case 5: presetInfo = "自定义配置"; break;
    }
    m_presetInfoLabel->setText(presetInfo);
}

// 槽函数实现
void PickingSystemDialog::onPresetConfigChanged(int index)
{
    if (m_updating) return;
    
    PickConfig config = m_pickingManager->getConfig();
    
    switch (index) {
        case 0: // 精确模式
            config.pickRadius = 2.0;
            config.vertexPickRadius = 4.0;
            config.edgePickRadius = 1.5;
            config.pickVertexFirst = true;
            config.pickEdgeSecond = true;
            config.pickFaceLast = true;
            break;
        case 1: // 标准模式
            config.pickRadius = 5.0;
            config.vertexPickRadius = 8.0;
            config.edgePickRadius = 3.0;
            config.pickVertexFirst = true;
            config.pickEdgeSecond = true;
            config.pickFaceLast = true;
            break;
        case 2: // 宽松模式
            config.pickRadius = 10.0;
            config.vertexPickRadius = 15.0;
            config.edgePickRadius = 8.0;
            config.pickVertexFirst = true;
            config.pickEdgeSecond = true;
            config.pickFaceLast = true;
            break;
        case 3: // 仅顶点模式
            config.pickVertexFirst = true;
            config.pickEdgeSecond = false;
            config.pickFaceLast = false;
            break;
        case 4: // 仅面模式
            config.pickVertexFirst = false;
            config.pickEdgeSecond = false;
            config.pickFaceLast = true;
            break;
        case 5: // 自定义
        default:
            return; // 不改变配置
    }
    
    m_pickingManager->setConfig(config);
    updateFromPickingSystem();
}

void PickingSystemDialog::onPickRadiusChanged(double value)
{
    if (m_updating) return;
    PickConfig config = m_pickingManager->getConfig();
    config.pickRadius = static_cast<float>(value);
    m_pickingManager->setConfig(config);
    updateToleranceUI();
}

void PickingSystemDialog::onVertexPickRadiusChanged(double value)
{
    if (m_updating) return;
    PickConfig config = m_pickingManager->getConfig();
    config.vertexPickRadius = static_cast<float>(value);
    m_pickingManager->setConfig(config);
    updateToleranceUI();
}

void PickingSystemDialog::onEdgePickRadiusChanged(double value)
{
    if (m_updating) return;
    PickConfig config = m_pickingManager->getConfig();
    config.edgePickRadius = static_cast<float>(value);
    m_pickingManager->setConfig(config);
    updateToleranceUI();
}

void PickingSystemDialog::onSnapThresholdChanged(double value)
{
    if (m_updating) return;
    PickConfig config = m_pickingManager->getConfig();
    config.snapThreshold = static_cast<float>(value);
    m_pickingManager->setConfig(config);
    updateSnappingUI();
}

void PickingSystemDialog::onSnappingToggled(bool enabled)
{
    if (m_updating) return;
    PickConfig config = m_pickingManager->getConfig();
    config.enableSnapping = enabled;
    m_pickingManager->setConfig(config);
    updateSnappingUI();
}

void PickingSystemDialog::onIndicatorToggled(bool enabled)
{
    if (m_updating) return;
    PickConfig config = m_pickingManager->getConfig();
    config.enableIndicator = enabled;
    m_pickingManager->setConfig(config);
    updateIndicatorUI();
}

void PickingSystemDialog::onHighlightToggled(bool enabled)
{
    if (m_updating) return;
    PickConfig config = m_pickingManager->getConfig();
    config.enableHighlight = enabled;
    m_pickingManager->setConfig(config);
    updateSnappingUI(); // 这里复用了snapping的更新函数
}

void PickingSystemDialog::onPickVertexFirstToggled(bool enabled)
{
    if (m_updating) return;
    PickConfig config = m_pickingManager->getConfig();
    config.pickVertexFirst = enabled;
    m_pickingManager->setConfig(config);
    updatePriorityUI();
}

void PickingSystemDialog::onPickEdgeSecondToggled(bool enabled)
{
    if (m_updating) return;
    PickConfig config = m_pickingManager->getConfig();
    config.pickEdgeSecond = enabled;
    m_pickingManager->setConfig(config);
    updatePriorityUI();
}

void PickingSystemDialog::onPickFaceLastToggled(bool enabled)
{
    if (m_updating) return;
    PickConfig config = m_pickingManager->getConfig();
    config.pickFaceLast = enabled;
    m_pickingManager->setConfig(config);
    updatePriorityUI();
}

void PickingSystemDialog::onIndicatorSizeChanged(double value)
{
    if (m_updating) return;
    PickConfig config = m_pickingManager->getConfig();
    config.indicatorSize = static_cast<float>(value);
    m_pickingManager->setConfig(config);
    updateIndicatorUI();
}

void PickingSystemDialog::onApplyClicked()
{
    // 验证配置
    const PickConfig& config = m_pickingManager->getConfig();
    
    if (!config.pickVertexFirst && !config.pickEdgeSecond && !config.pickFaceLast) {
        QMessageBox::warning(this, "配置错误", "至少需要启用一种拾取类型！");
        return;
    }
    
    if (config.pickRadius <= 0 || config.vertexPickRadius <= 0 || config.edgePickRadius <= 0) {
        QMessageBox::warning(this, "配置错误", "拾取半径必须大于0！");
        return;
    }
    
    accept();
}

void PickingSystemDialog::onResetClicked()
{
    int ret = QMessageBox::question(this, "重置确认", 
        "确定要重置为默认设置吗？\n这将恢复标准拾取模式的所有设置。",
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        PickConfig config;
        // 使用默认的标准模式配置
        config.pickRadius = 5.0f;
        config.vertexPickRadius = 8.0f;
        config.edgePickRadius = 3.0f;
        config.snapThreshold = 0.15f;
        config.enableSnapping = true;
        config.enableIndicator = true;
        config.enableHighlight = true;
        config.indicatorSize = 0.2f;
        config.pickVertexFirst = true;
        config.pickEdgeSecond = true;
        config.pickFaceLast = true;
        
        m_pickingManager->setConfig(config);
        updateFromPickingSystem();
    }
} 