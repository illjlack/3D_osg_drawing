#pragma once

#include "../GeometryBase.h"

// 贝塞尔曲线几何体类
class BezierCurve3D_Geo : public Geo3D
{
public:
    BezierCurve3D_Geo();
    virtual ~BezierCurve3D_Geo() = default;
    
    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void keyPressEvent(QKeyEvent* event) override;
    
    // 几何计算
    float calculateLength() const;
    
    // 拾取测试
    virtual bool hitTest(const Ray3D& ray, PickResult3D& result) const;

protected:
    virtual osg::ref_ptr<osg::Geometry> createGeometry();
    
    // 点线面几何体构建
    virtual void buildVertexGeometries();
    virtual void buildEdgeGeometries();
    virtual void buildFaceGeometries();
    
private:
    std::vector<glm::vec3> m_bezierPoints;
    void calculateBezierCurveParameters();
    void generateBezierCurvePoints();
}; 