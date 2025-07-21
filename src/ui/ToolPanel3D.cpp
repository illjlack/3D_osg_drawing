#include "ToolPanel3D.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QIcon>
#include <QStackedWidget>
#include <QMessageBox>

// ========================================= ToolPanel3D 实现 =========================================
ToolPanel3D::ToolPanel3D(QWidget* parent)
    : QWidget(parent)
    , m_currentMode(DrawSelect3D)
{
    setupUI();
}

void ToolPanel3D::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    
    createDrawingGroup();
    createViewGroup();
    createUtilityGroup();
    createSkyboxGroup();
    
    mainLayout->addWidget(m_drawingGroup);
    mainLayout->addWidget(m_viewGroup);
    mainLayout->addWidget(m_utilityGroup);
    mainLayout->addWidget(m_skyboxGroup);
    mainLayout->addStretch();
}

void ToolPanel3D::createDrawingGroup()
{
    m_drawingGroup = new QGroupBox("绘制工具", this);
    QVBoxLayout* mainLayout = new QVBoxLayout(m_drawingGroup);
    
    // 创建分类选择下拉框
    m_drawingCategoryCombo = new QComboBox();
    m_drawingCategoryCombo->addItem("选择模式");
    m_drawingCategoryCombo->addItem("基本几何体");
    m_drawingCategoryCombo->addItem("建筑类型");
    // m_drawingCategoryCombo->addItem("高级几何体"); // 移除高级几何体
    connect(m_drawingCategoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolPanel3D::onDrawingCategoryChanged);
    mainLayout->addWidget(m_drawingCategoryCombo);
    
    // 创建堆叠窗口来容纳不同类别的按钮
    m_drawingStackedWidget = new QStackedWidget();
    mainLayout->addWidget(m_drawingStackedWidget);
    
    // 创建选择模式页面
    createSelectPage();
    // 创建基本几何体页面
    createBasicGeometryPage();
    // 创建建筑类型页面
    createBuildingPage();
    // createAdvancedGeometryPage(); // 移除高级几何体页面
}

void ToolPanel3D::createSelectPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    m_selectButton = new QPushButton("选择");
    m_selectButton->setCheckable(true);
    m_selectButton->setChecked(true);
    m_selectButton->setToolTip("选择和编辑对象");
    m_selectButton->setProperty("drawMode", DrawSelect3D);
    layout->addWidget(m_selectButton);
    connect(m_selectButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    layout->addStretch();
    m_drawingStackedWidget->addWidget(page);
}

