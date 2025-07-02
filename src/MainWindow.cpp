#include "MainWindow.h"
#include "Geo3D.h"
#include <QTimer>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QButtonGroup>
#include <QApplication>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QDebug>

// ========================================= MainWindow 实现 =========================================
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_osgWidget(nullptr)
    , m_propertyEditor(nullptr)
    , m_toolPanel(nullptr)
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
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    updateDrawModeUI();
    updateStatusBar("Ready");
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
    
    // 帮助菜单
    m_helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    
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
    m_viewToolBar = addToolBar(tr("视图工具栏"));
    m_viewToolBar->setObjectName("ViewToolBar");
    
    m_viewToolBar->addAction(tr("重置相机"), this, &MainWindow::onViewResetCamera);
    m_viewToolBar->addAction(tr("适应窗口"), this, &MainWindow::onViewFitAll);
    m_viewToolBar->addSeparator();
    m_viewToolBar->addAction(tr("线框"), this, &MainWindow::onViewWireframe);
    m_viewToolBar->addAction(tr("着色"), this, &MainWindow::onViewShaded);
}

void MainWindow::createStatusBar()
{
    // 位置标签
    m_positionLabel = new QLabel(tr("位置: (0, 0, 0)"));
    m_positionLabel->setMinimumWidth(120);
    statusBar()->addWidget(m_positionLabel);
    
    // 模式标签
    m_modeLabel = new QLabel(tr("模式: 选择"));
    m_modeLabel->setMinimumWidth(100);
    statusBar()->addWidget(m_modeLabel);
    
    // 对象数量标签
    m_objectCountLabel = new QLabel(tr("对象: 0"));
    m_objectCountLabel->setMinimumWidth(80);
    statusBar()->addWidget(m_objectCountLabel);
    
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
    
    // 添加停靠窗口到视图菜单
    if (m_viewMenu)
    {
        m_viewMenu->addSeparator();
        m_viewMenu->addAction(m_propertyDock->toggleViewAction());
        m_viewMenu->addAction(m_toolDock->toggleViewAction());
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
        });
        connect(m_osgWidget, &OSGWidget::drawingProgress, [this](const QString& message) {
            updateStatusBar(message);
        });
    }
    
    // 工具面板信号连接
    if (m_toolPanel)
    {
        connect(m_toolPanel, &ToolPanel3D::drawModeChanged, this, &MainWindow::onDrawModeChanged);
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
}

void MainWindow::onFileOpen()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("打开3D文档"), "", tr("3D Drawing Files (*.3dd);;All Files (*)"));
    
    if (!fileName.isEmpty())
    {
        // TODO: 实现文件打开逻辑
        m_currentFilePath = fileName;
        m_modified = false;
        setWindowTitle(tr("3D Drawing Board - %1").arg(QFileInfo(fileName).baseName()));
        updateStatusBar(tr("打开文档: %1").arg(fileName));
    }
}

void MainWindow::onFileSave()
{
    if (m_currentFilePath.isEmpty())
    {
        onFileSaveAs();
        return;
    }
    
    // TODO: 实现文件保存逻辑
    m_modified = false;
    updateStatusBar(tr("保存文档: %1").arg(m_currentFilePath));
}

void MainWindow::onFileSaveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("保存3D文档"), "", tr("3D Drawing Files (*.3dd);;All Files (*)"));
    
    if (!fileName.isEmpty())
    {
        m_currentFilePath = fileName;
        onFileSave();
        setWindowTitle(tr("3D Drawing Board - %1").arg(QFileInfo(fileName).baseName()));
    }
}

void MainWindow::onFileExit()
{
    close();
}

// 编辑菜单槽函数
void MainWindow::onEditUndo()
{
    // TODO: 实现撤销功能
    updateStatusBar(tr("撤销"));
}

void MainWindow::onEditRedo()
{
    // TODO: 实现重做功能
    updateStatusBar(tr("重做"));
}

void MainWindow::onEditCopy()
{
    // TODO: 实现复制功能
    updateStatusBar(tr("复制"));
}

void MainWindow::onEditPaste()
{
    // TODO: 实现粘贴功能
    updateStatusBar(tr("粘贴"));
}

void MainWindow::onEditDelete()
{
    // TODO: 实现删除功能
    updateStatusBar(tr("删除"));
}

void MainWindow::onEditSelectAll()
{
    // TODO: 实现全选功能
    updateStatusBar(tr("全选"));
}

