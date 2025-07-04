#include "CoordinateSystemDialog.h"
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QLineEdit>

CoordinateSystemDialog::CoordinateSystemDialog(QWidget* parent)
    : QDialog(parent)
    , m_coordSystem(CoordinateSystem3D::getInstance())
    , m_updating(false)
{
    setWindowTitle("坐标系统设置");
    setModal(true);
    
    // 设置窗口大小 - 使用选项卡后可以更紧凑
    resize(500, 400);
    
    // 居中显示
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    setupUI();
    updateFromCoordinateSystem();
    
    // 连接坐标系统信号
    connect(m_coordSystem, &CoordinateSystem3D::coordinateRangeChanged,
            this, &CoordinateSystemDialog::updateFromCoordinateSystem);
    connect(m_coordSystem, &CoordinateSystem3D::skyboxRangeChanged,
            this, &CoordinateSystemDialog::updateFromCoordinateSystem);
    
    // 连接坐标系相关信号
    connect(m_coordSystem, &CoordinateSystem3D::coordinateSystemTypeChanged,
            this, &CoordinateSystemDialog::updateFromCoordinateSystem);
    connect(m_coordSystem, &CoordinateSystem3D::axisVisibleChanged,
            this, &CoordinateSystemDialog::updateFromCoordinateSystem);
    connect(m_coordSystem, &CoordinateSystem3D::gridVisibleChanged,
            this, &CoordinateSystemDialog::updateFromCoordinateSystem);
    connect(m_coordSystem, &CoordinateSystem3D::gridPlaneVisibleChanged,
            this, &CoordinateSystemDialog::updateFromCoordinateSystem);
    connect(m_coordSystem, &CoordinateSystem3D::scaleUnitChanged,
            this, &CoordinateSystemDialog::updateFromCoordinateSystem);
    connect(m_coordSystem, &CoordinateSystem3D::customUnitNameChanged,
            this, &CoordinateSystemDialog::updateFromCoordinateSystem);
    connect(m_coordSystem, &CoordinateSystem3D::scaleIntervalChanged,
            this, &CoordinateSystemDialog::updateFromCoordinateSystem);
    connect(m_coordSystem, &CoordinateSystem3D::axisLengthChanged,
            this, &CoordinateSystemDialog::updateFromCoordinateSystem);
    connect(m_coordSystem, &CoordinateSystem3D::axisThicknessChanged,
            this, &CoordinateSystemDialog::updateFromCoordinateSystem);
    connect(m_coordSystem, &CoordinateSystem3D::gridSpacingChanged,
            this, &CoordinateSystemDialog::updateFromCoordinateSystem);
    connect(m_coordSystem, &CoordinateSystem3D::gridThicknessChanged,
            this, &CoordinateSystemDialog::updateFromCoordinateSystem);
    connect(m_coordSystem, &CoordinateSystem3D::fontSizeChanged,
            this, &CoordinateSystemDialog::updateFromCoordinateSystem);
    connect(m_coordSystem, &CoordinateSystem3D::customFontSizeChanged,
            this, &CoordinateSystemDialog::updateFromCoordinateSystem);
}

void CoordinateSystemDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // 创建选项卡控件
    m_tabWidget = new QTabWidget(this);
    
    // 创建各个选项卡页面
    QWidget* rangeTab = new QWidget();
    QWidget* coordinateTab = new QWidget();
    QWidget* optionsTab = new QWidget();
    
    // 设置范围选项卡
    QVBoxLayout* rangeLayout = new QVBoxLayout(rangeTab);
    createPresetGroup();
    createCoordinateGroup();
    createSkyboxGroup();
    rangeLayout->addWidget(m_presetGroup);
    rangeLayout->addWidget(m_coordinateGroup);
    rangeLayout->addWidget(m_skyboxGroup);
    rangeLayout->addStretch();
    
    // 设置坐标系选项卡
    QVBoxLayout* coordinateLayout = new QVBoxLayout(coordinateTab);
    createCoordinateSystemGroup();
    coordinateLayout->addWidget(m_coordinateSystemGroup);
    coordinateLayout->addStretch();
    
    // 设置选项选项卡
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsTab);
    createOptionsGroup();
    optionsLayout->addWidget(m_optionsGroup);
    optionsLayout->addStretch();
    
    // 添加选项卡
    m_tabWidget->addTab(rangeTab, "范围设置");
    m_tabWidget->addTab(coordinateTab, "坐标系设置");
    m_tabWidget->addTab(optionsTab, "选项");
    
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

