#include "PropertyEditor3D.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QColorDialog>
#include <QScrollArea>
#include <QFrame>
#include <QSplitter>

PropertyEditor3D::PropertyEditor3D(QWidget* parent)
    : QWidget(parent)
    , m_currentGeo(nullptr)
    , m_updating(false)
{
    setupUI();
    setupStyles();
    updateGlobalSettings();
}

void PropertyEditor3D::setupUI()
{
    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // 创建滚动区域
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    
    // 内容widget
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(4);
    contentLayout->setContentsMargins(6, 6, 6, 6);
    
    // 创建各个属性模块
    createPointSection();
    createLineSection();
    createSurfaceSection();
    createDisplaySection();
    
    contentLayout->addWidget(m_pointGroup);
    contentLayout->addWidget(m_lineGroup);
    contentLayout->addWidget(m_surfaceGroup);
    contentLayout->addWidget(m_advancedGroup);
    contentLayout->addWidget(m_displayGroup);
    contentLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);
}

void PropertyEditor3D::createPointSection()
{
    m_pointGroup = new QGroupBox("🔘 点属性");
    m_pointGroup->setObjectName("collapsibleSection");
    QFormLayout* layout = new QFormLayout(m_pointGroup);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 15, 12, 12);
    
    // 点形状 (需要重新计算)
    m_pointShapeCombo = new QComboBox();
    m_pointShapeCombo->setObjectName("propertyCombo");
    m_pointShapeCombo->addItem("∙ 圆点", Point_Dot3D);
    m_pointShapeCombo->addItem("● 圆形", Point_Circle3D);
    m_pointShapeCombo->addItem("■ 方形", Point_Square3D);
    m_pointShapeCombo->addItem("▲ 三角形", Point_Triangle3D);
    m_pointShapeCombo->addItem("◆ 菱形", Point_Diamond3D);
    m_pointShapeCombo->addItem("✚ 十字", Point_Cross3D);
    m_pointShapeCombo->addItem("★ 星形", Point_Star3D);
    connect(m_pointShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertyEditor3D::onPointShapeChanged);
    
    QLabel* shapeLabel = new QLabel("形状:");
    shapeLabel->setObjectName("propertyLabel");
    layout->addRow(shapeLabel, m_pointShapeCombo);
    
    // 点大小 (只需渲染更新)
    m_pointSizeSpin = new QDoubleSpinBox();
    m_pointSizeSpin->setObjectName("propertySpinBox");
    m_pointSizeSpin->setRange(0.5, 15.0);
    m_pointSizeSpin->setSingleStep(0.5);
    m_pointSizeSpin->setDecimals(1);
    m_pointSizeSpin->setSuffix(" px");
    connect(m_pointSizeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyEditor3D::onPointSizeChanged);
    
    QLabel* sizeLabel = new QLabel("大小:");
    sizeLabel->setObjectName("propertyLabel");
    layout->addRow(sizeLabel, m_pointSizeSpin);
    
    // 点颜色 (只需渲染更新)
    m_pointColorButton = createColorButton(Qt::red);
    connect(m_pointColorButton, &QPushButton::clicked, this, &PropertyEditor3D::onPointColorChanged);
    
    QLabel* colorLabel = new QLabel("颜色:");
    colorLabel->setObjectName("propertyLabel");
    layout->addRow(colorLabel, m_pointColorButton);
}

