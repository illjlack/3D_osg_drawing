#include "Triangle3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Triangle3D_Geo::Triangle3D_Geo()
{
    m_geoType = Geo_Triangle3D;
    // 确保基类正确初始化
    initialize();
}

void Triangle3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void Triangle3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void Triangle3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
}