void CoordinateSystemDialog::createPresetGroup()
{
    m_presetGroup = new QGroupBox("预设范围", this);
    QFormLayout* layout = new QFormLayout(m_presetGroup);
    
    m_presetCombo = new QComboBox();
    m_presetCombo->addItem("小范围 (1km)", CoordinateSystem3D::Range_Small);
    m_presetCombo->addItem("中等范围 (100km)", CoordinateSystem3D::Range_Medium);
    m_presetCombo->addItem("大范围 (1000km)", CoordinateSystem3D::Range_Large);
    m_presetCombo->addItem("城市范围 (50km)", CoordinateSystem3D::Range_City);
    m_presetCombo->addItem("国家范围 (5000km)", CoordinateSystem3D::Range_Country);
    m_presetCombo->addItem("大陆范围 (10000km)", CoordinateSystem3D::Range_Continent);
    m_presetCombo->addItem("地球范围 (12742km)", CoordinateSystem3D::Range_Earth);
    m_presetCombo->addItem("自定义范围", CoordinateSystem3D::Range_Custom);
    
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CoordinateSystemDialog::onPresetRangeChanged);
    
    m_presetInfoLabel = new QLabel();
    m_presetInfoLabel->setWordWrap(true);
    m_presetInfoLabel->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
    
    layout->addRow("预设:", m_presetCombo);
    layout->addRow("说明:", m_presetInfoLabel);
}

void CoordinateSystemDialog::createCoordinateGroup()
{
    m_coordinateGroup = new QGroupBox("坐标范围", this);
    QVBoxLayout* mainLayout = new QVBoxLayout(m_coordinateGroup);
    
    // 创建双精度浮点数输入框
    m_minXSpin = new QDoubleSpinBox();
    m_minXSpin->setRange(-1e10, 1e10);
    m_minXSpin->setDecimals(2);
    m_minXSpin->setSingleStep(1000);
    m_minXSpin->setSuffix(" m");
    
    m_maxXSpin = new QDoubleSpinBox();
    m_maxXSpin->setRange(-1e10, 1e10);
    m_maxXSpin->setDecimals(2);
    m_maxXSpin->setSingleStep(1000);
    m_maxXSpin->setSuffix(" m");
    
    m_minYSpin = new QDoubleSpinBox();
    m_minYSpin->setRange(-1e10, 1e10);
    m_minYSpin->setDecimals(2);
    m_minYSpin->setSingleStep(1000);
    m_minYSpin->setSuffix(" m");
    
    m_maxYSpin = new QDoubleSpinBox();
    m_maxYSpin->setRange(-1e10, 1e10);
    m_maxYSpin->setDecimals(2);
    m_maxYSpin->setSingleStep(1000);
    m_maxYSpin->setSuffix(" m");
    
    m_minZSpin = new QDoubleSpinBox();
    m_minZSpin->setRange(-1e10, 1e10);
    m_minZSpin->setDecimals(2);
    m_minZSpin->setSingleStep(1000);
    m_minZSpin->setSuffix(" m");
    
    m_maxZSpin = new QDoubleSpinBox();
    m_maxZSpin->setRange(-1e10, 1e10);
    m_maxZSpin->setDecimals(2);
    m_maxZSpin->setSingleStep(1000);
    m_maxZSpin->setSuffix(" m");
    
    // 连接信号
    connect(m_minXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onCoordinateRangeChanged);
    connect(m_maxXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onCoordinateRangeChanged);
    connect(m_minYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onCoordinateRangeChanged);
    connect(m_maxYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onCoordinateRangeChanged);
    connect(m_minZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onCoordinateRangeChanged);
    connect(m_maxZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onCoordinateRangeChanged);
    
    // 使用两列布局
    QHBoxLayout* coordLayout = new QHBoxLayout();
    
    // 左列：X和Y坐标
    QFormLayout* leftLayout = new QFormLayout();
    leftLayout->addRow("X最小值:", m_minXSpin);
    leftLayout->addRow("X最大值:", m_maxXSpin);
    leftLayout->addRow("Y最小值:", m_minYSpin);
    leftLayout->addRow("Y最大值:", m_maxYSpin);
    
    // 右列：Z坐标
    QFormLayout* rightLayout = new QFormLayout();
    rightLayout->addRow("Z最小值:", m_minZSpin);
    rightLayout->addRow("Z最大值:", m_maxZSpin);
    
    coordLayout->addLayout(leftLayout);
    coordLayout->addLayout(rightLayout);
    
    m_coordinateInfoLabel = new QLabel();
    m_coordinateInfoLabel->setWordWrap(true);
    m_coordinateInfoLabel->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
    
    mainLayout->addLayout(coordLayout);
    mainLayout->addWidget(m_coordinateInfoLabel);
}