void PropertyEditor3D::createLineSection()
{
    m_lineGroup = new QGroupBox("📏 线属性");
    m_lineGroup->setObjectName("collapsibleSection");
    QFormLayout* layout = new QFormLayout(m_lineGroup);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 15, 12, 12);
    
    // 线型 (只需渲染更新)
    m_lineStyleCombo = new QComboBox();
    m_lineStyleCombo->setObjectName("propertyCombo");
    m_lineStyleCombo->addItem("━━━ 实线", Line_Solid3D);
    m_lineStyleCombo->addItem("┅┅┅ 虚线", Line_Dashed3D);
    m_lineStyleCombo->addItem("········ 点线", Line_Dotted3D);
    m_lineStyleCombo->addItem("┉┅┉┅ 点划线", Line_DashDot3D);
    m_lineStyleCombo->addItem("┉┅┅┉ 双点划线", Line_DashDotDot3D);
    m_lineStyleCombo->addItem("🎨 自定义", Line_Custom3D);
    connect(m_lineStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertyEditor3D::onLineStyleChanged);
    
    QLabel* styleLabel = new QLabel("线型:");
    styleLabel->setObjectName("propertyLabel");
    layout->addRow(styleLabel, m_lineStyleCombo);
    
    // 线宽 (只需渲染更新)
    m_lineWidthSpin = new QDoubleSpinBox();
    m_lineWidthSpin->setObjectName("propertySpinBox");
    m_lineWidthSpin->setRange(0.5, 20.0);
    m_lineWidthSpin->setSingleStep(0.5);
    m_lineWidthSpin->setDecimals(1);
    m_lineWidthSpin->setSuffix(" px");
    connect(m_lineWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyEditor3D::onLineWidthChanged);
    
    QLabel* widthLabel = new QLabel("线宽:");
    widthLabel->setObjectName("propertyLabel");
    layout->addRow(widthLabel, m_lineWidthSpin);
    
    // 线颜色 (只需渲染更新)
    m_lineColorButton = createColorButton(Qt::black);
    connect(m_lineColorButton, &QPushButton::clicked, this, &PropertyEditor3D::onLineColorChanged);
    
    QLabel* lineColorLabel = new QLabel("颜色:");
    lineColorLabel->setObjectName("propertyLabel");
    layout->addRow(lineColorLabel, m_lineColorButton);
    
    // 虚线样式 (只需渲染更新，仅在自定义线型时启用)
    m_lineDashPatternSpin = new QDoubleSpinBox();
    m_lineDashPatternSpin->setObjectName("propertySpinBox");
    m_lineDashPatternSpin->setRange(1.0, 20.0);
    m_lineDashPatternSpin->setSingleStep(1.0);
    m_lineDashPatternSpin->setDecimals(1);
    m_lineDashPatternSpin->setEnabled(false); // 默认禁用
    connect(m_lineDashPatternSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyEditor3D::onLineDashPatternChanged);
    
    QLabel* dashLabel = new QLabel("虚线间距:");
    dashLabel->setObjectName("propertyLabel");
    layout->addRow(dashLabel, m_lineDashPatternSpin);
}

void PropertyEditor3D::createSurfaceSection()
{
    m_surfaceGroup = new QGroupBox("🔷 面属性");
    m_surfaceGroup->setObjectName("collapsibleSection");
    QFormLayout* layout = new QFormLayout(m_surfaceGroup);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 15, 12, 12);
    
    // 填充颜色（包含透明度）
    m_fillColorButton = createColorButton(Qt::lightGray);
    connect(m_fillColorButton, &QPushButton::clicked, this, &PropertyEditor3D::onFillColorChanged);
    
    QLabel* fillLabel = new QLabel("填充颜色:");
    fillLabel->setObjectName("propertyLabel");
    layout->addRow(fillLabel, m_fillColorButton);
    
    // 高级设置组
    m_advancedGroup = new QGroupBox("⚙️ 高级设置");
    m_advancedGroup->setObjectName("collapsibleSection");
    QFormLayout* advLayout = new QFormLayout(m_advancedGroup);
    advLayout->setSpacing(8);
    advLayout->setContentsMargins(12, 15, 12, 12);
    
    // 细分级别 (需要重新计算)
    m_subdivisionLevelCombo = new QComboBox();
    m_subdivisionLevelCombo->setObjectName("propertyCombo");
    m_subdivisionLevelCombo->addItem("🔘 低 (8段)", Subdivision_Low3D);
    m_subdivisionLevelCombo->addItem("🔸 中 (16段)", Subdivision_Medium3D);
    m_subdivisionLevelCombo->addItem("🔹 高 (32段)", Subdivision_High3D);
    m_subdivisionLevelCombo->addItem("💎 超高 (64段)", Subdivision_Ultra3D);
    connect(m_subdivisionLevelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertyEditor3D::onSubdivisionLevelChanged);
    
    QLabel* subdivLabel = new QLabel("细分级别:");
    subdivLabel->setObjectName("propertyLabel");
    advLayout->addRow(subdivLabel, m_subdivisionLevelCombo);
}

