#include "MainWindow.h"
#include "../core/GeometryBase.h"
#include "../core/picking/PickingIndicator.h"
#include "PropertyEditor3D.h"
#include "ToolPanel3D.h"

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
#include <QPixmap>
#include <QGroupBox>
#include <QCheckBox>
#include <QSlider>

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
    
    GlobalStatusBar3D = statusBar();
    
    setupUI();
    createMenus();
    createToolBars();
    createStatusBar();
    createDockWidgets();
    connectSignals();
    
    if (m_statusBar3D && m_osgWidget) {
        m_statusBar3D->setOSGWidget(m_osgWidget);
    }
    
    resize(1200, 800);
    
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    updateDrawModeUI();
    updateStatusBar("Ready");
    updateObjectCount();
    
    if (m_projectionModeCombo && m_perspectiveFOVSpinBox && m_orthographicSizeSpinBox)
    {
        m_perspectiveFOVSpinBox->setEnabled(true);
        m_orthographicSizeSpinBox->setEnabled(false);
    }
    
    if (m_osgWidget)
    {
        connect(m_osgWidget, &OSGWidget::mousePositionChanged,
                this, [this](const glm::dvec3& pos) {
                    if (m_statusBar3D) {
                        m_statusBar3D->updateWorldCoordinates(pos);
                    }
                });
        
        connect(m_osgWidget->getCameraController(), &CameraController::cameraMoveSpeedChanged,
                this, [this](double speed) {
                    if (m_statusBar3D) {
                        m_statusBar3D->updateCameraSpeed(speed);
                    }
                });
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    m_osgWidget = new OSGWidget(this);
    setCentralWidget(m_osgWidget);
    
    m_propertyEditor = new PropertyEditor3D(this);
    
    m_toolPanel = new ToolPanel3D(this);
    
    m_logOutputWidget = new LogOutputWidget(this);
    
    LOG_INFO("3D绘图板启动完成", "系统");
    LOG_INFO("日志系统已初始化", "系统");
}

void MainWindow::createMenus()
{
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
    
    QAction* pickingSystemAction = m_viewMenu->addAction(tr("拾取系统设置(&P)"));
    pickingSystemAction->setShortcut(Qt::CTRL + Qt::Key_P);
    connect(pickingSystemAction, &QAction::triggered, this, &MainWindow::onPickingSystemSettings);
    
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
    m_mainToolBar = addToolBar(tr("主工具栏"));
    m_mainToolBar->setObjectName("MainToolBar");
    
    m_mainToolBar->addAction(tr("新建"), this, &MainWindow::onFileNew);
    m_mainToolBar->addAction(tr("打开"), this, &MainWindow::onFileOpen);
    m_mainToolBar->addAction(tr("保存"), this, &MainWindow::onFileSave);
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(tr("撤销"), this, &MainWindow::onEditUndo);
    m_mainToolBar->addAction(tr("重做"), this, &MainWindow::onEditRedo);
    
    m_viewToolBar = addToolBar(tr("视图"));
    m_viewToolBar->setObjectName("ViewToolBar");
    
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
    
    m_viewToolBar->addWidget(new QLabel(tr("投影:")));
    m_projectionModeCombo = new QComboBox();
    m_projectionModeCombo->addItem(tr("透视"), static_cast<int>(ProjectionMode::Perspective));
    m_projectionModeCombo->addItem(tr("正交"), static_cast<int>(ProjectionMode::Orthographic));
    m_projectionModeCombo->setCurrentIndex(0);
    m_projectionModeCombo->setToolTip(tr("切换投影模式"));
    connect(m_projectionModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProjectionModeChanged);
    m_viewToolBar->addWidget(m_projectionModeCombo);
    
    m_viewToolBar->addWidget(new QLabel(tr("FOV:")));
    m_perspectiveFOVSpinBox = new QDoubleSpinBox();
    m_perspectiveFOVSpinBox->setRange(1.0, 179.0);
    m_perspectiveFOVSpinBox->setValue(45.0);
    m_perspectiveFOVSpinBox->setSuffix("°");
    m_perspectiveFOVSpinBox->setToolTip(tr("透视投影视场角"));
    connect(m_perspectiveFOVSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onPerspectiveFOVChanged);
    m_viewToolBar->addWidget(m_perspectiveFOVSpinBox);
    
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
    m_statusBar3D = new StatusBar3D();
    statusBar()->addWidget(m_statusBar3D);
    
    if (m_osgWidget) {
        m_statusBar3D->setOSGWidget(m_osgWidget);
    }
}

