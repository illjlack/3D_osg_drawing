#pragma once

#include "../GeometryBase.h"

// 圆柱体几何体类
class Cylinder3D_Geo : public Geo3D
{
public:
    Cylinder3D_Geo();
    virtual ~Cylinder3D_Geo() = default;
    
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;
    
private:
    float m_radius = 1.0f;
    float m_height = 1.0f;
    int m_segments = 16;
    glm::vec3 m_axis = glm::vec3(0.0f, 0.0f, 1.0f);
};
