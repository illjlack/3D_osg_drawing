#include "MainWindow.h"
#include "../core/GeometryBase.h"
#include "../core/picking/OSGIndexPickingSystem.h"
#include <QTimer>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QButtonGroup>
#include <QApplication>
#include <QScreen>
#include <QFileDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QDebug>
#include <QPixmap>
#include <QGroupBox>
#include <QCheckBox>
#include <QSlider>

// ========================================= MainWindow 实现 =========================================
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_osgWidget(nullptr)
    , m_propertyEditor(nullptr)
    , m_toolPanel(nullptr)
    , m_logOutputWidget(nullptr)
    , m_modified(false)
{
    setWindowTitle("3D Drawing Board");
    setWindowIcon(QIcon(":/icons/app.png"));
    
    // 设置全局状态栏
    GlobalStatusBar3D = statusBar();
    
    setupUI();
    createMenus();
    createToolBars();
    createStatusBar();
    createDockWidgets();
    connectSignals();
    
    // 设置初始大小和位置
    resize(1200, 800);
    
    // 居中显示
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    updateDrawModeUI();
    updateStatusBar("Ready");
    updateObjectCount(); // 初始化对象数量显示
    
    // 初始化投影模式控件状态
    if (m_projectionModeCombo && m_perspectiveFOVSpinBox && m_orthographicSizeSpinBox)
    {
        // 默认启用透视FOV控件，禁用正交大小控件
        m_perspectiveFOVSpinBox->setEnabled(true);
        m_orthographicSizeSpinBox->setEnabled(false);
    }
    
    // 初始化坐标系统
    CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
    
    // 连接坐标系统信号
    connect(coordSystem, &CoordinateSystem3D::coordinateRangeChanged,
            this, [this](const CoordinateSystem3D::CoordinateRange& range) {
                updateCoordinateRangeLabel();
                LOG_INFO(tr("坐标系统范围已更新: X[%1,%2] Y[%3,%4] Z[%5,%6]")
                    .arg(range.minX, 0, 'f', 0)
                    .arg(range.maxX, 0, 'f', 0)
                    .arg(range.minY, 0, 'f', 0)
                    .arg(range.maxY, 0, 'f', 0)
                    .arg(range.minZ, 0, 'f', 0)
                    .arg(range.maxZ, 0, 'f', 0), "坐标系统");
            });
    
    // 连接OSGWidget的鼠标位置变化信号（在OSGWidget创建后）
    if (m_osgWidget)
    {
        connect(m_osgWidget, &OSGWidget::mousePositionChanged,
                this, [this](const glm::vec3& pos) {
                    m_positionLabel->setText(QString("位置: (%1, %2, %3)")
                        .arg(pos.x, 0, 'f', 2)
                        .arg(pos.y, 0, 'f', 2)
                        .arg(pos.z, 0, 'f', 2));
                });
        
        // 连接摄像机移动速度变化
        connect(m_osgWidget, &OSGWidget::cameraMoveSpeedChanged,
                this, [this](double speed) {
                    // 更新状态栏中的摄像机速度显示
                    QList<QLabel*> labels = findChildren<QLabel*>();
                    for (QLabel* label : labels)
                    {
                        if (label->text().startsWith("摄像机速度:"))
                        {
                            label->setText(QString("摄像机速度: %1").arg(speed, 0, 'f', 0));
                            break;
                        }
                    }
                    LOG_DEBUG(tr("摄像机移动速度已更改为: %1").arg(speed, 0, 'f', 0), "摄像机");
                });
    }
    
    updateCoordinateRangeLabel();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    // 创建中央widget
    m_osgWidget = new OSGWidget(this);
    setCentralWidget(m_osgWidget);
    
    // 创建属性编辑器
    m_propertyEditor = new PropertyEditor3D(this);
    
    // 创建工具面板
    m_toolPanel = new ToolPanel3D(this);
    
    // 创建日志输出栏
    m_logOutputWidget = new LogOutputWidget(this);
    
    // 添加初始日志信息（在UI创建完成后）
    LOG_INFO("3D绘图板启动完成", "系统");
    LOG_INFO("日志系统已初始化", "系统");
    LOG_DEBUG("调试模式已启用", "系统");
}

void MainWindow::createMenus()
{
    // 文件菜单
    m_fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    
    QAction* newAction = m_fileMenu->addAction(tr("新建(&N)"));
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::onFileNew);
    
    QAction* openAction = m_fileMenu->addAction(tr("打开(&O)"));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onFileOpen);
    
    m_fileMenu->addSeparator();
    
    QAction* saveAction = m_fileMenu->addAction(tr("保存(&S)"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onFileSave);
    
    QAction* saveAsAction = m_fileMenu->addAction(tr("另存为(&A)"));
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::onFileSaveAs);
    
    m_fileMenu->addSeparator();
    
    QAction* exitAction = m_fileMenu->addAction(tr("退出(&X)"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::onFileExit);
    
    // 编辑菜单
    m_editMenu = menuBar()->addMenu(tr("编辑(&E)"));
    
    QAction* undoAction = m_editMenu->addAction(tr("撤销(&U)"));
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, &MainWindow::onEditUndo);
    
    QAction* redoAction = m_editMenu->addAction(tr("重做(&R)"));
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, &MainWindow::onEditRedo);
    
    m_editMenu->addSeparator();
    
    QAction* copyAction = m_editMenu->addAction(tr("复制(&C)"));
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &MainWindow::onEditCopy);
    
    QAction* pasteAction = m_editMenu->addAction(tr("粘贴(&P)"));
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, &MainWindow::onEditPaste);
    
    QAction* deleteAction = m_editMenu->addAction(tr("删除(&D)"));
    deleteAction->setShortcut(QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onEditDelete);
    
    m_editMenu->addSeparator();
    
    QAction* selectAllAction = m_editMenu->addAction(tr("全选(&A)"));
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(selectAllAction, &QAction::triggered, this, &MainWindow::onEditSelectAll);
    
    // 视图菜单
    m_viewMenu = menuBar()->addMenu(tr("视图(&V)"));
    
    QAction* resetCameraAction = m_viewMenu->addAction(tr("重置相机(&R)"));
    connect(resetCameraAction, &QAction::triggered, this, &MainWindow::onViewResetCamera);
    
    QAction* fitAllAction = m_viewMenu->addAction(tr("适应窗口(&F)"));
    fitAllAction->setShortcut(Qt::Key_F);
    connect(fitAllAction, &QAction::triggered, this, &MainWindow::onViewFitAll);
    
    m_viewMenu->addSeparator();
    
    QAction* topViewAction = m_viewMenu->addAction(tr("俯视图(&T)"));
    topViewAction->setShortcut(Qt::Key_T);
    connect(topViewAction, &QAction::triggered, this, &MainWindow::onViewTop);
    
    QAction* frontViewAction = m_viewMenu->addAction(tr("前视图(&F)"));
    frontViewAction->setShortcut(Qt::Key_1);
    connect(frontViewAction, &QAction::triggered, this, &MainWindow::onViewFront);
    
    QAction* rightViewAction = m_viewMenu->addAction(tr("右视图(&R)"));
    rightViewAction->setShortcut(Qt::Key_3);
    connect(rightViewAction, &QAction::triggered, this, &MainWindow::onViewRight);
    
    QAction* isometricAction = m_viewMenu->addAction(tr("等轴测图(&I)"));
    isometricAction->setShortcut(Qt::Key_7);
    connect(isometricAction, &QAction::triggered, this, &MainWindow::onViewIsometric);
    
    m_viewMenu->addSeparator();
    
    QAction* wireframeAction = m_viewMenu->addAction(tr("线框模式(&W)"));
    wireframeAction->setCheckable(true);
    connect(wireframeAction, &QAction::triggered, this, &MainWindow::onViewWireframe);
    
    QAction* shadedAction = m_viewMenu->addAction(tr("着色模式(&S)"));
    shadedAction->setCheckable(true);
    shadedAction->setChecked(true);
    connect(shadedAction, &QAction::triggered, this, &MainWindow::onViewShaded);
    
    QAction* shadedWireframeAction = m_viewMenu->addAction(tr("着色+线框(&H)"));
    shadedWireframeAction->setCheckable(true);
    connect(shadedWireframeAction, &QAction::triggered, this, &MainWindow::onViewShadedWireframe);
    
    m_viewMenu->addSeparator();
    
    // 天空盒菜单
    QAction* skyboxAction = m_viewMenu->addAction(tr("天空盒(&K)"));
    skyboxAction->setCheckable(true);
    skyboxAction->setChecked(true);
    connect(skyboxAction, &QAction::triggered, this, &MainWindow::onViewSkybox);
    
    QMenu* skyboxMenu = m_viewMenu->addMenu(tr("天空盒样式(&S)"));
    
    QAction* gradientSkyboxAction = skyboxMenu->addAction(tr("渐变天空盒(&G)"));
    connect(gradientSkyboxAction, &QAction::triggered, this, &MainWindow::onSkyboxGradient);
    
    QAction* solidSkyboxAction = skyboxMenu->addAction(tr("纯色天空盒(&S)"));
    connect(solidSkyboxAction, &QAction::triggered, this, &MainWindow::onSkyboxSolid);
    
    QAction* customSkyboxAction = skyboxMenu->addAction(tr("自定义立方体贴图(&C)"));
    connect(customSkyboxAction, &QAction::triggered, this, &MainWindow::onSkyboxCustom);
    
    m_viewMenu->addSeparator();
    
    // 坐标系统设置
    QAction* coordSystemAction = m_viewMenu->addAction(tr("坐标系统设置(&C)"));
    coordSystemAction->setShortcut(Qt::CTRL + Qt::Key_C);
    connect(coordSystemAction, &QAction::triggered, this, &MainWindow::onCoordinateSystemSettings);
    
    // 帮助菜单
    m_helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    
    QAction* cameraControlAction = m_helpMenu->addAction(tr("摄像机控制说明"));
    connect(cameraControlAction, &QAction::triggered, this, [this]() {
        QMessageBox::information(this, tr("摄像机控制说明"), 
            tr("摄像机控制快捷键：\n\n"
               "W 或 ↑ - 摄像机上移\n"
               "S 或 ↓ - 摄像机下移\n"
               "A 或 ← - 摄像机左移\n"
               "D 或 → - 摄像机右移\n"
               "Q - 摄像机前进\n"
               "E - 摄像机后退\n\n"
               "鼠标操作：\n"
               "左键拖拽 - 旋转视角\n"
               "右键拖拽 - 缩放\n"
               "中键拖拽 - 平移\n\n"
               "您也可以使用工具栏按钮进行摄像机控制。"));
    });
    
    m_helpMenu->addSeparator();
    
    QAction* aboutAction = m_helpMenu->addAction(tr("关于(&A)"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onHelpAbout);
}

