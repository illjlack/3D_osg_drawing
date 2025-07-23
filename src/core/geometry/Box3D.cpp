#include "Box3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Box3D_Geo::Box3D_Geo()
{
    m_geoType = Geo_Box3D;
    // 确保基类正确初始化
    initialize();
}

void Box3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void Box3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void Box3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
}

