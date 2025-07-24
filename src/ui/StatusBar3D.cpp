#include "StatusBar3D.h"
#include "OSGWidget.h"
#include <QApplication>
#include <QScreen>
#include <QStyle>
#include <QFont>
#include <QDebug>

StatusBar3D::StatusBar3D(QWidget* parent)
    : QWidget(parent)
    , m_osgWidget(nullptr)
    , m_screenCoordLabel(nullptr)
    , m_worldCoordLabel(nullptr)
    , m_modeLabel(nullptr)
    , m_objectCountLabel(nullptr)
    , m_coordinateRangeLabel(nullptr)
    , m_cameraSpeedLabel(nullptr)
    , m_projectionModeLabel(nullptr)
    , m_manipulatorLabel(nullptr)
    , m_fpsLabel(nullptr)
    , m_memoryLabel(nullptr)
    , m_temporaryMessageLabel(nullptr)
    , m_performanceTimer(nullptr)
    , m_mainLayout(nullptr)
    , m_messageTimer(nullptr)
{
    setupUI();
    setupConnections();
    setFixedHeight(25);
}

StatusBar3D::~StatusBar3D()
{
    if (m_performanceTimer) {
        m_performanceTimer->stop();
        delete m_performanceTimer;
    }
    if (m_messageTimer) {
        m_messageTimer->stop();
        delete m_messageTimer;
    }
}

void StatusBar3D::setOSGWidget(OSGWidget* osgWidget)
{
    m_osgWidget = osgWidget;
    if (m_osgWidget) {
        connect(m_osgWidget, &OSGWidget::mousePositionChanged,
                this, [this](const glm::dvec3& pos) {
                    updateWorldCoordinates(pos);
                });
    }
}

void StatusBar3D::setupUI()
{
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(4, 2, 4, 2);
    m_mainLayout->setSpacing(2);
    
    // 创建固定显示标签
    m_screenCoordLabel = new QLabel(tr("屏幕: (0, 0)"));
    m_screenCoordLabel->setMinimumWidth(120);
    
    m_worldCoordLabel = new QLabel(tr("空间: (0.00, 0.00, 0.00)"));
    m_worldCoordLabel->setMinimumWidth(180);
    
    m_modeLabel = new QLabel(tr("模式: 选择"));
    m_modeLabel->setMinimumWidth(100);
    
    m_objectCountLabel = new QLabel(tr("对象: 0"));
    m_objectCountLabel->setMinimumWidth(80);
    
    m_coordinateRangeLabel = new QLabel(tr("范围: 地球"));
    m_coordinateRangeLabel->setMinimumWidth(100);
    
    m_cameraSpeedLabel = new QLabel(tr("相机速度: 100"));
    m_cameraSpeedLabel->setMinimumWidth(120);
    
    m_projectionModeLabel = new QLabel(tr("投影: 透视"));
    m_projectionModeLabel->setMinimumWidth(100);
    
    m_manipulatorLabel = new QLabel(tr("操作器: 轨道"));
    m_manipulatorLabel->setMinimumWidth(100);
    
    m_fpsLabel = new QLabel(tr("FPS: 60"));
    m_fpsLabel->setMinimumWidth(80);
    
    m_memoryLabel = new QLabel(tr("内存: 0MB"));
    m_memoryLabel->setMinimumWidth(100);
    
    m_temporaryMessageLabel = new QLabel(tr("就绪"));
    m_temporaryMessageLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    // 创建分隔符
    auto createSeparator = []() -> QFrame* {
        QFrame* separator = new QFrame();
        separator->setFrameShape(QFrame::VLine);
        separator->setFrameShadow(QFrame::Sunken);
        separator->setFixedWidth(1);
        separator->setFixedHeight(16);
        return separator;
    };
    
    m_separator1 = createSeparator();
    m_separator2 = createSeparator();
    m_separator3 = createSeparator();
    m_separator4 = createSeparator();
    m_separator5 = createSeparator();
    m_separator6 = createSeparator();
    m_separator7 = createSeparator();
    m_separator8 = createSeparator();
    m_separator9 = createSeparator();
    m_separator10 = createSeparator();
    
    // 添加组件到布局
    m_mainLayout->addWidget(m_screenCoordLabel);
    m_mainLayout->addWidget(m_separator1);
    m_mainLayout->addWidget(m_worldCoordLabel);
    m_mainLayout->addWidget(m_separator2);
    m_mainLayout->addWidget(m_modeLabel);
    m_mainLayout->addWidget(m_separator3);
    m_mainLayout->addWidget(m_objectCountLabel);
    m_mainLayout->addWidget(m_separator4);
    m_mainLayout->addWidget(m_coordinateRangeLabel);
    m_mainLayout->addWidget(m_separator5);
    m_mainLayout->addWidget(m_cameraSpeedLabel);
    m_mainLayout->addWidget(m_separator6);
    m_mainLayout->addWidget(m_projectionModeLabel);
    m_mainLayout->addWidget(m_separator7);
    m_mainLayout->addWidget(m_manipulatorLabel);
    m_mainLayout->addWidget(m_separator8);
    m_mainLayout->addWidget(m_fpsLabel);
    m_mainLayout->addWidget(m_separator9);
    m_mainLayout->addWidget(m_memoryLabel);
    m_mainLayout->addWidget(m_separator10);
    m_mainLayout->addStretch();
    m_mainLayout->addWidget(m_temporaryMessageLabel);
    
    // 创建定时器
    m_performanceTimer = new QTimer(this);
    m_performanceTimer->setInterval(1000);
    m_performanceTimer->start();
    
    m_messageTimer = new QTimer(this);
    m_messageTimer->setSingleShot(true);
}

