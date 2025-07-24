#include "Ellipsoid3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Ellipsoid3D_Geo::Ellipsoid3D_Geo()
{
    m_geoType = Geo_Ellipsoid3D;
    // 确保基类正确初始化
    initialize();
}

void Ellipsoid3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void Ellipsoid3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void Ellipsoid3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
} 

