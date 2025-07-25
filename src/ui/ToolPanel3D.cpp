#include "ToolPanel3D.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QIcon>
#include <QTabWidget>
#include <QMessageBox>
#include <QScrollArea>
#include <QFrame>
#include <QLabel>
#include <QSplitter>
#include <QVariant>

// ========================================= ToolPanel3D 实现 =========================================
ToolPanel3D::ToolPanel3D(QWidget* parent)
    : QWidget(parent)
    , m_currentMode(DrawSelect3D)
{
    setupUI();
    setupStyles();
}

void ToolPanel3D::setupUI()
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
    
    // 创建各个可折叠模块
    createCollapsibleDrawingSection(contentLayout);
    createCollapsibleViewSection(contentLayout);
    createCollapsibleUtilitySection(contentLayout);
    
    contentLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);
}

void ToolPanel3D::createCollapsibleDrawingSection(QVBoxLayout* parentLayout)
{
    // 绘制工具折叠组
    QFrame* sectionFrame = new QFrame();
    sectionFrame->setObjectName("collapsibleSection");
    QVBoxLayout* sectionLayout = new QVBoxLayout(sectionFrame);
    sectionLayout->setSpacing(2);
    sectionLayout->setContentsMargins(4, 4, 4, 4);
    
    // 标题
    QLabel* titleLabel = new QLabel("🎨 绘制工具");
    titleLabel->setObjectName("sectionTitle");
    sectionLayout->addWidget(titleLabel);
    
    // 选择工具单独放置
    m_selectButton = createStyledButton("🔘", "选择", "选择和编辑对象", DrawSelect3D);
    m_selectButton->setChecked(true);
    sectionLayout->addWidget(m_selectButton);
    
    // 模式选择下拉框
    m_drawingModeCombo = new QComboBox();
    m_drawingModeCombo->setObjectName("modeCombo");
    m_drawingModeCombo->addItem("📐 几何体");
    m_drawingModeCombo->addItem("🏠 建筑");
    m_drawingModeCombo->setCurrentIndex(0);
    sectionLayout->addWidget(m_drawingModeCombo);
    
    // 创建堆叠区域
    m_drawingStackedWidget = new QStackedWidget();
    m_drawingStackedWidget->setObjectName("toolStack");
    
    createGeometryPage();
    createBuildingPage();
    
    sectionLayout->addWidget(m_drawingStackedWidget);
    parentLayout->addWidget(sectionFrame);
    
    connect(m_drawingModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolPanel3D::onDrawingModeChanged);
}



