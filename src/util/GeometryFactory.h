#pragma once
#pragma execution_character_set("utf-8")

#include "../core/Common3D.h"

// 前向声明
class Geo3D;
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
    static Geo3D* createGeometry(DrawMode3D mode);
    
    // 根据几何体类型创建几何体
    static Geo3D* createGeometry(GeoType3D type);
    
    // 创建具体类型的几何体
    static Point3D_Geo* createPoint();
    static Line3D_Geo* createLine();
    static Arc3D_Geo* createArc();
    static BezierCurve3D_Geo* createBezierCurve();
    static Triangle3D_Geo* createTriangle();
    static Quad3D_Geo* createQuad();
    static Polygon3D_Geo* createPolygon();
    static Box3D_Geo* createBox();
    static Cube3D_Geo* createCube();
    static Cylinder3D_Geo* createCylinder();
    static Cone3D_Geo* createCone();
    static Sphere3D_Geo* createSphere();
    static Torus3D_Geo* createTorus();
    static Prism3D_Geo* createPrism();
    static Hemisphere3D_Geo* createHemisphere();
    static Ellipsoid3D_Geo* createEllipsoid();
    static UndefinedGeo3D_Geo* createUndefinedGeo();
    
    // 辅助函数
    static DrawMode3D geoTypeToDrawMode(GeoType3D type);
    static GeoType3D drawModeToGeoType(DrawMode3D mode);
    
private:
    GeometryFactory() = delete;  // 禁止实例化
}; 

