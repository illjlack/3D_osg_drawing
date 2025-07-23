#include "Line3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Line3D_Geo::Line3D_Geo()
{
    m_geoType = Geo_Line3D;
    // 确保基类正确初始化
    initialize();
}

void Line3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void Line3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void Line3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    // 线没有面
}