void ToolPanel3D::createGeometryPage()
{
    QWidget* geometryPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(geometryPage);
    layout->setSpacing(4);
    layout->setContentsMargins(4, 4, 4, 4);
    
    // 基础绘制工具
    QLabel* basicLabel = new QLabel("📏 基础绘制");
    basicLabel->setObjectName("subGroupLabel");
    layout->addWidget(basicLabel);
    
    m_pointButton = createStyledButton("🔘", "点", "绘制点", DrawPoint3D);
    m_lineButton = createStyledButton("📏", "线", "绘制直线", DrawLine3D);
    m_arcButton = createStyledButton("🌙", "圆弧", "绘制圆弧", DrawArc3D);
    m_bezierButton = createStyledButton("〰️", "贝塞尔", "绘制贝塞尔曲线", DrawBezierCurve3D);
    
    layout->addWidget(m_pointButton);
    layout->addWidget(m_lineButton);
    layout->addWidget(m_arcButton);
    layout->addWidget(m_bezierButton);
    
    // 平面几何
    QLabel* planeLabel = new QLabel("🔷 平面图形");
    planeLabel->setObjectName("subGroupLabel");
    layout->addWidget(planeLabel);
    
    m_triangleButton = createStyledButton("🔺", "三角形", "绘制三角形", DrawTriangle3D);
    m_quadButton = createStyledButton("🔸", "四边形", "绘制四边形", DrawQuad3D);
    m_polygonButton = createStyledButton("⬟", "多边形", "绘制多边形", DrawPolygon3D);
    
    layout->addWidget(m_triangleButton);
    layout->addWidget(m_quadButton);
    layout->addWidget(m_polygonButton);
    
    // 基础立体图形
    QLabel* basicSolidLabel = new QLabel("🧊 基础立体");
    basicSolidLabel->setObjectName("subGroupLabel");
    layout->addWidget(basicSolidLabel);
    
    m_cubeButton = createStyledButton("⬜", "正方体", "绘制正方体", DrawCube3D);
    m_boxButton = createStyledButton("📦", "长方体", "绘制长方体", DrawBox3D);
    m_sphereButton = createStyledButton("⚪", "球体", "绘制球体", DrawSphere3D);
    m_cylinderButton = createStyledButton("🛢️", "圆柱", "绘制圆柱", DrawCylinder3D);
    m_coneButton = createStyledButton("🦀", "圆锥", "绘制圆锥", DrawCone3D);
    m_torusButton = createStyledButton("🍩", "圆环", "绘制圆环", DrawTorus3D);
    
    layout->addWidget(m_cubeButton);
    layout->addWidget(m_boxButton);
    layout->addWidget(m_sphereButton);
    layout->addWidget(m_cylinderButton);
    layout->addWidget(m_coneButton);
    layout->addWidget(m_torusButton);
    
    // 高级立体图形
    QLabel* advancedSolidLabel = new QLabel("🔮 高级立体");
    advancedSolidLabel->setObjectName("subGroupLabel");
    layout->addWidget(advancedSolidLabel);
    
    m_prismButton = createStyledButton("🔶", "多棱柱", "绘制多棱柱", DrawPrism3D);
    m_hemisphereButton = createStyledButton("🌓", "半球", "绘制半球", DrawHemisphere3D);
    m_ellipsoidButton = createStyledButton("🥚", "椭球", "绘制椭球", DrawEllipsoid3D);
    
    layout->addWidget(m_prismButton);
    layout->addWidget(m_hemisphereButton);
    layout->addWidget(m_ellipsoidButton);
    
    layout->addStretch();
    
    m_drawingStackedWidget->addWidget(geometryPage);
}

void ToolPanel3D::createBuildingPage()
{
    QWidget* buildingPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(buildingPage);
    layout->setSpacing(4);
    layout->setContentsMargins(4, 4, 4, 4);
    
    // 建筑类型
    QLabel* buildingLabel = new QLabel("🏠 建筑类型");
    buildingLabel->setObjectName("subGroupLabel");
    layout->addWidget(buildingLabel);
    
    m_flatHouseButton = createStyledButton("🏢", "平顶房", "绘制平顶房屋", DrawFlatHouse3D);
    m_gableHouseButton = createStyledButton("🏘️", "人字房", "绘制人字形房屋", DrawGableHouse3D);
    m_spireHouseButton = createStyledButton("⛪", "尖顶房", "绘制尖顶房屋", DrawSpireHouse3D);
    m_domeHouseButton = createStyledButton("🕌", "穹顶房", "绘制穹顶房屋", DrawDomeHouse3D);
    m_lHouseButton = createStyledButton("🗗️", "L型房", "绘制L型房屋", DrawLHouse3D);
    
    layout->addWidget(m_flatHouseButton);
    layout->addWidget(m_gableHouseButton);
    layout->addWidget(m_spireHouseButton);
    layout->addWidget(m_domeHouseButton);
    layout->addWidget(m_lHouseButton);
    
    layout->addStretch();
    
    m_drawingStackedWidget->addWidget(buildingPage);
}

