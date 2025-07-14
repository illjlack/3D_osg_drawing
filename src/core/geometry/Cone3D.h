#pragma once

#include "../GeometryBase.h"

// 圆锥体几何体类
class Cone3D_Geo : public Geo3D
{
public:
    Cone3D_Geo();
    virtual ~Cone3D_Geo() = default;
    
    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void updateGeometry() override;
    
    // 拾取测试
    virtual bool hitTest(const Ray3D& ray, PickResult3D& result) const override;

protected:
    virtual osg::ref_ptr<osg::Geometry> createGeometry() override;
    
    // 点线面几何体构建
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;
    
private:
    float m_radius;
    float m_height;
    int m_segments;
    glm::vec3 m_axis;  // 圆锥轴方向
}; 