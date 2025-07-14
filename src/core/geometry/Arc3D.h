#pragma once

#include "../GeometryBase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 圆弧几何体类
class Arc3D_Geo : public Geo3D
{
public:
    Arc3D_Geo();
    virtual ~Arc3D_Geo() = default;
    
    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void updateGeometry() override;
    
    // 拾取测试
    virtual bool hitTest(const Ray3D& ray, PickResult3D& result) const override;

protected:
    virtual osg::ref_ptr<osg::Geometry> createGeometry() override;
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;
    
private:
    void calculateArcFromThreePoints();
    void calculateArcParameters();
    void generateArcPoints();
    
    glm::vec3 m_center;
    float m_radius;
    float m_startAngle;
    float m_endAngle;
    float m_sweepAngle;
    glm::vec3 m_normal;
    glm::vec3 m_uAxis;
    glm::vec3 m_vAxis;
    std::vector<Point3D> m_arcPoints;
}; 