// 视图菜单槽函数
void MainWindow::onViewResetCamera()
{
    if (m_osgWidget)
    {
        m_osgWidget->resetCamera();
        updateStatusBar(tr("重置相机"));
    }
}

void MainWindow::onViewFitAll()
{
    if (m_osgWidget)
    {
        m_osgWidget->fitAll();
        updateStatusBar(tr("适应窗口"));
    }
}

void MainWindow::onViewTop()
{
    if (m_osgWidget)
    {
        m_osgWidget->setViewDirection(glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
        updateStatusBar(tr("俯视图"));
    }
}

void MainWindow::onViewFront()
{
    if (m_osgWidget)
    {
        m_osgWidget->setViewDirection(glm::vec3(0, -1, 0), glm::vec3(0, 0, 1));
        updateStatusBar(tr("前视图"));
    }
}

void MainWindow::onViewRight()
{
    if (m_osgWidget)
    {
        m_osgWidget->setViewDirection(glm::vec3(-1, 0, 0), glm::vec3(0, 0, 1));
        updateStatusBar(tr("右视图"));
    }
}

void MainWindow::onViewIsometric()
{
    if (m_osgWidget)
    {
        m_osgWidget->setViewDirection(glm::vec3(-1, -1, -1), glm::vec3(0, 0, 1));
        updateStatusBar(tr("等轴测图"));
    }
}

void MainWindow::onViewWireframe()
{
    if (m_osgWidget)
    {
        m_osgWidget->setWireframeMode(true);
        updateStatusBar(tr("线框模式"));
    }
}

void MainWindow::onViewShaded()
{
    if (m_osgWidget)
    {
        m_osgWidget->setShadedMode(true);
        updateStatusBar(tr("着色模式"));
    }
}

void MainWindow::onViewShadedWireframe()
{
    if (m_osgWidget)
    {
        m_osgWidget->setWireframeMode(true);
        m_osgWidget->setShadedMode(true);
        updateStatusBar(tr("着色+线框模式"));
    }
}

void MainWindow::onHelpAbout()
{
    QMessageBox::about(this, tr("关于"), 
        tr("3D Drawing Board v1.0\n\n"
        "基于Qt + OSG的三维绘图板\n"
        "支持点、线、面、体的三维绘制\n\n"
        "开发者: Your Name\n"
        "版权所有  2024"));
}

// 绘制相关槽函数
void MainWindow::onDrawModeChanged(DrawMode3D mode)
{
    GlobalDrawMode3D = mode;
    updateDrawModeUI();
    updateStatusBar(tr("切换到: %1").arg(drawMode3DToString(mode)));
}

void MainWindow::onGeoSelected(Geo3D* geo)
{
    if (m_propertyEditor)
    {
        m_propertyEditor->setGeo(geo);
    }
    
    if (geo)
    {
        updateStatusBar(tr("选中几何对象"));
    }
    else
    {
        updateStatusBar(tr("取消选择"));
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
}

// ========================================= OSGWidget 实现 =========================================
OSGWidget::OSGWidget(QWidget* parent)
    : osgQOpenGLWidget(parent)
    , m_rootNode(new osg::Group)
    , m_sceneNode(new osg::Group)
    , m_geoNode(new osg::Group)
    , m_lightNode(new osg::Group)
    , m_trackballManipulator(new osgGA::TrackballManipulator)
    , m_currentDrawingGeo(nullptr)
    , m_selectedGeo(nullptr)
    , m_isDrawing(false)
    , m_lastMouseWorldPos(0.0f)
    , m_updateTimer(new QTimer(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    // 连接信号槽
    connect(m_updateTimer, &QTimer::timeout, this, [this]() {
        update();
    });
    
    // 连接初始化完成信号
    connect(this, &osgQOpenGLWidget::initialized, this, &OSGWidget::initializeScene);
    
    // 设置渲染循环
    m_updateTimer->start(16); // 约60FPS
}

OSGWidget::~OSGWidget()
{
    if (m_updateTimer)
    {
        m_updateTimer->stop();
    }
}

void OSGWidget::initializeScene()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    // 设置场景图
    m_rootNode->addChild(m_sceneNode);
    m_rootNode->addChild(m_lightNode);
    m_sceneNode->addChild(m_geoNode);
    
    viewer->setSceneData(m_rootNode);
    
    // 设置线程模型
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    
    // 设置相机操控器
    viewer->setCameraManipulator(m_trackballManipulator);
    
    setupCamera();
    setupLighting();
    setupEventHandlers();
}

void OSGWidget::setupCamera()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    osg::Camera* camera = viewer->getCamera();
    
    // 设置渲染状态
    osg::StateSet* stateSet = camera->getOrCreateStateSet();
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    
    // 启用反走样
    stateSet->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    stateSet->setMode(GL_POINT_SMOOTH, osg::StateAttribute::ON);
    
    // 设置背景色
    camera->setClearColor(osg::Vec4(0.2f, 0.2f, 0.2f, 1.0f));
    
    // 设置初始视点
    resetCamera();
}

void OSGWidget::setupLighting()
{
    // 创建光源
    osg::ref_ptr<osg::Light> light = new osg::Light();
    light->setLightNum(0);
    light->setPosition(osg::Vec4(10.0f, 10.0f, 10.0f, 1.0f));
    light->setDirection(osg::Vec3(-1.0f, -1.0f, -1.0f));
    light->setAmbient(osg::Vec4(0.3f, 0.3f, 0.3f, 1.0f));
    light->setDiffuse(osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f));
    light->setSpecular(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    
    osg::ref_ptr<osg::LightSource> lightSource = new osg::LightSource();
    lightSource->setLight(light.get());
    lightSource->setLocalStateSetModes(osg::StateAttribute::ON);
    
    m_lightNode->addChild(lightSource.get());
    
    // 设置全局环境光
    osg::StateSet* stateSet = m_rootNode->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHT0, osg::StateAttribute::ON);
}

