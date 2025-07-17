#pragma once

#include "../GeometryBase.h"

// 立方体几何体类
class Cube3D_Geo : public Geo3D
{
public:
    Cube3D_Geo();
    virtual ~Cube3D_Geo() = default;
    
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;
    
private:
    float m_size = 1.0f;
};
