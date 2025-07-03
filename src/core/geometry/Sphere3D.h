#pragma once

#include "../GeometryBase.h"

// 球几何体类
class Sphere3D_Geo : public RegularGeo3D
{
public:
    Sphere3D_Geo();
    virtual ~Sphere3D_Geo() = default;
    
    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void updateGeometry() override;

    // 拾取Feature支持
    virtual std::vector<FeatureType> getSupportedFeatureTypes() const override;

protected:
    virtual osg::ref_ptr<osg::Geometry> createGeometry() override;
    
    // Feature抽取
    virtual std::vector<PickingFeature> extractFaceFeatures() const override;
    virtual std::vector<PickingFeature> extractVertexFeatures() const override;
    
private:
    float m_radius;
}; 