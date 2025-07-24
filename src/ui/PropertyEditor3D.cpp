#include "PropertyEditor3D.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QColorDialog>
#include <QMessageBox>

// ========================================= PropertyEditor3D 实现 =========================================
PropertyEditor3D::PropertyEditor3D(QWidget* parent)
    : QWidget(parent)
    , m_currentGeo(nullptr)
    , m_updating(false)
{
    setupUI();
    updateGlobalSettings();
}

void PropertyEditor3D::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    
    createPointGroup();
    createLineGroup();
    createSurfaceGroup();
    createMaterialGroup();
    createVolumeGroup();
    createDisplayGroup();
    
    mainLayout->addWidget(m_pointGroup);
    mainLayout->addWidget(m_lineGroup);
    mainLayout->addWidget(m_surfaceGroup);
    mainLayout->addWidget(m_materialGroup);
    mainLayout->addWidget(m_volumeGroup);
    mainLayout->addWidget(m_displayGroup);
    mainLayout->addStretch();
}

void PropertyEditor3D::createPointGroup()
{
    m_pointGroup = new QGroupBox("点属性", this);
    QFormLayout* layout = new QFormLayout(m_pointGroup);
    
    // 点形状
    m_pointShapeCombo = new QComboBox();
    m_pointShapeCombo->addItem("圆形", Point_Circle3D);
    m_pointShapeCombo->addItem("方形", Point_Square3D);
    m_pointShapeCombo->addItem("三角形", Point_Triangle3D);
    m_pointShapeCombo->addItem("菱形", Point_Diamond3D);
    m_pointShapeCombo->addItem("十字", Point_Cross3D);
    m_pointShapeCombo->addItem("星形", Point_Star3D);
    connect(m_pointShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertyEditor3D::onPointShapeChanged);
    layout->addRow("形状:", m_pointShapeCombo);
    
    // 点大小
    m_pointSizeSpin = new QDoubleSpinBox();
    m_pointSizeSpin->setRange(0.1, 100.0);
    m_pointSizeSpin->setSingleStep(0.1);
    m_pointSizeSpin->setDecimals(1);
    connect(m_pointSizeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyEditor3D::onPointSizeChanged);
    layout->addRow("大小:", m_pointSizeSpin);
    
    // 点颜色
    m_pointColorButton = createColorButton(Qt::red);
    connect(m_pointColorButton, &QPushButton::clicked, this, &PropertyEditor3D::onPointColorChanged);
    layout->addRow("颜色:", m_pointColorButton);
}

void PropertyEditor3D::createLineGroup()
{
    m_lineGroup = new QGroupBox("线属性", this);
    QFormLayout* layout = new QFormLayout(m_lineGroup);
    
    // 线型
    m_lineStyleCombo = new QComboBox();
    m_lineStyleCombo->addItem("实线", Line_Solid3D);
    m_lineStyleCombo->addItem("虚线", Line_Dashed3D);
    m_lineStyleCombo->addItem("点线", Line_Dotted3D);
    m_lineStyleCombo->addItem("点划线", Line_DashDot3D);
    m_lineStyleCombo->addItem("双点划线", Line_DashDotDot3D);
    m_lineStyleCombo->addItem("自定义", Line_Custom3D);
    connect(m_lineStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertyEditor3D::onLineStyleChanged);
    layout->addRow("线型:", m_lineStyleCombo);
    
    // 线宽
    m_lineWidthSpin = new QDoubleSpinBox();
    m_lineWidthSpin->setRange(0.1, 20.0);
    m_lineWidthSpin->setSingleStep(0.1);
    m_lineWidthSpin->setDecimals(1);
    connect(m_lineWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyEditor3D::onLineWidthChanged);
    layout->addRow("线宽:", m_lineWidthSpin);
    
    // 线颜色
    m_lineColorButton = createColorButton(Qt::black);
    connect(m_lineColorButton, &QPushButton::clicked, this, &PropertyEditor3D::onLineColorChanged);
    layout->addRow("颜色:", m_lineColorButton);
    
    // 虚线样式
    m_lineDashPatternSpin = new QDoubleSpinBox();
    m_lineDashPatternSpin->setRange(1.0, 20.0);
    m_lineDashPatternSpin->setSingleStep(1.0);
    m_lineDashPatternSpin->setDecimals(1);
    connect(m_lineDashPatternSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PropertyEditor3D::onLineDashPatternChanged);
    layout->addRow("虚线样式:", m_lineDashPatternSpin);
    
    // 节点线型
    m_nodeLineStyleCombo = new QComboBox();
    m_nodeLineStyleCombo->addItem("折线", NodeLine_Polyline3D);
    m_nodeLineStyleCombo->addItem("样条曲线", NodeLine_Spline3D);
    m_nodeLineStyleCombo->addItem("贝塞尔曲线", NodeLine_Bezier3D);
    m_nodeLineStyleCombo->addItem("圆弧", NodeLine_Arc3D);
    m_nodeLineStyleCombo->addItem("三点弧", NodeLine_ThreePointArc3D);
    m_nodeLineStyleCombo->addItem("流线", NodeLine_Streamline3D);
    connect(m_nodeLineStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertyEditor3D::onNodeLineStyleChanged);
    layout->addRow("节点线型:", m_nodeLineStyleCombo);
}