void MainWindow::createDockWidgets()
{
    m_propertyDock = new QDockWidget(tr("属性"), this);
    m_propertyDock->setObjectName("PropertyDock");
    m_propertyDock->setWidget(m_propertyEditor);
    m_propertyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_propertyDock);
    
    m_toolDock = new QDockWidget(tr("工具"), this);
    m_toolDock->setObjectName("ToolDock");
    m_toolDock->setWidget(m_toolPanel);
    m_toolDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_toolDock);
    
    m_logDock = new QDockWidget(tr("日志输出"), this);
    m_logDock->setObjectName("LogDock");
    m_logDock->setWidget(m_logOutputWidget);
    m_logDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_logDock);
    
    m_logDock->resize(800, 200);
    
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
    if (m_osgWidget)
    {
        connect(m_osgWidget, &OSGWidget::geoSelected, this, &MainWindow::onGeoSelected);
        connect(m_osgWidget, &OSGWidget::mousePositionChanged, [this](const glm::dvec3& pos) {
            if (m_statusBar3D)
            {
                m_statusBar3D->updateWorldCoordinates(pos);
            }
        });
        
        connect(m_osgWidget, &OSGWidget::screenPositionChanged, [this](int x, int y) {
            if (m_statusBar3D)
            {
                m_statusBar3D->updateScreenCoordinates(x, y);
            }
        });
        connect(m_osgWidget, &OSGWidget::simplePickingResult, this, &MainWindow::onSimplePickingResult);
        connect(m_osgWidget->getCameraController(), &CameraController::manipulatorTypeChanged, this, [this](ManipulatorType type) {
            if (m_manipulatorCombo) {
                int index = static_cast<int>(type);
                if (index >= 0 && index < m_manipulatorCombo->count()) {
                    m_manipulatorCombo->setCurrentIndex(index);
                }
            }
        });
    }
    
    if (m_toolPanel)
    {
        connect(m_toolPanel, &ToolPanel3D::drawModeChanged, this, &MainWindow::onDrawModeChanged);
        connect(m_toolPanel, &ToolPanel3D::resetViewRequested, this, &MainWindow::onViewResetCamera);
        connect(m_toolPanel, &ToolPanel3D::fitViewRequested, this, &MainWindow::onViewFitAll);
        connect(m_toolPanel, &ToolPanel3D::topViewRequested, this, &MainWindow::onViewTop);
        connect(m_toolPanel, &ToolPanel3D::frontViewRequested, this, &MainWindow::onViewFront);
        connect(m_toolPanel, &ToolPanel3D::rightViewRequested, this, &MainWindow::onViewRight);
        connect(m_toolPanel, &ToolPanel3D::isometricViewRequested, this, &MainWindow::onViewIsometric);
        connect(m_toolPanel, &ToolPanel3D::clearSceneRequested, this, &MainWindow::onClearScene);
        connect(m_toolPanel, &ToolPanel3D::exportImageRequested, this, &MainWindow::onExportImage);
        connect(m_toolPanel, &ToolPanel3D::pickingSystemRequested, this, &MainWindow::onPickingSystemSettings);
        connect(m_toolPanel, &ToolPanel3D::displaySettingsRequested, this, &MainWindow::onDisplaySettings);
    }
    
    if (m_propertyEditor)
    {
        connect(m_propertyEditor, &PropertyEditor3D::geometryRecalculationRequired, 
                this, &MainWindow::onGeometryRecalculationRequired);
        connect(m_propertyEditor, &PropertyEditor3D::renderingParametersChanged, 
                this, &MainWindow::onRenderingParametersChanged);
    }
}

void MainWindow::updateStatusBar(const QString& message)
{
    if (m_statusBar3D) {
        m_statusBar3D->showTemporaryMessage(message, 3000);
    }
    LOG_INFO(message, "状态");
}

void MainWindow::updateDrawModeUI()
{
    if (m_toolPanel)
    {
        m_toolPanel->updateDrawMode(GlobalDrawMode3D);
    }
    
    if (m_statusBar3D)
    {
        m_statusBar3D->updateMode(drawMode3DToString(GlobalDrawMode3D));
    }
}

