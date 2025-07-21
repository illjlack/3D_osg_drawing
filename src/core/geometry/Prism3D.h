#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 多棱柱几何体类
class Prism3D_Geo : public Geo3D
{
public:
    Prism3D_Geo();
    virtual ~Prism3D_Geo() = default;
    
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
    int m_sides;  // 棱柱的边数
    float m_height;  // 棱柱的高度
    float m_radius;  // 底面半径
}; 