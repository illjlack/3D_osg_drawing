#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 线几何体类
class Line3D_Geo : public Geo3D
{
public:
    Line3D_Geo();
    virtual ~Line3D_Geo() = default;
    
    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void keyPressEvent(QKeyEvent* event) override;
    virtual void completeDrawing() override;
    
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
    void calculateLineParameters();
    void generatePolyline();
    void generateSpline();
    void generateBezierCurve();
    
    std::vector<Point3D> m_generatedPoints;
    float m_totalLength;
}; 