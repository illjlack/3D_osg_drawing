#include "Point3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "util/MathUtils.h"
#include "util/VertexShapeUtils.h"

Point3D_Geo::Point3D_Geo()
{
    m_geoType = Geo_Point3D;
    // 确保基类正确初始化
    initialize();
    
    // 点几何特定的可见性设置：只显示点
    GeoParameters3D params = getParameters();
    params.showPoints = true;
    params.showEdges = false;
    params.showFaces = false;

    setParameters(params);
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


