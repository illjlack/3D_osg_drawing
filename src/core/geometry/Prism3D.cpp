#include "Prism3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Prism3D_Geo::Prism3D_Geo()
{
    m_geoType = Geo_Prism3D;
    // 确保基类正确初始化
    initialize();
}

void Prism3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void Prism3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void Prism3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
} 