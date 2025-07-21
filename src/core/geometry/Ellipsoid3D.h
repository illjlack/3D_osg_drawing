#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 椭球几何体类
class Ellipsoid3D_Geo : public Geo3D
{
public:
    Ellipsoid3D_Geo();
    virtual ~Ellipsoid3D_Geo() = default;
    
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
    glm::vec3 m_radii;  // 椭球的三个轴半径
    int m_segments;      // 细分段数
}; 