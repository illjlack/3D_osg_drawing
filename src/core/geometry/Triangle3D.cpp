#include "Triangle3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"
#include "../../util/VertexShapeUtils.h"

Triangle3D_Geo::Triangle3D_Geo()
{
    m_geoType = Geo_Triangle3D;
    // 确保基类正确初始化
    initialize();
    
    // 平面几何特定的可见性设置：显示线面
    GeoParameters3D params = getParameters();
    params.showPoints = false;
    params.showEdges = true;
    params.showFaces = true;

    setParameters(params);
}

void Triangle3D_Geo::buildVertexGeometries()
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

    if (vertices->size() > 0)
    {
        // 从参数中获取点的显示属性
        const GeoParameters3D& params = getParameters();
        PointShape3D pointShape = params.pointShape;
        double pointSize = params.pointSize;

        // 使用顶点形状工具创建几何体
        osg::ref_ptr<osg::Geometry> shapeGeometry = 
            VertexShapeUtils::createVertexShapeGeometry(vertices, pointShape, pointSize);
        
        if (shapeGeometry.valid())
        {
            // 复制生成的几何体数据到现有几何体
            geometry->setVertexArray(shapeGeometry->getVertexArray());
            
            // 复制图元集合
            geometry->removePrimitiveSet(0, geometry->getNumPrimitiveSets());
            for (unsigned int i = 0; i < shapeGeometry->getNumPrimitiveSets(); ++i)
            {
                geometry->addPrimitiveSet(shapeGeometry->getPrimitiveSet(i));
            }
            
            // 复制渲染状态
            if (shapeGeometry->getStateSet())
            {
                geometry->setStateSet(shapeGeometry->getStateSet());
            }
        }
    }
}

void Triangle3D_Geo::buildEdgeGeometries()
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

    // 需要至少3个点才能绘制三角形的边
    if (allPoints.size() < 3)
    {
        // 不足3个点，绘制已有的线段
        if (allPoints.size() == 2)
        {
            osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
            auto lineVertices = MathUtils::generateLineVertices(allPoints[0], allPoints[1]);
            for (const auto& vertex : lineVertices)
            {
                vertices->push_back(MathUtils::glmToOsg(vertex));
            }
            
            geometry->setVertexArray(vertices);
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
            geometry->addPrimitiveSet(drawArrays);
        }
        return;
    }

    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 绘制三角形的三条边
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

void Triangle3D_Geo::buildFaceGeometries()
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

    // 需要3个点才能绘制三角形面
    if (allPoints.size() < 3)
    {
        return;
    }

    // 创建顶点数组和法向量数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    // 生成三角形顶点和法向量
    glm::dvec3 normal;
    auto triangleVertices = MathUtils::generateTriangleVertices(allPoints[0], allPoints[1], allPoints[2], normal);
    
    for (const auto& vertex : triangleVertices)
    {
        vertices->push_back(MathUtils::glmToOsg(vertex));
        normals->push_back(MathUtils::glmToOsg(normal));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}




