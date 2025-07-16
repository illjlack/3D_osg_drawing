#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 点几何体类
class Point3D_Geo : public Geo3D
{
public:
    Point3D_Geo();
    virtual ~Point3D_Geo() = default;
    
    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    
    // 几何计算
    float calculateVolume() const;
    float calculateSurfaceArea() const;
    glm::vec3 getCenter() const;
    
    // 拾取测试
    virtual bool hitTest(const Ray3D& ray, PickResult3D& result) const override;

protected:
    virtual osg::ref_ptr<osg::Geometry> createGeometry();
    
    // 点线面几何体构建
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;
    
private:
    void calculatePointParameters();
}; 