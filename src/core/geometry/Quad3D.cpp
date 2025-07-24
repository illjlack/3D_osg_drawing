#include "Quad3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Quad3D_Geo::Quad3D_Geo()
{
    m_geoType = Geo_Quad3D;
    // 确保基类正确初始化
    initialize();
}

void Quad3D_Geo::buildVertexGeometries()
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

    // 收集所有控制点
    for (auto& points : controlPointss)
        for (auto& point : points)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        }

    geometry->setVertexArray(vertices);
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void Quad3D_Geo::buildEdgeGeometries()
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

    // 根据点数绘制不同的边
    if (allPoints.size() < 2)
    {
        return;
    }

    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    if (allPoints.size() == 2)
    {
        // 两点连线
        auto lineVertices = MathUtils::generateLineVertices(allPoints[0], allPoints[1]);
        for (const auto& vertex : lineVertices)
        {
            vertices->push_back(MathUtils::glmToOsg(vertex));
        }
        geometry->setVertexArray(vertices);
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
        geometry->addPrimitiveSet(drawArrays);
    }
    else if (allPoints.size() == 3)
    {
        // 三点组成三角形的边
        vertices->push_back(MathUtils::glmToOsg(allPoints[0]));
        vertices->push_back(MathUtils::glmToOsg(allPoints[1]));
        vertices->push_back(MathUtils::glmToOsg(allPoints[1]));
        vertices->push_back(MathUtils::glmToOsg(allPoints[2]));
        vertices->push_back(MathUtils::glmToOsg(allPoints[2]));
        vertices->push_back(MathUtils::glmToOsg(allPoints[0]));
        
        geometry->setVertexArray(vertices);
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
        geometry->addPrimitiveSet(drawArrays);
    }
    else if (allPoints.size() >= 4)
    {
        // 四点组成四边形的边
        vertices->push_back(MathUtils::glmToOsg(allPoints[0]));
        vertices->push_back(MathUtils::glmToOsg(allPoints[1]));
        vertices->push_back(MathUtils::glmToOsg(allPoints[1]));
        vertices->push_back(MathUtils::glmToOsg(allPoints[2]));
        vertices->push_back(MathUtils::glmToOsg(allPoints[2]));
        vertices->push_back(MathUtils::glmToOsg(allPoints[3]));
        vertices->push_back(MathUtils::glmToOsg(allPoints[3]));
        vertices->push_back(MathUtils::glmToOsg(allPoints[0]));
        
        geometry->setVertexArray(vertices);
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
        geometry->addPrimitiveSet(drawArrays);
    }
}

void Quad3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();

    // 获取所有控制点
    const auto& controlPointss = mm_controlPoint()->getAllStageControlPoints();
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getFaceGeometry();
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

    // 需要至少3个点才能绘制面
    if (allPoints.size() < 3)
    {
        return;
    }

    // 创建顶点数组和法向量数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    if (allPoints.size() == 3)
    {
        // 三点组成三角形
        glm::dvec3 normal;
        auto triangleVertices = MathUtils::generateTriangleVertices(allPoints[0], allPoints[1], allPoints[2], normal);
        
        for (const auto& vertex : triangleVertices)
        {
            vertices->push_back(MathUtils::glmToOsg(vertex));
            normals->push_back(MathUtils::glmToOsg(normal));
        }
    }
    else if (allPoints.size() >= 4)
    {
        // 四点组成四边形（分解为两个三角形）
        std::vector<glm::dvec3> quadNormals;
        auto quadVertices = MathUtils::generateQuadVertices(allPoints[0], allPoints[1], allPoints[2], allPoints[3], quadNormals);
        
        for (size_t i = 0; i < quadVertices.size(); ++i)
        {
            vertices->push_back(MathUtils::glmToOsg(quadVertices[i]));
            normals->push_back(MathUtils::glmToOsg(quadNormals[i]));
        }
    }
    
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
} 