void StatusBar3D::setupConnections()
{
    connect(m_performanceTimer, &QTimer::timeout, this, &StatusBar3D::updatePerformanceInfo);
    connect(m_messageTimer, &QTimer::timeout, this, [this]() {
        if (m_temporaryMessageLabel) {
            m_temporaryMessageLabel->setText(tr("就绪"));
        }
    });
}

void StatusBar3D::updateScreenCoordinates(int x, int y)
{
    if (m_screenCoordLabel) {
        m_screenCoordLabel->setText(tr("屏幕: (%1, %2)").arg(x).arg(y));
    }
}

void StatusBar3D::updateWorldCoordinates(const glm::dvec3& pos)
{
    if (m_worldCoordLabel) {
        m_worldCoordLabel->setText(tr("空间: (%1, %2, %3)")
            .arg(pos.x, 0, 'f', 2)
            .arg(pos.y, 0, 'f', 2)
            .arg(pos.z, 0, 'f', 2));
    }
}

void StatusBar3D::updateMode(const QString& mode)
{
    if (m_modeLabel) {
        m_modeLabel->setText(tr("模式: %1").arg(mode));
    }
}

void StatusBar3D::updateObjectCount(int count)
{
    if (m_objectCountLabel) {
        m_objectCountLabel->setText(tr("对象: %1").arg(count));
    }
}

void StatusBar3D::updateCoordinateRange(const QString& range)
{
    if (m_coordinateRangeLabel) {
        m_coordinateRangeLabel->setText(tr("范围: %1").arg(range));
    }
}

void StatusBar3D::updateCameraSpeed(double speed)
{
    if (m_cameraSpeedLabel) {
        m_cameraSpeedLabel->setText(tr("相机速度: %1").arg(speed, 0, 'f', 2));
    }
}

void StatusBar3D::updateProjectionMode(const QString& mode)
{
    if (m_projectionModeLabel) {
        m_projectionModeLabel->setText(tr("投影: %1").arg(mode));
    }
}

void StatusBar3D::updateManipulatorType(const QString& type)
{
    if (m_manipulatorLabel) {
        m_manipulatorLabel->setText(tr("操作器: %1").arg(type));
    }
}

void StatusBar3D::updateFPS(double fps)
{
    if (m_fpsLabel) {
        m_fpsLabel->setText(tr("FPS: %1").arg(fps, 0, 'f', 0));
    }
}

void StatusBar3D::updateMemoryUsage(double mb)
{
    if (m_memoryLabel) {
        m_memoryLabel->setText(tr("内存: %1MB").arg(mb, 0, 'f', 1));
    }
}

void StatusBar3D::showTemporaryMessage(const QString& message, int duration)
{
    if (m_temporaryMessageLabel) {
        m_temporaryMessageLabel->setText(message);
        m_messageTimer->start(duration);
    }
}

void StatusBar3D::updatePerformanceInfo()
{
    // 简化实现：固定FPS和内存值
    updateFPS(60.0);
    updateMemoryUsage(512.0);
} 