void ToolPanel3D::createCollapsibleViewSection(QVBoxLayout* parentLayout)
{
    // 视图控制折叠组
    QFrame* sectionFrame = new QFrame();
    sectionFrame->setObjectName("collapsibleSection");
    QVBoxLayout* sectionLayout = new QVBoxLayout(sectionFrame);
    sectionLayout->setSpacing(2);
    sectionLayout->setContentsMargins(4, 4, 4, 4);
    
    // 标题和展开/折叠按钮
    QHBoxLayout* titleLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("👁️ 视图控制");
    titleLabel->setObjectName("sectionTitle");
    
    m_viewToggleButton = new QPushButton("▼");
    m_viewToggleButton->setObjectName("toggleButton");
    m_viewToggleButton->setFixedSize(20, 20);
    m_viewToggleButton->setCheckable(true);
    m_viewToggleButton->setChecked(false);
    
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(m_viewToggleButton);
    
    sectionLayout->addLayout(titleLayout);
    
    // 创建内容区域
    m_viewContentWidget = new QWidget();
    QVBoxLayout* viewLayout = new QVBoxLayout(m_viewContentWidget);
    viewLayout->setSpacing(3);
    viewLayout->setContentsMargins(0, 0, 0, 0);
    
    m_resetViewButton = createActionButton("🔄", "重置视图", "重置相机到默认位置");
    m_fitViewButton = createActionButton("🔍", "适应窗口", "适应所有对象到窗口");
    m_topViewButton = createActionButton("🔽", "俯视图", "切换到俯视图 (T)");
    m_frontViewButton = createActionButton("⬅️", "前视图", "切换到前视图 (1)");
    m_rightViewButton = createActionButton("➡️", "右视图", "切换到右视图 (3)");
    m_isometricViewButton = createActionButton("📐", "等轴测", "切换到等轴测图 (7)");
    
    viewLayout->addWidget(m_resetViewButton);
    viewLayout->addWidget(m_fitViewButton);
    viewLayout->addWidget(m_topViewButton);
    viewLayout->addWidget(m_frontViewButton);
    viewLayout->addWidget(m_rightViewButton);
    viewLayout->addWidget(m_isometricViewButton);
    
    m_viewContentWidget->setVisible(false); // 默认折叠
    sectionLayout->addWidget(m_viewContentWidget);
    
    parentLayout->addWidget(sectionFrame);
    
    // 连接信号
    connect(m_viewToggleButton, &QPushButton::clicked, this, &ToolPanel3D::onViewToggleClicked);
    connect(m_resetViewButton, &QPushButton::clicked, this, &ToolPanel3D::onResetViewClicked);
    connect(m_fitViewButton, &QPushButton::clicked, this, &ToolPanel3D::onFitViewClicked);
    connect(m_topViewButton, &QPushButton::clicked, this, &ToolPanel3D::onTopViewClicked);
    connect(m_frontViewButton, &QPushButton::clicked, this, &ToolPanel3D::onFrontViewClicked);
    connect(m_rightViewButton, &QPushButton::clicked, this, &ToolPanel3D::onRightViewClicked);
    connect(m_isometricViewButton, &QPushButton::clicked, this, &ToolPanel3D::onIsometricViewClicked);
}

