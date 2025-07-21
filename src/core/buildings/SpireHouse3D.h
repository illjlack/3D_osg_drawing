#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 尖顶房屋几何体类
class SpireHouse3D_Geo : public Geo3D
{
public:
    SpireHouse3D_Geo();
    virtual ~SpireHouse3D_Geo() = default;
    
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
    glm::vec3 m_size;      // 房屋尺寸
    float m_spireHeight;    // 尖顶高度
    float m_spireRadius;    // 尖顶半径
}; 