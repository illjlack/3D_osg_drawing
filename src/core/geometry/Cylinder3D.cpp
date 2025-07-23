#include "Cylinder3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Cylinder3D_Geo::Cylinder3D_Geo()
{
    m_geoType = Geo_Cylinder3D;
    // 确保基类正确初始化
    initialize();
}

void Cylinder3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void Cylinder3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void Cylinder3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
} 