void ToolPanel3D::createBasicGeometryPage()
{
    QWidget* page = new QWidget();
    QGridLayout* layout = new QGridLayout(page);
    // 点绘制
    m_pointButton = new QPushButton("点");
    m_pointButton->setCheckable(true);
    m_pointButton->setToolTip("绘制点");
    m_pointButton->setProperty("drawMode", DrawPoint3D);
    layout->addWidget(m_pointButton, 0, 0);
    // 线绘制
    m_lineButton = new QPushButton("线");
    m_lineButton->setCheckable(true);
    m_lineButton->setToolTip("绘制直线或折线");
    m_lineButton->setProperty("drawMode", DrawLine3D);
    layout->addWidget(m_lineButton, 1, 0);
    m_arcButton = new QPushButton("圆弧");
    m_arcButton->setCheckable(true);
    m_arcButton->setToolTip("绘制圆弧");
    m_arcButton->setProperty("drawMode", DrawArc3D);
    layout->addWidget(m_arcButton, 1, 1);
    m_bezierButton = new QPushButton("贝塞尔");
    m_bezierButton->setCheckable(true);
    m_bezierButton->setToolTip("绘制贝塞尔曲线");
    m_bezierButton->setProperty("drawMode", DrawBezierCurve3D);
    layout->addWidget(m_bezierButton, 2, 0);
    // 面绘制
    m_triangleButton = new QPushButton("三角形");
    m_triangleButton->setCheckable(true);
    m_triangleButton->setToolTip("绘制三角形");
    m_triangleButton->setProperty("drawMode", DrawTriangle3D);
    layout->addWidget(m_triangleButton, 2, 1);
    m_quadButton = new QPushButton("四边形");
    m_quadButton->setCheckable(true);
    m_quadButton->setToolTip("绘制四边形");
    m_quadButton->setProperty("drawMode", DrawQuad3D);
    layout->addWidget(m_quadButton, 3, 0);
    m_polygonButton = new QPushButton("多边形");
    m_polygonButton->setCheckable(true);
    m_polygonButton->setToolTip("绘制多边形");
    m_polygonButton->setProperty("drawMode", DrawPolygon3D);
    layout->addWidget(m_polygonButton, 3, 1);
    // 体绘制
    m_boxButton = new QPushButton("长方体");
    m_boxButton->setCheckable(true);
    m_boxButton->setToolTip("绘制长方体");
    m_boxButton->setProperty("drawMode", DrawBox3D);
    layout->addWidget(m_boxButton, 4, 0);
    m_cubeButton = new QPushButton("正方体");
    m_cubeButton->setCheckable(true);
    m_cubeButton->setToolTip("绘制正方体");
    m_cubeButton->setProperty("drawMode", DrawCube3D);
    layout->addWidget(m_cubeButton, 4, 1);
    m_cylinderButton = new QPushButton("圆柱");
    m_cylinderButton->setCheckable(true);
    m_cylinderButton->setToolTip("绘制圆柱");
    m_cylinderButton->setProperty("drawMode", DrawCylinder3D);
    layout->addWidget(m_cylinderButton, 5, 0);
    m_coneButton = new QPushButton("圆锥");
    m_coneButton->setCheckable(true);
    m_coneButton->setToolTip("绘制圆锥");
    m_coneButton->setProperty("drawMode", DrawCone3D);
    layout->addWidget(m_coneButton, 5, 1);
    m_sphereButton = new QPushButton("球");
    m_sphereButton->setCheckable(true);
    m_sphereButton->setToolTip("绘制球");
    m_sphereButton->setProperty("drawMode", DrawSphere3D);
    layout->addWidget(m_sphereButton, 6, 0);
    m_torusButton = new QPushButton("圆环");
    m_torusButton->setCheckable(true);
    m_torusButton->setToolTip("绘制圆环");
    m_torusButton->setProperty("drawMode", DrawTorus3D);
    layout->addWidget(m_torusButton, 6, 1);
    // 高级几何体
    m_prismButton = new QPushButton("多棱柱");
    m_prismButton->setCheckable(true);
    m_prismButton->setToolTip("绘制多棱柱");
    m_prismButton->setProperty("drawMode", DrawPrism3D);
    layout->addWidget(m_prismButton, 7, 0);
    m_hemisphereButton = new QPushButton("半球");
    m_hemisphereButton->setCheckable(true);
    m_hemisphereButton->setToolTip("绘制半球");
    m_hemisphereButton->setProperty("drawMode", DrawHemisphere3D);
    layout->addWidget(m_hemisphereButton, 7, 1);
    m_ellipsoidButton = new QPushButton("椭球");
    m_ellipsoidButton->setCheckable(true);
    m_ellipsoidButton->setToolTip("绘制椭球");
    m_ellipsoidButton->setProperty("drawMode", DrawEllipsoid3D);
    layout->addWidget(m_ellipsoidButton, 8, 0);
    // 连接信号
    connect(m_pointButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_lineButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_arcButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_bezierButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_triangleButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_quadButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_polygonButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_boxButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_cubeButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_cylinderButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_coneButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_sphereButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_torusButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_prismButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_hemisphereButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_ellipsoidButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    m_drawingStackedWidget->addWidget(page);
}

void ToolPanel3D::createBuildingPage()
{
    QWidget* page = new QWidget();
    QGridLayout* layout = new QGridLayout(page);
    
    // 建筑类型按钮
    m_gableHouseButton = new QPushButton("人字房");
    m_gableHouseButton->setCheckable(true);
    m_gableHouseButton->setToolTip("绘制人字形房屋");
    m_gableHouseButton->setProperty("drawMode", DrawGableHouse3D);
    layout->addWidget(m_gableHouseButton, 0, 0);
    
    m_spireHouseButton = new QPushButton("尖顶房");
    m_spireHouseButton->setCheckable(true);
    m_spireHouseButton->setToolTip("绘制尖顶房屋");
    m_spireHouseButton->setProperty("drawMode", DrawSpireHouse3D);
    layout->addWidget(m_spireHouseButton, 0, 1);
    
    m_domeHouseButton = new QPushButton("穹顶房");
    m_domeHouseButton->setCheckable(true);
    m_domeHouseButton->setToolTip("绘制穹顶房屋");
    m_domeHouseButton->setProperty("drawMode", DrawDomeHouse3D);
    layout->addWidget(m_domeHouseButton, 1, 0);
    
    m_flatHouseButton = new QPushButton("平顶房");
    m_flatHouseButton->setCheckable(true);
    m_flatHouseButton->setToolTip("绘制平顶房屋");
    m_flatHouseButton->setProperty("drawMode", DrawFlatHouse3D);
    layout->addWidget(m_flatHouseButton, 1, 1);
    
    m_lHouseButton = new QPushButton("L型房");
    m_lHouseButton->setCheckable(true);
    m_lHouseButton->setToolTip("绘制L型房屋");
    m_lHouseButton->setProperty("drawMode", DrawLHouse3D);
    layout->addWidget(m_lHouseButton, 2, 0);
    
    // 连接信号
    connect(m_gableHouseButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_spireHouseButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_domeHouseButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_flatHouseButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    connect(m_lHouseButton, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    
    m_drawingStackedWidget->addWidget(page);
}

void ToolPanel3D::createViewGroup()
{
    m_viewGroup = new QGroupBox("视图工具", this);
    QVBoxLayout* layout = new QVBoxLayout(m_viewGroup);
    
    // 重置视图按钮
    m_resetViewButton = new QPushButton("重置视图");
    m_resetViewButton->setToolTip("重置相机到默认位置");
    m_resetViewButton->setIcon(QIcon(":/icons/reset.png"));
    layout->addWidget(m_resetViewButton);
    
    // 适应窗口按钮
    m_fitViewButton = new QPushButton("适应窗口");
    m_fitViewButton->setToolTip("适应所有对象到窗口");
    m_fitViewButton->setIcon(QIcon(":/icons/fit.png"));
    layout->addWidget(m_fitViewButton);
    
    // 俯视图按钮
    m_topViewButton = new QPushButton("俯视图");
    m_topViewButton->setToolTip("切换到俯视图 (T)");
    m_topViewButton->setIcon(QIcon(":/icons/top.png"));
    layout->addWidget(m_topViewButton);
    
    // 前视图按钮
    m_frontViewButton = new QPushButton("前视图");
    m_frontViewButton->setToolTip("切换到前视图 (1)");
    m_frontViewButton->setIcon(QIcon(":/icons/front.png"));
    layout->addWidget(m_frontViewButton);
    
    // 右视图按钮
    m_rightViewButton = new QPushButton("右视图");
    m_rightViewButton->setToolTip("切换到右视图 (3)");
    m_rightViewButton->setIcon(QIcon(":/icons/right.png"));
    layout->addWidget(m_rightViewButton);
    
    // 等轴测图按钮
    m_isometricViewButton = new QPushButton("等轴测图");
    m_isometricViewButton->setToolTip("切换到等轴测图 (7)");
    m_isometricViewButton->setIcon(QIcon(":/icons/isometric.png"));
    layout->addWidget(m_isometricViewButton);
    
    // 连接信号
    connect(m_resetViewButton, &QPushButton::clicked, this, &ToolPanel3D::onResetViewClicked);
    connect(m_fitViewButton, &QPushButton::clicked, this, &ToolPanel3D::onFitViewClicked);
    connect(m_topViewButton, &QPushButton::clicked, this, &ToolPanel3D::onTopViewClicked);
    connect(m_frontViewButton, &QPushButton::clicked, this, &ToolPanel3D::onFrontViewClicked);
    connect(m_rightViewButton, &QPushButton::clicked, this, &ToolPanel3D::onRightViewClicked);
    connect(m_isometricViewButton, &QPushButton::clicked, this, &ToolPanel3D::onIsometricViewClicked);
}

void ToolPanel3D::createUtilityGroup()
{
    m_utilityGroup = new QGroupBox("实用工具", this);
    QVBoxLayout* layout = new QVBoxLayout(m_utilityGroup);
    
    // 清空场景按钮
    m_clearSceneButton = new QPushButton("清空场景");
    m_clearSceneButton->setToolTip("删除所有对象");
    m_clearSceneButton->setIcon(QIcon(":/icons/clear.png"));
    layout->addWidget(m_clearSceneButton);
    
    // 导出图像按钮
    m_exportImageButton = new QPushButton("导出图像");
    m_exportImageButton->setToolTip("导出当前视图为图像");
    m_exportImageButton->setIcon(QIcon(":/icons/export.png"));
    layout->addWidget(m_exportImageButton);
    
    // 坐标系统设置按钮
    m_coordinateSystemButton = new QPushButton("坐标系统设置");
    m_coordinateSystemButton->setToolTip("设置坐标系统参数");
    m_coordinateSystemButton->setIcon(QIcon(":/icons/coordinate.png"));
    layout->addWidget(m_coordinateSystemButton);
    
    // 拾取系统设置按钮
    m_pickingSystemButton = new QPushButton("拾取系统设置");
    m_pickingSystemButton->setToolTip("设置拾取系统参数");
    m_pickingSystemButton->setIcon(QIcon(":/icons/picking.png"));
    layout->addWidget(m_pickingSystemButton);
    
    // 显示设置按钮
    m_displaySettingsButton = new QPushButton("显示设置");
    m_displaySettingsButton->setToolTip("设置显示参数");
    m_displaySettingsButton->setIcon(QIcon(":/icons/display.png"));
    layout->addWidget(m_displaySettingsButton);
    
    // 连接信号
    connect(m_clearSceneButton, &QPushButton::clicked, this, &ToolPanel3D::onClearSceneClicked);
    connect(m_exportImageButton, &QPushButton::clicked, this, &ToolPanel3D::onExportImageClicked);
    connect(m_coordinateSystemButton, &QPushButton::clicked, this, &ToolPanel3D::onCoordinateSystemClicked);
    connect(m_pickingSystemButton, &QPushButton::clicked, this, &ToolPanel3D::onPickingSystemClicked);
    connect(m_displaySettingsButton, &QPushButton::clicked, this, &ToolPanel3D::onDisplaySettingsClicked);
}

void ToolPanel3D::createSkyboxGroup()
{
    m_skyboxGroup = new QGroupBox("天空盒设置", this);
    QVBoxLayout* layout = new QVBoxLayout(m_skyboxGroup);
    
    // 天空盒启用开关
    m_skyboxEnabledCheck = new QCheckBox("启用天空盒");
    m_skyboxEnabledCheck->setChecked(true);
    m_skyboxEnabledCheck->setToolTip("启用或禁用天空盒");
    layout->addWidget(m_skyboxEnabledCheck);
    
    // 天空盒样式按钮
    m_skyboxGradientButton = new QPushButton("渐变天空盒");
    m_skyboxGradientButton->setToolTip("设置渐变天空盒");
    layout->addWidget(m_skyboxGradientButton);
    
    m_skyboxSolidButton = new QPushButton("纯色天空盒");
    m_skyboxSolidButton->setToolTip("设置纯色天空盒");
    layout->addWidget(m_skyboxSolidButton);
    
    m_skyboxCustomButton = new QPushButton("自定义立方体贴图");
    m_skyboxCustomButton->setToolTip("设置自定义立方体贴图天空盒");
    layout->addWidget(m_skyboxCustomButton);
    
    // 连接信号
    connect(m_skyboxEnabledCheck, &QCheckBox::toggled, this, &ToolPanel3D::onSkyboxEnabledChanged);
    connect(m_skyboxGradientButton, &QPushButton::clicked, this, &ToolPanel3D::onSkyboxGradientClicked);
    connect(m_skyboxSolidButton, &QPushButton::clicked, this, &ToolPanel3D::onSkyboxSolidClicked);
    connect(m_skyboxCustomButton, &QPushButton::clicked, this, &ToolPanel3D::onSkyboxCustomClicked);
}

void ToolPanel3D::updateDrawMode(DrawMode3D mode)
{
    m_currentMode = mode;
    
    // 创建所有按钮的列表
    QList<QPushButton*> allButtons = {
        m_selectButton, m_pointButton, m_lineButton, m_arcButton, m_bezierButton,
        m_triangleButton, m_quadButton, m_polygonButton, m_boxButton, m_cubeButton,
        m_cylinderButton, m_coneButton, m_sphereButton, m_torusButton,
        m_gableHouseButton, m_spireHouseButton, m_domeHouseButton, m_flatHouseButton, m_lHouseButton
        // , m_prismButton, m_hemisphereButton, m_ellipsoidButton // 移除高级几何体按钮
    };
    
    // 更新按钮状态
    for (QPushButton* button : allButtons)
    {
        if (button)
        {
            DrawMode3D buttonMode = static_cast<DrawMode3D>(button->property("drawMode").toInt());
            button->setChecked(buttonMode == mode);
        }
    }
}

void ToolPanel3D::onDrawModeButtonClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    DrawMode3D mode = static_cast<DrawMode3D>(button->property("drawMode").toInt());
    
    // 创建所有按钮的列表
    QList<QPushButton*> allButtons = {
        m_selectButton, m_pointButton, m_lineButton, m_arcButton, m_bezierButton,
        m_triangleButton, m_quadButton, m_polygonButton, m_boxButton, m_cubeButton,
        m_cylinderButton, m_coneButton, m_sphereButton, m_torusButton,
        m_gableHouseButton, m_spireHouseButton, m_domeHouseButton, m_flatHouseButton, m_lHouseButton
        // , m_prismButton, m_hemisphereButton, m_ellipsoidButton // 移除高级几何体按钮
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

// ToolPanel3D 天空盒相关槽函数
void ToolPanel3D::onSkyboxEnabledChanged(bool enabled)
{
    emit skyboxEnabled(enabled);
}

void ToolPanel3D::onSkyboxGradientClicked()
{
    emit skyboxGradientRequested();
}

void ToolPanel3D::onSkyboxSolidClicked()
{
    emit skyboxSolidRequested();
}

void ToolPanel3D::onSkyboxCustomClicked()
{
    emit skyboxCustomRequested();
}

// ToolPanel3D 视图工具相关槽函数
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

// ToolPanel3D 实用工具相关槽函数
void ToolPanel3D::onClearSceneClicked()
{
    emit clearSceneRequested();
}

void ToolPanel3D::onExportImageClicked()
{
    emit exportImageRequested();
}

void ToolPanel3D::onCoordinateSystemClicked()
{
    emit coordinateSystemRequested();
}

void ToolPanel3D::onPickingSystemClicked()
{
    emit pickingSystemRequested();
}

void ToolPanel3D::onDisplaySettingsClicked()
{
    emit displaySettingsRequested();
}

void ToolPanel3D::onDrawingCategoryChanged(int index)
{
    if (m_drawingStackedWidget) {
        m_drawingStackedWidget->setCurrentIndex(index);
    }
}
