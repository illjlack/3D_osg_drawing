#pragma once

#include "Common3D.h"
#include <osg/Node>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Drawable>
#include <osg/Geometry>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/Material>
#include <osg/StateSet>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/PolygonMode>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/MatrixTransform>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>
#include <QMouseEvent>
#include <QKeyEvent>
#include <vector>
#include <memory>

// 前向声明
class Geo3D;
class Point3D_Geo;
class Line3D_Geo;
class Arc3D_Geo;
class BezierCurve3D_Geo;
class Triangle3D_Geo;
class Quad3D_Geo;
class Polygon3D_Geo;
class Box3D_Geo;
class Cube3D_Geo;
class Cylinder3D_Geo;
class Cone3D_Geo;
class Sphere3D_Geo;
class Torus3D_Geo;

// 几何对象工厂
Geo3D* createGeo3D(DrawMode3D mode);

// 三维几何对象基类
class Geo3D
{
public:
    Geo3D();
    virtual ~Geo3D();

    // 类型相关
    GeoType3D getGeoType() const { return m_geoType; }
    void setGeoType(GeoType3D type) { m_geoType = type; }

    // 状态管理
    bool isStateInitialized() const { return m_geoState & GeoState_Initialized3D; }
    bool isStateComplete() const { return m_geoState & GeoState_Complete3D; }
    bool isStateInvalid() const { return m_geoState & GeoState_Invalid3D; }
    bool isStateSelected() const { return m_geoState & GeoState_Selected3D; }
    bool isStateEditing() const { return m_geoState & GeoState_Editing3D; }

    void setStateInitialized() { m_geoState |= GeoState_Initialized3D; }
    void setStateComplete() { m_geoState |= GeoState_Complete3D; }
    void setStateInvalid() { m_geoState |= GeoState_Invalid3D; }
    void setStateSelected() { m_geoState |= GeoState_Selected3D; }
    void setStateEditing() { m_geoState |= GeoState_Editing3D; }

    void clearStateComplete() { m_geoState &= ~GeoState_Complete3D; }
    void clearStateInvalid() { m_geoState &= ~GeoState_Invalid3D; }
    void clearStateSelected() { m_geoState &= ~GeoState_Selected3D; }
    void clearStateEditing() { m_geoState &= ~GeoState_Editing3D; }

    // 参数管理
    const GeoParameters3D& getParameters() const { return m_parameters; }
    void setParameters(const GeoParameters3D& params);

    // 控制点管理
    const std::vector<Point3D>& getControlPoints() const { return m_controlPoints; }
    void addControlPoint(const Point3D& point);
    void setControlPoint(int index, const Point3D& point);
    void removeControlPoint(int index);
    void clearControlPoints();
    bool hasControlPoints() const { return !m_controlPoints.empty(); }
    
    // 临时点（用于绘制过程中的预览）
    const Point3D& getTempPoint() const { return m_tempPoint; }
    void setTempPoint(const Point3D& point) { m_tempPoint = point; }

    // 变换
    const Transform3D& getTransform() const { return m_transform; }
    void setTransform(const Transform3D& transform);

    // 包围盒
    const BoundingBox3D& getBoundingBox() const { return m_boundingBox; }

    // OSG节点
    osg::ref_ptr<osg::Group> getOSGNode() const { return m_osgNode; }

    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos);
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos);
    virtual void mouseReleaseEvent(QMouseEvent* event, const glm::vec3& worldPos);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);

    // 拾取测试
    virtual bool hitTest(const Ray3D& ray, PickResult3D& result) const;

    // 绘制完成
    virtual void completeDrawing();
    
    // 更新几何体
    virtual void updateGeometry() = 0;

protected:
    // 初始化（第一次调用时）
    virtual void initialize();
    
    // 更新OSG节点
    virtual void updateOSGNode();
    virtual void updateMaterial();
    virtual void updateControlPointsVisualization();
    
    // 创建几何体（子类实现）
    virtual osg::ref_ptr<osg::Geometry> createGeometry() = 0;
    
    // 辅助函数
    osg::Vec3 glmToOsgVec3(const glm::vec3& v) const;
    osg::Vec4 glmToOsgVec4(const glm::vec4& v) const;
    glm::vec3 osgToGlmVec3(const osg::Vec3& v) const;
    glm::vec4 osgToGlmVec4(const osg::Vec4& v) const;
    
    void updateBoundingBox();
    
    void markGeometryDirty() { m_geometryDirty = true; }
    bool isGeometryDirty() const { return m_geometryDirty; }
    void clearGeometryDirty() { m_geometryDirty = false; }

protected:
    GeoType3D m_geoType;
    int m_geoState;
    GeoParameters3D m_parameters;
    
    std::vector<Point3D> m_controlPoints;
    Point3D m_tempPoint;
    Transform3D m_transform;
    BoundingBox3D m_boundingBox;
    
    // OSG相关 - 使用现代OSG API (Group+Drawable而不是Geode)
    osg::ref_ptr<osg::Group> m_osgNode;
    osg::ref_ptr<osg::Group> m_drawableGroup;  // 替代Geode
    osg::ref_ptr<osg::Geometry> m_geometry;
    osg::ref_ptr<osg::MatrixTransform> m_transformNode;
    osg::ref_ptr<osg::Group> m_controlPointsNode;
    
    bool m_geometryDirty;
    bool m_initialized;
};