void ToolPanel3D::createCollapsibleUtilitySection(QVBoxLayout* parentLayout)
{
    // 实用工具折叠组
    QFrame* sectionFrame = new QFrame();
    sectionFrame->setObjectName("collapsibleSection");
    QVBoxLayout* sectionLayout = new QVBoxLayout(sectionFrame);
    sectionLayout->setSpacing(2);
    sectionLayout->setContentsMargins(4, 4, 4, 4);
    
    // 标题和展开/折叠按钮
    QHBoxLayout* titleLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("🛠️ 实用工具");
    titleLabel->setObjectName("sectionTitle");
    
    m_utilityToggleButton = new QPushButton("▼");
    m_utilityToggleButton->setObjectName("toggleButton");
    m_utilityToggleButton->setFixedSize(20, 20);
    m_utilityToggleButton->setCheckable(true);
    m_utilityToggleButton->setChecked(false);
    
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(m_utilityToggleButton);
    
    sectionLayout->addLayout(titleLayout);
    
    // 创建内容区域
    m_utilityContentWidget = new QWidget();
    QVBoxLayout* utilityLayout = new QVBoxLayout(m_utilityContentWidget);
    utilityLayout->setSpacing(3);
    utilityLayout->setContentsMargins(0, 0, 0, 0);
    
    m_clearSceneButton = createActionButton("🗑️", "清空场景", "删除所有对象");
    m_exportImageButton = createActionButton("📸", "导出图像", "导出当前视图为图像");
    m_pickingSystemButton = createActionButton("🎯", "拾取设置", "设置拾取系统参数");
    m_displaySettingsButton = createActionButton("⚙️", "显示设置", "设置显示参数");
    
    utilityLayout->addWidget(m_clearSceneButton);
    utilityLayout->addWidget(m_exportImageButton);
    utilityLayout->addWidget(m_pickingSystemButton);
    utilityLayout->addWidget(m_displaySettingsButton);
    
    m_utilityContentWidget->setVisible(false); // 默认折叠
    sectionLayout->addWidget(m_utilityContentWidget);
    
    parentLayout->addWidget(sectionFrame);
    
    // 连接信号
    connect(m_utilityToggleButton, &QPushButton::clicked, this, &ToolPanel3D::onUtilityToggleClicked);
    connect(m_clearSceneButton, &QPushButton::clicked, this, &ToolPanel3D::onClearSceneClicked);
    connect(m_exportImageButton, &QPushButton::clicked, this, &ToolPanel3D::onExportImageClicked);
    connect(m_pickingSystemButton, &QPushButton::clicked, this, &ToolPanel3D::onPickingSystemClicked);
    connect(m_displaySettingsButton, &QPushButton::clicked, this, &ToolPanel3D::onDisplaySettingsClicked);
}

