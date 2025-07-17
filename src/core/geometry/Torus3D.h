#pragma once

#include "../GeometryBase.h"

// 圆环体几何体类
class Torus3D_Geo : public Geo3D
{
public:
    Torus3D_Geo();
    virtual ~Torus3D_Geo() = default;

    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;
    
private:
    float m_majorRadius = 1.0f;
    float m_minorRadius = 0.3f;
    glm::vec3 m_axis = glm::vec3(0.0f, 0.0f, 1.0f);
};