void MainWindow::updateObjectCount()
{
    if (m_statusBar3D && m_osgWidget)
    {
        const auto& allGeos = m_osgWidget->getAllGeos();
        int count = static_cast<int>(allGeos.size());
        m_statusBar3D->updateObjectCount(count);
    }
}

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
    updateObjectCount();
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
            
            std::vector<osg::ref_ptr<Geo3D>> loadedGeos = GeoOsgbIO::loadGeoList(fileName);
            if (!loadedGeos.empty())
            {
                LOG_INFO(QString("开始添加 %1 个几何对象到场景").arg(loadedGeos.size()), "文件");
                for (osg::ref_ptr<Geo3D> geo : loadedGeos)
                {
                    if (geo)
                    {
                        m_osgWidget->addGeo(geo);
                    }
                }
                
                m_currentFilePath = fileName;
                m_modified = false;
                setWindowTitle(tr("3D Drawing Board - %1").arg(QFileInfo(fileName).baseName()));
                updateStatusBar(tr("打开文档: %1，包含 %2 个对象").arg(fileName).arg(loadedGeos.size()));
                LOG_SUCCESS(tr("打开文档: %1，包含 %2 个对象").arg(fileName).arg(loadedGeos.size()), "文件");
                
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

void MainWindow::onFileSave()
{
    LOG_INFO("开始执行保存操作", "文件");
    
    if (m_osgWidget)
    {
        LOG_INFO("OSGWidget存在，准备显示保存对话框", "文件");
        
        QString savePath = QFileDialog::getSaveFileName(this,
            tr("保存3D场景"), "", tr("OSGB Files (*.osgb);;3D Drawing Files (*.3dd);;All Files (*)"));
        
        if (savePath.isEmpty())
        {
            LOG_INFO("用户取消了保存操作", "文件");
            return;
        }
        
        LOG_INFO(QString("用户选择了保存路径: %1").arg(savePath), "文件");
        
        m_currentFilePath = savePath;
        setWindowTitle(tr("3D Drawing Board - %1").arg(QFileInfo(savePath).baseName()));
        
        const auto& allGeos = m_osgWidget->getAllGeos();
        
        std::vector<osg::ref_ptr<Geo3D>> geoList;
        for (const auto& geoRef : allGeos)
        {
            if (geoRef)
            {
                geoList.push_back(geoRef.get());
            }
        }
        
        if (GeoOsgbIO::saveGeoList(savePath, geoList))
        {
            m_modified = false;
            updateStatusBar(tr("保存文档: %1，包含 %2 个对象").arg(savePath).arg(geoList.size()));
            LOG_SUCCESS(tr("保存文档: %1，包含 %2 个对象").arg(savePath).arg(geoList.size()), "文件");
        }
        else
        {
            QMessageBox::warning(this, tr("保存失败"), tr("无法保存文件: %1").arg(savePath));
            LOG_ERROR(tr("保存文档失败: %1").arg(savePath), "文件");
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
            
            const auto& allGeos = m_osgWidget->getAllGeos();
            
            std::vector<osg::ref_ptr<Geo3D>> geoList;
            for (const auto& geoRef : allGeos)
            {
                if (geoRef)
                {
                    geoList.push_back(geoRef.get());
                }
            }
            
            if (GeoOsgbIO::saveGeoList(fileName, geoList))
            {
                m_currentFilePath = fileName;
                m_modified = false;
                setWindowTitle(tr("3D Drawing Board - %1").arg(QFileInfo(fileName).baseName()));
                updateStatusBar(tr("另存为文档: %1，包含 %2 个对象").arg(fileName).arg(geoList.size()));
                LOG_SUCCESS(tr("另存为文档: %1，包含 %2 个对象").arg(fileName).arg(geoList.size()), "文件");
            }
            else
            {
                QMessageBox::warning(this, tr("保存失败"), tr("无法保存文件: %1").arg(fileName));
                LOG_ERROR(tr("另存为失败: %1").arg(fileName), "文件");
            }
        }
    }
}

void MainWindow::onFileExit()
{
    LOG_INFO("用户请求退出应用程序", "系统");
    close();
}
void MainWindow::onEditUndo()
{
    updateStatusBar(tr("撤销"));
    LOG_INFO("执行撤销操作", "编辑");
}

void MainWindow::onEditRedo()
{
    updateStatusBar(tr("重做"));
    LOG_INFO("执行重做操作", "编辑");
}

void MainWindow::onEditCopy()
{
    updateStatusBar(tr("复制"));
    LOG_INFO("执行复制操作", "编辑");
}

void MainWindow::onEditPaste()
{
    updateStatusBar(tr("粘贴"));
    LOG_INFO("执行粘贴操作", "编辑");
}

void MainWindow::onEditDelete()
{
    updateStatusBar(tr("删除"));
    LOG_INFO("执行删除操作", "编辑");
}

void MainWindow::onEditSelectAll()
{
    updateStatusBar(tr("全选"));
    LOG_INFO("执行全选操作", "编辑");
}

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
        m_osgWidget->setViewDirection(glm::dvec3(0, 0, -1), glm::dvec3(0, 1, 0));
        updateStatusBar(tr("俯视图"));
        LOG_INFO("切换到俯视图", "视图");
    }
}

void MainWindow::onViewFront()
{
    if (m_osgWidget)
    {
        m_osgWidget->setViewDirection(glm::dvec3(0, -1, 0), glm::dvec3(0, 0, 1));
        updateStatusBar(tr("前视图"));
        LOG_INFO("切换到前视图", "视图");
    }
}

