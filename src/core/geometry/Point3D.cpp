#include "Point3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Point3D_Geo::Point3D_Geo()
{
    m_geoType = Geo_Point3D;
    // 确保基类正确初始化
    initialize();
}

void Point3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void Point3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    // 点没有边
}

void Point3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    // 点没有面
}