// ========================================= 点类 =========================================
class Point3D_Geo : public Geo3D
{
public:
    Point3D_Geo();
    
    void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void completeDrawing() override;
    void updateGeometry() override;

protected:
    osg::ref_ptr<osg::Geometry> createGeometry() override;
    
private:
    osg::ref_ptr<osg::Geometry> createPointGeometry(PointShape3D shape, float size);
};

// ========================================= 线类 =========================================
class Line3D_Geo : public Geo3D
{
public:
    Line3D_Geo();
    
    void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void keyPressEvent(QKeyEvent* event) override;
    void updateGeometry() override;

protected:
    osg::ref_ptr<osg::Geometry> createGeometry() override;
    
private:
    void generatePolyline();
    void generateSpline();
    void generateBezierCurve();
    
    std::vector<Point3D> m_generatedPoints;
};

// ========================================= 圆弧类 =========================================
class Arc3D_Geo : public Geo3D
{
public:
    Arc3D_Geo();
    
    void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void updateGeometry() override;

protected:
    osg::ref_ptr<osg::Geometry> createGeometry() override;
    
private:
    void calculateArcFromThreePoints();
    void generateArcPoints();
    
    glm::vec3 m_center;
    float m_radius;
    float m_startAngle;
    float m_endAngle;
    glm::vec3 m_normal;
    std::vector<Point3D> m_arcPoints;
};

// ========================================= 贝塞尔曲线类 =========================================
class BezierCurve3D_Geo : public Geo3D
{
public:
    BezierCurve3D_Geo();
    
    void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void keyPressEvent(QKeyEvent* event) override;
    void updateGeometry() override;

protected:
    osg::ref_ptr<osg::Geometry> createGeometry() override;
    
private:
    void generateBezierPoints();
    glm::vec3 calculateBezierPoint(float t) const;
    
    std::vector<Point3D> m_bezierPoints;
};

// ========================================= 三角形类 =========================================
class Triangle3D_Geo : public Geo3D
{
public:
    Triangle3D_Geo();
    
    void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void updateGeometry() override;

protected:
    osg::ref_ptr<osg::Geometry> createGeometry() override;
    
private:
    void calculateNormal();
    
    glm::vec3 m_normal;
};

// ========================================= 四边形类 =========================================
class Quad3D_Geo : public Geo3D
{
public:
    Quad3D_Geo();
    
    void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void updateGeometry() override;

protected:
    osg::ref_ptr<osg::Geometry> createGeometry() override;
    
private:
    void calculateNormal();
    
    glm::vec3 m_normal;
};

// ========================================= 多边形类 =========================================
class Polygon3D_Geo : public Geo3D
{
public:
    Polygon3D_Geo();
    
    void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void keyPressEvent(QKeyEvent* event) override;
    void updateGeometry() override;

protected:
    osg::ref_ptr<osg::Geometry> createGeometry() override;
    
private:
    void calculateNormal();
    void triangulatePolygon();
    
    glm::vec3 m_normal;
    std::vector<unsigned int> m_triangleIndices;
};

// ========================================= 长方体类 =========================================
class Box3D_Geo : public Geo3D
{
public:
    Box3D_Geo();
    
    void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void updateGeometry() override;

protected:
    osg::ref_ptr<osg::Geometry> createGeometry() override;
    
private:
    glm::vec3 m_size;
};

// ========================================= 正方体类 =========================================
class Cube3D_Geo : public Geo3D
{
public:
    Cube3D_Geo();
    
    void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void updateGeometry() override;

protected:
    osg::ref_ptr<osg::Geometry> createGeometry() override;
    
private:
    float m_size;
};

// ========================================= 圆柱类 =========================================
class Cylinder3D_Geo : public Geo3D
{
public:
    Cylinder3D_Geo();
    
    void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void updateGeometry() override;

protected:
    osg::ref_ptr<osg::Geometry> createGeometry() override;
    
private:
    float m_radius;
    float m_height;
    glm::vec3 m_axis;
};

// ========================================= 圆锥类 =========================================
class Cone3D_Geo : public Geo3D
{
public:
    Cone3D_Geo();
    
    void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void updateGeometry() override;

protected:
    osg::ref_ptr<osg::Geometry> createGeometry() override;
    
private:
    float m_radius;
    float m_height;
    glm::vec3 m_axis;
};

// ========================================= 球类 =========================================
class Sphere3D_Geo : public Geo3D
{
public:
    Sphere3D_Geo();
    
    void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void updateGeometry() override;

protected:
    osg::ref_ptr<osg::Geometry> createGeometry() override;
    
private:
    float m_radius;
};

// ========================================= 圆环类 =========================================
class Torus3D_Geo : public Geo3D
{
public:
    Torus3D_Geo();
    
    void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    void updateGeometry() override;

protected:
    osg::ref_ptr<osg::Geometry> createGeometry() override;
    
private:
    float m_majorRadius;  // 主半径
    float m_minorRadius;  // 次半径
    glm::vec3 m_axis;
}; 