void PropertyEditor3D::createSurfaceGroup()
{
    m_surfaceGroup = new QGroupBox("面属性", this);
    QFormLayout* layout = new QFormLayout(m_surfaceGroup);
    
    // 填充类型
    m_fillTypeCombo = new QComboBox();
    m_fillTypeCombo->addItem("无填充", Fill_None3D);
    m_fillTypeCombo->addItem("实心填充", Fill_Solid3D);
    m_fillTypeCombo->addItem("线框", Fill_Wireframe3D);
    m_fillTypeCombo->addItem("点填充", Fill_Points3D);
    m_fillTypeCombo->addItem("纹理填充", Fill_Texture3D);
    connect(m_fillTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertyEditor3D::onFillTypeChanged);
    layout->addRow("填充类型:", m_fillTypeCombo);
    
    // 填充颜色
    m_fillColorButton = createColorButton(Qt::gray);
    connect(m_fillColorButton, &QPushButton::clicked, this, &PropertyEditor3D::onFillColorChanged);
    layout->addRow("填充颜色:", m_fillColorButton);
    
    // 边界颜色
    m_borderColorButton = createColorButton(Qt::black);
    connect(m_borderColorButton, &QPushButton::clicked, this, &PropertyEditor3D::onBorderColorChanged);
    layout->addRow("边界颜色:", m_borderColorButton);
    
    // 显示边界
    m_showBorderCheck = new QCheckBox();
    connect(m_showBorderCheck, &QCheckBox::toggled, this, &PropertyEditor3D::onShowBorderChanged);
    layout->addRow("显示边界:", m_showBorderCheck);
}

void PropertyEditor3D::createMaterialGroup()
{
    m_materialGroup = new QGroupBox("材质属性", this);
    QFormLayout* layout = new QFormLayout(m_materialGroup);
    
    // 材质类型
    m_materialTypeCombo = new QComboBox();
    m_materialTypeCombo->addItem("基础材质", Material_Basic3D);
    m_materialTypeCombo->addItem("Phong材质", Material_Phong3D);
    m_materialTypeCombo->addItem("Blinn材质", Material_Blinn3D);
    m_materialTypeCombo->addItem("Lambert材质", Material_Lambert3D);
    m_materialTypeCombo->addItem("PBR材质", Material_PBR3D);
    connect(m_materialTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertyEditor3D::onMaterialTypeChanged);
    layout->addRow("材质类型:", m_materialTypeCombo);
    
    // 光泽度
    m_shininessSlider = new QSlider(Qt::Horizontal);
    m_shininessSlider->setRange(1, 128);
    m_shininessSlider->setValue(32);
    connect(m_shininessSlider, &QSlider::valueChanged, this, &PropertyEditor3D::onShininessChanged);
    layout->addRow("光泽度:", m_shininessSlider);
    
    // 透明度
    m_transparencySlider = new QSlider(Qt::Horizontal);
    m_transparencySlider->setRange(0, 100);
    m_transparencySlider->setValue(100);
    connect(m_transparencySlider, &QSlider::valueChanged, this, &PropertyEditor3D::onTransparencyChanged);
    layout->addRow("透明度:", m_transparencySlider);
}

