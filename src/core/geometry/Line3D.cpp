#include "Line3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Line3D_Geo::Line3D_Geo()
{
    m_geoType = Geo_Line3D;
    // 确保基类正确初始化
    initialize();
}

void Line3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() >= 2)
        {
            generatePolyline();
            updateGeometry();
        }
    }
}

void Line3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && !m_controlPoints.empty())
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void Line3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if (m_controlPoints.size() >= 2)
        {
            completeDrawing();
        }
    }
    else if (event->key() == Qt::Key_Escape)
    {
        if (!m_controlPoints.empty())
        {
            removeControlPoint(m_controlPoints.size() - 1);
            updateGeometry();
        }
    }
}

void Line3D_Geo::updateGeometry()
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

osg::ref_ptr<osg::Geometry> Line3D_Geo::createGeometry()
{
    if (m_controlPoints.size() < 2 && getTempPoint().position == glm::vec3(0))
        return nullptr;
    
    switch (m_parameters.nodeLineStyle)
    {
    case NodeLine_Polyline3D:
        generatePolyline();
        break;
    case NodeLine_Spline3D:
        generateSpline();
        break;
    case NodeLine_Bezier3D:
        generateBezierCurve();
        break;
    default:
        generatePolyline();
        break;
    }
    
    if (m_generatedPoints.empty())
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    for (const Point3D& point : m_generatedPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    // 如果正在绘制且有临时点，添加到最后一个点的线
    if (!isStateComplete() && getTempPoint().position != glm::vec3(0))
    {
        vertices->push_back(osg::Vec3(getTempPoint().x(), getTempPoint().y(), getTempPoint().z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a * 0.5f));
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
    
    return geometry;
}

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void Line3D_Geo::buildVertexGeometries()
{
    clearVertexGeometries();
    
    if (m_controlPoints.empty())
        return;
    
    // 创建线端点的几何体
    osg::ref_ptr<osg::Geometry> vertexGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 添加起点和终点
    for (const auto& point : m_controlPoints)
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

void Line3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    if (m_controlPoints.size() < 2)
        return;
    
    // 创建线的几何体
    osg::ref_ptr<osg::Geometry> edgeGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 添加起点和终点
    for (const auto& point : m_controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    edgeGeometry->setVertexArray(vertices.get());
    edgeGeometry->setColorArray(colors.get());
    edgeGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用线绘制
    edgeGeometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
    
    // 设置线宽
    osg::ref_ptr<osg::StateSet> stateSet = edgeGeometry->getOrCreateStateSet();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth();
    lineWidth->setWidth(m_parameters.lineWidth);
    stateSet->setAttribute(lineWidth.get());
    
    addEdgeGeometry(edgeGeometry.get());
}

void Line3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    // 线对象没有面
}

void Line3D_Geo::generatePolyline()
{
    m_generatedPoints = m_controlPoints;
}

void Line3D_Geo::generateSpline()
{
    if (m_controlPoints.size() < 3)
    {
        generatePolyline();
        return;
    }
    
    m_generatedPoints.clear();
    
    int steps = m_parameters.steps > 0 ? m_parameters.steps : 20;
    
    // 简单的Catmull-Rom样条曲线实现
    for (int i = 0; i < static_cast<int>(m_controlPoints.size()) - 1; ++i)
    {
        glm::vec3 p0 = (i > 0) ? m_controlPoints[i-1].position : m_controlPoints[i].position;
        glm::vec3 p1 = m_controlPoints[i].position;
        glm::vec3 p2 = m_controlPoints[i+1].position;
        glm::vec3 p3 = (i+2 < static_cast<int>(m_controlPoints.size())) ? 
                       m_controlPoints[i+2].position : m_controlPoints[i+1].position;
        
        for (int j = 0; j < steps; ++j)
        {
            float t = static_cast<float>(j) / steps;
            float t2 = t * t;
            float t3 = t2 * t;
            
            glm::vec3 point = 0.5f * (
                (2.0f * p1) +
                (-p0 + p2) * t +
                (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
            );
            
            m_generatedPoints.push_back(Point3D(point));
        }
    }
    
    // 添加最后一个点
    m_generatedPoints.push_back(m_controlPoints.back());
}

void Line3D_Geo::generateBezierCurve()
{
    if (m_controlPoints.size() < 2)
        return;
    
    m_generatedPoints.clear();
    
    int steps = m_parameters.steps > 0 ? m_parameters.steps : 50;
    
    for (int i = 0; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / steps;
        glm::vec3 point = glm::vec3(0);
        
        // De Casteljau算法计算贝塞尔曲线点
        std::vector<glm::vec3> tempPoints;
        for (const Point3D& cp : m_controlPoints)
        {
            tempPoints.push_back(cp.position);
        }
        
        while (tempPoints.size() > 1)
        {
            std::vector<glm::vec3> newPoints;
            for (size_t j = 0; j < tempPoints.size() - 1; ++j)
            {
                newPoints.push_back(glm::mix(tempPoints[j], tempPoints[j+1], t));
            }
            tempPoints = newPoints;
        }
        
        if (!tempPoints.empty())
        {
            m_generatedPoints.push_back(Point3D(tempPoints[0]));
        }
    }
} 