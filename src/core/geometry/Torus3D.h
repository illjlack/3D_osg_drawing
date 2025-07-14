#pragma once

#include "../GeometryBase.h"

// 圆环几何体类
class Torus3D_Geo : public Geo3D
{
public:
    Torus3D_Geo();
    virtual ~Torus3D_Geo() = default;
    
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
    float m_majorRadius;  // 主半径
    float m_minorRadius;  // 次半径
    glm::vec3 m_axis;
}; 