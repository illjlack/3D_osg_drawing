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
    if (!mm_state()->isStateComplete())
    {
        mm_controlPoint()->addControlPoint(Point3D(worldPos));
        
        const auto& controlPoints = mm_controlPoint()->getControlPoints();
        if (controlPoints.size() >= 2)
        {
            mm_state()->setStateComplete();
        }
        
        updateGeometry();
    }
}

void Line3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!mm_state()->isStateComplete())
    {
        // 设置临时点用于预览
        // 这里需要实现临时点机制
        updateGeometry();
    }
}

void Line3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if (mm_controlPoint()->getControlPoints().size() >= 2)
        {
            mm_state()->setStateComplete();
        }
    }
    else if (event->key() == Qt::Key_Escape)
    {
        if (!mm_controlPoint()->getControlPoints().empty())
        {
            mm_controlPoint()->removeLastControlPoint();
            updateGeometry();
        }
    }
}

void Line3D_Geo::completeDrawing()
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() >= 2)
    {
        Geo3D::completeDrawing();
    }
}

float Line3D_Geo::calculateLength() const
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    
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
    mm_node()->clearAllGeometries();
    
    // 构建点线面几何体
    buildVertexGeometries();
    buildEdgeGeometries();
    buildFaceGeometries();
    
    // 更新OSG节点
    updateOSGNode();
    
    // 更新捕捉点
    mm_snapPoint()->updateSnapPoints();
    
    // 更新包围盒
    mm_boundingBox()->updateBoundingBox();
    
    // 更新空间索引
    mm_node()->updateSpatialIndex();
}

osg::ref_ptr<osg::Geometry> Line3D_Geo::createGeometry()
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() < 2)
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
    mm_node()->clearVertexGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getVertexGeometry();
    if (!geometry.valid())
        return;
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 添加控制点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a));
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
}

void Line3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid())
        return;
    
    // 创建边的几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 添加边的顶点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 线绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置线的宽度
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth;
    lineWidth->setWidth(m_parameters.lineWidth);
    stateSet->setAttribute(lineWidth);
}

void Line3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    // 线对象没有面
}

void Line3D_Geo::generatePolyline()
{
    m_generatedPoints.clear();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
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
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    
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
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    
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

bool Line3D_Geo::hitTest(const Ray3D& ray, PickResult3D& result) const
{
    // 获取线的起点和终点
    glm::vec3 startPos = getStartPoint();
    glm::vec3 endPos = getEndPoint();
    
    // 计算线的方向向量
    glm::vec3 lineDir = glm::normalize(endPos - startPos);
    glm::vec3 rayDir = glm::normalize(ray.direction);
    
    // 计算两条线之间的最短距离
    glm::vec3 crossProduct = glm::cross(lineDir, rayDir);
    float crossLength = glm::length(crossProduct);
    
    if (crossLength < 1e-6f) // 两条线平行
    {
        return false;
    }
    
    // 计算参数
    glm::vec3 diff = ray.origin - startPos;
    float det1 = glm::determinant(glm::mat3(diff, lineDir, rayDir));
    float det2 = glm::determinant(glm::mat3(diff, lineDir, crossProduct));
    float det3 = glm::determinant(glm::mat3(diff, rayDir, crossProduct));
    
    float t1 = det1 / (crossLength * crossLength);
    float t2 = det2 / (crossLength * crossLength);
    
    // 计算最近点
    glm::vec3 closestPointOnLine = startPos + t1 * lineDir;
    glm::vec3 closestPointOnRay = ray.origin + t2 * rayDir;
    
    // 计算距离
    float distance = glm::length(closestPointOnLine - closestPointOnRay);
    
    // 检查参数是否在有效范围内
    if (t1 >= 0.0f && t1 <= 1.0f && t2 >= 0.0f && distance <= 0.1f)
    {
        result.hit = true;
        result.distance = t2;
        result.userData = const_cast<Line3D_Geo*>(this);
        result.point = closestPointOnLine;
        return true;
    }
    
    return false;
} 