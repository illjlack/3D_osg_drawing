#pragma once

#include "../GeometryBase.h"

// 四边形几何体类
class Quad3D_Geo : public Geo3D
{
public:
    Quad3D_Geo();
    virtual ~Quad3D_Geo() = default;
    
    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    
    // 几何计算
    float calculateArea() const;
    glm::vec3 getCenter() const;
    
    // 拾取测试
    virtual bool hitTest(const Ray3D& ray, PickResult3D& result) const;

protected:
    virtual osg::ref_ptr<osg::Geometry> createGeometry();
    
    // 点线面几何体构建
    virtual void buildVertexGeometries();
    virtual void buildEdgeGeometries();
    virtual void buildFaceGeometries();
    
private:
    float m_area = 0.0f;
    glm::vec3 m_normal = glm::vec3(0.0f);
    void calculateQuadParameters();
}; 