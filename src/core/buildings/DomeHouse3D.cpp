#include "DomeHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

DomeHouse3D_Geo::DomeHouse3D_Geo()
{
    m_geoType = Geo_DomeHouse3D;
    // 确保基类正确初始化
    initialize();
}

void DomeHouse3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void DomeHouse3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void DomeHouse3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
} 

