#include "LHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

LHouse3D_Geo::LHouse3D_Geo()
{
    m_geoType = Geo_LHouse3D;
    // 确保基类正确初始化
    initialize();
}

void LHouse3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void LHouse3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void LHouse3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
} 