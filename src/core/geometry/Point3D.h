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
    virtual void completeDrawing() override;
    virtual void updateGeometry() override;

protected:
    virtual osg::ref_ptr<osg::Geometry> createGeometry() override;
    
    // 点线面几何体构建
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;
    
private:
    osg::ref_ptr<osg::Geometry> createPointGeometry(PointShape3D shape, float size);
}; 