void PropertyEditor3D::createVolumeGroup()
{
    m_volumeGroup = new QGroupBox("体属性", this);
    QFormLayout* layout = new QFormLayout(m_volumeGroup);
    
    // 细分级别
    m_subdivisionLevelCombo = new QComboBox();
    m_subdivisionLevelCombo->addItem("低", Subdivision_Low3D);
    m_subdivisionLevelCombo->addItem("中", Subdivision_Medium3D);
    m_subdivisionLevelCombo->addItem("高", Subdivision_High3D);
    m_subdivisionLevelCombo->addItem("超高", Subdivision_Ultra3D);
    connect(m_subdivisionLevelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertyEditor3D::onSubdivisionLevelChanged);
    layout->addRow("细分级别:", m_subdivisionLevelCombo);
}

void PropertyEditor3D::createDisplayGroup()
{
    m_displayGroup = new QGroupBox("显示控制", this);
    QFormLayout* layout = new QFormLayout(m_displayGroup);
    
    // 显示点
    m_showPointsCheck = new QCheckBox();
    m_showPointsCheck->setChecked(true);
    connect(m_showPointsCheck, &QCheckBox::toggled, this, &PropertyEditor3D::onShowPointsChanged);
    layout->addRow("显示点:", m_showPointsCheck);
    
    // 显示边
    m_showEdgesCheck = new QCheckBox();
    m_showEdgesCheck->setChecked(true);
    connect(m_showEdgesCheck, &QCheckBox::toggled, this, &PropertyEditor3D::onShowEdgesChanged);
    layout->addRow("显示边:", m_showEdgesCheck);
    
    // 显示面
    m_showFacesCheck = new QCheckBox();
    m_showFacesCheck->setChecked(true);
    connect(m_showFacesCheck, &QCheckBox::toggled, this, &PropertyEditor3D::onShowFacesChanged);
    layout->addRow("显示面:", m_showFacesCheck);
}

QPushButton* PropertyEditor3D::createColorButton(const QColor& color)
{
    QPushButton* button = new QPushButton();
    button->setFixedSize(50, 25);
    updateColorButton(button, color);
    return button;
}

void PropertyEditor3D::updateColorButton(QPushButton* button, const QColor& color)
{
    QString style = QString("background-color: %1; border: 1px solid black;").arg(color.name());
    button->setStyleSheet(style);
    button->setToolTip(color.name());
}

void PropertyEditor3D::setGeo(Geo3D* geo)
{
    m_currentGeo = geo;
    updateFromGeo();
}