void MainWindow::createToolBars()
{
    // 主工具栏
    m_mainToolBar = addToolBar(tr("主工具栏"));
    m_mainToolBar->setObjectName("MainToolBar");
    
    // 添加常用操作
    m_mainToolBar->addAction(tr("新建"), this, &MainWindow::onFileNew);
    m_mainToolBar->addAction(tr("打开"), this, &MainWindow::onFileOpen);
    m_mainToolBar->addAction(tr("保存"), this, &MainWindow::onFileSave);
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(tr("撤销"), this, &MainWindow::onEditUndo);
    m_mainToolBar->addAction(tr("重做"), this, &MainWindow::onEditRedo);
    
    // 视图工具栏
    m_viewToolBar = addToolBar(tr("视图"));
    m_viewToolBar->setObjectName("ViewToolBar");
    
    // 摄像机控制按钮
    QAction* cameraUpAction = m_viewToolBar->addAction(tr("上移"));
    cameraUpAction->setIcon(QIcon(":/icons/up.png"));
    cameraUpAction->setToolTip(tr("摄像机上移 (W/↑)"));
    connect(cameraUpAction, &QAction::triggered, this, [this]() {
        if (m_osgWidget && m_osgWidget->getCameraController()) 
            m_osgWidget->getCameraController()->moveUp(1.0);
    });
    
    QAction* cameraDownAction = m_viewToolBar->addAction(tr("下移"));
    cameraDownAction->setIcon(QIcon(":/icons/down.png"));
    cameraDownAction->setToolTip(tr("摄像机下移 (S/↓)"));
    connect(cameraDownAction, &QAction::triggered, this, [this]() {
        if (m_osgWidget && m_osgWidget->getCameraController()) 
            m_osgWidget->getCameraController()->moveDown(1.0);
    });
    
    QAction* cameraLeftAction = m_viewToolBar->addAction(tr("左移"));
    cameraLeftAction->setIcon(QIcon(":/icons/left.png"));
    cameraLeftAction->setToolTip(tr("摄像机左移 (A/←)"));
    connect(cameraLeftAction, &QAction::triggered, this, [this]() {
        if (m_osgWidget && m_osgWidget->getCameraController()) 
            m_osgWidget->getCameraController()->moveLeft(1.0);
    });
    
    QAction* cameraRightAction = m_viewToolBar->addAction(tr("右移"));
    cameraRightAction->setIcon(QIcon(":/icons/right.png"));
    cameraRightAction->setToolTip(tr("摄像机右移 (D/→)"));
    connect(cameraRightAction, &QAction::triggered, this, [this]() {
        if (m_osgWidget && m_osgWidget->getCameraController()) 
            m_osgWidget->getCameraController()->moveRight(1.0);
    });
    
    QAction* cameraForwardAction = m_viewToolBar->addAction(tr("前进"));
    cameraForwardAction->setIcon(QIcon(":/icons/forward.png"));
    cameraForwardAction->setToolTip(tr("摄像机前进 (Q)"));
    connect(cameraForwardAction, &QAction::triggered, this, [this]() {
        if (m_osgWidget && m_osgWidget->getCameraController()) 
            m_osgWidget->getCameraController()->moveForward(1.0);
    });
    
    QAction* cameraBackwardAction = m_viewToolBar->addAction(tr("后退"));
    cameraBackwardAction->setIcon(QIcon(":/icons/backward.png"));
    cameraBackwardAction->setToolTip(tr("摄像机后退 (E)"));
    connect(cameraBackwardAction, &QAction::triggered, this, [this]() {
        if (m_osgWidget && m_osgWidget->getCameraController()) 
            m_osgWidget->getCameraController()->moveBackward(1.0);
    });
    
    m_viewToolBar->addSeparator();
    
    // 投影模式控制
    m_viewToolBar->addWidget(new QLabel(tr("投影:")));
    m_projectionModeCombo = new QComboBox();
    m_projectionModeCombo->addItem(tr("透视"), static_cast<int>(ProjectionMode::Perspective));
    m_projectionModeCombo->addItem(tr("正交"), static_cast<int>(ProjectionMode::Orthographic));
    m_projectionModeCombo->setCurrentIndex(0);
    m_projectionModeCombo->setToolTip(tr("切换投影模式"));
    connect(m_projectionModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProjectionModeChanged);
    m_viewToolBar->addWidget(m_projectionModeCombo);
    
    // 透视FOV控制
    m_viewToolBar->addWidget(new QLabel(tr("FOV:")));
    m_perspectiveFOVSpinBox = new QDoubleSpinBox();
    m_perspectiveFOVSpinBox->setRange(1.0, 179.0);
    m_perspectiveFOVSpinBox->setValue(45.0);
    m_perspectiveFOVSpinBox->setSuffix("°");
    m_perspectiveFOVSpinBox->setToolTip(tr("透视投影视场角"));
    connect(m_perspectiveFOVSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onPerspectiveFOVChanged);
    m_viewToolBar->addWidget(m_perspectiveFOVSpinBox);
    
    // 正交大小控制
    m_viewToolBar->addWidget(new QLabel(tr("正交大小:")));
    m_orthographicSizeSpinBox = new QDoubleSpinBox();
    m_orthographicSizeSpinBox->setRange(0.1, 1000.0);
    m_orthographicSizeSpinBox->setValue(10.0);
    m_orthographicSizeSpinBox->setSuffix("m");
    m_orthographicSizeSpinBox->setToolTip(tr("正交投影大小"));
    connect(m_orthographicSizeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onOrthographicSizeChanged);
    m_viewToolBar->addWidget(m_orthographicSizeSpinBox);
    
    m_viewToolBar->addSeparator();
    
    // 相机操控器切换
    m_viewToolBar->addWidget(new QLabel(tr("相机:")));
    m_manipulatorCombo = new QComboBox();
    m_manipulatorCombo->addItem(tr("轨道球"), static_cast<int>(ManipulatorType::Trackball));
    m_manipulatorCombo->addItem(tr("第一人称"), static_cast<int>(ManipulatorType::FirstPerson));
    m_manipulatorCombo->addItem(tr("飞行"), static_cast<int>(ManipulatorType::Flight));
    m_manipulatorCombo->addItem(tr("驾驶"), static_cast<int>(ManipulatorType::Drive));
    m_manipulatorCombo->setCurrentIndex(0);
    m_manipulatorCombo->setToolTip(tr("切换相机操控器"));
    connect(m_manipulatorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onManipulatorTypeChanged);
    m_viewToolBar->addWidget(m_manipulatorCombo);
    
    m_viewToolBar->addSeparator();
    
    // 原有的视图控制按钮
    QAction* resetCameraAction = m_viewToolBar->addAction(tr("重置相机"));
    resetCameraAction->setIcon(QIcon(":/icons/reset.png"));
    connect(resetCameraAction, &QAction::triggered, this, &MainWindow::onViewResetCamera);
    
    QAction* fitAllAction = m_viewToolBar->addAction(tr("适应窗口"));
    fitAllAction->setIcon(QIcon(":/icons/fit.png"));
    connect(fitAllAction, &QAction::triggered, this, &MainWindow::onViewFitAll);
    
    m_viewToolBar->addSeparator();
    
    m_viewToolBar->addAction(tr("线框"), this, &MainWindow::onViewWireframe);
    m_viewToolBar->addAction(tr("着色"), this, &MainWindow::onViewShaded);
}

