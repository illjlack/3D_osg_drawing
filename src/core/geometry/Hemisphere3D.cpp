#include "Hemisphere3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Hemisphere3D_Geo::Hemisphere3D_Geo()
{
    m_geoType = Geo_Hemisphere3D;
    // 确保基类正确初始化
    initialize();
}

void Hemisphere3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
}

void Hemisphere3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
}

void Hemisphere3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
} 

