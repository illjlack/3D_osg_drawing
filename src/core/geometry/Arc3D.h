#pragma once

#include "../GeometryBase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 圆弧几何体类
class Arc3D_Geo : public Geo3D
{
public:
    Arc3D_Geo();
    virtual ~Arc3D_Geo() = default;

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
    float m_radius = 0.0f;
    float m_startAngle = 0.0f;
    float m_endAngle = 0.0f;
    float m_sweepAngle = 0.0f;
    glm::vec3 m_normal = glm::vec3(0.0f);
    glm::vec3 m_center = glm::vec3(0.0f);
    glm::vec3 m_uAxis = glm::vec3(0.0f);
    glm::vec3 m_vAxis = glm::vec3(0.0f);
    std::vector<glm::vec3> m_arcPoints;
};
