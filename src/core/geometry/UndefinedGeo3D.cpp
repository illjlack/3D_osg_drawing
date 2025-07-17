#include "UndefinedGeo3D.h"
#include "../Common3D.h"
#include "../../util/LogManager.h"
#include "../../util/MathUtils.h"
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
    if (!mm_state()->isStateDrawComplete())
    {
        // 添加控制点
        mm_controlPoint()->addControlPoint(Point3D(worldPos.x, worldPos.y, worldPos.z));
        
        if (mm_controlPoint()->getControlPoints().size() >= 1)
        {
            mm_state()->setStateDrawComplete();
        }
        
        mm_state()->setControlPointsUpdated();
    }
}

void UndefinedGeo3D::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 未定义几何体的鼠标移动事件 - 默认实现
    // 未定义几何体不需要处理鼠标移动事件
}

void UndefinedGeo3D::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> vertexGeometry = mm_node()->getVertexGeometry();
    if (!vertexGeometry.valid())
        return;
    
    // 创建未定义几何体顶点的几何体
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
}

void UndefinedGeo3D::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> edgeGeometry = mm_node()->getEdgeGeometry();
    if (!edgeGeometry.valid())
        return;
    
    // 创建未定义几何体边的几何体
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
    
    // 几何体已经通过mm_node()->getEdgeGeometry()获取，直接使用
}

void UndefinedGeo3D::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() < 3)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> faceGeometry = mm_node()->getFaceGeometry();
    if (!faceGeometry.valid())
        return;
    
    // 创建未定义几何体面的几何体
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
    
    // 几何体已经通过mm_node()->getFaceGeometry()获取，直接使用
}