#include "Quad3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Quad3D_Geo::Quad3D_Geo()
{
    m_geoType = Geo_Quad3D;
    // 确保基类正确初始化
    initialize();
}

void Quad3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void Quad3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void Quad3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
} 
