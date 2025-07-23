#include "FlatHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

FlatHouse3D_Geo::FlatHouse3D_Geo()
{
    m_geoType = Geo_FlatHouse3D;
    // 确保基类正确初始化
    initialize();
}

void FlatHouse3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void FlatHouse3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void FlatHouse3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
} 