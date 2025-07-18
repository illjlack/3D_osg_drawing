﻿#include "GeometryFactory.h"
#include "../core/GeometryBase.h"
#include "../core/geometry/Point3D.h"
#include "../core/geometry/Line3D.h"
#include "../core/geometry/Triangle3D.h"
#include "../core/geometry/Quad3D.h"
#include "../core/geometry/Polygon3D.h"
#include "../core/geometry/Arc3D.h"
#include "../core/geometry/BezierCurve3D.h"
#include "../core/geometry/Box3D.h"
#include "../core/geometry/Cube3D.h"
#include "../core/geometry/Cylinder3D.h"
#include "../core/geometry/Cone3D.h"
#include "../core/geometry/Sphere3D.h"
#include "../core/geometry/Torus3D.h"
#include "../core/geometry/UndefinedGeo3D.h"

Geo3D* GeometryFactory::createGeometry(DrawMode3D mode)
{
    switch (mode)
    {
    case DrawPoint3D:
        return createPoint();
    case DrawLine3D:
        return createLine();
    case DrawArc3D:
    case DrawThreePointArc3D:
        return createArc();
    case DrawBezierCurve3D:
        return createBezierCurve();
    case DrawTriangle3D:
        return createTriangle();
    case DrawQuad3D:
        return createQuad();
    case DrawPolygon3D:
        return createPolygon();
    case DrawBox3D:
        return createBox();
    case DrawCube3D:
        return createCube();
    case DrawCylinder3D:
        return createCylinder();
    case DrawCone3D:
        return createCone();
    case DrawSphere3D:
        return createSphere();
    case DrawTorus3D:
        return createTorus();
    default:
        return createUndefinedGeo();
    }
}

Geo3D* GeometryFactory::createGeometry(GeoType3D type)
{
    return createGeometry(geoTypeToDrawMode(type));
}

Point3D_Geo* GeometryFactory::createPoint()
{
    return new Point3D_Geo();
}

Line3D_Geo* GeometryFactory::createLine()
{
    return new Line3D_Geo();
}

Arc3D_Geo* GeometryFactory::createArc()
{
    return new Arc3D_Geo();
}

BezierCurve3D_Geo* GeometryFactory::createBezierCurve()
{
    return new BezierCurve3D_Geo();
}

Triangle3D_Geo* GeometryFactory::createTriangle()
{
    return new Triangle3D_Geo();
}

Quad3D_Geo* GeometryFactory::createQuad()
{
    return new Quad3D_Geo();
}

Polygon3D_Geo* GeometryFactory::createPolygon()
{
    return new Polygon3D_Geo();
}

Box3D_Geo* GeometryFactory::createBox()
{
    return new Box3D_Geo();
}

Cube3D_Geo* GeometryFactory::createCube()
{
    return new Cube3D_Geo();
}

Cylinder3D_Geo* GeometryFactory::createCylinder()
{
    return new Cylinder3D_Geo();
}

Cone3D_Geo* GeometryFactory::createCone()
{
    return new Cone3D_Geo();
}

Sphere3D_Geo* GeometryFactory::createSphere()
{
    return new Sphere3D_Geo();
}

Torus3D_Geo* GeometryFactory::createTorus()
{
    return new Torus3D_Geo();
}

UndefinedGeo3D* GeometryFactory::createUndefinedGeo()
{
    return new UndefinedGeo3D();
}

DrawMode3D GeometryFactory::geoTypeToDrawMode(GeoType3D type)
{
    switch (type)
    {
    case Geo_Point3D: return DrawPoint3D;
    case Geo_Line3D: return DrawLine3D;
    case Geo_Arc3D: return DrawArc3D;
    case Geo_BezierCurve3D: return DrawBezierCurve3D;
    case Geo_Triangle3D: return DrawTriangle3D;
    case Geo_Quad3D: return DrawQuad3D;
    case Geo_Polygon3D: return DrawPolygon3D;
    case Geo_Box3D: return DrawBox3D;
    case Geo_Cube3D: return DrawCube3D;
    case Geo_Cylinder3D: return DrawCylinder3D;
    case Geo_Cone3D: return DrawCone3D;
    case Geo_Sphere3D: return DrawSphere3D;
    case Geo_Torus3D: return DrawTorus3D;
    case Geo_UndefinedGeo3D: return DrawSelect3D;
    default: return DrawSelect3D;
    }
}

GeoType3D GeometryFactory::drawModeToGeoType(DrawMode3D mode)
{
    switch (mode)
    {
    case DrawPoint3D: return Geo_Point3D;
    case DrawLine3D: return Geo_Line3D;
    case DrawArc3D:
    case DrawThreePointArc3D: return Geo_Arc3D;
    case DrawBezierCurve3D: return Geo_BezierCurve3D;
    case DrawTriangle3D: return Geo_Triangle3D;
    case DrawQuad3D: return Geo_Quad3D;
    case DrawPolygon3D: return Geo_Polygon3D;
    case DrawBox3D: return Geo_Box3D;
    case DrawCube3D: return Geo_Cube3D;
    case DrawCylinder3D: return Geo_Cylinder3D;
    case DrawCone3D: return Geo_Cone3D;
    case DrawSphere3D: return Geo_Sphere3D;
    case DrawTorus3D: return Geo_Torus3D;
    default: return Geo_UndefinedGeo3D;
    }
} 