#include "Arc3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Arc3D_Geo::Arc3D_Geo()
    : m_radius(0.0f)
    , m_startAngle(0.0f)
    , m_endAngle(0.0f)
    , m_normal(0, 0, 1)
{
    m_geoType = Geo_Arc3D;
    // 确保基类正确初始化
    initialize();
}

void Arc3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        const auto& controlPoints = getControlPoints();
        
        if (controlPoints.size() == 3)
        {
            // 计算圆弧参数
            calculateArcParameters();
            completeDrawing();
        }
        
        updateGeometry();
        emit stateChanged(this);
    }
}

void Arc3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    const auto& controlPoints = getControlPoints();
    if (!isStateComplete())
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void Arc3D_Geo::updateGeometry()
{
    if (!isGeometryDirty()) return;
    
    // 清除点线面节点
    clearVertexGeometries();
    clearEdgeGeometries();
    clearFaceGeometries();
    
    const auto& controlPoints = getControlPoints();
    
    // 临时显示预览
    if (controlPoints.size() == 2)
    {
        const Point3D& tempPoint = getTempPoint();
        if (tempPoint.position != glm::vec3(0))
        {
            // 存储原始控制点
            std::vector<Point3D> tempPoints = controlPoints;
            tempPoints.push_back(tempPoint);
            
            // 临时计算圆弧参数
            auto oldPoints = controlPoints;
            // 临时添加点进行计算
            addControlPoint(tempPoint);
            calculateArcParameters();
            
            // 构建几何体
            buildVertexGeometries();
            buildEdgeGeometries();
            
            // 恢复原始控制点
            clearControlPoints();
            for (const auto& pt : oldPoints)
            {
                addControlPoint(pt);
            }
        }
    }
    else
    {
        // 构建几何体
        buildVertexGeometries();
        buildEdgeGeometries();
        buildFaceGeometries();
    }
    
    updateOSGNode();
    clearGeometryDirty();
    
    emit geometryUpdated(this);
}



osg::ref_ptr<osg::Geometry> Arc3D_Geo::createGeometry()
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 3)
        return nullptr;
    
    glm::vec3 p1 = controlPoints[0].position;
    glm::vec3 p2 = controlPoints[1].position;
    glm::vec3 p3 = controlPoints[2].position;
    
    // 计算圆弧参数
    calculateArcParameters();
    
    // 创建圆弧几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 生成圆弧点
    int segments = static_cast<int>(m_parameters.subdivisionLevel);
    if (segments < 8) segments = 16;
    
    for (int i = 0; i <= segments; ++i)
    {
        float t = static_cast<float>(i) / segments;
        float angle = m_startAngle + t * m_sweepAngle;
        
        glm::vec3 point = m_center + m_radius * (
            cosf(angle) * m_uAxis + 
            sinf(angle) * m_vAxis
        );
        
        vertices->push_back(osg::Vec3(point.x, point.y, point.z));
    }
    
    geometry->setVertexArray(vertices);
    
    // 线绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    return geometry;
}

void Arc3D_Geo::buildVertexGeometries()
{
    clearVertexGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 添加控制点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
    }
    
    geometry->setVertexArray(vertices);
    
    // 点绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    addVertexGeometry(geometry);
}

void Arc3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 创建圆弧几何体
    osg::ref_ptr<osg::Geometry> geometry = createGeometry();
    if (geometry.valid())
    {
        addEdgeGeometry(geometry);
    }
}

void Arc3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 3)
        return;
    
    // 圆弧通常不需要面，这里留空
    // 如果需要扇形，可以在这里实现
}

void Arc3D_Geo::calculateArcFromThreePoints()
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 3)
        return;
    
    glm::vec3 p1 = controlPoints[0].position;
    glm::vec3 p2 = controlPoints[1].position;
    glm::vec3 p3 = controlPoints[2].position;
    
    // 计算圆心
    glm::vec3 a = p2 - p1;
    glm::vec3 b = p3 - p2;
    
    // 计算法向量
    m_normal = glm::normalize(glm::cross(a, b));
    
    // 计算圆心
    glm::vec3 midAB = (p1 + p2) * 0.5f;
    glm::vec3 midBC = (p2 + p3) * 0.5f;
    
    glm::vec3 perpA = glm::cross(a, m_normal);
    glm::vec3 perpB = glm::cross(b, m_normal);
    
    // 解线性方程组找圆心
    float t = 0.0f;
    if (glm::length(perpA) > 1e-6 && glm::length(perpB) > 1e-6)
    {
        glm::vec3 diff = midBC - midAB;
        float denom = glm::dot(perpA, perpB);
        if (abs(denom) > 1e-6)
        {
            t = glm::dot(diff, perpB) / denom;
        }
    }
    
    m_center = midAB + t * perpA;
    m_radius = glm::length(p1 - m_center);
    
    // 计算起始和结束角度
    glm::vec3 ref = glm::normalize(p1 - m_center);
    glm::vec3 perpRef = glm::normalize(glm::cross(m_normal, ref));
    
    glm::vec3 v1 = glm::normalize(p1 - m_center);
    glm::vec3 v3 = glm::normalize(p3 - m_center);
    
    m_startAngle = atan2(glm::dot(v1, perpRef), glm::dot(v1, ref));
    m_endAngle = atan2(glm::dot(v3, perpRef), glm::dot(v3, ref));
    
    // 确保角度范围正确
    if (m_endAngle < m_startAngle)
        m_endAngle += 2.0f * static_cast<float>(M_PI);
}

void Arc3D_Geo::generateArcPoints()
{
    m_arcPoints.clear();
    
    if (m_radius <= 0)
        return;
    
    int segments = 50;
    float angleRange = m_endAngle - m_startAngle;
    
    for (int i = 0; i <= segments; ++i)
    {
        float t = static_cast<float>(i) / segments;
        float angle = m_startAngle + t * angleRange;
        
        const auto& controlPoints = getControlPoints();
        glm::vec3 ref = glm::normalize(controlPoints[0].position - m_center);
        glm::vec3 perpRef = glm::normalize(glm::cross(m_normal, ref));
        
        glm::vec3 point = m_center + m_radius * (static_cast<float>(cos(angle)) * ref + static_cast<float>(sin(angle)) * perpRef);
        m_arcPoints.push_back(Point3D(point));
    }
}

void Arc3D_Geo::calculateArcParameters()
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 3)
        return;
    
    // 使用三点计算圆弧
    calculateArcFromThreePoints();
    
    // 计算扫掠角度
    m_sweepAngle = m_endAngle - m_startAngle;
    if (m_sweepAngle < 0)
        m_sweepAngle += 2.0f * static_cast<float>(M_PI);
    
    // 计算坐标轴
    const auto& p1 = controlPoints[0].position;
    glm::vec3 ref = glm::normalize(p1 - m_center);
    m_uAxis = ref;
    m_vAxis = glm::normalize(glm::cross(m_normal, ref));
    
    // 生成圆弧点
    generateArcPoints();
} 