void MainWindow::onViewRight()
{
    if (m_osgWidget)
    {
        m_osgWidget->setViewDirection(glm::dvec3(-1, 0, 0), glm::dvec3(0, 0, 1));
        updateStatusBar(tr("右视图"));
        LOG_INFO("切换到右视图", "视图");
    }
}

void MainWindow::onViewIsometric()
{
    if (m_osgWidget)
    {
        m_osgWidget->setViewDirection(glm::dvec3(-1, -1, -1), glm::dvec3(0, 0, 1));
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

void MainWindow::onDrawModeChanged(DrawMode3D mode)
{
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

void MainWindow::onGeoSelected(osg::ref_ptr<Geo3D> geo)
{
    if (m_propertyEditor)
    {
        if (m_osgWidget && m_osgWidget->getSelectionCount() > 1)
        {
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

void MainWindow::onGeometryRecalculationRequired()
{
    m_modified = true;
    if (m_osgWidget) {
        m_osgWidget->update();
    }
}

void MainWindow::onRenderingParametersChanged()
{
    m_modified = true;
}

void MainWindow::onSimplePickingResult(const PickResult& result)
{
    if (!result.hasResult)
        return;
    
    QString typeStr;
    switch (static_cast<int>(result.featureType))
    {
    case static_cast<int>(PickFeatureType::VERTEX):
        typeStr = tr("顶点");
        break;
    case static_cast<int>(PickFeatureType::EDGE):
        typeStr = tr("边");
        break;
    case static_cast<int>(PickFeatureType::FACE):
        typeStr = tr("面");
        break;
    default:
        typeStr = tr("未知");
        break;
    }
    
    QString snapInfo = result.isSnapped ? tr(" (已捕捉)") : "";
    
    if (m_statusBar3D)
    {
        m_statusBar3D->updateWorldCoordinates(result.worldPosition);
    }
}

void MainWindow::onPickingSystemSettings()
{
    updateStatusBar("拾取指示器使用固定配置，无需设置");
    LOG_INFO("拾取指示器使用简化的固定配置", "拾取系统");
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
        updateObjectCount();
        
        if (m_statusBar3D)
        {
            m_statusBar3D->updateObjectCount(0);
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
    QDialog dialog(this);
    dialog.setWindowTitle(tr("显示设置"));
    dialog.setModal(true);
    dialog.resize(400, 300);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    
    QGroupBox* displayModeGroup = new QGroupBox(tr("显示模式"), &dialog);
    QVBoxLayout* displayModeLayout = new QVBoxLayout(displayModeGroup);
    
    QCheckBox* wireframeCheck = new QCheckBox(tr("线框模式"), displayModeGroup);
    QCheckBox* shadedCheck = new QCheckBox(tr("着色模式"), displayModeGroup);
    QCheckBox* pointModeCheck = new QCheckBox(tr("点模式"), displayModeGroup);
    
    if (m_osgWidget)
    {
        shadedCheck->setChecked(true);
    }
    
    displayModeLayout->addWidget(wireframeCheck);
    displayModeLayout->addWidget(shadedCheck);
    displayModeLayout->addWidget(pointModeCheck);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton(tr("确定"), &dialog);
    QPushButton* cancelButton = new QPushButton(tr("取消"), &dialog);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addWidget(displayModeGroup);
    mainLayout->addLayout(buttonLayout);
    
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
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
    
    if (dialog.exec() == QDialog::Accepted)
    {
        updateStatusBar(tr("显示设置已更新"));
        LOG_SUCCESS("显示设置已更新", "显示");
    }
}

void MainWindow::onProjectionModeChanged()
{
    if (!m_osgWidget) return;
    
    int index = m_projectionModeCombo->currentIndex();
    ProjectionMode mode = static_cast<ProjectionMode>(m_projectionModeCombo->itemData(index).toInt());
    
    m_osgWidget->getCameraController()->setProjectionMode(mode);
    
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
    m_osgWidget->getCameraController()->setFOV(fov);
    
    updateStatusBar(tr("FOV设置为: %1°").arg(fov));
}

void MainWindow::onOrthographicSizeChanged()
{
    if (!m_osgWidget) return;
    
    double size = m_orthographicSizeSpinBox->value();
    m_osgWidget->getCameraController()->setViewSize(-size, size, -size, size);
    
    updateStatusBar(tr("视图大小设置为: ±%1m").arg(size));
}

void MainWindow::onManipulatorTypeChanged()
{
    if (!m_osgWidget) return;
    
    int index = m_manipulatorCombo->currentIndex();
    ManipulatorType type = static_cast<ManipulatorType>(m_manipulatorCombo->itemData(index).toInt());
    
    m_osgWidget->getCameraController()->setManipulatorType(type);
    
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




