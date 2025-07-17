#pragma once

#include "../GeometryBase.h"

// 多边形几何体类
class Polygon3D_Geo : public Geo3D
{
public:
    Polygon3D_Geo();
    virtual ~Polygon3D_Geo() = default;

    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void keyPressEvent(QKeyEvent* event) override;

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    glm::vec3 m_normal = glm::vec3(0.0f);
    std::vector<unsigned int> m_triangleIndices;
};
