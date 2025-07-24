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
    // 不用绘制顶点
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

    // 从参数获取细分级别
    int subdivisionLevel = static_cast<int>(m_parameters.subdivisionLevel);

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
            // 精确的三点圆弧，使用细分级别参数
            auto arcVertices = MathUtils::generateArcPointsFromThreePoints(allPoints[0], allPoints[1], allPoints[2], subdivisionLevel);
            for (const auto& vertex : arcVertices)
            {
                vertices->push_back(MathUtils::glmToOsg(vertex));
            }
        }
        else
        {
            // 多个点，绘制连续的平滑圆弧段
            // 第一段：使用前三个控制点
            auto firstArcVertices = MathUtils::generateArcPointsFromThreePoints(allPoints[0], allPoints[1], allPoints[2], subdivisionLevel);
            
            // 添加第一段的所有顶点
            for (const auto& vertex : firstArcVertices)
            {
                vertices->push_back(MathUtils::glmToOsg(vertex));
            }
            
            for (size_t i = 3; i < allPoints.size(); ++i)
            {
                // 获取上一段的最后一个点（作为起始点）
                assert(vertices->size() > 2);
                glm::dvec3 lastPoint1 = MathUtils::osgToGlm((*vertices)[vertices->size() - 2]);
                glm::dvec3 lastPoint2 = MathUtils::osgToGlm((*vertices)[vertices->size() - 1]);
                auto arcVertices = MathUtils::generateArcPointsFromThreePoints(lastPoint1, lastPoint2, allPoints[i], subdivisionLevel);
                
                // 跳过第一个点（避免重复），添加其余顶点
                for (size_t j = 1; j < arcVertices.size(); ++j)
                {
                    vertices->push_back(MathUtils::glmToOsg(arcVertices[j]));
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