void PropertyEditor3D::setSelectedGeos(const std::vector<Geo3D*>& geos)
{
    m_selectedGeos = geos;
    if (!geos.empty())
    {
        m_currentGeo = geos[0]; // 显示第一个对象的属性
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
    
    const GeoParameters3D& params = m_currentGeo->getParameters();
    
    updatePointUI();
    updateLineUI();
    updateSurfaceUI();
    updateMaterialUI();
    updateVolumeUI();
    updateDisplayUI();
    
    m_updating = false;
}

void PropertyEditor3D::updateGlobalSettings()
{
    m_updating = true;
    
    updatePointUI();
    updateLineUI();
    updateSurfaceUI();
    updateMaterialUI();
    updateVolumeUI();
    updateDisplayUI();
    
    m_updating = false;
}

void PropertyEditor3D::updatePointUI()
{
    PointShape3D shape = m_currentGeo ? m_currentGeo->getParameters().pointShape : GlobalPointShape3D;
    double size = m_currentGeo ? m_currentGeo->getParameters().pointSize : GlobalPointSize3D;
    QColor color = m_currentGeo ? m_currentGeo->getParameters().pointColor.toQColor() : GlobalPointColor3D;
    
    // 更新控件
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
    NodeLineStyle3D nodeStyle = m_currentGeo ? m_currentGeo->getParameters().nodeLineStyle : GlobalNodeLineStyle3D;
    
    // 更新控件
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
    
    for (int i = 0; i < m_nodeLineStyleCombo->count(); ++i)
    {
        if (m_nodeLineStyleCombo->itemData(i).toInt() == nodeStyle)
        {
            m_nodeLineStyleCombo->setCurrentIndex(i);
            break;
        }
    }
}

void PropertyEditor3D::updateSurfaceUI()
{
    FillType3D fillType = m_currentGeo ? m_currentGeo->getParameters().fillType : GlobalFillType3D;
    QColor fillColor = m_currentGeo ? m_currentGeo->getParameters().fillColor.toQColor() : GlobalFillColor3D;
    QColor borderColor = m_currentGeo ? m_currentGeo->getParameters().borderColor.toQColor() : GlobalBorderColor3D;
    bool showBorder = m_currentGeo ? m_currentGeo->getParameters().showBorder : GlobalShowBorder3D;
    
    // 更新控件
    for (int i = 0; i < m_fillTypeCombo->count(); ++i)
    {
        if (m_fillTypeCombo->itemData(i).toInt() == fillType)
        {
            m_fillTypeCombo->setCurrentIndex(i);
            break;
        }
    }
    
    updateColorButton(m_fillColorButton, fillColor);
    updateColorButton(m_borderColorButton, borderColor);
    m_showBorderCheck->setChecked(showBorder);
}

void PropertyEditor3D::updateMaterialUI()
{
    MaterialType3D matType = m_currentGeo ? m_currentGeo->getParameters().material.type : GlobalMaterialType3D;
    double shininess = m_currentGeo ? m_currentGeo->getParameters().material.shininess : GlobalShininess3D;
    double transparency = m_currentGeo ? m_currentGeo->getParameters().material.transparency : GlobalTransparency3D;
    
    // 更新控件
    for (int i = 0; i < m_materialTypeCombo->count(); ++i)
    {
        if (m_materialTypeCombo->itemData(i).toInt() == matType)
        {
            m_materialTypeCombo->setCurrentIndex(i);
            break;
        }
    }
    
    m_shininessSlider->setValue(static_cast<int>(shininess));
    m_transparencySlider->setValue(static_cast<int>(transparency * 100));
}

void PropertyEditor3D::updateVolumeUI()
{
    SubdivisionLevel3D level = m_currentGeo ? m_currentGeo->getParameters().subdivisionLevel : GlobalSubdivisionLevel3D;
    
    // 更新控件
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

// PropertyEditor3D 槽函数
void PropertyEditor3D::onPointShapeChanged()
{
    if (m_updating) return;
    
    PointShape3D shape = static_cast<PointShape3D>(m_pointShapeCombo->currentData().toInt());
    
    if (!m_selectedGeos.empty())
    {
        // 多选情况：应用到所有选中的对象
        for (auto* geo : m_selectedGeos)
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
        // 单选情况
        GeoParameters3D params = m_currentGeo->getParameters();
        params.pointShape = shape;
        m_currentGeo->setParameters(params);
    }
    else
    {
        // 全局设置
        GlobalPointShape3D = shape;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onPointSizeChanged()
{
    if (m_updating) return;
    
    double size = static_cast<double>(m_pointSizeSpin->value());
    
    if (!m_selectedGeos.empty())
    {
        // 多选情况：应用到所有选中的对象
        for (auto* geo : m_selectedGeos)
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
        // 单选情况
        GeoParameters3D params = m_currentGeo->getParameters();
        params.pointSize = size;
        m_currentGeo->setParameters(params);
    }
    else
    {
        // 全局设置
        GlobalPointSize3D = size;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onPointColorChanged()
{
    if (m_updating) return;
    
    QColor color = QColorDialog::getColor(m_pointColorButton->palette().button().color(), this);
    if (color.isValid())
    {
        updateColorButton(m_pointColorButton, color);
        
        if (!m_selectedGeos.empty())
        {
            // 多选情况：应用到所有选中的对象
            for (auto* geo : m_selectedGeos)
            {
                if (geo)
                {
                    GeoParameters3D params = geo->getParameters();
                    params.pointColor = color; // 使用QColor直接赋值
                    geo->setParameters(params);
                }
            }
        }
        else if (m_currentGeo)
        {
            // 单选情况
            GeoParameters3D params = m_currentGeo->getParameters();
            params.pointColor = color; // 使用QColor直接赋值
            m_currentGeo->setParameters(params);
        }
        else
        {
            // 全局设置
            GlobalPointColor3D = color;
        }
        
        emit parametersChanged();
    }
}

void PropertyEditor3D::onLineStyleChanged()
{
    if (m_updating) return;
    
    LineStyle3D style = static_cast<LineStyle3D>(m_lineStyleCombo->currentData().toInt());
    
    if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.lineStyle = style;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalLineStyle3D = style;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onLineWidthChanged()
{
    if (m_updating) return;
    
    double width = static_cast<double>(m_lineWidthSpin->value());
    
    if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.lineWidth = width;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalLineWidth3D = width;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onLineColorChanged()
{
    if (m_updating) return;
    
    QColor currentColor = m_currentGeo ? m_currentGeo->getParameters().lineColor.toQColor() : GlobalLineColor3D;
    QColor color = QColorDialog::getColor(currentColor, this, "选择线颜色");
    
    if (color.isValid())
    {
        updateColorButton(m_lineColorButton, color);
        
        if (!m_selectedGeos.empty())
        {
            // 多选情况：应用到所有选中的对象
            for (auto* geo : m_selectedGeos)
            {
                if (geo)
                {
                    GeoParameters3D params = geo->getParameters();
                    params.lineColor = color; // 使用QColor直接赋值
                    geo->setParameters(params);
                }
            }
        }
        else if (m_currentGeo)
        {
            // 单选情况
            GeoParameters3D params = m_currentGeo->getParameters();
            params.lineColor = color; // 使用QColor直接赋值
            m_currentGeo->setParameters(params);
        }
        else
        {
            // 全局设置
            GlobalLineColor3D = color;
        }
        
        emit parametersChanged();
    }
}

void PropertyEditor3D::onLineDashPatternChanged()
{
    if (m_updating) return;
    
    double pattern = static_cast<double>(m_lineDashPatternSpin->value());
    
    if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.lineDashPattern = pattern;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalLineDashPattern3D = pattern;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onNodeLineStyleChanged()
{
    if (m_updating) return;
    
    NodeLineStyle3D style = static_cast<NodeLineStyle3D>(m_nodeLineStyleCombo->currentData().toInt());
    
    if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.nodeLineStyle = style;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalNodeLineStyle3D = style;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onFillTypeChanged()
{
    if (m_updating) return;
    
    FillType3D fillType = static_cast<FillType3D>(m_fillTypeCombo->currentData().toInt());
    
    if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.fillType = fillType;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalFillType3D = fillType;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onFillColorChanged()
{
    if (m_updating) return;
    
    QColor currentColor = m_currentGeo ? m_currentGeo->getParameters().fillColor.toQColor() : GlobalFillColor3D;
    QColor color = QColorDialog::getColor(currentColor, this, "选择填充颜色");
    
    if (color.isValid())
    {
        updateColorButton(m_fillColorButton, color);
        
        if (!m_selectedGeos.empty())
        {
            // 多选情况：应用到所有选中的对象
            for (auto* geo : m_selectedGeos)
            {
                if (geo)
                {
                    GeoParameters3D params = geo->getParameters();
                    params.fillColor = color; // 使用QColor直接赋值
                    geo->setParameters(params);
                }
            }
        }
        else if (m_currentGeo)
        {
            // 单选情况
            GeoParameters3D params = m_currentGeo->getParameters();
            params.fillColor = color; // 使用QColor直接赋值
            m_currentGeo->setParameters(params);
        }
        else
        {
            // 全局设置
            GlobalFillColor3D = color;
        }
        
        emit parametersChanged();
    }
}

void PropertyEditor3D::onBorderColorChanged()
{
    if (m_updating) return;
    
    QColor currentColor = m_currentGeo ? m_currentGeo->getParameters().borderColor.toQColor() : GlobalBorderColor3D;
    QColor color = QColorDialog::getColor(currentColor, this, "选择边界颜色");
    
    if (color.isValid())
    {
        updateColorButton(m_borderColorButton, color);
        
        if (!m_selectedGeos.empty())
        {
            // 多选情况：应用到所有选中的对象
            for (auto* geo : m_selectedGeos)
            {
                if (geo)
                {
                    GeoParameters3D params = geo->getParameters();
                    params.borderColor = color; // 使用QColor直接赋值
                    geo->setParameters(params);
                }
            }
        }
        else if (m_currentGeo)
        {
            // 单选情况
            GeoParameters3D params = m_currentGeo->getParameters();
            params.borderColor = color; // 使用QColor直接赋值
            m_currentGeo->setParameters(params);
        }
        else
        {
            // 全局设置
            GlobalBorderColor3D = color;
        }
        
        emit parametersChanged();
    }
}

void PropertyEditor3D::onShowBorderChanged()
{
    if (m_updating) return;
    
    bool showBorder = m_showBorderCheck->isChecked();
    
    if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.showBorder = showBorder;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalShowBorder3D = showBorder;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onMaterialTypeChanged()
{
    if (m_updating) return;
    
    MaterialType3D matType = static_cast<MaterialType3D>(m_materialTypeCombo->currentData().toInt());
    
    if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.material.type = matType;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalMaterialType3D = matType;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onShininessChanged()
{
    if (m_updating) return;
    
    double shininess = static_cast<double>(m_shininessSlider->value());
    
    if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.material.shininess = shininess;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalShininess3D = shininess;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onTransparencyChanged()
{
    if (m_updating) return;
    
    double transparency = static_cast<double>(m_transparencySlider->value()) / 100.0;
    
    if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.material.transparency = transparency;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalTransparency3D = transparency;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onSubdivisionLevelChanged()
{
    if (m_updating) return;
    
    SubdivisionLevel3D level = static_cast<SubdivisionLevel3D>(m_subdivisionLevelCombo->currentData().toInt());
    
    if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.subdivisionLevel = level;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalSubdivisionLevel3D = level;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onShowPointsChanged()
{
    if (m_updating) return;
    
    bool show = m_showPointsCheck->isChecked();
    
    if (!m_selectedGeos.empty())
    {
        // 多选情况：应用到所有选中的对象
        for (auto* geo : m_selectedGeos)
        {
            if (geo)
            {
                GeoParameters3D params = geo->getParameters();
                params.showPoints = show;
                geo->setParameters(params);
            }
        }
    }
    else if (m_currentGeo)
    {
        // 单选情况
        GeoParameters3D params = m_currentGeo->getParameters();
        params.showPoints = show;
        m_currentGeo->setParameters(params);
    }
    else
    {
        // 全局设置
        GlobalShowPoints3D = show;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onShowEdgesChanged()
{
    if (m_updating) return;
    
    bool show = m_showEdgesCheck->isChecked();
    
    if (!m_selectedGeos.empty())
    {
        // 多选情况：应用到所有选中的对象
        for (auto* geo : m_selectedGeos)
        {
            if (geo)
            {
                GeoParameters3D params = geo->getParameters();
                params.showEdges = show;
                geo->setParameters(params);
            }
        }
    }
    else if (m_currentGeo)
    {
        // 单选情况
        GeoParameters3D params = m_currentGeo->getParameters();
        params.showEdges = show;
        m_currentGeo->setParameters(params);
    }
    else
    {
        // 全局设置
        GlobalShowEdges3D = show;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onShowFacesChanged()
{
    if (m_updating) return;
    
    bool show = m_showFacesCheck->isChecked();
    
    if (!m_selectedGeos.empty())
    {
        // 多选情况：应用到所有选中的对象
        for (auto* geo : m_selectedGeos)
        {
            if (geo)
            {
                GeoParameters3D params = geo->getParameters();
                params.showFaces = show;
                geo->setParameters(params);
            }
        }
    }
    else if (m_currentGeo)
    {
        // 单选情况
        GeoParameters3D params = m_currentGeo->getParameters();
        params.showFaces = show;
        m_currentGeo->setParameters(params);
    }
    else
    {
        // 全局设置
        GlobalShowFaces3D = show;
    }
    
    emit parametersChanged();
}




