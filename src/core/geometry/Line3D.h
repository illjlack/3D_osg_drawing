#pragma once

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
    virtual void updateGeometry() override;

    // 拾取Feature支持
    virtual std::vector<FeatureType> getSupportedFeatureTypes() const override;

protected:
    virtual osg::ref_ptr<osg::Geometry> createGeometry() override;
    
    // Feature抽取
    virtual std::vector<PickingFeature> extractEdgeFeatures() const override;
    virtual std::vector<PickingFeature> extractVertexFeatures() const override;
    
private:
    void generatePolyline();
    void generateSpline();
    void generateBezierCurve();
    
    std::vector<Point3D> m_generatedPoints;
}; 