#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 未定义几何体类
class UndefinedGeo3D : public Geo3D
{
public:
    UndefinedGeo3D();
    virtual ~UndefinedGeo3D() = default;

    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    glm::vec3 m_normal = glm::vec3(0.0f);
}; 