void PropertyEditor3D::createDisplaySection()
{
    m_displayGroup = new QGroupBox("👁️ 显示控制");
    m_displayGroup->setObjectName("collapsibleSection");
    QVBoxLayout* layout = new QVBoxLayout(m_displayGroup);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 15, 12, 12);
    
    // 显示点 (只需渲染更新)
    m_showPointsCheck = new QCheckBox("🔘 显示点");
    m_showPointsCheck->setObjectName("propertyCheckBox");
    m_showPointsCheck->setChecked(true);
    connect(m_showPointsCheck, &QCheckBox::toggled, this, &PropertyEditor3D::onShowPointsChanged);
    layout->addWidget(m_showPointsCheck);
    
    // 显示边 (只需渲染更新)
    m_showEdgesCheck = new QCheckBox("📏 显示边");
    m_showEdgesCheck->setObjectName("propertyCheckBox");
    m_showEdgesCheck->setChecked(true);
    connect(m_showEdgesCheck, &QCheckBox::toggled, this, &PropertyEditor3D::onShowEdgesChanged);
    layout->addWidget(m_showEdgesCheck);
    
    // 显示面 (只需渲染更新)
    m_showFacesCheck = new QCheckBox("🔷 显示面");
    m_showFacesCheck->setObjectName("propertyCheckBox");
    m_showFacesCheck->setChecked(true);
    connect(m_showFacesCheck, &QCheckBox::toggled, this, &PropertyEditor3D::onShowFacesChanged);
    layout->addWidget(m_showFacesCheck);
}

void PropertyEditor3D::setupStyles()
{
    QString styleSheet = R"(
        /* 整体面板样式 */
        PropertyEditor3D {
            background-color: #f8f9fa;
            border: none;
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
        }
        
        /* 滚动区域 */
        QScrollArea {
            border: none;
            background-color: transparent;
        }
        
        /* 可折叠区域样式 */
        QGroupBox#collapsibleSection {
            background-color: white;
            border: 1px solid #e9ecef;
            border-radius: 8px;
            margin: 2px;
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
            font-size: 16px;
            font-weight: bold;
            color: #2c3e50;
            padding-top: 15px;
        }
        
        QGroupBox#collapsibleSection::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 8px 12px 4px 12px;
            margin-left: 10px;
            color: #2c3e50;
        }
        
        /* 属性标签样式 */
        QLabel#propertyLabel {
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
            font-size: 14px;
            font-weight: 600;
            color: #495057;
            min-width: 80px;
        }
        
        /* 下拉框样式 */
        QComboBox#propertyCombo {
            background-color: #ffffff;
            border: 2px solid #dee2e6;
            border-radius: 6px;
            padding: 8px 12px;
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
            font-size: 14px;
            font-weight: 500;
            color: #495057;
            min-height: 20px;
        }
        
        QComboBox#propertyCombo:hover {
            border-color: #adb5bd;
        }
        
        QComboBox#propertyCombo:focus {
            border-color: #007bff;
            outline: none;
        }
        
        QComboBox#propertyCombo::drop-down {
            border: none;
            width: 30px;
        }
        
        QComboBox#propertyCombo::down-arrow {
            image: none;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 5px solid #6c757d;
            margin-right: 5px;
        }
        
        QComboBox#propertyCombo QAbstractItemView {
            background-color: #ffffff;
            border: 2px solid #dee2e6;
            border-radius: 6px;
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
            font-size: 14px;
            font-weight: 500;
            color: #495057;
            selection-background-color: #007bff;
            selection-color: white;
            outline: none;
        }
        
        QComboBox#propertyCombo QAbstractItemView::item {
            padding: 10px 12px;
            margin: 1px;
        }
        
        QComboBox#propertyCombo QAbstractItemView::item:hover {
            background-color: #f8f9fa;
        }
        
        /* 数值输入框样式 */
        QDoubleSpinBox#propertySpinBox, QSpinBox#propertySpinBox {
            background-color: #ffffff;
            border: 2px solid #dee2e6;
            border-radius: 6px;
            padding: 8px 12px;
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
            font-size: 14px;
            font-weight: 500;
            color: #495057;
            min-height: 20px;
        }
        
        QDoubleSpinBox#propertySpinBox:hover, QSpinBox#propertySpinBox:hover {
            border-color: #adb5bd;
        }
        
        QDoubleSpinBox#propertySpinBox:focus, QSpinBox#propertySpinBox:focus {
            border-color: #007bff;
            outline: none;
        }
        
        /* 颜色按钮样式 */
        QPushButton#colorButton {
            border: 2px solid #dee2e6;
            border-radius: 6px;
            min-width: 60px;
            min-height: 32px;
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
            font-size: 12px;
            font-weight: 600;
        }
        
        QPushButton#colorButton:hover {
            border-color: #adb5bd;
        }
        
        QPushButton#colorButton:pressed {
            border-color: #007bff;
        }
        
        /* 复选框样式 */
        QCheckBox#propertyCheckBox {
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
            font-size: 14px;
            font-weight: 600;
            color: #495057;
            spacing: 8px;
        }
        
        QCheckBox#propertyCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 2px solid #dee2e6;
            border-radius: 4px;
            background-color: white;
        }
        
        QCheckBox#propertyCheckBox::indicator:checked {
            background-color: #28a745;
            border-color: #28a745;
            image: none;
        }
        
        QCheckBox#propertyCheckBox::indicator:checked:after {
            content: "✓";
            color: white;
            font-weight: bold;
        }
        
        QCheckBox#propertyCheckBox::indicator:hover {
            border-color: #adb5bd;
        }
        
        /* 滑块样式 */
        QSlider#propertySlider {
            height: 25px;
        }
        
        QSlider#propertySlider::groove:horizontal {
            height: 6px;
            background-color: #dee2e6;
            border-radius: 3px;
        }
        
        QSlider#propertySlider::handle:horizontal {
            background-color: #007bff;
            border: 2px solid #007bff;
            width: 20px;
            height: 20px;
            border-radius: 10px;
            margin: -7px 0;
        }
        
        QSlider#propertySlider::handle:horizontal:hover {
            background-color: #0056b3;
            border-color: #0056b3;
        }
        
        QSlider#propertySlider::add-page:horizontal {
            background-color: #dee2e6;
            border-radius: 3px;
        }
        
        QSlider#propertySlider::sub-page:horizontal {
            background-color: #007bff;
            border-radius: 3px;
        }
    )";
    
    this->setStyleSheet(styleSheet);
}

