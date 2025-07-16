#pragma once

#include "../GeometryBase.h"

// 球体几何体类
class Sphere3D_Geo : public Geo3D
{
public:
    Sphere3D_Geo();
    virtual ~Sphere3D_Geo() = default;

    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;

protected:
    virtual void buildVertexGeometries();
    virtual void buildEdgeGeometries();
    virtual void buildFaceGeometries();

private:
    float m_radius = 1.0f;
    int m_segments = 16;
}; 