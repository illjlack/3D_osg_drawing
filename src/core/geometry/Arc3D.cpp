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
    if (!mm_state()->isStateComplete())
    {
        mm_controlPoint()->addControlPoint(Point3D(worldPos));
        const auto& controlPoints = mm_controlPoint()->getControlPoints();
        
        if (controlPoints.size() == 3)
        {
            mm_state()->setStateComplete();
        }
        
        
        mm_state()->setVertexGeometryInvalid();
        mm_state()->setEdgeGeometryInvalid();
        emit stateChanged(this);
    }
}

void Arc3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!mm_state()->isStateComplete())
    {
        const auto& controlPoints = mm_controlPoint()->getControlPoints();
        
        // 设置临时点用于预览
        if (!controlPoints.empty())
        {
            mm_controlPoint()->setTempPoint(Point3D(worldPos));
        }
        
        // 通过状态管理器通知临时点更新
        mm_state()->setTemporaryPointsUpdated();
        emit stateChanged(this);
    }
}

void Arc3D_Geo::buildVertexGeometries()
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
    
    // 添加控制点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
    }
    
    geometry->setVertexArray(vertices);
    
    // 点绘制 - 控制点使用较大的点大小以便拾取
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置点的大小和颜色
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::Point> point = new osg::Point;
    point->setSize(8.0f);  // 控制点大小
    stateSet->setAttribute(point);
    
    // 通过状态管理器清除顶点几何体无效状态
    mm_state()->clearVertexGeometryInvalid();
}

void Arc3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() < 3)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid())
        return;
    
    // 创建圆弧边界线几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 计算圆弧参数
    calculateArcParameters();
    
    // 生成圆弧边界线点
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
    
    // 线绘制 - 边界线
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置线的宽度
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth;
    lineWidth->setWidth(2.0f);  // 边界线宽度
    stateSet->setAttribute(lineWidth);
    
    // 通过状态管理器清除边几何体无效状态
    mm_state()->clearEdgeGeometryInvalid();
}

void Arc3D_Geo::buildFaceGeometries()
{
    // 圆弧不需要画面
    return;
}

void Arc3D_Geo::calculateArcFromThreePoints()
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
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
        
        const auto& controlPoints = mm_controlPoint()->getControlPoints();
        glm::vec3 ref = glm::normalize(controlPoints[0].position - m_center);
        glm::vec3 perpRef = glm::normalize(glm::cross(m_normal, ref));
        
        glm::vec3 point = m_center + m_radius * (static_cast<float>(cos(angle)) * ref + static_cast<float>(sin(angle)) * perpRef);
        m_arcPoints.push_back(point);
    }
}

void Arc3D_Geo::calculateArcParameters()
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
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
    
    // 通过状态管理器设置参数更新状态
    mm_state()->setParametersUpdated();
}