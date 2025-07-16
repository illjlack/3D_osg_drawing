#pragma once

#include "../GeometryBase.h"

// 长方体几何体类
class Box3D_Geo : public Geo3D
{
public:
    Box3D_Geo();
    virtual ~Box3D_Geo() = default;
    
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;

protected:
    virtual void buildVertexGeometries();
    virtual void buildEdgeGeometries();
    virtual void buildFaceGeometries();
    
private:
    glm::vec3 m_size;
}; 