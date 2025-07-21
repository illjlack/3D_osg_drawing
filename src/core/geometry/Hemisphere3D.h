#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 半球几何体类
class Hemisphere3D_Geo : public Geo3D
{
public:
    Hemisphere3D_Geo();
    virtual ~Hemisphere3D_Geo() = default;
    
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
    float m_radius;  // 半球半径
    int m_segments;  // 细分段数
}; 