#include <QApplication>
#include <QDir>
#include <QLocale>
#include <QTranslator>
#include <QTextCodec>
#include <QStyleFactory>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <osgDB/Registry>
#include <osgViewer/Viewer>
#include <osg/Notify>

#include "ui/MainWindow.h"
#include "core/Common3D.h"
#include "util/LogManager.h"

int main(int argc, char *argv[])
{
    // 设置OSG通知级别，让OSG_WARN等消息可以输出
    osg::setNotifyLevel(osg::WARN);
    // 如果需要更详细的调试信息，可以设置为：
    // osg::setNotifyLevel(osg::DEBUG_INFO);
    
    // 设置 OSG 插件搜索路径（不想设置环境变量）
    #ifdef OSG_PLUGIN_PATH
    std::string s = OSG_PLUGIN_PATH;
    osgDB::Registry::instance()->setLibraryFilePathList({std::string(s)});
    std::cout<<s;
    #endif

    QApplication app(argc, argv);
    
    // 设置应用程序基本信息
    app.setApplicationName("OSG 3D Drawing Board");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("3D Graphics Lab");
    app.setOrganizationDomain("3dgraphics.com");
    
    // 设置字符编码 (Qt 5.15+ 兼容性处理)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif
    
    // 设置应用程序样式
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // 初始化日志系统
    LogManager* logManager = LogManager::getInstance();
    LOG_INFO("应用程序启动", "系统");
    LOG_INFO("日志系统初始化完成", "系统");
    
    //// 设置深色主题样式表
    //QString darkStyleSheet = R"(
    //    QMainWindow {
    //        background-color: #2b2b2b;
    //        color: #ffffff;
    //    }
    //    
    //    QMenuBar {
    //        background-color: #3c3c3c;
    //        color: #ffffff;
    //        border: 1px solid #555555;
    //    }
    //    
    //    QMenuBar::item {
    //        background-color: transparent;
    //        padding: 4px 8px;
    //    }
    //    
    //    QMenuBar::item:selected {
    //        background-color: #4a4a4a;
    //    }
    //    
    //    QMenu {
    //        background-color: #3c3c3c;
    //        color: #ffffff;
    //        border: 1px solid #555555;
    //    }
    //    
    //    QMenu::item:selected {
    //        background-color: #4a4a4a;
    //    }
    //    
    //    QToolBar {
    //        background-color: #3c3c3c;
    //        border: 1px solid #555555;
    //        spacing: 2px;
    //    }
    //    
    //    QToolButton {
    //        background-color: #4a4a4a;
    //        border: 1px solid #666666;
    //        padding: 4px;
    //        margin: 1px;
    //    }
    //    
    //    QToolButton:hover {
    //        background-color: #5a5a5a;
    //    }
    //    
    //    QToolButton:pressed {
    //        background-color: #6a6a6a;
    //    }
    //    
    //    QToolButton:checked {
    //        background-color: #0078d4;
    //    }
    //    
    //    QStatusBar {
    //        background-color: #3c3c3c;
    //        color: #ffffff;
    //        border-top: 1px solid #555555;
    //    }
    //    
    //    QDockWidget {
    //        background-color: #2b2b2b;
    //        color: #ffffff;
    //        titlebar-close-icon: url(close.png);
    //        titlebar-normal-icon: url(double.png);
    //    }
    //    
    //    QDockWidget::title {
    //        background-color: #3c3c3c;
    //        border: 1px solid #555555;
    //        padding: 4px;
    //    }
    //    
    //    QGroupBox {
    //        background-color: #3c3c3c;
    //        border: 2px solid #555555;
    //        border-radius: 4px;
    //        margin: 4px;
    //        padding-top: 8px;
    //        font-weight: bold;
    //    }
    //    
    //    QGroupBox::title {
    //        subcontrol-origin: margin;
    //        left: 8px;
    //        padding: 0 4px 0 4px;
    //    }
    //    
    //    QPushButton {
    //        background-color: #4a4a4a;
    //        border: 1px solid #666666;
    //        padding: 6px 12px;
    //        border-radius: 3px;
    //        min-width: 60px;
    //    }
    //    
    //    QPushButton:hover {
    //        background-color: #5a5a5a;
    //    }
    //    
    //    QPushButton:pressed {
    //        background-color: #6a6a6a;
    //    }
    //    
    //    QComboBox {
    //        background-color: #4a4a4a;
    //        border: 1px solid #666666;
    //        padding: 4px;
    //        min-width: 100px;
    //    }
    //    
    //    QComboBox::drop-down {
    //        border: none;
    //        width: 20px;
    //    }
    //    
    //    QComboBox::down-arrow {
    //        image: url(down_arrow.png);
    //    }
    //    
    //    QComboBox QAbstractItemView {
    //        background-color: #3c3c3c;
    //        border: 1px solid #555555;
    //        selection-background-color: #4a4a4a;
    //    }
    //    
    //    QSpinBox, QDoubleSpinBox {
    //        background-color: #4a4a4a;
    //        border: 1px solid #666666;
    //        padding: 4px;
    //    }
    //    
    //    QSlider::groove:horizontal {
    //        background-color: #4a4a4a;
    //        height: 8px;
    //        border-radius: 4px;
    //    }
    //    
    //    QSlider::handle:horizontal {
    //        background-color: #0078d4;
    //        border: 1px solid #0078d4;
    //        width: 16px;
    //        border-radius: 8px;
    //        margin: -4px 0;
    //    }
    //    
    //    QCheckBox {
    //        spacing: 8px;
    //    }
    //    
    //    QCheckBox::indicator {
    //        width: 16px;
    //        height: 16px;
    //    }
    //    
    //    QCheckBox::indicator:unchecked {
    //        background-color: #4a4a4a;
    //        border: 1px solid #666666;
    //    }
    //    
    //    QCheckBox::indicator:checked {
    //        background-color: #0078d4;
    //        border: 1px solid #0078d4;
    //    }
    //    
    //    QLabel {
    //        color: #ffffff;
    //    }
    //)";
    //
    //app.setStyleSheet(darkStyleSheet);
    
    // 初始化全局3D设置
    initializeGlobal3DSettings();
    
    // 初始化配置管理系统
    if (Config3D::initializeConfigSystem()) {
        LOG_INFO("配置管理系统初始化成功", "系统");
    } else {
        LOG_WARNING("配置管理系统初始化失败，使用默认设置", "系统");
    }
    
    // 创建启动画面
    QPixmap pixmap(400, 300);
    pixmap.fill(QColor(43, 43, 43));
    
    QSplashScreen splash(pixmap);
    splash.setFont(QFont("Arial", 12, QFont::Bold));
    splash.showMessage(QObject::tr("正在初始化 OSG 3D 绘图板..."), Qt::AlignCenter | Qt::AlignBottom, Qt::white);
    splash.show();
    
    app.processEvents();
    
    // 创建主窗口
    MainWindow window;
    
    QTimer::singleShot(3000, [&]() {
        splash.finish(&window);
        window.show();
        window.raise();
        window.activateWindow();
    });
    
    // 程序结束时保存配置并清理配置系统
    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        LOG_INFO("应用程序即将退出，保存配置", "系统");
        Config3D::finalizeConfigSystem();
    });
    
    return app.exec();
}
