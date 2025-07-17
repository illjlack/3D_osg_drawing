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
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;
    
    // 绘制完成检查和控制点验证
    virtual bool isDrawingComplete() const override;
    virtual bool areControlPointsValid() const override;

private:
    float m_radius = 1.0f;
    int m_segments = 16;
};