void CoordinateSystemDialog::createSkyboxGroup()
{
    m_skyboxGroup = new QGroupBox("天空盒范围", this);
    QVBoxLayout* mainLayout = new QVBoxLayout(m_skyboxGroup);
    
    // 创建天空盒范围输入框
    m_skyboxMinXSpin = new QDoubleSpinBox();
    m_skyboxMinXSpin->setRange(-1e10, 1e10);
    m_skyboxMinXSpin->setDecimals(2);
    m_skyboxMinXSpin->setSingleStep(1000);
    m_skyboxMinXSpin->setSuffix(" m");
    
    m_skyboxMaxXSpin = new QDoubleSpinBox();
    m_skyboxMaxXSpin->setRange(-1e10, 1e10);
    m_skyboxMaxXSpin->setDecimals(2);
    m_skyboxMaxXSpin->setSingleStep(1000);
    m_skyboxMaxXSpin->setSuffix(" m");
    
    m_skyboxMinYSpin = new QDoubleSpinBox();
    m_skyboxMinYSpin->setRange(-1e10, 1e10);
    m_skyboxMinYSpin->setDecimals(2);
    m_skyboxMinYSpin->setSingleStep(1000);
    m_skyboxMinYSpin->setSuffix(" m");
    
    m_skyboxMaxYSpin = new QDoubleSpinBox();
    m_skyboxMaxYSpin->setRange(-1e10, 1e10);
    m_skyboxMaxYSpin->setDecimals(2);
    m_skyboxMaxYSpin->setSingleStep(1000);
    m_skyboxMaxYSpin->setSuffix(" m");
    
    m_skyboxMinZSpin = new QDoubleSpinBox();
    m_skyboxMinZSpin->setRange(-1e10, 1e10);
    m_skyboxMinZSpin->setDecimals(2);
    m_skyboxMinZSpin->setSingleStep(1000);
    m_skyboxMinZSpin->setSuffix(" m");
    
    m_skyboxMaxZSpin = new QDoubleSpinBox();
    m_skyboxMaxZSpin->setRange(-1e10, 1e10);
    m_skyboxMaxZSpin->setDecimals(2);
    m_skyboxMaxZSpin->setSingleStep(1000);
    m_skyboxMaxZSpin->setSuffix(" m");
    
    // 连接信号
    connect(m_skyboxMinXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onSkyboxRangeChanged);
    connect(m_skyboxMaxXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onSkyboxRangeChanged);
    connect(m_skyboxMinYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onSkyboxRangeChanged);
    connect(m_skyboxMaxYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onSkyboxRangeChanged);
    connect(m_skyboxMinZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onSkyboxRangeChanged);
    connect(m_skyboxMaxZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onSkyboxRangeChanged);
    
    // 使用两列布局
    QHBoxLayout* skyboxLayout = new QHBoxLayout();
    
    // 左列：X和Y坐标
    QFormLayout* leftLayout = new QFormLayout();
    leftLayout->addRow("X最小值:", m_skyboxMinXSpin);
    leftLayout->addRow("X最大值:", m_skyboxMaxXSpin);
    leftLayout->addRow("Y最小值:", m_skyboxMinYSpin);
    leftLayout->addRow("Y最大值:", m_skyboxMaxYSpin);
    
    // 右列：Z坐标
    QFormLayout* rightLayout = new QFormLayout();
    rightLayout->addRow("Z最小值:", m_skyboxMinZSpin);
    rightLayout->addRow("Z最大值:", m_skyboxMaxZSpin);
    
    skyboxLayout->addLayout(leftLayout);
    skyboxLayout->addLayout(rightLayout);
    
    m_skyboxInfoLabel = new QLabel();
    m_skyboxInfoLabel->setWordWrap(true);
    m_skyboxInfoLabel->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
    
    mainLayout->addLayout(skyboxLayout);
    mainLayout->addWidget(m_skyboxInfoLabel);
}

