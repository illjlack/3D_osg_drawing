#include "SpireHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

SpireHouse3D_Geo::SpireHouse3D_Geo()
{
    m_geoType = Geo_SpireHouse3D;
    // 确保基类正确初始化
    initialize();
}

void SpireHouse3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void SpireHouse3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void SpireHouse3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
} 