QPushButton* ToolPanel3D::createStyledButton(const QString& emoji, const QString& text, const QString& tooltip, DrawMode3D mode)
{
    QPushButton* button = new QPushButton();
    button->setObjectName("geometryButton");
    button->setText(QString("%1 %2").arg(emoji).arg(text));
    button->setToolTip(tooltip);
    button->setCheckable(true);
    button->setProperty("drawMode", static_cast<int>(mode));
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setMinimumHeight(50);
    
    connect(button, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    
    return button;
}

QPushButton* ToolPanel3D::createActionButton(const QString& emoji, const QString& text, const QString& tooltip)
{
    QPushButton* button = new QPushButton();
    button->setObjectName("actionButton");
    button->setText(QString("%1 %2").arg(emoji).arg(text));
    button->setToolTip(tooltip);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setMinimumHeight(45);
    
    return button;
}

void ToolPanel3D::setupStyles()
{
    QString styleSheet = R"(
        /* 整体面板样式 */
        ToolPanel3D {
            background-color: #f8f9fa;
            border: none;
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
        }
        
        /* 滚动区域 */
        QScrollArea {
            border: none;
            background-color: transparent;
        }
        
        /* 标题样式 */
        QLabel#sectionTitle {
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
            font-size: 18px;
            font-weight: bold;
            color: #2c3e50;
            padding: 8px 4px 4px 4px;
            margin-top: 4px;
        }
        
        QLabel#subGroupLabel {
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
            font-size: 15px;
            font-weight: bold;
            color: #34495e;
            padding: 6px 2px 2px 2px;
            margin-top: 8px;
        }
        
        /* 工具框架样式 */
        QFrame#toolFrame {
            background-color: white;
            border: 1px solid #e9ecef;
            border-radius: 8px;
            margin: 2px;
        }
        
        /* Tab Widget样式 */
        QTabWidget#drawingTabs {
            border: none;
        }
        
        QTabWidget#drawingTabs::pane {
            border: 1px solid #dee2e6;
            border-radius: 8px;
            background-color: white;
            margin-top: -1px;
        }
        
        QTabWidget#drawingTabs::tab-bar {
            alignment: center;
        }
        
        QTabBar::tab {
            background-color: #f8f9fa;
            border: 1px solid #dee2e6;
            border-bottom: none;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            padding: 8px 16px;
            margin-right: 2px;
            font-weight: 500;
            color: #6c757d;
        }
        
        QTabBar::tab:selected {
            background-color: white;
            color: #495057;
            border-bottom: 2px solid #007bff;
        }
        
        QTabBar::tab:hover:!selected {
            background-color: #e9ecef;
            color: #495057;
        }
        
        /* 几何体按钮样式 */
        QPushButton#geometryButton {
            background-color: #ffffff;
            border: 1px solid #dee2e6;
            border-radius: 6px;
            padding: 12px;
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
            font-size: 14px;
            font-weight: 600;
            color: #495057;
            text-align: center;
        }
        
        QPushButton#geometryButton:hover {
            background-color: #f8f9fa;
            border-color: #adb5bd;
            transform: translateY(-1px);
        }
        
        QPushButton#geometryButton:checked {
            background-color: #007bff;
            border-color: #007bff;
            color: white;
            font-weight: 600;
        }
        
        QPushButton#geometryButton:pressed {
            background-color: #0056b3;
            border-color: #0056b3;
        }
        
        /* 操作按钮样式 */
        QPushButton#actionButton {
            background-color: #ffffff;
            border: 1px solid #dee2e6;
            border-radius: 4px;
            padding: 10px 14px;
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
            font-size: 15px;
            font-weight: 600;
            color: #495057;
            text-align: left;
        }
        
        QPushButton#actionButton:hover {
            background-color: #f8f9fa;
            border-color: #adb5bd;
        }
        
        QPushButton#actionButton:pressed {
            background-color: #e9ecef;
            border-color: #adb5bd;
        }
        
        /* 复选框样式 */
        QCheckBox#enableCheck {
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
            font-size: 15px;
            font-weight: 600;
            color: #495057;
            spacing: 8px;
        }
        
        QCheckBox#enableCheck::indicator {
            width: 16px;
            height: 16px;
            border: 2px solid #dee2e6;
            border-radius: 3px;
            background-color: white;
        }
        
        QCheckBox#enableCheck::indicator:checked {
            background-color: #28a745;
            border-color: #28a745;
            image: url(:/icons/check.png);
        }
        
        QCheckBox#enableCheck::indicator:hover {
            border-color: #adb5bd;
        }
        
        /* 下拉框样式 */
        QComboBox#modeCombo {
            background-color: #ffffff;
            border: 1px solid #dee2e6;
            border-radius: 4px;
            padding: 10px 14px;
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
            font-size: 15px;
            font-weight: 600;
            color: #495057;
            margin: 4px 0px;
        }
        
        QComboBox#modeCombo:hover {
            border-color: #adb5bd;
        }
        
        QComboBox#modeCombo::drop-down {
            border: none;
            width: 20px;
        }
        
        QComboBox#modeCombo QAbstractItemView {
            background-color: #ffffff;
            border: 1px solid #dee2e6;
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
            font-size: 15px;
            font-weight: 600;
            color: #495057;
            selection-background-color: #007bff;
            selection-color: white;
        }
        
        QComboBox#modeCombo QAbstractItemView::item {
            padding: 8px 12px;
            margin: 1px;
        }
        
        QComboBox#modeCombo QAbstractItemView::item:hover {
            background-color: #f8f9fa;
        }
        
        /* 折叠按钮样式 */
        QPushButton#toggleButton {
            background-color: #ffffff;
            border: 1px solid #dee2e6;
            border-radius: 3px;
            font-family: "Microsoft YaHei", "SimHei", "Arial", sans-serif;
            font-size: 14px;
            font-weight: bold;
            color: #495057;
        }
        
        QPushButton#toggleButton:hover {
            background-color: #f8f9fa;
            border-color: #adb5bd;
        }
    )";
    
    this->setStyleSheet(styleSheet);
}

