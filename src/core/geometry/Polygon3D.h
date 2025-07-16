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
    glm::vec3 m_normal = glm::vec3(0.0f);
    std::vector<unsigned int> m_triangleIndices;
    void calculatePolygonParameters();
    void triangulatePolygon();
}; 