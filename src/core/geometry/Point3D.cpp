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
    
    // 获取所有控制点
    const auto& controlPointss = mm_controlPoint()->getAllStageControlPoints();
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getVertexGeometry();
    if (!geometry.valid())
    {
        return;
    }

    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;

    // 收集所有控制点 - 点几何体直接显示所有控制点
    for (auto& points : controlPointss)
        for (auto& point : points)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        }

    if (vertices->size() > 0)
    {
        geometry->setVertexArray(vertices);
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
        geometry->addPrimitiveSet(drawArrays);
    }
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


