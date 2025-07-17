#pragma once

#include "../GeometryBase.h"

// 贝塞尔曲线几何体类
class BezierCurve3D_Geo : public Geo3D
{
public:
    BezierCurve3D_Geo();
    virtual ~BezierCurve3D_Geo() = default;
    
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void keyPressEvent(QKeyEvent* event) override;
    
protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;
    
private:
    std::vector<glm::vec3> m_bezierPoints;
};