void MainWindow::createStatusBar()
{
    // 创建状态栏标签
    m_positionLabel = new QLabel("位置: (0, 0, 0)");
    m_positionLabel->setMinimumWidth(200);
    statusBar()->addWidget(m_positionLabel);
    
    // 添加摄像机移动速度显示
    QLabel* cameraSpeedLabel = new QLabel("摄像机速度: 100");
    cameraSpeedLabel->setMinimumWidth(150);
    statusBar()->addWidget(cameraSpeedLabel);
    
    // 模式标签
    m_modeLabel = new QLabel(tr("模式: 选择"));
    m_modeLabel->setMinimumWidth(100);
    statusBar()->addWidget(m_modeLabel);
    
    // 对象数量标签
    m_objectCountLabel = new QLabel(tr("对象: 0"));
    m_objectCountLabel->setMinimumWidth(80);
    statusBar()->addWidget(m_objectCountLabel);
    
    // 坐标范围标签
    m_coordinateRangeLabel = new QLabel(tr("范围: 地球"));
    m_coordinateRangeLabel->setMinimumWidth(120);
    statusBar()->addWidget(m_coordinateRangeLabel);
    
    statusBar()->addPermanentWidget(new QLabel(tr("就绪")));
}

void MainWindow::createDockWidgets()
{
    // 属性面板停靠窗口
    m_propertyDock = new QDockWidget(tr("属性"), this);
    m_propertyDock->setObjectName("PropertyDock");
    m_propertyDock->setWidget(m_propertyEditor);
    m_propertyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_propertyDock);
    
    // 工具面板停靠窗口
    m_toolDock = new QDockWidget(tr("工具"), this);
    m_toolDock->setObjectName("ToolDock");
    m_toolDock->setWidget(m_toolPanel);
    m_toolDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_toolDock);
    
    // 日志输出栏停靠窗口
    m_logDock = new QDockWidget(tr("日志输出"), this);
    m_logDock->setObjectName("LogDock");
    m_logDock->setWidget(m_logOutputWidget);
    m_logDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_logDock);
    
    // 设置日志输出栏的初始大小
    m_logDock->resize(800, 200);
    
    // 添加停靠窗口到视图菜单
    if (m_viewMenu)
    {
        m_viewMenu->addSeparator();
        m_viewMenu->addAction(m_propertyDock->toggleViewAction());
        m_viewMenu->addAction(m_toolDock->toggleViewAction());
        m_viewMenu->addAction(m_logDock->toggleViewAction());
    }
}

void MainWindow::connectSignals()
{
    // OSG Widget 信号连接
    if (m_osgWidget)
    {
        connect(m_osgWidget, &OSGWidget::geoSelected, this, &MainWindow::onGeoSelected);
        connect(m_osgWidget, &OSGWidget::mousePositionChanged, [this](const glm::vec3& pos) {
            if (m_positionLabel)
            {
                m_positionLabel->setText(tr("位置: (%1, %2, %3)")
                    .arg(pos.x, 0, 'f', 2)
                    .arg(pos.y, 0, 'f', 2)
                    .arg(pos.z, 0, 'f', 2));
            }
            // 注意：鼠标位置变化太频繁，不输出到日志，避免日志过多
        });
        connect(m_osgWidget, &OSGWidget::simplePickingResult, this, &MainWindow::onSimplePickingResult);
        connect(m_osgWidget, &OSGWidget::manipulatorTypeChanged, this, [this](ManipulatorType type) {
            if (m_manipulatorCombo) {
                int index = static_cast<int>(type);
                if (index >= 0 && index < m_manipulatorCombo->count()) {
                    m_manipulatorCombo->setCurrentIndex(index);
                }
            }
        });
    }
    
    // 工具面板信号连接
    if (m_toolPanel)
    {
        connect(m_toolPanel, &ToolPanel3D::drawModeChanged, this, &MainWindow::onDrawModeChanged);
        connect(m_toolPanel, &ToolPanel3D::skyboxEnabled, this, &MainWindow::onToolPanelSkyboxEnabled);
        connect(m_toolPanel, &ToolPanel3D::skyboxGradientRequested, this, &MainWindow::onSkyboxGradient);
        connect(m_toolPanel, &ToolPanel3D::skyboxSolidRequested, this, &MainWindow::onSkyboxSolid);
        connect(m_toolPanel, &ToolPanel3D::skyboxCustomRequested, this, &MainWindow::onSkyboxCustom);
        
        // 视图工具信号连接
        connect(m_toolPanel, &ToolPanel3D::resetViewRequested, this, &MainWindow::onViewResetCamera);
        connect(m_toolPanel, &ToolPanel3D::fitViewRequested, this, &MainWindow::onViewFitAll);
        connect(m_toolPanel, &ToolPanel3D::topViewRequested, this, &MainWindow::onViewTop);
        connect(m_toolPanel, &ToolPanel3D::frontViewRequested, this, &MainWindow::onViewFront);
        connect(m_toolPanel, &ToolPanel3D::rightViewRequested, this, &MainWindow::onViewRight);
        connect(m_toolPanel, &ToolPanel3D::isometricViewRequested, this, &MainWindow::onViewIsometric);
        
        // 实用工具信号连接
        connect(m_toolPanel, &ToolPanel3D::clearSceneRequested, this, &MainWindow::onClearScene);
        connect(m_toolPanel, &ToolPanel3D::exportImageRequested, this, &MainWindow::onExportImage);
        connect(m_toolPanel, &ToolPanel3D::coordinateSystemRequested, this, &MainWindow::onCoordinateSystemSettings);
        connect(m_toolPanel, &ToolPanel3D::displaySettingsRequested, this, &MainWindow::onDisplaySettings);
    }
    
    // 属性编辑器信号连接
    if (m_propertyEditor)
    {
        connect(m_propertyEditor, &PropertyEditor3D::parametersChanged, this, &MainWindow::onGeoParametersChanged);
    }
}

void MainWindow::updateStatusBar(const QString& message)
{
    statusBar()->showMessage(message, 3000);
    // 同时输出到日志系统
    LOG_INFO(message, "状态");
}

void MainWindow::updateDrawModeUI()
{
    if (m_toolPanel)
    {
        m_toolPanel->updateDrawMode(GlobalDrawMode3D);
    }
    
    if (m_modeLabel)
    {
        m_modeLabel->setText(tr("模式: %1").arg(drawMode3DToString(GlobalDrawMode3D)));
    }
}

