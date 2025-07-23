#include "GableHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

GableHouse3D_Geo::GableHouse3D_Geo()
{
    m_geoType = Geo_GableHouse3D;
    // 确保基类正确初始化
    initialize();
}

void GableHouse3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void GableHouse3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void GableHouse3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
} 