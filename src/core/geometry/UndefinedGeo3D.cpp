#include "UndefinedGeo3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

UndefinedGeo3D_Geo::UndefinedGeo3D_Geo()
{
    m_geoType = Geo_UndefinedGeo3D;
    // 确保基类正确初始化
    initialize();
}

void UndefinedGeo3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void UndefinedGeo3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void UndefinedGeo3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
}


