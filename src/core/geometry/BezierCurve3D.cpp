#include "BezierCurve3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

BezierCurve3D_Geo::BezierCurve3D_Geo()
{
    m_geoType = Geo_BezierCurve3D;
    // 确保基类正确初始化
    initialize();
}

void BezierCurve3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void BezierCurve3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void BezierCurve3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    // 贝塞尔曲线没有面
}