void CoordinateSystemDialog::createCoordinateSystemGroup()
{
    m_coordinateSystemGroup = new QGroupBox("坐标系设置", this);
    QVBoxLayout* mainLayout = new QVBoxLayout(m_coordinateSystemGroup);
    
    // 第一行：坐标系类型和网格可见性
    QHBoxLayout* firstRow = new QHBoxLayout();
    
    // 坐标系类型选择
    QFormLayout* typeLayout = new QFormLayout();
    m_coordSystemTypeCombo = new QComboBox();
    m_coordSystemTypeCombo->addItem("无坐标系", CoordSystem_None3D);
    m_coordSystemTypeCombo->addItem("光轴线", CoordSystem_Axis3D);
    m_coordSystemTypeCombo->addItem("网格线", CoordSystem_Grid3D);
    m_coordSystemTypeCombo->addItem("光轴线+网格线", CoordSystem_Both3D);
    connect(m_coordSystemTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CoordinateSystemDialog::onCoordinateSystemTypeChanged);
    typeLayout->addRow("坐标系类型:", m_coordSystemTypeCombo);
    
    // 网格可见性
    m_gridVisibleCheck = new QCheckBox("显示网格");
    m_gridVisibleCheck->setToolTip("显示坐标网格线");
    connect(m_gridVisibleCheck, &QCheckBox::toggled, this, &CoordinateSystemDialog::onGridVisibleToggled);
    
    // 网格平面选择
    m_gridXYCheck = new QCheckBox("XY平面");
    m_gridXYCheck->setToolTip("显示XY平面网格");
    connect(m_gridXYCheck, &QCheckBox::toggled, this, &CoordinateSystemDialog::onGridPlaneToggled);
    
    m_gridYZCheck = new QCheckBox("YZ平面");
    m_gridYZCheck->setToolTip("显示YZ平面网格");
    connect(m_gridYZCheck, &QCheckBox::toggled, this, &CoordinateSystemDialog::onGridPlaneToggled);
    
    m_gridXZCheck = new QCheckBox("XZ平面");
    m_gridXZCheck->setToolTip("显示XZ平面网格");
    connect(m_gridXZCheck, &QCheckBox::toggled, this, &CoordinateSystemDialog::onGridPlaneToggled);
    
    firstRow->addLayout(typeLayout);
    firstRow->addWidget(m_gridVisibleCheck);
    firstRow->addStretch();
    
    // 第二行：坐标轴可见性和网格平面选择
    QHBoxLayout* secondRow = new QHBoxLayout();
    QLabel* axisLabel = new QLabel("坐标轴:");
    m_axisXCheck = new QCheckBox("X轴");
    m_axisXCheck->setToolTip("显示X轴光轴线");
    connect(m_axisXCheck, &QCheckBox::toggled, this, &CoordinateSystemDialog::onAxisVisibleToggled);
    
    m_axisYCheck = new QCheckBox("Y轴");
    m_axisYCheck->setToolTip("显示Y轴光轴线");
    connect(m_axisYCheck, &QCheckBox::toggled, this, &CoordinateSystemDialog::onAxisVisibleToggled);
    
    m_axisZCheck = new QCheckBox("Z轴");
    m_axisZCheck->setToolTip("显示Z轴光轴线");
    connect(m_axisZCheck, &QCheckBox::toggled, this, &CoordinateSystemDialog::onAxisVisibleToggled);
    
    QLabel* gridPlaneLabel = new QLabel("网格平面:");
    
    secondRow->addWidget(axisLabel);
    secondRow->addWidget(m_axisXCheck);
    secondRow->addWidget(m_axisYCheck);
    secondRow->addWidget(m_axisZCheck);
    secondRow->addSpacing(20);
    secondRow->addWidget(gridPlaneLabel);
    secondRow->addWidget(m_gridXYCheck);
    secondRow->addWidget(m_gridYZCheck);
    secondRow->addWidget(m_gridXZCheck);
    secondRow->addStretch();
    
    // 第三行：刻度单位和自定义单位
    QHBoxLayout* thirdRow = new QHBoxLayout();
    QFormLayout* unitLayout = new QFormLayout();
    m_scaleUnitCombo = new QComboBox();
    m_scaleUnitCombo->addItem("米 (m)", Unit_Meter3D);
    m_scaleUnitCombo->addItem("千米 (km)", Unit_Kilometer3D);
    m_scaleUnitCombo->addItem("厘米 (cm)", Unit_Centimeter3D);
    m_scaleUnitCombo->addItem("毫米 (mm)", Unit_Millimeter3D);
    m_scaleUnitCombo->addItem("自定义", Unit_Custom3D);
    connect(m_scaleUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CoordinateSystemDialog::onScaleUnitChanged);
    unitLayout->addRow("刻度单位:", m_scaleUnitCombo);
    
    m_customUnitEdit = new QLineEdit();
    m_customUnitEdit->setPlaceholderText("输入自定义单位名称");
    m_customUnitEdit->setToolTip("当选择自定义单位时，在此输入单位名称");
    connect(m_customUnitEdit, &QLineEdit::textChanged,
            this, &CoordinateSystemDialog::onCustomUnitNameChanged);
    unitLayout->addRow("自定义单位:", m_customUnitEdit);
    
    thirdRow->addLayout(unitLayout);
    thirdRow->addStretch();
    
    // 第四行：刻度间隔和轴长度
    QHBoxLayout* fourthRow = new QHBoxLayout();
    QFormLayout* intervalLayout = new QFormLayout();
    m_scaleIntervalSpin = new QDoubleSpinBox();
    m_scaleIntervalSpin->setRange(1.0, 1e6);
    m_scaleIntervalSpin->setDecimals(2);
    m_scaleIntervalSpin->setSingleStep(100.0);
    m_scaleIntervalSpin->setSuffix(" m");
    m_scaleIntervalSpin->setToolTip("坐标轴上的刻度间隔");
    connect(m_scaleIntervalSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onScaleIntervalChanged);
    intervalLayout->addRow("刻度间隔:", m_scaleIntervalSpin);
    
    m_axisLengthSpin = new QDoubleSpinBox();
    m_axisLengthSpin->setRange(100.0, 1e8);
    m_axisLengthSpin->setDecimals(2);
    m_axisLengthSpin->setSingleStep(1000.0);
    m_axisLengthSpin->setSuffix(" m");
    m_axisLengthSpin->setToolTip("坐标轴的长度（会自动根据天空盒范围调整）");
    connect(m_axisLengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onAxisLengthChanged);
    intervalLayout->addRow("轴长度:", m_axisLengthSpin);
    
    fourthRow->addLayout(intervalLayout);
    fourthRow->addStretch();
    
    // 第五行：轴粗细和网格间距
    QHBoxLayout* fifthRow = new QHBoxLayout();
    QFormLayout* thicknessLayout = new QFormLayout();
    m_axisThicknessSpin = new QDoubleSpinBox();
    m_axisThicknessSpin->setRange(0.1, 10.0);
    m_axisThicknessSpin->setDecimals(1);
    m_axisThicknessSpin->setSingleStep(0.5);
    m_axisThicknessSpin->setSuffix(" m");
    m_axisThicknessSpin->setToolTip("坐标轴的粗细");
    connect(m_axisThicknessSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onAxisThicknessChanged);
    thicknessLayout->addRow("轴粗细:", m_axisThicknessSpin);
    
    m_gridSpacingSpin = new QDoubleSpinBox();
    m_gridSpacingSpin->setRange(10.0, 1e5);
    m_gridSpacingSpin->setDecimals(2);
    m_gridSpacingSpin->setSingleStep(100.0);
    m_gridSpacingSpin->setSuffix(" m");
    m_gridSpacingSpin->setToolTip("网格线的间距");
    connect(m_gridSpacingSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onGridSpacingChanged);
    thicknessLayout->addRow("网格间距:", m_gridSpacingSpin);
    
    fifthRow->addLayout(thicknessLayout);
    fifthRow->addStretch();
    
    // 第六行：网格粗细和字体大小
    QHBoxLayout* sixthRow = new QHBoxLayout();
    QFormLayout* gridLayout = new QFormLayout();
    m_gridThicknessSpin = new QDoubleSpinBox();
    m_gridThicknessSpin->setRange(0.1, 5.0);
    m_gridThicknessSpin->setDecimals(1);
    m_gridThicknessSpin->setSingleStep(0.2);
    m_gridThicknessSpin->setSuffix(" m");
    m_gridThicknessSpin->setToolTip("网格线的粗细");
    connect(m_gridThicknessSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onGridThicknessChanged);
    gridLayout->addRow("网格粗细:", m_gridThicknessSpin);
    
    m_fontSizeCombo = new QComboBox();
    m_fontSizeCombo->addItem("小字体", FontSize_Small3D);
    m_fontSizeCombo->addItem("中等字体", FontSize_Medium3D);
    m_fontSizeCombo->addItem("大字体", FontSize_Large3D);
    m_fontSizeCombo->addItem("自定义", FontSize_Custom3D);
    connect(m_fontSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CoordinateSystemDialog::onFontSizeChanged);
    gridLayout->addRow("字体大小:", m_fontSizeCombo);
    
    sixthRow->addLayout(gridLayout);
    sixthRow->addStretch();
    
    // 第七行：自定义字体大小
    QHBoxLayout* seventhRow = new QHBoxLayout();
    QFormLayout* customFontLayout = new QFormLayout();
    m_customFontSizeSpin = new QDoubleSpinBox();
    m_customFontSizeSpin->setRange(10.0, 500.0);
    m_customFontSizeSpin->setDecimals(1);
    m_customFontSizeSpin->setSingleStep(10.0);
    m_customFontSizeSpin->setSuffix(" px");
    m_customFontSizeSpin->setToolTip("自定义字体大小");
    connect(m_customFontSizeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CoordinateSystemDialog::onCustomFontSizeChanged);
    customFontLayout->addRow("自定义字体大小:", m_customFontSizeSpin);
    
    seventhRow->addLayout(customFontLayout);
    seventhRow->addStretch();
    
    // 信息标签
    m_coordinateSystemInfoLabel = new QLabel();
    m_coordinateSystemInfoLabel->setWordWrap(true);
    m_coordinateSystemInfoLabel->setStyleSheet("QLabel { color: gray; font-size: 10px; }");
    
    // 添加到主布局
    mainLayout->addLayout(firstRow);
    mainLayout->addLayout(secondRow);
    mainLayout->addLayout(thirdRow);
    mainLayout->addLayout(fourthRow);
    mainLayout->addLayout(fifthRow);
    mainLayout->addLayout(sixthRow);
    mainLayout->addLayout(seventhRow);
    mainLayout->addWidget(m_coordinateSystemInfoLabel);
}

