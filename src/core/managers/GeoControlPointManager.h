#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <QObject>
#include <vector>

// 前向声明
class Geo3D;

/**
 * @brief 控制点管理器
 * 负责管理几何对象的控制点，包括添加、删除、修改、查询等操作
 */
class GeoControlPointManager : public QObject
{
    Q_OBJECT

public:
    explicit GeoControlPointManager(Geo3D* parent);
    ~GeoControlPointManager() = default;

    // 控制点访问
    const std::vector<Point3D>& getControlPoints() const { return m_controlPoints; }
    Point3D getControlPoint(int index) const;
    int getControlPointCount() const { return static_cast<int>(m_controlPoints.size()); }
    bool hasControlPoints() const { return !m_controlPoints.empty(); }

    // 控制点操作
    void addControlPoint(const Point3D& point);
    void insertControlPoint(int index, const Point3D& point);
    void setControlPoint(int index, const Point3D& point);
    void removeControlPoint(int index);
    void removeLastControlPoint();
    void clearControlPoints();

    // 控制点查询
    int findNearestControlPoint(const Point3D& point, float threshold = 0.1f) const;
    bool isValidIndex(int index) const;
    
    // 控制点变换
    void translateControlPoints(const glm::vec3& offset);
    void rotateControlPoints(const glm::vec3& axis, float angle, const glm::vec3& center);
    void scaleControlPoints(const glm::vec3& scale, const glm::vec3& center);
    void transformControlPoints(const glm::mat4& matrix);

    // 控制点验证
    bool validateControlPoints() const;
    bool isMinimumPointsMet() const;
    int getMinimumPointsRequired() const;
    void setMinimumPointsRequired(int count);

    // 控制点预览
    void startPreview();
    void stopPreview();
    bool isPreviewActive() const { return m_previewActive; }

    // 控制点显示
    void setControlPointsVisible(bool visible);
    bool areControlPointsVisible() const { return m_controlPointsVisible; }

    // 控制点样式
    void setControlPointSize(float size);
    float getControlPointSize() const { return m_controlPointSize; }
    void setControlPointColor(const Color3D& color);
    const Color3D& getControlPointColor() const { return m_controlPointColor; }

signals:
    void controlPointAdded(int index, const Point3D& point);
    void controlPointRemoved(int index);
    void controlPointChanged(int index, const Point3D& oldPoint, const Point3D& newPoint);
    void controlPointsChanged();
    void controlPointsCleared();
    void controlPointsTransformed();
    void previewStarted();
    void previewStopped();
    void visibilityChanged(bool visible);

private:
    void validateIndex(int index) const;
    void notifyGeometryChanged();
    void updateControlPointVisualization();

private:
    Geo3D* m_parent;
    std::vector<Point3D> m_controlPoints;
    
    // 控制点属性
    int m_minimumPointsRequired;
    bool m_previewActive;
    bool m_controlPointsVisible;
    float m_controlPointSize;
    Color3D m_controlPointColor;
}; 