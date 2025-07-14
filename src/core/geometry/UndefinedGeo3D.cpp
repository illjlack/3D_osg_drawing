#include "UndefinedGeo3D.h"
#include "../Common3D.h"
#include "../../util/LogManager.h"
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/Material>
#include <osg/StateSet>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/PolygonMode>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/MatrixTransform>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>
#include <GL/gl.h>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>
#include <cmath>

UndefinedGeo3D::UndefinedGeo3D()
{
    setGeoType(Geo_UndefinedGeo3D);
    // 确保基类正确初始化
    initialize();
    LOG_INFO("创建未定义几何体", "几何体");
}

void UndefinedGeo3D::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos.x, worldPos.y, worldPos.z));
        
        if (getControlPoints().size() >= 1)
        {
            completeDrawing();
        }
    }
}

void UndefinedGeo3D::completeDrawing()
{
    setStateComplete();
    clearStateEditing();
    updateGeometry();
    
    LOG_INFO("完成未定义几何体绘制", "几何体");
}

void UndefinedGeo3D::updateGeometry()
{
    if (isGeometryDirty())
    {
        // 清除点线面节点
        clearVertexGeometries();
        clearEdgeGeometries();
        clearFaceGeometries();
        
        updateOSGNode();
        
        // 构建点线面几何体
        buildVertexGeometries();
        buildEdgeGeometries();
        buildFaceGeometries();
        
        // 更新可见性
        updateFeatureVisibility();
    }
}



osg::ref_ptr<osg::Geometry> UndefinedGeo3D::createGeometry()
{
    return createDefaultGeometry();
}

osg::ref_ptr<osg::Geometry> UndefinedGeo3D::createDefaultGeometry()
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    
    if (getControlPoints().empty())
    {
        // 如果没有控制点，创建一个默认的立方体
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
        
        // 创建一个简单的立方体
        float size = 1.0f;
        vertices->push_back(osg::Vec3(-size, -size, -size));
        vertices->push_back(osg::Vec3( size, -size, -size));
        vertices->push_back(osg::Vec3( size,  size, -size));
        vertices->push_back(osg::Vec3(-size,  size, -size));
        vertices->push_back(osg::Vec3(-size, -size,  size));
        vertices->push_back(osg::Vec3( size, -size,  size));
        vertices->push_back(osg::Vec3( size,  size,  size));
        vertices->push_back(osg::Vec3(-size,  size,  size));
        
        // 设置颜色
        osg::Vec4 color(0.8f, 0.8f, 0.8f, 1.0f);
        for (int i = 0; i < 8; ++i)
        {
            colors->push_back(color);
        }
        
        geometry->setVertexArray(vertices.get());
        geometry->setColorArray(colors.get());
        geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        
        // 添加线框绘制
        osg::ref_ptr<osg::DrawElementsUInt> lines = new osg::DrawElementsUInt(GL_LINES);
        // 立方体的12条边
        int indices[] = {
            0,1, 1,2, 2,3, 3,0,  // 底面
            4,5, 5,6, 6,7, 7,4,  // 顶面
            0,4, 1,5, 2,6, 3,7   // 竖边
        };
        for (int i = 0; i < 24; ++i)
        {
            lines->push_back(indices[i]);
        }
        geometry->addPrimitiveSet(lines.get());
        
        LOG_DEBUG("创建默认立方体几何体", "几何体");
    }
    else
    {
        // 根据控制点创建几何体
        const auto& controlPoints = getControlPoints();
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
        
        for (const auto& point : controlPoints)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
            colors->push_back(osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f));
        }
        
        geometry->setVertexArray(vertices.get());
        geometry->setColorArray(colors.get());
        geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        
        // 绘制点
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));
        
        LOG_DEBUG(QString("根据控制点创建几何体，点数: %1").arg(controlPoints.size()), "几何体");
    }
    
    return geometry;
} 

void UndefinedGeo3D::buildVertexGeometries()
{
    clearVertexGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 创建未定义几何体顶点的几何体
    osg::ref_ptr<osg::Geometry> vertexGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 添加控制点作为顶点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a));
    }
    
    vertexGeometry->setVertexArray(vertices.get());
    vertexGeometry->setColorArray(colors.get());
    vertexGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用点绘制
    vertexGeometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));
    
    // 设置点大小
    osg::ref_ptr<osg::StateSet> stateSet = vertexGeometry->getOrCreateStateSet();
    osg::ref_ptr<osg::Point> point = new osg::Point();
    point->setSize(m_parameters.pointSize);
    stateSet->setAttribute(point.get());
    
    addVertexGeometry(vertexGeometry.get());
}

void UndefinedGeo3D::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 创建未定义几何体边的几何体
    osg::ref_ptr<osg::Geometry> edgeGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 添加所有边
    for (size_t i = 0; i < controlPoints.size() - 1; ++i)
    {
        vertices->push_back(osg::Vec3(controlPoints[i].x(), controlPoints[i].y(), controlPoints[i].z()));
        vertices->push_back(osg::Vec3(controlPoints[i + 1].x(), controlPoints[i + 1].y(), controlPoints[i + 1].z()));
        
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    edgeGeometry->setVertexArray(vertices.get());
    edgeGeometry->setColorArray(colors.get());
    edgeGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用线绘制
    edgeGeometry->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, vertices->size()));
    
    // 设置线宽
    osg::ref_ptr<osg::StateSet> stateSet = edgeGeometry->getOrCreateStateSet();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth();
    lineWidth->setWidth(m_parameters.lineWidth);
    stateSet->setAttribute(lineWidth.get());
    
    addEdgeGeometry(edgeGeometry.get());
}

void UndefinedGeo3D::buildFaceGeometries()
{
    clearFaceGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 3)
        return;
    
    // 创建未定义几何体面的几何体
    osg::ref_ptr<osg::Geometry> faceGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 添加所有顶点形成面
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                   m_parameters.fillColor.b, m_parameters.fillColor.a));
    }
    
    faceGeometry->setVertexArray(vertices.get());
    faceGeometry->setColorArray(colors.get());
    faceGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用三角形绘制（简单的扇形三角剖分）
    if (controlPoints.size() >= 3)
    {
        osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
        for (size_t i = 1; i < controlPoints.size() - 1; ++i)
        {
            indices->push_back(0);
            indices->push_back(i);
            indices->push_back(i + 1);
        }
        faceGeometry->addPrimitiveSet(indices.get());
    }
    
    addFaceGeometry(faceGeometry.get());
} 