QPushButton* PropertyEditor3D::createColorButton(const QColor& color)
{
    QPushButton* button = new QPushButton();
    button->setObjectName("colorButton");
    button->setFixedSize(60, 32);
    updateColorButton(button, color);
    return button;
}

void PropertyEditor3D::updateColorButton(QPushButton* button, const QColor& color)
{
    QString style = QString("background-color: %1;").arg(color.name());
    button->setStyleSheet(button->styleSheet() + style);
    
    // 包含透明度信息的提示
    QString tooltip = QString("颜色: %1\n透明度: %2%")
                     .arg(color.name())
                     .arg(qRound(color.alphaF() * 100));
    button->setToolTip(tooltip);
    
    // 在按钮上显示透明度百分比
    if (color.alphaF() < 1.0) {
        button->setText(QString("%1%").arg(qRound(color.alphaF() * 100)));
    } else {
        button->setText("");
    }
}

// 剩余部分保持与当前实现类似的逻辑，但简化了参数处理
void PropertyEditor3D::setGeo(osg::ref_ptr<Geo3D> geo)
{
    m_currentGeo = geo;
    updateFromGeo();
}

void PropertyEditor3D::setSelectedGeos(const std::vector<osg::ref_ptr<Geo3D>>& geos)
{
    m_selectedGeos = geos;
    if (!geos.empty())
    {
        m_currentGeo = geos[0];
    }
    else
    {
        m_currentGeo = nullptr;
    }
    updateFromGeo();
}

void PropertyEditor3D::updateFromGeo()
{
    if (!m_currentGeo)
    {
        updateGlobalSettings();
        return;
    }
    
    m_updating = true;
    
    updatePointUI();
    updateLineUI();
    updateSurfaceUI();
    updateDisplayUI();
    
    m_updating = false;
}

void PropertyEditor3D::updateGlobalSettings()
{
    m_updating = true;
    
    updatePointUI();
    updateLineUI();
    updateSurfaceUI();
    updateDisplayUI();
    
    m_updating = false;
}

void PropertyEditor3D::updatePointUI()
{
    PointShape3D shape = m_currentGeo ? m_currentGeo->getParameters().pointShape : GlobalPointShape3D;
    double size = m_currentGeo ? m_currentGeo->getParameters().pointSize : GlobalPointSize3D;
    QColor color = m_currentGeo ? m_currentGeo->getParameters().pointColor.toQColor() : GlobalPointColor3D;
    
    for (int i = 0; i < m_pointShapeCombo->count(); ++i)
    {
        if (m_pointShapeCombo->itemData(i).toInt() == shape)
        {
            m_pointShapeCombo->setCurrentIndex(i);
            break;
        }
    }
    
    m_pointSizeSpin->setValue(size);
    updateColorButton(m_pointColorButton, color);
}

