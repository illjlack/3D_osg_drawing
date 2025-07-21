#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// L型房屋几何体类
class LHouse3D_Geo : public Geo3D
{
public:
    LHouse3D_Geo();
    virtual ~LHouse3D_Geo() = default;
    
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
    glm::vec3 m_mainSize;    // 主体尺寸
    glm::vec3 m_wingSize;    // 翼部尺寸
    float m_height;          // 房屋高度
}; 