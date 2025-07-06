#include "BezierCurve3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>

BezierCurve3D_Geo::BezierCurve3D_Geo()
{
    m_geoType = Geo_BezierCurve3D;
}

void BezierCurve3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() >= 2)
        {
            generateBezierPoints();
            updateGeometry();
        }
    }
}

void BezierCurve3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && !m_controlPoints.empty())
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void BezierCurve3D_Geo::keyPressEvent(QKeyEvent* event)
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

void BezierCurve3D_Geo::updateGeometry()
{
    updateOSGNode();
}

std::vector<FeatureType> BezierCurve3D_Geo::getSupportedFeatureTypes() const
{
    return {FeatureType::EDGE, FeatureType::VERTEX};
}

osg::ref_ptr<osg::Geometry> BezierCurve3D_Geo::createGeometry()
{
    if (m_controlPoints.size() < 2)
        return nullptr;
    
    generateBezierPoints();
    
    if (m_bezierPoints.empty())
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    for (const Point3D& point : m_bezierPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    // 如果正在绘制且有临时点，计算包含临时点的贝塞尔曲线
    if (!isStateComplete() && getTempPoint().position != glm::vec3(0))
    {
        std::vector<Point3D> tempControlPoints = m_controlPoints;
        tempControlPoints.push_back(getTempPoint());
        
        // 生成临时贝塞尔曲线点
        std::vector<Point3D> tempBezierPoints;
        int steps = m_parameters.steps > 0 ? m_parameters.steps : 50;
        
        for (int i = 0; i <= steps; ++i)
        {
            float t = static_cast<float>(i) / steps;
            // 临时计算贝塞尔点
            std::vector<glm::vec3> tempVecs;
            for (const Point3D& cp : tempControlPoints)
            {
                tempVecs.push_back(cp.position);
            }
            
            while (tempVecs.size() > 1)
            {
                std::vector<glm::vec3> newVecs;
                for (size_t j = 0; j < tempVecs.size() - 1; ++j)
                {
                    newVecs.push_back(glm::mix(tempVecs[j], tempVecs[j+1], t));
                }
                tempVecs = newVecs;
            }
            
            if (!tempVecs.empty())
            {
                tempBezierPoints.push_back(Point3D(tempVecs[0]));
            }
        }
        
        // 添加临时点（半透明）
        for (const Point3D& point : tempBezierPoints)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a * 0.5f));
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
    
    return geometry;
}

std::vector<PickingFeature> BezierCurve3D_Geo::extractEdgeFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_bezierPoints.size() >= 2)
    {
        // 贝塞尔曲线作为一个整体边Feature
        PickingFeature feature(FeatureType::EDGE, 0);
        
        // 计算曲线中心点（简单取中点）
        size_t midIndex = m_bezierPoints.size() / 2;
        glm::vec3 center = m_bezierPoints[midIndex].position;
        feature.center = osg::Vec3(center.x, center.y, center.z);
        feature.size = 0.08f; // 边指示器大小
        
        features.push_back(feature);
    }
    
    return features;
}

std::vector<PickingFeature> BezierCurve3D_Geo::extractVertexFeatures() const
{
    std::vector<PickingFeature> features;
    
    // 从控制点中提取顶点Feature
    for (size_t i = 0; i < m_controlPoints.size(); ++i)
    {
        PickingFeature feature(FeatureType::VERTEX, static_cast<uint32_t>(i));
        feature.center = osg::Vec3(m_controlPoints[i].x(), m_controlPoints[i].y(), m_controlPoints[i].z());
        feature.size = 0.05f; // 顶点指示器大小
        features.push_back(feature);
    }
    
    return features;
}

void BezierCurve3D_Geo::generateBezierPoints()
{
    m_bezierPoints.clear();
    
    if (m_controlPoints.size() < 2)
        return;
    
    int steps = m_parameters.steps > 0 ? m_parameters.steps : 50;
    
    for (int i = 0; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / steps;
        glm::vec3 point = calculateBezierPoint(t);
        m_bezierPoints.push_back(Point3D(point));
    }
}

glm::vec3 BezierCurve3D_Geo::calculateBezierPoint(float t) const
{
    if (m_controlPoints.empty())
        return glm::vec3(0);
    
    // De Casteljau算法
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
    
    return tempPoints.empty() ? glm::vec3(0) : tempPoints[0];
} 