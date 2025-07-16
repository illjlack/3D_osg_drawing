#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 未定义几何体类
class UndefinedGeo3D : public Geo3D
{
public:
    UndefinedGeo3D();
    virtual ~UndefinedGeo3D() = default;
    
    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    
    // 几何计算
    float calculateVolume() const;
    float calculateSurfaceArea() const;
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
    void calculateUndefinedParameters();
}; 