void MainWindow::updateCoordinateRangeLabel()
{
    if (m_coordinateRangeLabel)
    {
        CoordinateSystem3D* coordSystem = CoordinateSystem3D::getInstance();
        const CoordinateSystem3D::CoordinateRange& range = coordSystem->getCoordinateRange();
        
        // 根据范围大小确定显示名称
        double maxRange = range.maxRange();
        QString rangeName;
        
        if (maxRange <= 1000) rangeName = "小范围";
        else if (maxRange <= 100000) rangeName = "中等范围";
        else if (maxRange <= 1000000) rangeName = "大范围";
        else if (maxRange <= 50000) rangeName = "城市范围";
        else if (maxRange <= 5000000) rangeName = "国家范围";
        else if (maxRange <= 10000000) rangeName = "大陆范围";
        else if (maxRange <= 12742000) rangeName = "地球范围";
        else rangeName = "自定义范围";
        
        m_coordinateRangeLabel->setText(tr("范围: %1").arg(rangeName));
    }
}

void MainWindow::updateObjectCount()
{
    if (m_objectCountLabel && m_osgWidget)
    {
        const auto& allGeos = m_osgWidget->getAllGeos();
        int count = static_cast<int>(allGeos.size());
        m_objectCountLabel->setText(tr("对象: %1").arg(count));
    }
}

// 文件菜单槽函数
void MainWindow::onFileNew()
{
    if (m_modified)
    {
        int ret = QMessageBox::question(this, tr("新建"), tr("当前文档已修改，是否保存？"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (ret == QMessageBox::Save)
        {
            onFileSave();
        }
        else if (ret == QMessageBox::Cancel)
        {
            return;
        }
    }
    
    if (m_osgWidget)
    {
        m_osgWidget->removeAllGeos();
        

    }
    
    m_currentFilePath.clear();
    m_modified = false;
    setWindowTitle(tr("3D Drawing Board - 未命名"));
    updateStatusBar(tr("新建文档"));
    updateObjectCount(); // 更新对象数量显示
    LOG_SUCCESS("新建文档成功", "文件");
}

void MainWindow::onFileOpen()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("打开3D文档"), "", tr("OSGB Files (*.osgb);;3D Drawing Files (*.3dd);;All Files (*)"));
    
    if (!fileName.isEmpty())
    {
        if (m_osgWidget)
        {
            m_osgWidget->removeAllGeos();
            
            // 尝试加载场景数据（多个对象）
            SceneData sceneData = GeoOsgbIO::loadSceneFromOsgb(fileName);
            if (!sceneData.objects.empty())
            {
                // 成功加载场景数据
                LOG_INFO(QString("开始添加 %1 个几何对象到场景").arg(sceneData.objects.size()), "文件");
                for (Geo3D* geo : sceneData.objects)
                {
                    if (geo)
                    {
                        m_osgWidget->addGeo(geo);
                        LOG_DEBUG(QString("已添加几何对象到场景: 类型=%1").arg(geo->getGeoType()), "文件");
                    }
                }
                
                // 确保所有从文件IO读入的几何对象都加入了拾取系统
                m_osgWidget->ensureAllGeosInPickingSystem();
                
                // 记录拾取系统状态
                LOG_INFO(m_osgWidget->getPickingSystemStatus(), "拾取");
                
                m_currentFilePath = fileName;
                m_modified = false;
                setWindowTitle(tr("3D Drawing Board - %1").arg(QFileInfo(fileName).baseName()));
                updateStatusBar(tr("打开场景文档: %1，包含 %2 个对象").arg(fileName).arg(sceneData.objects.size()));
                LOG_SUCCESS(tr("打开场景文档: %1，包含 %2 个对象").arg(fileName).arg(sceneData.objects.size()), "文件");
                
                // 更新对象数量显示
                updateObjectCount();
            }
            else
            {
                // 尝试加载单个对象（向后兼容）
                Geo3D* loadedGeo = GeoOsgbIO::loadFromOsgb(fileName);
                if (loadedGeo)
                {
                    LOG_INFO(QString("加载单个几何对象: 类型=%1").arg(loadedGeo->getGeoType()), "文件");
                    m_osgWidget->addGeo(loadedGeo);
                    
                    // 确保从文件IO读入的几何对象加入了拾取系统
                    m_osgWidget->ensureAllGeosInPickingSystem();
                    
                    // 记录拾取系统状态
                    LOG_INFO(m_osgWidget->getPickingSystemStatus(), "拾取");
                    
                    m_currentFilePath = fileName;
                    m_modified = false;
                    setWindowTitle(tr("3D Drawing Board - %1").arg(QFileInfo(fileName).baseName()));
                    updateStatusBar(tr("打开文档: %1").arg(fileName));
                    LOG_SUCCESS(tr("打开文档: %1").arg(fileName), "文件");
                    updateObjectCount();
                }
                else
                {
                    QMessageBox::warning(this, tr("打开失败"), tr("无法打开文件: %1").arg(fileName));
                    LOG_ERROR(tr("打开文档失败: %1").arg(fileName), "文件");
                }
            }
        }
    }
}

void MainWindow::onFileSave()
{
    LOG_INFO("开始执行保存操作", "文件");
    
    if (m_osgWidget)
    {
        LOG_INFO("OSGWidget存在，准备显示保存对话框", "文件");
        
        // 总是显示保存对话框让用户选择路径
        QString savePath = QFileDialog::getSaveFileName(this,
            tr("保存3D场景"), "", tr("OSGB Files (*.osgb);;3D Drawing Files (*.3dd);;All Files (*)"));
        
        if (savePath.isEmpty())
        {
            LOG_INFO("用户取消了保存操作", "文件");
            return; // 用户取消了保存
        }
        
        LOG_INFO(QString("用户选择了保存路径: %1").arg(savePath), "文件");
        
        m_currentFilePath = savePath;
        setWindowTitle(tr("3D Drawing Board - %1").arg(QFileInfo(savePath).baseName()));
        
        // 获取所有几何对象
        const auto& allGeos = m_osgWidget->getAllGeos();
        
        if (!allGeos.empty())
        {
            // 创建场景数据
            SceneData sceneData;
            sceneData.sceneName = QFileInfo(savePath).baseName();
            sceneData.description = tr("3D绘图板场景");
            sceneData.author = tr("用户");
            sceneData.version = "1.0";
            sceneData.createTime = QDateTime::currentDateTime();
            sceneData.modifyTime = QDateTime::currentDateTime();
            
            // 添加所有几何对象到场景数据
            for (const auto& geoRef : allGeos)
            {
                if (geoRef)
                {
                    sceneData.objects.push_back(geoRef.get());
                    
                    // 为每个对象创建标签
                    SceneObjectTag tag;
                    tag.name = tr("对象_%1").arg(sceneData.objects.size());
                    tag.description = tr("几何对象");
                    tag.category = tr("几何体");
                    tag.visible = true;
                    tag.selectable = true;
                    tag.opacity = 1.0f;
                    tag.layer = 0;
                    tag.createTime = QDateTime::currentDateTime();
                    tag.modifyTime = QDateTime::currentDateTime();
                    
                    sceneData.objectTags[geoRef.get()] = tag;
                }
            }
            
            // 使用GeoOsgbIO保存场景数据
            if (GeoOsgbIO::saveSceneToOsgb(savePath, sceneData))
            {
                m_modified = false;
                updateStatusBar(tr("保存场景文档: %1，包含 %2 个对象").arg(savePath).arg(sceneData.objects.size()));
                LOG_SUCCESS(tr("保存场景文档: %1，包含 %2 个对象").arg(savePath).arg(sceneData.objects.size()), "文件");
            }
            else
            {
                QMessageBox::warning(this, tr("保存失败"), tr("无法保存文件: %1").arg(savePath));
                LOG_ERROR(tr("保存文档失败: %1").arg(savePath), "文件");
            }
        }
        else
        {
            // 即使没有几何对象，也创建一个空的场景文件
            SceneData sceneData;
            sceneData.sceneName = QFileInfo(savePath).baseName();
            sceneData.description = tr("3D绘图板场景");
            sceneData.author = tr("用户");
            sceneData.version = "1.0";
            sceneData.createTime = QDateTime::currentDateTime();
            sceneData.modifyTime = QDateTime::currentDateTime();
            
            // 使用GeoOsgbIO保存空场景数据
            if (GeoOsgbIO::saveSceneToOsgb(savePath, sceneData))
            {
                m_modified = false;
                updateStatusBar(tr("保存空场景文档: %1").arg(savePath));
                LOG_SUCCESS(tr("保存空场景文档: %1").arg(savePath), "文件");
            }
            else
            {
                QMessageBox::warning(this, tr("保存失败"), tr("无法保存文件: %1").arg(savePath));
                LOG_ERROR(tr("保存文档失败: %1").arg(savePath), "文件");
            }
        }
    }
}

