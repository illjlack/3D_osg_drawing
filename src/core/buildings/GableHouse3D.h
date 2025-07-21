#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 人字形房屋几何体类
class GableHouse3D_Geo : public Geo3D
{
public:
    GableHouse3D_Geo();
    virtual ~GableHouse3D_Geo() = default;
    
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
    float m_roofHeight;     // 屋顶高度
    float m_roofAngle;      // 屋顶角度
}; 