void ToolPanel3D::updateDrawMode(DrawMode3D mode)
{
    m_currentMode = mode;
    
    // 创建所有按钮的列表
    QList<QPushButton*> allButtons = {
        m_selectButton, m_pointButton, m_lineButton, m_arcButton, m_bezierButton,
        m_triangleButton, m_quadButton, m_polygonButton, m_boxButton, m_cubeButton,
        m_cylinderButton, m_coneButton, m_sphereButton, m_torusButton,
        m_prismButton, m_hemisphereButton, m_ellipsoidButton,
        m_gableHouseButton, m_spireHouseButton, m_domeHouseButton, m_flatHouseButton, m_lHouseButton
    };
    
    // 更新按钮状态
    for (QPushButton* button : allButtons)
    {
        if (button)
        {
            QVariant property = button->property("drawMode");
            DrawMode3D buttonMode = static_cast<DrawMode3D>(property.toInt());
            button->setChecked(buttonMode == mode);
        }
    }
}

void ToolPanel3D::onDrawModeButtonClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    QVariant property = button->property("drawMode");
    DrawMode3D mode = static_cast<DrawMode3D>(property.toInt());
    
    // 创建所有按钮的列表
    QList<QPushButton*> allButtons = {
        m_selectButton, m_pointButton, m_lineButton, m_arcButton, m_bezierButton,
        m_triangleButton, m_quadButton, m_polygonButton, m_boxButton, m_cubeButton,
        m_cylinderButton, m_coneButton, m_sphereButton, m_torusButton,
        m_prismButton, m_hemisphereButton, m_ellipsoidButton,
        m_gableHouseButton, m_spireHouseButton, m_domeHouseButton, m_flatHouseButton, m_lHouseButton
    };
    
    // 取消其他按钮的选中状态
    for (QPushButton* otherButton : allButtons)
    {
        if (otherButton && otherButton != button)
        {
            otherButton->setChecked(false);
        }
    }
    
    button->setChecked(true);
    m_currentMode = mode;
    
    emit drawModeChanged(mode);
}

// 视图工具相关槽函数
void ToolPanel3D::onResetViewClicked()
{
    emit resetViewRequested();
}

void ToolPanel3D::onFitViewClicked()
{
    emit fitViewRequested();
}

void ToolPanel3D::onTopViewClicked()
{
    emit topViewRequested();
}

void ToolPanel3D::onFrontViewClicked()
{
    emit frontViewRequested();
}

void ToolPanel3D::onRightViewClicked()
{
    emit rightViewRequested();
}

void ToolPanel3D::onIsometricViewClicked()
{
    emit isometricViewRequested();
}

// 实用工具相关槽函数
void ToolPanel3D::onClearSceneClicked()
{
    emit clearSceneRequested();
}

void ToolPanel3D::onExportImageClicked()
{
    emit exportImageRequested();
}

void ToolPanel3D::onPickingSystemClicked()
{
    emit pickingSystemRequested();
}

void ToolPanel3D::onDisplaySettingsClicked()
{
    emit displaySettingsRequested();
}

void ToolPanel3D::onDrawingModeChanged(int index)
{
    if (m_drawingStackedWidget) {
        m_drawingStackedWidget->setCurrentIndex(index);
    }
}

// 折叠/展开槽函数
void ToolPanel3D::onViewToggleClicked()
{
    bool isExpanded = m_viewToggleButton->isChecked();
    m_viewContentWidget->setVisible(isExpanded);
    m_viewToggleButton->setText(isExpanded ? "▲" : "▼");
}

void ToolPanel3D::onUtilityToggleClicked()
{
    bool isExpanded = m_utilityToggleButton->isChecked();
    m_utilityContentWidget->setVisible(isExpanded);
    m_utilityToggleButton->setText(isExpanded ? "▲" : "▼");
}