void OSGWidget::setupEventHandlers()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    // 添加事件处理器
    viewer->addEventHandler(new osgViewer::StatsHandler());
    viewer->addEventHandler(new osgViewer::WindowSizeHandler());
    viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
}

void OSGWidget::resetCamera()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    if (viewer->getCameraManipulator())
    {
        viewer->getCameraManipulator()->setHomePosition(
            osg::Vec3d(10, 10, 10),  // eye
            osg::Vec3d(0, 0, 0),     // center
            osg::Vec3d(0, 0, 1)      // up
        );
        viewer->getCameraManipulator()->home(0.0);
    }
}

void OSGWidget::fitAll()
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer || !m_geoNode.valid()) return;
    
    if (viewer->getCameraManipulator())
    {
        osg::BoundingSphere bs = m_geoNode->getBound();
        if (bs.valid())
        {
            viewer->getCameraManipulator()->setHomePosition(
                bs.center() + osg::Vec3d(bs.radius() * 2, bs.radius() * 2, bs.radius() * 2),
                bs.center(),
                osg::Vec3d(0, 0, 1)
            );
            viewer->getCameraManipulator()->home(0.0);
        }
    }
}

void OSGWidget::setViewDirection(const glm::vec3& direction, const glm::vec3& up)
{
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    if (viewer->getCameraManipulator())
    {
        osg::BoundingSphere bs = m_geoNode->getBound();
        osg::Vec3d center = bs.valid() ? osg::Vec3d(bs.center()) : osg::Vec3d(0, 0, 0);
        float distance = bs.valid() ? bs.radius() * 3.0f : 10.0f;
        
        osg::Vec3d eye = center - osg::Vec3d(direction.x, direction.y, direction.z) * distance;
        
        viewer->getCameraManipulator()->setHomePosition(
            eye,
            center,
            osg::Vec3d(up.x, up.y, up.z)
        );
        viewer->getCameraManipulator()->home(0.0);
    }
}

void OSGWidget::setWireframeMode(bool wireframe)
{
    osg::StateSet* stateSet = m_geoNode->getOrCreateStateSet();
    if (wireframe)
    {
        osg::PolygonMode* polyMode = new osg::PolygonMode();
        polyMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
        stateSet->setAttributeAndModes(polyMode, osg::StateAttribute::ON);
    }
    else
    {
        stateSet->removeAttribute(osg::StateAttribute::POLYGONMODE);
    }
}

void OSGWidget::setShadedMode(bool shaded)
{
    osg::StateSet* stateSet = m_geoNode->getOrCreateStateSet();
    if (shaded)
    {
        stateSet->removeAttribute(osg::StateAttribute::POLYGONMODE);
    }
}

