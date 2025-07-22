#include "ScaleBarRenderer.h"
#include "LogManager.h"
#include <QPen>
#include <QFont>
#include <QColor>
#include <QRect>
#include <cmath>
#include <osg/Math>

ScaleBarRenderer::ScaleBarRenderer()
    : m_cameraController(nullptr)
    , m_enabled(false)  // 暂时禁用比例尺
    , m_position(10, 10)
    , m_size(200, 60)
    , m_cachedScaleValue(0.0)
{
    LOG_INFO("创建比例尺渲染器（暂时禁用）", "比例尺");
}

void ScaleBarRenderer::setCameraController(CameraController* controller)
{
    if (m_cameraController != controller) {
        m_cameraController = controller;
        invalidateCache();
        LOG_INFO("设置相机控制器到比例尺渲染器", "比例尺");
    }
}

void ScaleBarRenderer::drawScaleBar(QPainter& painter, int viewportWidth, int viewportHeight)
{
    if (!m_enabled || !m_cameraController) return;

    // 计算比例尺值
    double scaleValue = calculateScaleValue(viewportWidth, viewportHeight);
    QString scaleText = formatScaleText(scaleValue);

    // 设置绘制区域
    QRect scaleRect(m_position.x(), m_position.y(), m_size.width(), m_size.height());

    // 绘制背景
    drawBackground(painter, scaleRect);

    // 绘制比例尺线条和刻度
    drawScaleLines(painter, scaleRect);

    // 绘制文本
    drawScaleText(painter, scaleRect, scaleText);
}

void ScaleBarRenderer::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        LOG_INFO(QString("比例尺渲染器%1").arg(enabled ? "启用" : "禁用"), "比例尺");
    }
}

void ScaleBarRenderer::setPosition(const QPoint& position)
{
    if (m_position != position) {
        m_position = position;
        LOG_DEBUG(QString("设置比例尺位置: (%1, %2)").arg(position.x()).arg(position.y()), "比例尺");
    }
}

void ScaleBarRenderer::setSize(int width, int height)
{
    setSize(QSize(width, height));
}

void ScaleBarRenderer::setSize(const QSize& size)
{
    if (m_size != size) {
        m_size = size;
        invalidateCache(); // 大小改变会影响计算
        LOG_DEBUG(QString("设置比例尺大小: %1x%2").arg(size.width()).arg(size.height()), "比例尺");
    }
}

void ScaleBarRenderer::invalidateCache()
{
    m_lastCalculation = QDateTime();
    LOG_DEBUG("比例尺缓存已失效", "比例尺");
}

double ScaleBarRenderer::calculateScaleValue(int viewportWidth, int viewportHeight)
{
    if (!m_cameraController) return 1.0;

    // 检查缓存是否有效
    QDateTime currentTime = QDateTime::currentDateTime();
    if (m_lastCalculation.isValid() && 
        m_lastCalculation.msecsTo(currentTime) < CACHE_DURATION)
    {
        return m_cachedScaleValue;
    }

    ProjectionMode mode = m_cameraController->getProjectionMode();

    if (mode == ProjectionMode::Orthographic)
    {
        // 正交投影模式：直接使用正交投影的大小计算比例尺
        double orthoWidth = m_cameraController->getRight() - m_cameraController->getLeft();
        double scaleBarPixels = m_size.width() - 20; // 减去边距
        double scaleBarWorldUnits = (orthoWidth * scaleBarPixels) / viewportWidth;

        m_cachedScaleValue = scaleBarWorldUnits;
        m_lastCalculation = currentTime;
        return scaleBarWorldUnits;
    }
    else
    {
        // 透视投影模式：计算基于相机距离的比例尺
        osg::Vec3d eye = m_cameraController->getEyePosition();
        osg::Vec3d center = m_cameraController->getCenterPosition();

        // 计算相机到中心的距离
        double distance = (eye - center).length();

        // 计算屏幕像素对应的世界单位
        double fov = m_cameraController->getFOV();
        double worldHeight = 2.0 * distance * tan(osg::DegreesToRadians(fov / 2.0));
        double pixelsPerUnit = viewportHeight / worldHeight;

        // 计算比例尺对应的世界单位
        double scaleBarPixels = m_size.width() - 20; // 减去边距
        double scaleBarWorldUnits = scaleBarPixels / pixelsPerUnit;

        m_cachedScaleValue = scaleBarWorldUnits;
        m_lastCalculation = currentTime;
        return scaleBarWorldUnits;
    }
}

QString ScaleBarRenderer::formatScaleText(double worldUnits)
{
    QString unit = "m";
    double value = worldUnits;

    // 根据数值大小选择合适的单位
    if (value >= 1000.0)
    {
        value /= 1000.0;
        unit = "km";
    }
    else if (value < 1.0 && value >= 0.01)
    {
        value *= 100.0;
        unit = "cm";
    }
    else if (value < 0.01)
    {
        value *= 1000.0;
        unit = "mm";
    }

    // 格式化数值
    if (value >= 100.0)
    {
        return QString("%1 %2").arg(static_cast<int>(value)).arg(unit);
    }
    else if (value >= 10.0)
    {
        return QString("%1 %2").arg(value, 0, 'f', 1).arg(unit);
    }
    else
    {
        return QString("%1 %2").arg(value, 0, 'f', 2).arg(unit);
    }
}

void ScaleBarRenderer::drawBackground(QPainter& painter, const QRect& scaleRect)
{
    // 绘制背景
    painter.fillRect(scaleRect, QColor(0, 0, 0, 100));
    painter.setPen(QPen(QColor(255, 255, 255), 1));
    painter.drawRect(scaleRect);
}

void ScaleBarRenderer::drawScaleLines(QPainter& painter, const QRect& scaleRect)
{
    // 绘制比例尺线条
    int barWidth = m_size.width() - 20;
    int barHeight = 4;
    int barY = scaleRect.center().y() - barHeight / 2;

    // 绘制主线条
    painter.setPen(QPen(QColor(255, 255, 255), 2));
    painter.drawLine(scaleRect.left() + 10, barY, scaleRect.left() + 10 + barWidth, barY);

    // 绘制刻度线
    painter.setPen(QPen(QColor(255, 255, 255), 1));
    for (int i = 0; i <= 10; ++i)
    {
        int x = scaleRect.left() + 10 + (barWidth * i) / 10;
        int tickHeight = (i % 5 == 0) ? 8 : 4;
        painter.drawLine(x, barY - tickHeight, x, barY + tickHeight);
    }
}

void ScaleBarRenderer::drawScaleText(QPainter& painter, const QRect& scaleRect, const QString& text)
{
    // 绘制文本
    painter.setPen(QColor(255, 255, 255));
    painter.setFont(QFont("Arial", 8));

    // 计算文本区域
    int barY = scaleRect.center().y();
    QRect textRect = scaleRect.adjusted(5, barY + 10, -5, -5);
    painter.drawText(textRect, Qt::AlignCenter, text);
} 