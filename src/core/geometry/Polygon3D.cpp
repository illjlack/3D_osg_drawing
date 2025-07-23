#include "Polygon3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Polygon3D_Geo::Polygon3D_Geo()
{
    m_geoType = Geo_Polygon3D;
    // 确保基类正确初始化
    initialize();
}

void Polygon3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void Polygon3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void Polygon3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
}
