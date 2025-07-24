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

    // 获取所有控制点用于绘制
    const auto& controlPointss = mm_controlPoint()->getAllStageControlPoints();

    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getVertexGeometry();
    if (!geometry.valid())
    {
        return;
    }

    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;

    // 收集所有控制点
    std::vector<glm::dvec3> allPoints;
    for (auto& points : controlPointss)
        for (auto& point : points)
        {
            allPoints.push_back(MathUtils::osgToGlm(osg::Vec3(point.x(), point.y(), point.z())));
        }

    // 根据控制点个数决定如何显示顶点
    if (allPoints.size() >= 3)
    {
        // 如果有足够的点形成圆弧，显示圆弧上的关键点
        auto arcVertices = MathUtils::generateArcPointsFromThreePoints(allPoints[0], allPoints[1], allPoints[2], 10);
        for (const auto& vertex : arcVertices)
        {
            vertices->push_back(MathUtils::glmToOsg(vertex));
        }
    }
    else
    {
        // 显示原始控制点
        for (const auto& point : allPoints)
        {
            vertices->push_back(MathUtils::glmToOsg(point));
        }
    }

    geometry->setVertexArray(vertices);
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void Arc3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();

    // 获取所有控制点
    const auto& controlPointss = mm_controlPoint()->getAllStageControlPoints();
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid())
    {
        return;
    }

    // 收集所有控制点
    std::vector<glm::dvec3> allPoints;
    for (auto& points : controlPointss)
        for (auto& point : points)
        {
            allPoints.push_back(MathUtils::osgToGlm(osg::Vec3(point.x(), point.y(), point.z())));
        }

    // 根据控制点个数决定如何绘制
    if (allPoints.size() < 2)
    {
        return; // 点数不足，无法绘制
    }

    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    if (allPoints.size() == 2)
    {
        // 只有两个点，绘制直线
        auto lineVertices = MathUtils::generateLineVertices(allPoints[0], allPoints[1]);
        for (const auto& vertex : lineVertices)
        {
            vertices->push_back(MathUtils::glmToOsg(vertex));
        }
        
        geometry->setVertexArray(vertices);
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
        geometry->addPrimitiveSet(drawArrays);
    }
    else if (allPoints.size() >= 3)
    {
        // 至少3个点，可以绘制圆弧
        if (allPoints.size() == 3)
        {
            // 精确的三点圆弧
            auto arcVertices = MathUtils::generateArcPointsFromThreePoints(allPoints[0], allPoints[1], allPoints[2], 50);
            for (const auto& vertex : arcVertices)
            {
                vertices->push_back(MathUtils::glmToOsg(vertex));
            }
        }
        else
        {
            // 多个点，绘制连续的圆弧段
            for (size_t i = 0; i < allPoints.size() - 2; ++i)
            {
                auto arcVertices = MathUtils::generateArcPointsFromThreePoints(allPoints[i], allPoints[i+1], allPoints[i+2], 20);
                for (const auto& vertex : arcVertices)
                {
                    vertices->push_back(MathUtils::glmToOsg(vertex));
                }
            }
        }
        
        geometry->setVertexArray(vertices);
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size());
        geometry->addPrimitiveSet(drawArrays);
    }
}

void Arc3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    // 圆弧没有面
}




