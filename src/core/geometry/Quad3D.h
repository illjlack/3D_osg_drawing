#pragma once

#include "../GeometryBase.h"

// 四边形几何体类
class Quad3D_Geo : public Geo3D
{
public:
    Quad3D_Geo();
    virtual ~Quad3D_Geo() = default;
    
    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void updateGeometry() override;
    
    // 几何计算
    float calculateArea() const;

protected:
    virtual osg::ref_ptr<osg::Geometry> createGeometry() override;
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;
    
private:
    void calculateNormal();
    
    glm::vec3 m_normal;
    float m_area;
}; 