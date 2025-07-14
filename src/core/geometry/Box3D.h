#pragma once

#include "../GeometryBase.h"

// 长方体几何体类
class Box3D_Geo : public Geo3D
{
public:
    Box3D_Geo();
    virtual ~Box3D_Geo() = default;
    
    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void updateGeometry() override;
    
    // 几何计算
    float calculateVolume() const;
    float calculateSurfaceArea() const;
    glm::vec3 getCenter() const;
    
    // 尺寸访问
    glm::vec3 getSize() const;
    void setSize(const glm::vec3& size);
    
    // 拾取测试
    virtual bool hitTest(const Ray3D& ray, PickResult3D& result) const override;

protected:
    virtual osg::ref_ptr<osg::Geometry> createGeometry() override;
    
    // 点线面几何体构建
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;
    
private:
    glm::vec3 m_size;
}; 