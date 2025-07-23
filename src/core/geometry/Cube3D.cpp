#include "Cube3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Cube3D_Geo::Cube3D_Geo()
{
    m_geoType = Geo_Cube3D;
    // 确保基类正确初始化
    initialize();
}

void Cube3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void Cube3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void Cube3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
}