void CoordinateSystemDialog::createOptionsGroup()
{
    m_optionsGroup = new QGroupBox("选项", this);
    QFormLayout* layout = new QFormLayout(m_optionsGroup);
    
    m_rangeLimitCheck = new QCheckBox("启用坐标范围限制");
    m_rangeLimitCheck->setToolTip("启用后，超出坐标范围的点将被限制在范围内");
    connect(m_rangeLimitCheck, &QCheckBox::toggled,
            this, &CoordinateSystemDialog::onRangeLimitToggled);
    
    m_skyboxBindingCheck = new QCheckBox("绑定天空盒范围");
    m_skyboxBindingCheck->setToolTip("启用后，天空盒范围将自动跟随坐标范围变化");
    connect(m_skyboxBindingCheck, &QCheckBox::toggled,
            this, &CoordinateSystemDialog::onSkyboxBindingToggled);
    
    layout->addRow(m_rangeLimitCheck);
    layout->addRow(m_skyboxBindingCheck);
}

void CoordinateSystemDialog::createButtons()
{
    m_applyButton = new QPushButton("应用");
    m_applyButton->setDefault(true);
    connect(m_applyButton, &QPushButton::clicked, this, &CoordinateSystemDialog::onApplyClicked);
    
    m_resetButton = new QPushButton("重置");
    connect(m_resetButton, &QPushButton::clicked, this, &CoordinateSystemDialog::onResetClicked);
    
    m_cancelButton = new QPushButton("取消");
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void CoordinateSystemDialog::updateFromCoordinateSystem()
{
    m_updating = true;
    
    updateCoordinateRangeUI();
    updateSkyboxRangeUI();
    updateCoordinateSystemUI();
    updatePresetCombo();
    
    // 更新选项
    m_rangeLimitCheck->setChecked(m_coordSystem->isRangeLimitEnabled());
    m_skyboxBindingCheck->setChecked(m_coordSystem->isSkyboxRangeBinding());
    
    m_updating = false;
}

void CoordinateSystemDialog::updateCoordinateRangeUI()
{
    const CoordinateSystem3D::CoordinateRange& range = m_coordSystem->getCoordinateRange();
    
    m_minXSpin->setValue(range.minX);
    m_maxXSpin->setValue(range.maxX);
    m_minYSpin->setValue(range.minY);
    m_maxYSpin->setValue(range.maxY);
    m_minZSpin->setValue(range.minZ);
    m_maxZSpin->setValue(range.maxZ);
    
    m_coordinateInfoLabel->setText(m_coordSystem->getRangeInfo());
}

void CoordinateSystemDialog::updateSkyboxRangeUI()
{
    const CoordinateSystem3D::CoordinateRange& range = m_coordSystem->getSkyboxRange();
    
    m_skyboxMinXSpin->setValue(range.minX);
    m_skyboxMaxXSpin->setValue(range.maxX);
    m_skyboxMinYSpin->setValue(range.minY);
    m_skyboxMaxYSpin->setValue(range.maxY);
    m_skyboxMinZSpin->setValue(range.minZ);
    m_skyboxMaxZSpin->setValue(range.maxZ);
    
    m_skyboxInfoLabel->setText(m_coordSystem->getSkyboxRangeInfo());
}

void CoordinateSystemDialog::updatePresetCombo()
{
    // 根据当前范围确定预设类型
    const CoordinateSystem3D::CoordinateRange& range = m_coordSystem->getCoordinateRange();
    
    // 简单的范围匹配逻辑
    double maxRange = range.maxRange();
    int presetIndex = 7; // 默认自定义
    
    if (maxRange <= 1000) presetIndex = 0;      // 小范围
    else if (maxRange <= 100000) presetIndex = 1; // 中等范围
    else if (maxRange <= 1000000) presetIndex = 2; // 大范围
    else if (maxRange <= 50000) presetIndex = 3;   // 城市范围
    else if (maxRange <= 5000000) presetIndex = 4; // 国家范围
    else if (maxRange <= 10000000) presetIndex = 5; // 大陆范围
    else if (maxRange <= 12742000) presetIndex = 6; // 地球范围
    
    m_presetCombo->setCurrentIndex(presetIndex);
    
    // 更新预设信息
    if (presetIndex < 7)
    {
        QString presetName = m_presetCombo->itemText(presetIndex);
        m_presetInfoLabel->setText(QString("当前使用: %1").arg(presetName));
    }
    else
    {
        m_presetInfoLabel->setText("当前使用: 自定义范围");
    }
}

void CoordinateSystemDialog::updateCoordinateSystemUI()
{
    // 更新坐标系类型
    int typeIndex = static_cast<int>(m_coordSystem->getCoordinateSystemType());
    m_coordSystemTypeCombo->setCurrentIndex(typeIndex);
    
    // 更新轴可见性
    m_axisXCheck->setChecked(m_coordSystem->isAxisVisible(Axis_X3D));
    m_axisYCheck->setChecked(m_coordSystem->isAxisVisible(Axis_Y3D));
    m_axisZCheck->setChecked(m_coordSystem->isAxisVisible(Axis_Z3D));
    
    // 更新网格可见性
    m_gridVisibleCheck->setChecked(m_coordSystem->isGridVisible());
    
    // 更新网格平面选择
    m_gridXYCheck->setChecked(m_coordSystem->isGridPlaneVisible(GridPlane_XY3D));
    m_gridYZCheck->setChecked(m_coordSystem->isGridPlaneVisible(GridPlane_YZ3D));
    m_gridXZCheck->setChecked(m_coordSystem->isGridPlaneVisible(GridPlane_XZ3D));
    
    // 更新刻度单位
    int unitIndex = static_cast<int>(m_coordSystem->getScaleUnit());
    m_scaleUnitCombo->setCurrentIndex(unitIndex);
    
    // 更新自定义单位名称
    m_customUnitEdit->setText(m_coordSystem->getCustomUnitName());
    
    // 更新其他参数
    m_scaleIntervalSpin->setValue(m_coordSystem->getScaleInterval());
    m_axisLengthSpin->setValue(m_coordSystem->getAxisLength());
    m_axisThicknessSpin->setValue(m_coordSystem->getAxisThickness());
    m_gridSpacingSpin->setValue(m_coordSystem->getGridSpacing());
    m_gridThicknessSpin->setValue(m_coordSystem->getGridThickness());
    
    // 更新字体大小
    int fontSizeIndex = static_cast<int>(m_coordSystem->getFontSize());
    m_fontSizeCombo->setCurrentIndex(fontSizeIndex);
    m_customFontSizeSpin->setValue(m_coordSystem->getCustomFontSize());
    m_customFontSizeSpin->setEnabled(m_coordSystem->getFontSize() == FontSize_Custom3D);
    
    // 更新信息标签
    QString fontSizeText;
    switch (m_coordSystem->getFontSize())
    {
        case FontSize_Small3D: fontSizeText = "小字体"; break;
        case FontSize_Medium3D: fontSizeText = "中等字体"; break;
        case FontSize_Large3D: fontSizeText = "大字体"; break;
        case FontSize_Custom3D: fontSizeText = QString("自定义(%1px)").arg(m_coordSystem->getCustomFontSize()); break;
    }
    
    // 获取天空盒范围信息
    const CoordinateSystem3D::CoordinateRange& skyboxRange = m_coordSystem->getSkyboxRange();
    double skyboxMaxRange = skyboxRange.maxRange();
    double currentAxisLength = m_coordSystem->getAxisLength();
    double axisRatio = (currentAxisLength / skyboxMaxRange) * 100.0;
    
    QString info = QString("坐标系: %1, 单位: %2, 刻度间隔: %3%4, 字体: %5\n轴长度: %6m (天空盒的%7%)")
        .arg(m_coordSystemTypeCombo->currentText())
        .arg(m_coordSystem->getUnitName())
        .arg(m_coordSystem->getScaleInterval())
        .arg(m_coordSystem->getUnitName())
        .arg(fontSizeText)
        .arg(currentAxisLength, 0, 'f', 0)
        .arg(axisRatio, 0, 'f', 1);
    m_coordinateSystemInfoLabel->setText(info);
}

void CoordinateSystemDialog::onPresetRangeChanged(int index)
{
    if (m_updating) return;
    
    CoordinateSystem3D::PresetRange preset = static_cast<CoordinateSystem3D::PresetRange>(
        m_presetCombo->itemData(index).toInt());
    
    m_coordSystem->setPresetRange(preset);
}

void CoordinateSystemDialog::onRangeLimitToggled(bool enabled)
{
    if (m_updating) return;
    
    m_coordSystem->setRangeLimitEnabled(enabled);
}

void CoordinateSystemDialog::onSkyboxBindingToggled(bool enabled)
{
    if (m_updating) return;
    
    m_coordSystem->setSkyboxRangeBinding(enabled);
    
    // 如果启用绑定，更新天空盒范围
    if (enabled)
    {
        updateSkyboxRangeUI();
    }
}

void CoordinateSystemDialog::onCoordinateRangeChanged()
{
    if (m_updating) return;
    
    CoordinateSystem3D::CoordinateRange range(
        m_minXSpin->value(), m_maxXSpin->value(),
        m_minYSpin->value(), m_maxYSpin->value(),
        m_minZSpin->value(), m_maxZSpin->value()
    );
    
    m_coordSystem->setCoordinateRange(range);
}

void CoordinateSystemDialog::onSkyboxRangeChanged()
{
    if (m_updating) return;
    
    CoordinateSystem3D::CoordinateRange range(
        m_skyboxMinXSpin->value(), m_skyboxMaxXSpin->value(),
        m_skyboxMinYSpin->value(), m_skyboxMaxYSpin->value(),
        m_skyboxMinZSpin->value(), m_skyboxMaxZSpin->value()
    );
    
    m_coordSystem->setSkyboxRange(range);
}

void CoordinateSystemDialog::onCoordinateSystemTypeChanged(int index)
{
    if (m_updating) return;
    
    CoordinateSystemType3D type = static_cast<CoordinateSystemType3D>(
        m_coordSystemTypeCombo->itemData(index).toInt());
    
    m_coordSystem->setCoordinateSystemType(type);
}

void CoordinateSystemDialog::onAxisVisibleToggled(bool enabled)
{
    if (m_updating) return;
    
    // 根据发送信号的复选框确定是哪个轴
    if (sender() == m_axisXCheck)
    {
        m_coordSystem->setAxisVisible(Axis_X3D, enabled);
    }
    else if (sender() == m_axisYCheck)
    {
        m_coordSystem->setAxisVisible(Axis_Y3D, enabled);
    }
    else if (sender() == m_axisZCheck)
    {
        m_coordSystem->setAxisVisible(Axis_Z3D, enabled);
    }
}

void CoordinateSystemDialog::onGridVisibleToggled(bool enabled)
{
    if (m_updating) return;
    
    m_coordSystem->setGridVisible(enabled);
}

void CoordinateSystemDialog::onGridPlaneToggled(bool enabled)
{
    if (m_updating) return;
    
    if (sender() == m_gridXYCheck)
    {
        m_coordSystem->setGridPlaneVisible(GridPlane_XY3D, enabled);
    }
    else if (sender() == m_gridYZCheck)
    {
        m_coordSystem->setGridPlaneVisible(GridPlane_YZ3D, enabled);
    }
    else if (sender() == m_gridXZCheck)
    {
        m_coordSystem->setGridPlaneVisible(GridPlane_XZ3D, enabled);
    }
}

void CoordinateSystemDialog::onScaleUnitChanged(int index)
{
    if (m_updating) return;
    
    ScaleUnit3D unit = static_cast<ScaleUnit3D>(
        m_scaleUnitCombo->itemData(index).toInt());
    
    m_coordSystem->setScaleUnit(unit);
    
    // 如果选择自定义单位，启用自定义单位输入框
    m_customUnitEdit->setEnabled(unit == Unit_Custom3D);
}

void CoordinateSystemDialog::onCustomUnitNameChanged(const QString& text)
{
    if (m_updating) return;
    
    m_coordSystem->setCustomUnitName(text);
}

void CoordinateSystemDialog::onScaleIntervalChanged(double value)
{
    if (m_updating) return;
    
    m_coordSystem->setScaleInterval(value);
}

void CoordinateSystemDialog::onAxisLengthChanged(double value)
{
    if (m_updating) return;
    
    m_coordSystem->setAxisLength(value);
}

void CoordinateSystemDialog::onAxisThicknessChanged(double value)
{
    if (m_updating) return;
    
    m_coordSystem->setAxisThickness(value);
}

void CoordinateSystemDialog::onGridSpacingChanged(double value)
{
    if (m_updating) return;
    
    m_coordSystem->setGridSpacing(value);
}

void CoordinateSystemDialog::onGridThicknessChanged(double value)
{
    if (m_updating) return;
    
    m_coordSystem->setGridThickness(value);
}

void CoordinateSystemDialog::onFontSizeChanged(int index)
{
    if (m_updating) return;
    
    FontSize3D fontSize = static_cast<FontSize3D>(
        m_fontSizeCombo->itemData(index).toInt());
    
    m_coordSystem->setFontSize(fontSize);
    
    // 如果选择自定义字体大小，启用自定义字体大小输入框
    m_customFontSizeSpin->setEnabled(fontSize == FontSize_Custom3D);
}

void CoordinateSystemDialog::onCustomFontSizeChanged(double value)
{
    if (m_updating) return;
    
    m_coordSystem->setCustomFontSize(value);
}

void CoordinateSystemDialog::onApplyClicked()
{
    // 验证输入
    if (m_minXSpin->value() >= m_maxXSpin->value() ||
        m_minYSpin->value() >= m_maxYSpin->value() ||
        m_minZSpin->value() >= m_maxZSpin->value())
    {
        QMessageBox::warning(this, "输入错误", "坐标范围的最小值必须小于最大值！");
        return;
    }
    
    if (m_skyboxMinXSpin->value() >= m_skyboxMaxXSpin->value() ||
        m_skyboxMinYSpin->value() >= m_skyboxMaxYSpin->value() ||
        m_skyboxMinZSpin->value() >= m_skyboxMaxZSpin->value())
    {
        QMessageBox::warning(this, "输入错误", "天空盒范围的最小值必须小于最大值！");
        return;
    }
    
    // 新增：验证天空盒范围必须包含坐标范围
    if (m_skyboxMinXSpin->value() > m_minXSpin->value() ||
        m_skyboxMaxXSpin->value() < m_maxXSpin->value() ||
        m_skyboxMinYSpin->value() > m_minYSpin->value() ||
        m_skyboxMaxYSpin->value() < m_maxYSpin->value() ||
        m_skyboxMinZSpin->value() > m_minZSpin->value() ||
        m_skyboxMaxZSpin->value() < m_maxZSpin->value())
    {
        QMessageBox::warning(this, "输入错误", "天空盒范围必须完全包含坐标范围！");
        return;
    }
    
    accept();
}

void CoordinateSystemDialog::onResetClicked()
{
    int ret = QMessageBox::question(this, "重置确认", 
        "确定要重置为默认设置吗？\n这将恢复为地球范围设置和默认坐标系设置。",
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes)
    {
        m_coordSystem->setPresetRange(CoordinateSystem3D::Range_Earth);
        m_coordSystem->setRangeLimitEnabled(true);
        m_coordSystem->setSkyboxRangeBinding(true);
        
        // 重置坐标系设置
        m_coordSystem->setCoordinateSystemType(CoordSystem_Axis3D);
        m_coordSystem->setAxisVisible(Axis_All3D, true);
        m_coordSystem->setGridVisible(true);
        m_coordSystem->setScaleUnit(Unit_Meter3D);
        m_coordSystem->setCustomUnitName("单位");
        m_coordSystem->setScaleInterval(1000.0);
        m_coordSystem->setAxisLength(5000.0); // 调整默认轴长度为更合理的值
        m_coordSystem->setAxisThickness(2.0);
        m_coordSystem->setGridSpacing(1000.0);
        m_coordSystem->setGridThickness(1.0);
        m_coordSystem->setFontSize(FontSize_Medium3D);
        m_coordSystem->setCustomFontSize(100.0);
        
        updateFromCoordinateSystem();
    }
} 