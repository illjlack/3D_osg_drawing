#include "Line3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"
#include "../../util/VertexShapeUtils.h"

Line3D_Geo::Line3D_Geo()
{
    m_geoType = Geo_Line3D;
    // 确保基类正确初始化
    initialize();
    
    // 线类几何特定的可见性设置：只显示线
    GeoParameters3D params = getParameters();
    params.showPoints = false;
    params.showEdges = true;
    params.showFaces = false;
    
    // 更新渲染参数
    if (mm_render()) {
        mm_render()->updateRenderingParameters(params);
    }
    // 同步参数到基类
    setParameters(params);
}

void Line3D_Geo::buildVertexGeometries()
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

void Line3D_Geo::buildEdgeGeometries()
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

    // 需要至少2个点才能绘制线
    if (allPoints.size() < 2)
    {
        return;
    }

    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    if (allPoints.size() == 2)
    {
        // 两点之间的直线
        auto lineVertices = MathUtils::generateLineVertices(allPoints[0], allPoints[1]);
        for (const auto& vertex : lineVertices)
        {
            vertices->push_back(MathUtils::glmToOsg(vertex));
        }
        
        geometry->setVertexArray(vertices);
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
        geometry->addPrimitiveSet(drawArrays);
    }
    else
    {
        // 多点连线
        for (const auto& point : allPoints)
        {
            vertices->push_back(MathUtils::glmToOsg(point));
        }
        
        geometry->setVertexArray(vertices);
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size());
        geometry->addPrimitiveSet(drawArrays);
    }
}

void Line3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    // 线没有面
}



