#pragma once

#include "../GeometryBase.h"

// 圆锥体几何体类
class Cone3D_Geo : public Geo3D
{
public:
    Cone3D_Geo();
    virtual ~Cone3D_Geo() = default;
    
    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    

protected:
    // 点线面几何体构建
    virtual void buildVertexGeometries();
    virtual void buildEdgeGeometries();
    virtual void buildFaceGeometries();
    
private:
    float m_radius = 1.0f;
    float m_height = 1.0f;
    int m_segments = 16;
    glm::vec3 m_axis = glm::vec3(0.0f, 0.0f, 1.0f);
}; 