void PropertyEditor3D::updateLineUI()
{
    LineStyle3D style = m_currentGeo ? m_currentGeo->getParameters().lineStyle : GlobalLineStyle3D;
    double width = m_currentGeo ? m_currentGeo->getParameters().lineWidth : GlobalLineWidth3D;
    QColor color = m_currentGeo ? m_currentGeo->getParameters().lineColor.toQColor() : GlobalLineColor3D;
    double dashPattern = m_currentGeo ? m_currentGeo->getParameters().lineDashPattern : GlobalLineDashPattern3D;
    
    for (int i = 0; i < m_lineStyleCombo->count(); ++i)
    {
        if (m_lineStyleCombo->itemData(i).toInt() == style)
        {
            m_lineStyleCombo->setCurrentIndex(i);
            break;
        }
    }
    
    m_lineWidthSpin->setValue(width);
    updateColorButton(m_lineColorButton, color);
    m_lineDashPatternSpin->setValue(dashPattern);
    
    // 自定义虚线时才启用间距设置
    m_lineDashPatternSpin->setEnabled(style == Line_Custom3D);
}

void PropertyEditor3D::updateSurfaceUI()
{
    QColor fillColor = m_currentGeo ? m_currentGeo->getParameters().fillColor.toQColor() : GlobalFillColor3D;
    SubdivisionLevel3D level = m_currentGeo ? m_currentGeo->getParameters().subdivisionLevel : GlobalSubdivisionLevel3D;
    
    updateColorButton(m_fillColorButton, fillColor);
    
    for (int i = 0; i < m_subdivisionLevelCombo->count(); ++i)
    {
        if (m_subdivisionLevelCombo->itemData(i).toInt() == level)
        {
            m_subdivisionLevelCombo->setCurrentIndex(i);
            break;
        }
    }
}

void PropertyEditor3D::updateDisplayUI()
{
    bool showPoints = m_currentGeo ? m_currentGeo->getParameters().showPoints : GlobalShowPoints3D;
    bool showEdges = m_currentGeo ? m_currentGeo->getParameters().showEdges : GlobalShowEdges3D;
    bool showFaces = m_currentGeo ? m_currentGeo->getParameters().showFaces : GlobalShowFaces3D;
    
    m_showPointsCheck->setChecked(showPoints);
    m_showEdgesCheck->setChecked(showEdges);
    m_showFacesCheck->setChecked(showFaces);
}

// ============= 需要重新计算几何体的参数变化 =============

void PropertyEditor3D::onPointShapeChanged()
{
    if (m_updating) return;
    
    PointShape3D shape = static_cast<PointShape3D>(m_pointShapeCombo->currentData().toInt());
    
    if (!m_selectedGeos.empty())
    {
        for (auto& geo : m_selectedGeos)
        {
            if (geo)
            {
                GeoParameters3D params = geo->getParameters();
                params.pointShape = shape;
                geo->setParameters(params);
            }
        }
    }
    else if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.pointShape = shape;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalPointShape3D = shape;
    }
    
    emit geometryRecalculationRequired();
}

void PropertyEditor3D::onSubdivisionLevelChanged()
{
    if (m_updating) return;
    
    SubdivisionLevel3D level = static_cast<SubdivisionLevel3D>(m_subdivisionLevelCombo->currentData().toInt());
    
    if (!m_selectedGeos.empty())
    {
        for (auto& geo : m_selectedGeos)
        {
            if (geo)
            {
                GeoParameters3D params = geo->getParameters();
                params.subdivisionLevel = level;
                geo->setParameters(params);
            }
        }
    }
    else if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.subdivisionLevel = level;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalSubdivisionLevel3D = level;
    }
    
    emit geometryRecalculationRequired();
}

// ============= 只需要更新渲染的参数变化 =============

