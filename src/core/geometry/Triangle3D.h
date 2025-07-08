#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 三角形几何体类
class Triangle3D_Geo : public RegularGeo3D
{
public:
    Triangle3D_Geo();
    virtual ~Triangle3D_Geo() = default;
    
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
    virtual std::vector<PickingFeature> extractEdgeFeatures() const override;
    virtual std::vector<PickingFeature> extractVertexFeatures() const override;
    
private:
    void calculateNormal();
    
    glm::vec3 m_normal;
}; 