void MainWindow::onFileSaveAs()
{
    LOG_INFO("开始执行另存为操作", "文件");
    
    if (m_osgWidget)
    {
        LOG_INFO("OSGWidget存在，准备显示另存为对话框", "文件");
        
        QString fileName = QFileDialog::getSaveFileName(this,
            tr("另存为3D场景"), "", tr("OSGB Files (*.osgb);;3D Drawing Files (*.3dd);;All Files (*)"));
        
        if (!fileName.isEmpty())
        {
            LOG_INFO(QString("用户选择了另存为路径: %1").arg(fileName), "文件");
            
            // 获取所有几何对象
            const auto& allGeos = m_osgWidget->getAllGeos();
            
            if (!allGeos.empty())
            {
                // 创建场景数据
                SceneData sceneData;
                sceneData.sceneName = QFileInfo(fileName).baseName();
                sceneData.description = tr("3D绘图板场景");
                sceneData.author = tr("用户");
                sceneData.version = "1.0";
                sceneData.createTime = QDateTime::currentDateTime();
                sceneData.modifyTime = QDateTime::currentDateTime();
                
                // 添加所有几何对象到场景数据
                for (const auto& geoRef : allGeos)
                {
                    if (geoRef)
                    {
                        sceneData.objects.push_back(geoRef.get());
                        
                        // 为每个对象创建标签
                        SceneObjectTag tag;
                        tag.name = tr("对象_%1").arg(sceneData.objects.size());
                        tag.description = tr("几何对象");
                        tag.category = tr("几何体");
                        tag.visible = true;
                        tag.selectable = true;
                        tag.opacity = 1.0f;
                        tag.layer = 0;
                        tag.createTime = QDateTime::currentDateTime();
                        tag.modifyTime = QDateTime::currentDateTime();
                        
                        sceneData.objectTags[geoRef.get()] = tag;
                    }
                }
                
                // 使用GeoOsgbIO保存场景数据
                if (GeoOsgbIO::saveSceneToOsgb(fileName, sceneData))
                {
                    m_currentFilePath = fileName;
                    m_modified = false;
                    setWindowTitle(tr("3D Drawing Board - %1").arg(QFileInfo(fileName).baseName()));
                    updateStatusBar(tr("另存为场景文档: %1，包含 %2 个对象").arg(fileName).arg(sceneData.objects.size()));
                    LOG_SUCCESS(tr("另存为场景文档: %1，包含 %2 个对象").arg(fileName).arg(sceneData.objects.size()), "文件");
                }
                else
                {
                    QMessageBox::warning(this, tr("保存失败"), tr("无法保存文件: %1").arg(fileName));
                    LOG_ERROR(tr("另存为失败: %1").arg(fileName), "文件");
                }
            }
            else
            {
                // 即使没有几何对象，也创建一个空的场景文件
                SceneData sceneData;
                sceneData.sceneName = QFileInfo(fileName).baseName();
                sceneData.description = tr("3D绘图板场景");
                sceneData.author = tr("用户");
                sceneData.version = "1.0";
                sceneData.createTime = QDateTime::currentDateTime();
                sceneData.modifyTime = QDateTime::currentDateTime();
                
                // 使用GeoOsgbIO保存空场景数据
                if (GeoOsgbIO::saveSceneToOsgb(fileName, sceneData))
                {
                    m_currentFilePath = fileName;
                    m_modified = false;
                    setWindowTitle(tr("3D Drawing Board - %1").arg(QFileInfo(fileName).baseName()));
                    updateStatusBar(tr("另存为空场景文档: %1").arg(fileName));
                    LOG_SUCCESS(tr("另存为空场景文档: %1").arg(fileName), "文件");
                }
                else
                {
                    QMessageBox::warning(this, tr("保存失败"), tr("无法保存文件: %1").arg(fileName));
                    LOG_ERROR(tr("另存为失败: %1").arg(fileName), "文件");
                }
            }
        }
    }
}

void MainWindow::onFileExit()
{
    LOG_INFO("用户请求退出应用程序", "系统");
    close();
}
// 编辑菜单槽函数
void MainWindow::onEditUndo()
{
    // TODO: 实现撤销功能
    updateStatusBar(tr("撤销"));
    LOG_INFO("执行撤销操作", "编辑");
}

void MainWindow::onEditRedo()
{
    // TODO: 实现重做功能
    updateStatusBar(tr("重做"));
    LOG_INFO("执行重做操作", "编辑");
}

void MainWindow::onEditCopy()
{
    // TODO: 实现复制功能
    updateStatusBar(tr("复制"));
    LOG_INFO("执行复制操作", "编辑");
}

void MainWindow::onEditPaste()
{
    // TODO: 实现粘贴功能
    updateStatusBar(tr("粘贴"));
    LOG_INFO("执行粘贴操作", "编辑");
}

void MainWindow::onEditDelete()
{
    // TODO: 实现删除功能
    updateStatusBar(tr("删除"));
    LOG_INFO("执行删除操作", "编辑");
}

void MainWindow::onEditSelectAll()
{
    // TODO: 实现全选功能
    updateStatusBar(tr("全选"));
    LOG_INFO("执行全选操作", "编辑");
}

// 视图菜单槽函数
void MainWindow::onViewResetCamera()
{
    if (m_osgWidget)
    {
        m_osgWidget->resetCamera();
        updateStatusBar(tr("重置相机"));
        LOG_INFO("重置相机视角", "视图");
    }
}

void MainWindow::onViewFitAll()
{
    if (m_osgWidget)
    {
        m_osgWidget->fitAll();
        updateStatusBar(tr("适应窗口"));
        LOG_INFO("适应窗口显示", "视图");
    }
}