void PropertyEditor3D::onPointSizeChanged()
{
    if (m_updating) return;
    
    double size = m_pointSizeSpin->value();
    
    if (!m_selectedGeos.empty())
    {
        for (auto& geo : m_selectedGeos)
        {
            if (geo)
            {
                GeoParameters3D params = geo->getParameters();
                params.pointSize = size;
                geo->setParameters(params);
            }
        }
    }
    else if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.pointSize = size;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalPointSize3D = size;
    }
    
    emit renderingParametersChanged();
}

void PropertyEditor3D::onPointColorChanged()
{
    if (m_updating) return;
    
    QColor currentColor = m_currentGeo ? m_currentGeo->getParameters().pointColor.toQColor() : GlobalPointColor3D;
    QColor color = QColorDialog::getColor(currentColor, this, "选择点颜色", QColorDialog::ShowAlphaChannel);
    
    if (color.isValid())
    {
        updateColorButton(m_pointColorButton, color);
        
        if (!m_selectedGeos.empty())
        {
            for (auto& geo : m_selectedGeos)
            {
                if (geo)
                {
                    GeoParameters3D params = geo->getParameters();
                    params.pointColor = color;
                    geo->setParameters(params);
                }
            }
        }
        else if (m_currentGeo)
        {
            GeoParameters3D params = m_currentGeo->getParameters();
            params.pointColor = color;
            m_currentGeo->setParameters(params);
        }
        else
        {
            GlobalPointColor3D = color;
        }
        
        emit renderingParametersChanged();
    }
}

void PropertyEditor3D::onLineWidthChanged()
{
    if (m_updating) return;
    
    double width = m_lineWidthSpin->value();
    
    if (!m_selectedGeos.empty())
    {
        for (auto& geo : m_selectedGeos)
        {
            if (geo)
            {
                GeoParameters3D params = geo->getParameters();
                params.lineWidth = width;
                geo->setParameters(params);
            }
        }
    }
    else if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.lineWidth = width;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalLineWidth3D = width;
    }
    
    emit renderingParametersChanged();
}

void PropertyEditor3D::onLineColorChanged()
{
    if (m_updating) return;
    
    QColor currentColor = m_currentGeo ? m_currentGeo->getParameters().lineColor.toQColor() : GlobalLineColor3D;
    QColor color = QColorDialog::getColor(currentColor, this, "选择线颜色", QColorDialog::ShowAlphaChannel);
    
    if (color.isValid())
    {
        updateColorButton(m_lineColorButton, color);
        
        if (!m_selectedGeos.empty())
        {
            for (auto& geo : m_selectedGeos)
            {
                if (geo)
                {
                    GeoParameters3D params = geo->getParameters();
                    params.lineColor = color;
                    geo->setParameters(params);
                }
            }
        }
        else if (m_currentGeo)
        {
            GeoParameters3D params = m_currentGeo->getParameters();
            params.lineColor = color;
            m_currentGeo->setParameters(params);
        }
        else
        {
            GlobalLineColor3D = color;
        }
        
        emit renderingParametersChanged();
    }
}

void PropertyEditor3D::onLineStyleChanged()
{
    if (m_updating) return;
    
    LineStyle3D style = static_cast<LineStyle3D>(m_lineStyleCombo->currentData().toInt());
    
    // 启用/禁用自定义虚线间距
    m_lineDashPatternSpin->setEnabled(style == Line_Custom3D);
    
    if (!m_selectedGeos.empty())
    {
        for (auto& geo : m_selectedGeos)
        {
            if (geo)
            {
                GeoParameters3D params = geo->getParameters();
                params.lineStyle = style;
                geo->setParameters(params);
            }
        }
    }
    else if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.lineStyle = style;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalLineStyle3D = style;
    }
    
    emit renderingParametersChanged();
}

void PropertyEditor3D::onLineDashPatternChanged()
{
    if (m_updating) return;
    
    double pattern = m_lineDashPatternSpin->value();
    
    if (!m_selectedGeos.empty())
    {
        for (auto& geo : m_selectedGeos)
        {
            if (geo)
            {
                GeoParameters3D params = geo->getParameters();
                params.lineDashPattern = pattern;
                geo->setParameters(params);
            }
        }
    }
    else if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.lineDashPattern = pattern;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalLineDashPattern3D = pattern;
    }
    
    emit renderingParametersChanged();
}

