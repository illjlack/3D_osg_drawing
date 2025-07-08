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
}

void Arc3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 3)
        {
            calculateArcFromThreePoints();
            generateArcPoints();
            completeDrawing();
        }
    }
}

void Arc3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        if (m_controlPoints.size() == 2)
        {
            // 临时计算圆弧预览
            std::vector<Point3D> tempPoints = m_controlPoints;
            tempPoints.push_back(getTempPoint());
            
            // 临时保存当前控制点
            auto oldPoints = m_controlPoints;
            m_controlPoints = tempPoints;
            
            calculateArcFromThreePoints();
            generateArcPoints();
            
            // 恢复控制点
            m_controlPoints = oldPoints;
        }
        
        updateGeometry();
    }
}

void Arc3D_Geo::updateGeometry()
{
    updateOSGNode();
}

std::vector<FeatureType> Arc3D_Geo::getSupportedFeatureTypes() const
{
    return {FeatureType::EDGE, FeatureType::VERTEX};
}

osg::ref_ptr<osg::Geometry> Arc3D_Geo::createGeometry()
{
    if (m_arcPoints.empty())
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    for (const Point3D& point : m_arcPoints)
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

std::vector<PickingFeature> Arc3D_Geo::extractEdgeFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_arcPoints.size() >= 2)
    {
        // 圆弧作为一个整体边Feature
        PickingFeature feature(FeatureType::EDGE, 0);
        
        // 计算圆弧中心点
        glm::vec3 center = m_center;
        feature.center = osg::Vec3(center.x, center.y, center.z);
        feature.size = 0.08f; // 边指示器大小
        
        features.push_back(feature);
    }
    
    return features;
}

std::vector<PickingFeature> Arc3D_Geo::extractVertexFeatures() const
{
    std::vector<PickingFeature> features;
    
    // 从控制点中提取顶点Feature（圆弧的端点）
    if (m_controlPoints.size() >= 1)
    {
        // 起点
        PickingFeature startFeature(FeatureType::VERTEX, 0);
        startFeature.center = osg::Vec3(m_controlPoints[0].x(), m_controlPoints[0].y(), m_controlPoints[0].z());
        startFeature.size = 0.05f;
        features.push_back(startFeature);
    }
    
    if (m_controlPoints.size() >= 3)
    {
        // 终点
        PickingFeature endFeature(FeatureType::VERTEX, 1);
        endFeature.center = osg::Vec3(m_controlPoints[2].x(), m_controlPoints[2].y(), m_controlPoints[2].z());
        endFeature.size = 0.05f;
        features.push_back(endFeature);
        
        // 圆心
        PickingFeature centerFeature(FeatureType::VERTEX, 2);
        centerFeature.center = osg::Vec3(m_center.x, m_center.y, m_center.z);
        centerFeature.size = 0.05f;
        features.push_back(centerFeature);
    }
    
    return features;
}

void Arc3D_Geo::calculateArcFromThreePoints()
{
    if (m_controlPoints.size() < 3)
        return;
    
    glm::vec3 p1 = m_controlPoints[0].position;
    glm::vec3 p2 = m_controlPoints[1].position;
    glm::vec3 p3 = m_controlPoints[2].position;
    
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
        
        glm::vec3 ref = glm::normalize(m_controlPoints[0].position - m_center);
        glm::vec3 perpRef = glm::normalize(glm::cross(m_normal, ref));
        
        glm::vec3 point = m_center + m_radius * (static_cast<float>(cos(angle)) * ref + static_cast<float>(sin(angle)) * perpRef);
        m_arcPoints.push_back(Point3D(point));
    }
} 