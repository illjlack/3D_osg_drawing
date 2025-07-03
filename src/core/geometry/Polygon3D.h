#pragma once

#include "../GeometryBase.h"

// 多边形几何体类
class Polygon3D_Geo : public Geo3D
{
public:
    Polygon3D_Geo();
    virtual ~Polygon3D_Geo() = default;
    
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
    virtual std::vector<PickingFeature> extractFaceFeatures() const override;
    virtual std::vector<PickingFeature> extractEdgeFeatures() const override;
    virtual std::vector<PickingFeature> extractVertexFeatures() const override;
    
private:
    void calculateNormal();
    void triangulatePolygon();
    
    glm::vec3 m_normal;
    std::vector<uint32_t> m_triangleIndices;
}; 