void PropertyEditor3D::onFillColorChanged()
{
    if (m_updating) return;
    
    QColor currentColor = m_currentGeo ? m_currentGeo->getParameters().fillColor.toQColor() : GlobalFillColor3D;
    QColor color = QColorDialog::getColor(currentColor, this, "选择填充颜色", QColorDialog::ShowAlphaChannel);
    
    if (color.isValid())
    {
        updateColorButton(m_fillColorButton, color);
        
        if (!m_selectedGeos.empty())
        {
            for (auto& geo : m_selectedGeos)
            {
                if (geo)
                {
                    GeoParameters3D params = geo->getParameters();
                    params.fillColor = color;
                    geo->setParameters(params);
                }
            }
        }
        else if (m_currentGeo)
        {
            GeoParameters3D params = m_currentGeo->getParameters();
            params.fillColor = color;
            m_currentGeo->setParameters(params);
        }
        else
        {
            GlobalFillColor3D = color;
        }
        
        emit renderingParametersChanged();
    }
}

void PropertyEditor3D::onShowPointsChanged()
{
    if (m_updating) return;
    
    bool show = m_showPointsCheck->isChecked();
    
    // 显示约束：至少保持一个组件可见
    if (!show && !m_showEdgesCheck->isChecked() && !m_showFacesCheck->isChecked()) {
        // 如果要隐藏所有组件，强制显示线框
        m_updating = true;
        m_showEdgesCheck->setChecked(true);
        m_updating = false;
    }
    
    if (!m_selectedGeos.empty())
    {
        for (auto& geo : m_selectedGeos)
        {
            if (geo)
            {
                GeoParameters3D params = geo->getParameters();
                params.showPoints = show;
                // 如果强制显示了边，也要更新
                if (!show && !params.showEdges && !params.showFaces) {
                    params.showEdges = true;
                }
                geo->setParameters(params);
            }
        }
    }
    else if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.showPoints = show;
        if (!show && !params.showEdges && !params.showFaces) {
            params.showEdges = true;
        }
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalShowPoints3D = show;
        if (!show && !GlobalShowEdges3D && !GlobalShowFaces3D) {
            GlobalShowEdges3D = true;
        }
    }
    
    emit renderingParametersChanged();
}

void PropertyEditor3D::onShowEdgesChanged()
{
    if (m_updating) return;
    
    bool show = m_showEdgesCheck->isChecked();
    
    // 显示约束：至少保持一个组件可见
    if (!show && !m_showPointsCheck->isChecked() && !m_showFacesCheck->isChecked()) {
        // 如果要隐藏所有组件，强制显示点
        m_updating = true;
        m_showPointsCheck->setChecked(true);
        m_updating = false;
    }
    
    if (!m_selectedGeos.empty())
    {
        for (auto& geo : m_selectedGeos)
        {
            if (geo)
            {
                GeoParameters3D params = geo->getParameters();
                params.showEdges = show;
                if (!show && !params.showPoints && !params.showFaces) {
                    params.showPoints = true;
                }
                geo->setParameters(params);
            }
        }
    }
    else if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.showEdges = show;
        if (!show && !params.showPoints && !params.showFaces) {
            params.showPoints = true;
        }
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalShowEdges3D = show;
        if (!show && !GlobalShowPoints3D && !GlobalShowFaces3D) {
            GlobalShowPoints3D = true;
        }
    }
    
    emit renderingParametersChanged();
}

void PropertyEditor3D::onShowFacesChanged()
{
    if (m_updating) return;
    
    bool show = m_showFacesCheck->isChecked();
    
    // 显示约束：至少保持一个组件可见
    if (!show && !m_showPointsCheck->isChecked() && !m_showEdgesCheck->isChecked()) {
        // 如果要隐藏所有组件，强制显示线框
        m_updating = true;
        m_showEdgesCheck->setChecked(true);
        m_updating = false;
    }
    
    if (!m_selectedGeos.empty())
    {
        for (auto& geo : m_selectedGeos)
        {
            if (geo)
            {
                GeoParameters3D params = geo->getParameters();
                params.showFaces = show;
                if (!show && !params.showPoints && !params.showEdges) {
                    params.showEdges = true;
                }
                geo->setParameters(params);
            }
        }
    }
    else if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.showFaces = show;
        if (!show && !params.showPoints && !params.showEdges) {
            params.showEdges = true;
        }
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalShowFaces3D = show;
        if (!show && !GlobalShowPoints3D && !GlobalShowEdges3D) {
            GlobalShowEdges3D = true;
        }
    }
    
    emit renderingParametersChanged();
}




