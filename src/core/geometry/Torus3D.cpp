#include "Torus3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Torus3D_Geo::Torus3D_Geo()
{
    m_geoType = Geo_Torus3D;
    // 确保基类正确初始化
    initialize();
}

void Torus3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
}

void Torus3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void Torus3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
}