void OSGWidget::setPointMode(bool point)
{
    osg::StateSet* stateSet = m_geoNode->getOrCreateStateSet();
    if (point)
    {
        osg::PolygonMode* polyMode = new osg::PolygonMode();
        polyMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::POINT);
        stateSet->setAttributeAndModes(polyMode, osg::StateAttribute::ON);
    }
    else
    {
        stateSet->removeAttribute(osg::StateAttribute::POLYGONMODE);
    }
}

void OSGWidget::addGeo(Geo3D* geo)
{
    if (geo && m_geoNode.valid())
    {
        m_geoList.push_back(geo);
        m_geoNode->addChild(geo->getOSGNode().get());
    }
}

void OSGWidget::removeGeo(Geo3D* geo)
{
    if (geo && m_geoNode.valid())
    {
        auto it = std::find(m_geoList.begin(), m_geoList.end(), geo);
        if (it != m_geoList.end())
        {
            m_geoList.erase(it);
            m_geoNode->removeChild(geo->getOSGNode().get());
        }
    }
}

void OSGWidget::removeAllGeos()
{
    if (m_geoNode.valid())
    {
        m_geoNode->removeChildren(0, m_geoNode->getNumChildren());
        m_geoList.clear();
        m_selectedGeo = nullptr;
        m_currentDrawingGeo = nullptr;
    }
}

void OSGWidget::selectGeo(Geo3D* geo)
{
    if (m_selectedGeo)
    {
        m_selectedGeo->clearStateSelected();
    }
    
    m_selectedGeo = geo;
    if (m_selectedGeo)
    {
        m_selectedGeo->setStateSelected();
    }
    
    emit geoSelected(geo);
}

void OSGWidget::deselectAll()
{
    selectGeo(nullptr);
}

PickResult3D OSGWidget::pick(int x, int y)
{
    PickResult3D result;
    
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return result;
    
    osg::Camera* camera = viewer->getCamera();
    
    // 创建射线
    osg::Vec3f nearPoint, farPoint;
    if (camera->getViewport())
    {
        osg::Matrix VPW = camera->getViewMatrix() * 
                         camera->getProjectionMatrix() * 
                         camera->getViewport()->computeWindowMatrix();
        osg::Matrix invVPW;
        invVPW.invert(VPW);
        
        nearPoint = osg::Vec3f(x, height() - y, 0.0f) * invVPW;
        farPoint = osg::Vec3f(x, height() - y, 1.0f) * invVPW;
    }
    
    Ray3D ray(glm::vec3(nearPoint.x(), nearPoint.y(), nearPoint.z()),
              glm::vec3(farPoint.x() - nearPoint.x(), farPoint.y() - nearPoint.y(), farPoint.z() - nearPoint.z()));
    
    // 测试所有几何对象
    float minDistance = FLT_MAX;
    for (Geo3D* geo : m_geoList)
    {
        PickResult3D geoResult;
        if (geo->hitTest(ray, geoResult))
        {
            if (geoResult.distance < minDistance)
            {
                minDistance = geoResult.distance;
                result = geoResult;
            }
        }
    }
    
    return result;
}

glm::vec3 OSGWidget::screenToWorld(int x, int y, float depth)
{
    osg::Vec3f worldPoint;
    
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return glm::vec3(0, 0, 0);
    
    osg::Camera* camera = viewer->getCamera();
    if (camera->getViewport())
    {
        osg::Matrix VPW = camera->getViewMatrix() * 
                         camera->getProjectionMatrix() * 
                         camera->getViewport()->computeWindowMatrix();
        osg::Matrix invVPW;
        invVPW.invert(VPW);
        
        worldPoint = osg::Vec3f(x, height() - y, depth) * invVPW;
    }
    
    return glm::vec3(worldPoint.x(), worldPoint.y(), worldPoint.z());
}

glm::vec2 OSGWidget::worldToScreen(const glm::vec3& worldPos)
{
    osg::Vec2f screenPoint;
    
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return glm::vec2(0, 0);
    
    osg::Camera* camera = viewer->getCamera();
    if (camera->getViewport())
    {
        osg::Matrix VPW = camera->getViewMatrix() * 
                         camera->getProjectionMatrix() * 
                         camera->getViewport()->computeWindowMatrix();
        
        osg::Vec3f world(worldPos.x, worldPos.y, worldPos.z);
        osg::Vec3f screen = world * VPW;
        screenPoint = osg::Vec2f(screen.x(), height() - screen.y());
    }
    
    return glm::vec2(screenPoint.x(), screenPoint.y());
}

