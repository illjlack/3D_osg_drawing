#pragma once
#pragma execution_character_set("utf-8")

#include <QPainter>
#include <QPoint>
#include <QSize>
#include <QDateTime>
#include "../core/camera/CameraController.h"

/**
 * @brief 比例尺渲染器类
 * 负责计算和绘制比例尺，从OSGWidget中分离出来以提高代码组织性
 */
class ScaleBarRenderer
{
public:
    explicit ScaleBarRenderer();
    virtual ~ScaleBarRenderer() = default;

    // 设置相机控制器
    void setCameraController(CameraController* controller);

    // 绘制比例尺
    void drawScaleBar(QPainter& painter, int viewportWidth, int viewportHeight);

    // 启用/禁用比例尺
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    // 设置比例尺位置和大小
    void setPosition(const QPoint& position);
    void setSize(int width, int height);
    void setSize(const QSize& size);

    // 获取比例尺属性
    const QPoint& getPosition() const { return m_position; }
    const QSize& getSize() const { return m_size; }

    // 强制刷新比例尺计算缓存
    void invalidateCache();

private:
    // 计算比例尺值（世界单位）
    double calculateScaleValue(int viewportWidth, int viewportHeight);

    // 格式化比例尺文本
    QString formatScaleText(double worldUnits);

    // 绘制比例尺背景
    void drawBackground(QPainter& painter, const QRect& scaleRect);

    // 绘制比例尺线条和刻度
    void drawScaleLines(QPainter& painter, const QRect& scaleRect);

    // 绘制比例尺文本
    void drawScaleText(QPainter& painter, const QRect& scaleRect, const QString& text);

private:
    // 相机控制器引用
    CameraController* m_cameraController;

    // 比例尺属性
    bool m_enabled;
    QPoint m_position;        // 比例尺位置
    QSize m_size;             // 比例尺大小

    // 缓存相关
    double m_cachedScaleValue;     // 缓存的比例尺值
    QDateTime m_lastCalculation;   // 上次计算时间
    static const int CACHE_DURATION = 100; // 缓存持续时间（毫秒）
}; 

