#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <QObject>
#include <vector>

// 前向声明
class Geo3D;

/**
 * @brief 控制点管理器
 * 负责管理几何对象的控制点，在绘制过程中临时点作为控制点的最后一个
 */
class GeoControlPointManager : public QObject
{
    Q_OBJECT

public:
    explicit GeoControlPointManager(Geo3D* parent);
    ~GeoControlPointManager() = default;

    // 控制点访问 - 对外统一接口
    const std::vector<Point3D>& getControlPoints() const;
    Point3D getControlPoint(int index) const;
    int getControlPointCountWithoutTempPoint() const;
    bool hasControlPoints() const;

    // 控制点操作
    void addControlPoint(const Point3D& point);
    void setControlPoint(int index, const Point3D& point);
    void removeControlPoint(int index);
    void clearControlPoints();

    // 控制点查询
    int findNearestControlPoint(const Point3D& point, float threshold = 0.1f) const;
    bool isValidIndex(int index) const;

    // 临时点管理 - 内部使用
    void setTempPoint(const Point3D& point);
    void clearTempPoint();

    // 绘制相关通知方法
    void notifyGeometryChanged();

signals:

private:
    void validateIndex(int index) const;
    
    bool isDrawingComplete() const; // 检查绘制是否完成

private:
    Geo3D* m_parent;
    std::vector<Point3D> m_controlPoints;
    Point3D m_tempPoint; // 临时点（内部使用）
};