// OSGWidget 事件处理
void OSGWidget::paintEvent(QPaintEvent* event)
{
    // osgQOpenGLWidget已经处理了渲染，这里不需要手动调用frame()
    QOpenGLWidget::paintEvent(event);
}

void OSGWidget::resizeEvent(QResizeEvent* event)
{
    osgQOpenGLWidget::resizeEvent(event);
    
    osgViewer::Viewer* viewer = getOsgViewer();
    if (!viewer) return;
    
    osg::Camera* camera = viewer->getCamera();
    if (camera)
    {
        camera->setViewport(0, 0, width(), height());
        double aspectRatio = static_cast<double>(width()) / static_cast<double>(height());
        camera->setProjectionMatrixAsPerspective(45.0f, aspectRatio, 0.1f, 1000.0f);
    }
}

void OSGWidget::mousePressEvent(QMouseEvent* event)
{
    osgQOpenGLWidget::mousePressEvent(event);
    handleDrawingInput(event);
}

void OSGWidget::mouseMoveEvent(QMouseEvent* event)
{
    osgQOpenGLWidget::mouseMoveEvent(event);
    
    // 更新鼠标世界坐标
    glm::vec3 worldPos = screenToWorld(event->x(), event->y(), 0.5f);
    m_lastMouseWorldPos = worldPos;
    emit mousePositionChanged(worldPos);
    
    // 处理绘制预览
    if (m_isDrawing && m_currentDrawingGeo)
    {
        updateCurrentDrawing(worldPos);
    }
}

void OSGWidget::mouseReleaseEvent(QMouseEvent* event)
{
    osgQOpenGLWidget::mouseReleaseEvent(event);
}

void OSGWidget::wheelEvent(QWheelEvent* event)
{
    osgQOpenGLWidget::wheelEvent(event);
}

void OSGWidget::keyPressEvent(QKeyEvent* event)
{
    osgQOpenGLWidget::keyPressEvent(event);
    
    // 处理绘制相关按键
    if (m_isDrawing && m_currentDrawingGeo)
    {
        m_currentDrawingGeo->keyPressEvent(event);
        
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        {
            completeCurrentDrawing();
        }
        else if (event->key() == Qt::Key_Escape)
        {
            cancelCurrentDrawing();
        }
    }
    
    // 处理选择模式
    if (GlobalDrawMode3D == DrawSelect3D && event->key() == Qt::Key_Delete)
    {
        if (m_selectedGeo)
        {
            removeGeo(m_selectedGeo);
            delete m_selectedGeo;
            m_selectedGeo = nullptr;
            emit geoSelected(nullptr);
        }
    }
}

void OSGWidget::keyReleaseEvent(QKeyEvent* event)
{
    osgQOpenGLWidget::keyReleaseEvent(event);
    
    if (m_isDrawing && m_currentDrawingGeo)
    {
        m_currentDrawingGeo->keyReleaseEvent(event);
    }
}

void OSGWidget::handleDrawingInput(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return;
    
    glm::vec3 worldPos = screenToWorld(event->x(), event->y(), 0.5f);
    
    if (GlobalDrawMode3D == DrawSelect3D)
    {
        // 选择模式：拾取对象
        PickResult3D pickResult = pick(event->x(), event->y());
        if (pickResult.hit)
        {
            selectGeo(static_cast<Geo3D*>(pickResult.userData));
        }
        else
        {
            deselectAll();
        }
    }
    else
    {
        // 绘制模式
        if (!m_isDrawing)
        {
            // 开始新的绘制
            m_currentDrawingGeo = createGeo3D(GlobalDrawMode3D);
            if (m_currentDrawingGeo)
            {
                m_isDrawing = true;
                addGeo(m_currentDrawingGeo);
                emit drawingProgress("开始绘制...");
            }
        }
        
        if (m_currentDrawingGeo)
        {
            m_currentDrawingGeo->mousePressEvent(event, worldPos);
            
            // 检查是否完成绘制
            if (m_currentDrawingGeo->isStateComplete())
            {
                completeCurrentDrawing();
            }
        }
    }
}

