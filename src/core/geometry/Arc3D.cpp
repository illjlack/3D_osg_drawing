#include "Arc3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Arc3D_Geo::Arc3D_Geo()
{
    m_geoType = Geo_Arc3D;
    // 确保基类正确初始化
    initialize();
}

void Arc3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void Arc3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void Arc3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    // 圆弧没有面
}

