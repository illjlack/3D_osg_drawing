#include "Cone3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Cone3D_Geo::Cone3D_Geo()
{
    m_geoType = Geo_Cone3D;
    // 确保基类正确初始化
    initialize();
}

void Cone3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void Cone3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void Cone3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
}