void MainWindow::onViewTop()
{
    if (m_osgWidget)
    {
        m_osgWidget->setViewDirection(glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
        updateStatusBar(tr("俯视图"));
        LOG_INFO("切换到俯视图", "视图");
    }
}

void MainWindow::onViewFront()
{
    if (m_osgWidget)
    {
        m_osgWidget->setViewDirection(glm::vec3(0, -1, 0), glm::vec3(0, 0, 1));
        updateStatusBar(tr("前视图"));
        LOG_INFO("切换到前视图", "视图");
    }
}

void MainWindow::onViewRight()
{
    if (m_osgWidget)
    {
        m_osgWidget->setViewDirection(glm::vec3(-1, 0, 0), glm::vec3(0, 0, 1));
        updateStatusBar(tr("右视图"));
        LOG_INFO("切换到右视图", "视图");
    }
}

void MainWindow::onViewIsometric()
{
    if (m_osgWidget)
    {
        m_osgWidget->setViewDirection(glm::vec3(-1, -1, -1), glm::vec3(0, 0, 1));
        updateStatusBar(tr("等轴测图"));
        LOG_INFO("切换到等轴测图", "视图");
    }
}

void MainWindow::onViewWireframe()
{
    if (m_osgWidget)
    {
        m_osgWidget->setWireframeMode(true);
        updateStatusBar(tr("线框模式"));
        LOG_INFO("切换到线框模式", "显示");
    }
}

void MainWindow::onViewShaded()
{
    if (m_osgWidget)
    {
        m_osgWidget->setShadedMode(true);
        updateStatusBar(tr("着色模式"));
        LOG_INFO("切换到着色模式", "显示");
    }
}

void MainWindow::onViewShadedWireframe()
{
    if (m_osgWidget)
    {
        m_osgWidget->setWireframeMode(true);
        m_osgWidget->setShadedMode(true);
        updateStatusBar(tr("着色+线框模式"));
        LOG_INFO("切换到着色+线框模式", "显示");
    }
}

void MainWindow::onHelpAbout()
{
    LOG_INFO("用户查看关于信息", "帮助");
    QMessageBox::about(this, tr("关于"), 
        tr("3D Drawing Board v1.0\n\n"
        "基于Qt + OSG的三维绘图板\n"
        "支持点、线、面、体的三维绘制\n\n"
        "开发者: liushisheng\n"
        "版权所有  2024"));
}

// 绘制相关槽函数
void MainWindow::onDrawModeChanged(DrawMode3D mode)
{
    // 通过OSGWidget设置绘制模式，确保状态正确重置
    if (m_osgWidget)
    {
        m_osgWidget->setDrawMode(mode);
    }
    else
    {
        GlobalDrawMode3D = mode;
    }
    
    updateDrawModeUI();
    updateStatusBar(tr("切换到: %1").arg(drawMode3DToString(mode)));
    LOG_INFO(tr("切换到绘制模式: %1").arg(drawMode3DToString(mode)), "模式");
}

void MainWindow::onGeoSelected(Geo3D* geo)
{
    if (m_propertyEditor)
    {
        // 检查是否有多个选中的对象
        if (m_osgWidget && m_osgWidget->getSelectionCount() > 1)
        {
            // 多选情况：显示第一个选中对象的属性
            const auto& selectedGeos = m_osgWidget->getSelectedGeos();
            if (!selectedGeos.empty())
            {
                m_propertyEditor->setSelectedGeos(selectedGeos);
                updateStatusBar(tr("选中 %1 个几何对象").arg(m_osgWidget->getSelectionCount()));
                LOG_INFO(tr("选中 %1 个几何对象").arg(m_osgWidget->getSelectionCount()), "选择");
            }
        }
        else
        {
            // 单选情况：显示当前选中对象的属性
            m_propertyEditor->setGeo(geo);
            
            if (geo)
            {
                updateStatusBar(tr("选中几何对象"));
                LOG_INFO("选中几何对象", "选择");
            }
            else
            {
                updateStatusBar(tr("取消选择"));
                LOG_INFO("取消选择", "选择");
            }
        }
    }
}

void MainWindow::onGeoParametersChanged()
{
    m_modified = true;
    QString title = windowTitle();
    if (!title.endsWith(" *"))
    {
        setWindowTitle(title + " *");
    }
    updateStatusBar(tr("属性已修改"));
    LOG_INFO("几何对象属性已修改", "属性");
}

void MainWindow::onSimplePickingResult(const OSGIndexPickResult& result)
{
    if (!result.hasResult)
        return;
    
    // 更新状态栏信息
    QString typeStr;
    switch (result.featureType)
    {
    case PickFeatureType::VERTEX:
        typeStr = tr("顶点");
        break;
    case PickFeatureType::EDGE:
        typeStr = tr("边");
        break;
    case PickFeatureType::FACE:
        typeStr = tr("面");
        break;
    default:
        typeStr = tr("未知");
        break;
    }
    
    QString snapInfo = result.isSnapped ? tr(" (已捕捉)") : "";
    
    updateStatusBar(tr("拾取到 %1 - 几何体: %2%3")
        .arg(typeStr)
        .arg(result.geometry ? result.geometry->getGeoType() : -1)
        .arg(snapInfo));
    
    LOG_DEBUG(tr("拾取到 %1 - 几何体: %2, 位置: (%3, %4, %5)%6")
        .arg(typeStr)
        .arg(result.geometry ? result.geometry->getGeoType() : -1)
        .arg(result.worldPosition.x, 0, 'f', 3)
        .arg(result.worldPosition.y, 0, 'f', 3)
        .arg(result.worldPosition.z, 0, 'f', 3)
        .arg(snapInfo), "拾取");
    
    // 更新坐标显示
    if (m_positionLabel)
    {
        m_positionLabel->setText(tr("拾取点: (%1, %2, %3)")
            .arg(result.worldPosition.x, 0, 'f', 3)
            .arg(result.worldPosition.y, 0, 'f', 3)
            .arg(result.worldPosition.z, 0, 'f', 3));
    }
}

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
    float size = m_currentGeo ? m_currentGeo->getParameters().pointSize : GlobalPointSize3D;
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
    float width = m_currentGeo ? m_currentGeo->getParameters().lineWidth : GlobalLineWidth3D;
    QColor color = m_currentGeo ? m_currentGeo->getParameters().lineColor.toQColor() : GlobalLineColor3D;
    float dashPattern = m_currentGeo ? m_currentGeo->getParameters().lineDashPattern : GlobalLineDashPattern3D;
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
    float shininess = m_currentGeo ? m_currentGeo->getParameters().material.shininess : GlobalShininess3D;
    float transparency = m_currentGeo ? m_currentGeo->getParameters().material.transparency : GlobalTransparency3D;
    
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
    bool showPoints = m_currentGeo ? m_currentGeo->mm_render()->isShowPoints() : GlobalShowPoints3D;
    bool showEdges = m_currentGeo ? m_currentGeo->mm_render()->isShowEdges() : GlobalShowEdges3D;
    bool showFaces = m_currentGeo ? m_currentGeo->mm_render()->isShowFaces() : GlobalShowFaces3D;
    
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
    
    float size = static_cast<float>(m_pointSizeSpin->value());
    
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
        Color3D color3D(color); // 使用Color3D的QColor构造函数
        
        if (!m_selectedGeos.empty())
        {
            // 多选情况：应用到所有选中的对象
            for (auto* geo : m_selectedGeos)
            {
                if (geo)
                {
                    GeoParameters3D params = geo->getParameters();
                    params.pointColor = color3D;
                    geo->setParameters(params);
                }
            }
        }
        else if (m_currentGeo)
        {
            // 单选情况
            GeoParameters3D params = m_currentGeo->getParameters();
            params.pointColor = color3D;
            m_currentGeo->setParameters(params);
        }
        else
        {
            // 全局设置
            GlobalPointColor3D = color3D.toQColor();
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
    
    float width = static_cast<float>(m_lineWidthSpin->value());
    
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
                    params.lineColor = Color3D(color);
                    geo->setParameters(params);
                }
            }
        }
        else if (m_currentGeo)
        {
            // 单选情况
            GeoParameters3D params = m_currentGeo->getParameters();
            params.lineColor = Color3D(color);
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
    
    float pattern = static_cast<float>(m_lineDashPatternSpin->value());
    
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
                    params.fillColor = Color3D(color);
                    geo->setParameters(params);
                }
            }
        }
        else if (m_currentGeo)
        {
            // 单选情况
            GeoParameters3D params = m_currentGeo->getParameters();
            params.fillColor = Color3D(color);
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
                    params.borderColor = Color3D(color);
                    geo->setParameters(params);
                }
            }
        }
        else if (m_currentGeo)
        {
            // 单选情况
            GeoParameters3D params = m_currentGeo->getParameters();
            params.borderColor = Color3D(color);
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
    
    float shininess = static_cast<float>(m_shininessSlider->value());
    
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
    
    float transparency = static_cast<float>(m_transparencySlider->value()) / 100.0f;
    
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
    
    if (m_currentGeo)
    {
        m_currentGeo->mm_render()->setShowPoints(show);
    }
    else
    {
        GlobalShowPoints3D = show;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onShowEdgesChanged()
{
    if (m_updating) return;
    
    bool show = m_showEdgesCheck->isChecked();
    
    if (m_currentGeo)
    {
        m_currentGeo->mm_render()->setShowEdges(show);
    }
    else
    {
        GlobalShowEdges3D = show;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onShowFacesChanged()
{
    if (m_updating) return;
    
    bool show = m_showFacesCheck->isChecked();
    
    if (m_currentGeo)
    {
        m_currentGeo->mm_render()->setShowFaces(show);
    }
    else
    {
        GlobalShowFaces3D = show;
    }
    
    emit parametersChanged();
}

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
    QGridLayout* layout = new QGridLayout(m_drawingGroup);
    
    // 选择工具
    m_selectButton = new QPushButton("选择");
    m_selectButton->setCheckable(true);
    m_selectButton->setChecked(true);
    m_selectButton->setToolTip("选择和编辑对象");
    m_selectButton->setProperty("drawMode", DrawSelect3D);
    layout->addWidget(m_selectButton, 0, 0);
    
    // 点绘制
    m_pointButton = new QPushButton("点");
    m_pointButton->setCheckable(true);
    m_pointButton->setToolTip("绘制点");
    m_pointButton->setProperty("drawMode", DrawPoint3D);
    layout->addWidget(m_pointButton, 0, 1);
    
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
    
    // 将所有按钮添加到列表
    m_drawButtons = {
        m_selectButton, m_pointButton, m_lineButton, m_arcButton, m_bezierButton,
        m_triangleButton, m_quadButton, m_polygonButton, m_boxButton, m_cubeButton,
        m_cylinderButton, m_coneButton, m_sphereButton, m_torusButton
    };
    
    // 连接信号
    for (QPushButton* button : m_drawButtons)
    {
        connect(button, &QPushButton::clicked, this, &ToolPanel3D::onDrawModeButtonClicked);
    }
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
    
    // 显示设置按钮
    m_displaySettingsButton = new QPushButton("显示设置");
    m_displaySettingsButton->setToolTip("设置显示参数");
    m_displaySettingsButton->setIcon(QIcon(":/icons/display.png"));
    layout->addWidget(m_displaySettingsButton);
    
    // 连接信号
    connect(m_clearSceneButton, &QPushButton::clicked, this, &ToolPanel3D::onClearSceneClicked);
    connect(m_exportImageButton, &QPushButton::clicked, this, &ToolPanel3D::onExportImageClicked);
    connect(m_coordinateSystemButton, &QPushButton::clicked, this, &ToolPanel3D::onCoordinateSystemClicked);
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
    
    // 更新按钮状态
    for (QPushButton* button : m_drawButtons)
    {
        DrawMode3D buttonMode = static_cast<DrawMode3D>(button->property("drawMode").toInt());
        button->setChecked(buttonMode == mode);
    }
}

void ToolPanel3D::onDrawModeButtonClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    DrawMode3D mode = static_cast<DrawMode3D>(button->property("drawMode").toInt());
    
    // 取消其他按钮的选中状态
    for (QPushButton* otherButton : m_drawButtons)
    {
        if (otherButton != button)
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

void ToolPanel3D::onDisplaySettingsClicked()
{
    emit displaySettingsRequested();
}

// ========================================= 天空盒相关方法 =========================================

void MainWindow::onViewSkybox()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action || !m_osgWidget) return;
    
    bool enabled = action->isChecked();
    m_osgWidget->enableSkybox(enabled);
    
    updateStatusBar(enabled ? "天空盒已启用" : "天空盒已禁用");
}

void MainWindow::onToolPanelSkyboxEnabled(bool enabled)
{
    if (!m_osgWidget) return;
    
    m_osgWidget->enableSkybox(enabled);
    
    updateStatusBar(enabled ? "天空盒已启用" : "天空盒已禁用");
    LOG_INFO(enabled ? "天空盒已启用" : "天空盒已禁用", "天空盒");
}

void MainWindow::onSkyboxGradient()
{
    if (!m_osgWidget) return;
    
    // 创建颜色选择对话框
    QColorDialog dialog(this);
    dialog.setWindowTitle("选择天空盒顶部颜色");
    dialog.setCurrentColor(QColor(128, 179, 255)); // 默认天蓝色
    
    if (dialog.exec() == QDialog::Accepted)
    {
        QColor topColor = dialog.selectedColor();
        
        dialog.setWindowTitle("选择天空盒底部颜色");
        dialog.setCurrentColor(QColor(204, 230, 255)); // 默认浅蓝色
        
        if (dialog.exec() == QDialog::Accepted)
        {
            QColor bottomColor = dialog.selectedColor();
            
            // 转换为OSG颜色格式
            osg::Vec4 osgTopColor(topColor.redF(), topColor.greenF(), topColor.blueF(), topColor.alphaF());
            osg::Vec4 osgBottomColor(bottomColor.redF(), bottomColor.greenF(), bottomColor.blueF(), bottomColor.alphaF());
            
            m_osgWidget->setSkyboxGradient(osgTopColor, osgBottomColor);
            updateStatusBar("已设置渐变天空盒");
            LOG_SUCCESS("已设置渐变天空盒", "天空盒");
        }
    }
}

void MainWindow::onSkyboxSolid()
{
    if (!m_osgWidget) return;
    
    QColorDialog dialog(this);
    dialog.setWindowTitle("选择天空盒颜色");
    dialog.setCurrentColor(QColor(51, 51, 51)); // 默认深灰色
    
    if (dialog.exec() == QDialog::Accepted)
    {
        QColor color = dialog.selectedColor();
        
        // 转换为OSG颜色格式
        osg::Vec4 osgColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
        
        m_osgWidget->setSkyboxSolidColor(osgColor);
        updateStatusBar("已设置纯色天空盒");
        LOG_SUCCESS("已设置纯色天空盒", "天空盒");
    }
}

void MainWindow::onSkyboxCustom()
{
    if (!m_osgWidget) return;
    
    QMessageBox::information(this, "自定义立方体贴图", 
        "请选择六个面的纹理文件：\n"
        "1. 正面 (+X)\n"
        "2. 背面 (-X)\n"
        "3. 顶面 (+Y)\n"
        "4. 底面 (-Y)\n"
        "5. 右面 (+Z)\n"
        "6. 左面 (-Z)\n\n"
        "注意：所有纹理文件应该具有相同的尺寸。");
    
    QStringList fileNames = QFileDialog::getOpenFileNames(
        this,
        "选择立方体贴图纹理文件",
        "",
        "图像文件 (*.png *.jpg *.jpeg *.bmp *.tga *.dds)"
    );
    
    if (fileNames.size() >= 6)
    {
        std::string positiveX = fileNames[0].toStdString();
        std::string negativeX = fileNames[1].toStdString();
        std::string positiveY = fileNames[2].toStdString();
        std::string negativeY = fileNames[3].toStdString();
        std::string positiveZ = fileNames[4].toStdString();
        std::string negativeZ = fileNames[5].toStdString();
        
        m_osgWidget->setSkyboxCubeMap(positiveX, negativeX, positiveY, negativeY, positiveZ, negativeZ);
        updateStatusBar("已设置自定义立方体贴图天空盒");
        LOG_SUCCESS("已设置自定义立方体贴图天空盒", "天空盒");
    }
    else if (fileNames.size() > 0)
    {
        QMessageBox::warning(this, "文件数量不足", 
            "需要选择6个纹理文件来创建立方体贴图天空盒。");
    }
}

void MainWindow::onCoordinateSystemSettings()
{
    CoordinateSystemDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        // 如果天空盒已启用，重新设置天空盒以应用新的范围
        if (m_osgWidget && m_osgWidget->isSkyboxEnabled())
        {
            m_osgWidget->refreshSkybox();
        }
        
        // 刷新坐标系显示
        if (m_osgWidget)
        {
            m_osgWidget->refreshCoordinateSystem();
        }
        
        // 更新状态栏显示
        updateCoordinateRangeLabel();
        updateStatusBar("坐标系统设置已更新");
        LOG_SUCCESS("坐标系统设置已更新", "坐标系统");
    }
}

void MainWindow::onClearScene()
{
    if (!m_osgWidget) return;
    
    int ret = QMessageBox::question(this, tr("清空场景"), 
        tr("确定要删除所有对象吗？此操作不可撤销。"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes)
    {
        m_osgWidget->removeAllGeos();
        m_modified = true;
        updateStatusBar(tr("场景已清空"));
        updateObjectCount(); // 更新对象数量显示
        LOG_SUCCESS("场景已清空", "场景");
        
        // 更新对象数量显示
        if (m_objectCountLabel)
        {
            m_objectCountLabel->setText(tr("对象: 0"));
        }
    }
}

void MainWindow::onExportImage()
{
    if (!m_osgWidget) return;
    
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("导出图像"), "", tr("PNG Files (*.png);;JPEG Files (*.jpg);;BMP Files (*.bmp);;All Files (*)"));
    
    if (!fileName.isEmpty())
    {
        // 获取当前视图的截图
        QPixmap pixmap = m_osgWidget->grab();
        
        if (pixmap.save(fileName))
        {
            updateStatusBar(tr("图像已导出: %1").arg(fileName));
            LOG_SUCCESS(tr("图像已导出: %1").arg(fileName), "导出");
        }
        else
        {
            QMessageBox::warning(this, tr("导出失败"), tr("无法保存图像文件: %1").arg(fileName));
            LOG_ERROR(tr("导出图像失败: %1").arg(fileName), "导出");
        }
    }
}

void MainWindow::onDisplaySettings()
{
    // 创建显示设置对话框
    QDialog dialog(this);
    dialog.setWindowTitle(tr("显示设置"));
    dialog.setModal(true);
    dialog.resize(400, 300);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    
    // 显示模式设置
    QGroupBox* displayModeGroup = new QGroupBox(tr("显示模式"), &dialog);
    QVBoxLayout* displayModeLayout = new QVBoxLayout(displayModeGroup);
    
    QCheckBox* wireframeCheck = new QCheckBox(tr("线框模式"), displayModeGroup);
    QCheckBox* shadedCheck = new QCheckBox(tr("着色模式"), displayModeGroup);
    QCheckBox* pointModeCheck = new QCheckBox(tr("点模式"), displayModeGroup);
    
    // 根据当前状态设置复选框
    if (m_osgWidget)
    {
        // 这里需要根据实际的显示状态来设置复选框
        // 暂时设置为默认值
        shadedCheck->setChecked(true);
    }
    
    displayModeLayout->addWidget(wireframeCheck);
    displayModeLayout->addWidget(shadedCheck);
    displayModeLayout->addWidget(pointModeCheck);
    
    // 背景设置
    QGroupBox* backgroundGroup = new QGroupBox(tr("背景设置"), &dialog);
    QVBoxLayout* backgroundLayout = new QVBoxLayout(backgroundGroup);
    
    QPushButton* backgroundColorButton = new QPushButton(tr("选择背景颜色"), backgroundGroup);
    QCheckBox* skyboxCheck = new QCheckBox(tr("启用天空盒"), backgroundGroup);
    skyboxCheck->setChecked(m_osgWidget ? m_osgWidget->isSkyboxEnabled() : true);
    
    backgroundLayout->addWidget(backgroundColorButton);
    backgroundLayout->addWidget(skyboxCheck);
    
    // 坐标系统设置
    QGroupBox* coordinateGroup = new QGroupBox(tr("坐标系统"), &dialog);
    QVBoxLayout* coordinateLayout = new QVBoxLayout(coordinateGroup);
    
    QCheckBox* coordinateSystemCheck = new QCheckBox(tr("显示坐标系统"), coordinateGroup);
    QCheckBox* scaleBarCheck = new QCheckBox(tr("显示比例尺"), coordinateGroup);
    
    coordinateSystemCheck->setChecked(m_osgWidget ? m_osgWidget->isCoordinateSystemEnabled() : true);
    scaleBarCheck->setChecked(m_osgWidget ? m_osgWidget->isScaleBarEnabled() : true);
    
    coordinateLayout->addWidget(coordinateSystemCheck);
    coordinateLayout->addWidget(scaleBarCheck);
    
    // 按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton(tr("确定"), &dialog);
    QPushButton* cancelButton = new QPushButton(tr("取消"), &dialog);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    
    // 添加到主布局
    mainLayout->addWidget(displayModeGroup);
    mainLayout->addWidget(backgroundGroup);
    mainLayout->addWidget(coordinateGroup);
    mainLayout->addLayout(buttonLayout);
    
    // 连接信号
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    // 连接显示模式信号
    connect(wireframeCheck, &QCheckBox::toggled, [this](bool checked) {
        if (checked && m_osgWidget) {
            m_osgWidget->setWireframeMode(true);
            m_osgWidget->setShadedMode(false);
            m_osgWidget->setPointMode(false);
        }
    });
    
    connect(shadedCheck, &QCheckBox::toggled, [this](bool checked) {
        if (checked && m_osgWidget) {
            m_osgWidget->setWireframeMode(false);
            m_osgWidget->setShadedMode(true);
            m_osgWidget->setPointMode(false);
        }
    });
    
    connect(pointModeCheck, &QCheckBox::toggled, [this](bool checked) {
        if (checked && m_osgWidget) {
            m_osgWidget->setWireframeMode(false);
            m_osgWidget->setShadedMode(false);
            m_osgWidget->setPointMode(true);
        }
    });
    
    // 连接天空盒信号
    connect(skyboxCheck, &QCheckBox::toggled, [this](bool enabled) {
        if (m_osgWidget) {
            m_osgWidget->enableSkybox(enabled);
        }
    });
    
    // 连接坐标系统信号
    connect(coordinateSystemCheck, &QCheckBox::toggled, [this](bool enabled) {
        if (m_osgWidget) {
            m_osgWidget->enableCoordinateSystem(enabled);
        }
    });
    
    connect(scaleBarCheck, &QCheckBox::toggled, [this](bool enabled) {
        if (m_osgWidget) {
            m_osgWidget->enableScaleBar(enabled);
        }
    });
    
    if (dialog.exec() == QDialog::Accepted)
    {
        updateStatusBar(tr("显示设置已更新"));
        LOG_SUCCESS("显示设置已更新", "显示");
    }
}

// ========================================= 投影模式相关槽函数 =========================================

void MainWindow::onProjectionModeChanged()
{
    if (!m_osgWidget) return;
    
    int index = m_projectionModeCombo->currentIndex();
    ProjectionMode mode = static_cast<ProjectionMode>(m_projectionModeCombo->itemData(index).toInt());
    
    m_osgWidget->setProjectionMode(mode);
    
    // 根据投影模式启用/禁用相关控件
    bool isPerspective = (mode == ProjectionMode::Perspective);
    m_perspectiveFOVSpinBox->setEnabled(isPerspective);
    m_orthographicSizeSpinBox->setEnabled(!isPerspective);
    
    QString modeName = isPerspective ? tr("透视") : tr("正交");
    updateStatusBar(tr("投影模式切换为: %1").arg(modeName));
    LOG_INFO(tr("投影模式切换为: %1").arg(modeName), "投影");
}

void MainWindow::onPerspectiveFOVChanged()
{
    if (!m_osgWidget) return;
    
    double fov = m_perspectiveFOVSpinBox->value();
    m_osgWidget->setFOV(fov);
    
    updateStatusBar(tr("FOV设置为: %1°").arg(fov));
}

void MainWindow::onOrthographicSizeChanged()
{
    if (!m_osgWidget) return;
    
    double size = m_orthographicSizeSpinBox->value();
    m_osgWidget->setViewSize(-size, size, -size, size);
    
    updateStatusBar(tr("视图大小设置为: ±%1m").arg(size));
    LOG_DEBUG(tr("视图大小设置为: ±%1m").arg(size), "投影");
}

// ========================================= 相机操控器相关槽函数 =========================================

void MainWindow::onManipulatorTypeChanged()
{
    if (!m_osgWidget) return;
    
    int index = m_manipulatorCombo->currentIndex();
    ManipulatorType type = static_cast<ManipulatorType>(m_manipulatorCombo->itemData(index).toInt());
    
    m_osgWidget->setManipulatorType(type);
    
    QString typeName;
    switch (type) {
        case ManipulatorType::Trackball:
            typeName = tr("轨道球");
            break;
        case ManipulatorType::FirstPerson:
            typeName = tr("第一人称");
            break;
        case ManipulatorType::Flight:
            typeName = tr("飞行");
            break;
        case ManipulatorType::Drive:
            typeName = tr("驾驶");
            break;
    }
    
    updateStatusBar(tr("相机操控器切换为: %1").arg(typeName));
    LOG_INFO(tr("相机操控器切换为: %1").arg(typeName), "相机");
}
