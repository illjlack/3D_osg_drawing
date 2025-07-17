#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 三角形几何体类
class Triangle3D_Geo : public Geo3D
{
public:
    Triangle3D_Geo();
    virtual ~Triangle3D_Geo() = default;

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
    float m_area = 0.0f;
    glm::vec3 m_normal = glm::vec3(0.0f);
};
