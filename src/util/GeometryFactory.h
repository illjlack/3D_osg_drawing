#pragma once
#pragma execution_character_set("utf-8")

#include "../core/Common3D.h"
#include "../core/GeometryBase.h"

// 前向声明
class Point3D_Geo;
class Line3D_Geo;
class Arc3D_Geo;
class BezierCurve3D_Geo;
class Triangle3D_Geo;
class Quad3D_Geo;
class Polygon3D_Geo;
class Box3D_Geo;
class Cube3D_Geo;
class Cylinder3D_Geo;
class Cone3D_Geo;
class Sphere3D_Geo;
class Torus3D_Geo;
class Prism3D_Geo;
class Hemisphere3D_Geo;
class Ellipsoid3D_Geo;
class UndefinedGeo3D_Geo;

// 几何体工厂类
class GeometryFactory
{
public:
    // 根据绘制模式创建几何体
    static Geo3D::Ptr createGeometry(DrawMode3D mode);
    
    // 根据几何体类型创建几何体
    static Geo3D::Ptr createGeometry(GeoType3D type);
    
    // 创建具体类型的几何体
    static Geo3D::Ptr createPoint();
    static Geo3D::Ptr createLine();
    static Geo3D::Ptr createArc();
    static Geo3D::Ptr createBezierCurve();
    static Geo3D::Ptr createTriangle();
    static Geo3D::Ptr createQuad();
    static Geo3D::Ptr createPolygon();
    static Geo3D::Ptr createBox();
    static Geo3D::Ptr createCube();
    static Geo3D::Ptr createCylinder();
    static Geo3D::Ptr createCone();
    static Geo3D::Ptr createSphere();
    static Geo3D::Ptr createTorus();
    static Geo3D::Ptr createPrism();
    static Geo3D::Ptr createHemisphere();
    static Geo3D::Ptr createEllipsoid();
    static Geo3D::Ptr createUndefinedGeo();
    
    // 辅助函数
    static DrawMode3D geoTypeToDrawMode(GeoType3D type);
    static GeoType3D drawModeToGeoType(DrawMode3D mode);
    
private:
    GeometryFactory() = delete;  // 禁止实例化
}; 

