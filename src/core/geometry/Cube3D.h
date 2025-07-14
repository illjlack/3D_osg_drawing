#pragma once

#include "../GeometryBase.h"

// 正方体几何体类
class Cube3D_Geo : public Geo3D
{
public:
    Cube3D_Geo();
    virtual ~Cube3D_Geo() = default;
    
    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void updateGeometry() override;
    
    // 几何计算
    float calculateVolume() const;
    float calculateSurfaceArea() const;
    glm::vec3 getCenter() const;
    
    // 尺寸访问
    float getSize() const;
    void setSize(float size);

protected:
    virtual osg::ref_ptr<osg::Geometry> createGeometry() override;
    
    // 点线面几何体构建
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;
    
private:
    float m_size;
}; 