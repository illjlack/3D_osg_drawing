#include "Sphere3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Sphere3D_Geo::Sphere3D_Geo()
{
    m_geoType = Geo_Sphere3D;
    // 确保基类正确初始化
    initialize();
}

void Sphere3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void Sphere3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void Sphere3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
}


