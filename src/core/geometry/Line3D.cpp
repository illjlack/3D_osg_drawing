#include "Line3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Line3D_Geo::Line3D_Geo()
    : m_totalLength(0.0f)
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
        
        const auto& controlPoints = getControlPoints();
        if (controlPoints.size() >= 2)
        {
            completeDrawing();
        }
        else
        {
            markGeometryDirty();
        }
        
        updateGeometry();
    }
}

void Line3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
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
        if (getControlPoints().size() >= 2)
        {
            completeDrawing();
        }
    }
    else if (event->key() == Qt::Key_Escape)
    {
        if (!getControlPoints().empty())
        {
            removeControlPoint(getControlPoints().size() - 1);
            updateGeometry();
        }
    }
}

void Line3D_Geo::completeDrawing()
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() >= 2)
    {
        Geo3D::completeDrawing();
    }
}

float Line3D_Geo::calculateLength() const
{
    const auto& controlPoints = getControlPoints();
    
    float totalLength = 0.0f;
    if (controlPoints.size() < 2)
        return totalLength;
    
    for (size_t i = 1; i < controlPoints.size(); ++i)
    {
        glm::vec3 diff = controlPoints[i].position - controlPoints[i-1].position;
        totalLength += glm::length(diff);
    }
    
    return totalLength;
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
    
    // 确保节点名称正确设置（用于拾取识别）
    setupNodeNames();
    
    // 更新捕捉点
    updateSnapPoints();
    
    // 更新可见性
    updateFeatureVisibility();
}

osg::ref_ptr<osg::Geometry> Line3D_Geo::createGeometry()
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 2 && getTempPoint().position == glm::vec3(0))
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
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 只为控制点创建几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 添加控制点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a));
    }
    
    // 如果有临时点且几何体未完成，添加临时点
    if (!isStateComplete() && getTempPoint().position != glm::vec3(0))
    {
        vertices->push_back(osg::Vec3(getTempPoint().x(), getTempPoint().y(), getTempPoint().z()));
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a * 0.5f));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 点绘制 - 控制点使用较大的点大小以便拾取
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置点的大小
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::Point> point = new osg::Point;
    point->setSize(8.0f);  // 控制点大小
    stateSet->setAttribute(point);
    
    addVertexGeometry(geometry);
}

void Line3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 创建线段边界线几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 构建所有点（包括临时点）
    std::vector<Point3D> allPoints = controlPoints;
    if (!isStateComplete() && getTempPoint().position != glm::vec3(0))
    {
        allPoints.push_back(getTempPoint());
    }
    
    // 添加所有点用于线段绘制
    for (const auto& point : allPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 线绘制 - 边界线
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置线的宽度
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth;
    lineWidth->setWidth(2.0f);  // 边界线宽度
    stateSet->setAttribute(lineWidth);
    
    addEdgeGeometry(geometry);
}

void Line3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    // 线对象没有面
}

void Line3D_Geo::generatePolyline()
{
    m_generatedPoints.clear();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 直接使用控制点作为多段线点
    for (const Point3D& point : controlPoints)
    {
        m_generatedPoints.push_back(point);
    }
}

void Line3D_Geo::generateSpline()
{
    // 样条曲线生成（简化实现）
    const auto& controlPoints = getControlPoints();
    
    m_generatedPoints.clear();
    if (controlPoints.size() < 2)
        return;
    
    // 简化的样条曲线：在控制点之间插值
    int steps = m_parameters.steps;
    for (size_t i = 0; i < controlPoints.size() - 1; ++i)
    {
        const Point3D& p0 = controlPoints[i];
        const Point3D& p1 = controlPoints[i + 1];
        
        for (int j = 0; j < steps; ++j)
        {
            float t = static_cast<float>(j) / steps;
            Point3D interpolated;
            interpolated.position = p0.position + t * (p1.position - p0.position);
            m_generatedPoints.push_back(interpolated);
        }
    }
    
    // 添加最后一个点
    if (!controlPoints.empty())
    {
        m_generatedPoints.push_back(controlPoints.back());
    }
}

void Line3D_Geo::generateBezierCurve()
{
    // 贝塞尔曲线生成（简化实现）
    const auto& controlPoints = getControlPoints();
    
    m_generatedPoints.clear();
    if (controlPoints.size() < 2)
        return;
    
    // 简化的贝塞尔曲线：二次贝塞尔
    if (controlPoints.size() == 3)
    {
        // 二次贝塞尔曲线
        int steps = m_parameters.steps;
        for (int i = 0; i <= steps; ++i)
        {
            float t = static_cast<float>(i) / steps;
            float u = 1.0f - t;
            
            Point3D bezierPoint;
            bezierPoint.position = u*u * controlPoints[0].position + 
                                 2*u*t * controlPoints[1].position + 
                                 t*t * controlPoints[2].position;
            m_generatedPoints.push_back(bezierPoint);
        }
    }
    else
    {
        // 回退到多段线
        generatePolyline();
    }
} 