void OSGWidget::updateCurrentDrawing(const glm::vec3& worldPos)
{
    if (m_currentDrawingGeo)
    {
        QMouseEvent moveEvent(QEvent::MouseMove, QPoint(0, 0), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        m_currentDrawingGeo->mouseMoveEvent(&moveEvent, worldPos);
    }
}

void OSGWidget::completeCurrentDrawing()
{
    if (m_currentDrawingGeo)
    {
        m_currentDrawingGeo->completeDrawing();
        emit drawingProgress("绘制完成");
        m_currentDrawingGeo = nullptr;
        m_isDrawing = false;
    }
}

void OSGWidget::cancelCurrentDrawing()
{
    if (m_currentDrawingGeo)
    {
        removeGeo(m_currentDrawingGeo);
        delete m_currentDrawingGeo;
        m_currentDrawingGeo = nullptr;
        m_isDrawing = false;
        emit drawingProgress("取消绘制");
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
    
    mainLayout->addWidget(m_pointGroup);
    mainLayout->addWidget(m_lineGroup);
    mainLayout->addWidget(m_surfaceGroup);
    mainLayout->addWidget(m_materialGroup);
    mainLayout->addWidget(m_volumeGroup);
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

// PropertyEditor3D 槽函数
void PropertyEditor3D::onPointShapeChanged()
{
    if (m_updating) return;
    
    PointShape3D shape = static_cast<PointShape3D>(m_pointShapeCombo->currentData().toInt());
    
    if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.pointShape = shape;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalPointShape3D = shape;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onPointSizeChanged()
{
    if (m_updating) return;
    
    float size = static_cast<float>(m_pointSizeSpin->value());
    
    if (m_currentGeo)
    {
        GeoParameters3D params = m_currentGeo->getParameters();
        params.pointSize = size;
        m_currentGeo->setParameters(params);
    }
    else
    {
        GlobalPointSize3D = size;
    }
    
    emit parametersChanged();
}

void PropertyEditor3D::onPointColorChanged()
{
    if (m_updating) return;
    
    QColor currentColor = m_currentGeo ? m_currentGeo->getParameters().pointColor.toQColor() : GlobalPointColor3D;
    QColor color = QColorDialog::getColor(currentColor, this, "选择点颜色");
    
    if (color.isValid())
    {
        updateColorButton(m_pointColorButton, color);
        
        if (m_currentGeo)
        {
            GeoParameters3D params = m_currentGeo->getParameters();
            params.pointColor = Color3D(color);
            m_currentGeo->setParameters(params);
        }
        else
        {
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
        
        if (m_currentGeo)
        {
            GeoParameters3D params = m_currentGeo->getParameters();
            params.lineColor = Color3D(color);
            m_currentGeo->setParameters(params);
        }
        else
        {
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
        
        if (m_currentGeo)
        {
            GeoParameters3D params = m_currentGeo->getParameters();
            params.fillColor = Color3D(color);
            m_currentGeo->setParameters(params);
        }
        else
        {
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
        
        if (m_currentGeo)
        {
            GeoParameters3D params = m_currentGeo->getParameters();
            params.borderColor = Color3D(color);
            m_currentGeo->setParameters(params);
        }
        else
        {
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
    
    mainLayout->addWidget(m_drawingGroup);
    mainLayout->addWidget(m_viewGroup);
    mainLayout->addWidget(m_utilityGroup);
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
    
    QPushButton* resetButton = new QPushButton("重置视图");
    resetButton->setToolTip("重置相机到默认位置");
    layout->addWidget(resetButton);
    
    QPushButton* fitButton = new QPushButton("适应窗口");
    fitButton->setToolTip("适应所有对象到窗口");
    layout->addWidget(fitButton);
    
    QPushButton* topButton = new QPushButton("俯视图");
    topButton->setToolTip("切换到俯视图");
    layout->addWidget(topButton);
    
    QPushButton* frontButton = new QPushButton("前视图");
    frontButton->setToolTip("切换到前视图");
    layout->addWidget(frontButton);
    
    QPushButton* rightButton = new QPushButton("右视图");
    rightButton->setToolTip("切换到右视图");
    layout->addWidget(rightButton);
    
    QPushButton* isoButton = new QPushButton("等轴测图");
    isoButton->setToolTip("切换到等轴测图");
    layout->addWidget(isoButton);
}

void ToolPanel3D::createUtilityGroup()
{
    m_utilityGroup = new QGroupBox("实用工具", this);
    QVBoxLayout* layout = new QVBoxLayout(m_utilityGroup);
    
    QPushButton* clearButton = new QPushButton("清空场景");
    clearButton->setToolTip("删除所有对象");
    layout->addWidget(clearButton);
    
    QPushButton* exportButton = new QPushButton("导出图像");
    exportButton->setToolTip("导出当前视图为图像");
    layout->addWidget(exportButton);
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
