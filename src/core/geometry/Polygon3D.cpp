#include "Polygon3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>

Polygon3D_Geo::Polygon3D_Geo()
    : m_normal(0, 0, 1)
{
    m_geoType = Geo_Polygon3D;
}

void Polygon3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        updateGeometry();
    }
}

void Polygon3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void Polygon3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if (m_controlPoints.size() >= 3)
        {
            calculateNormal();
            triangulatePolygon();
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

void Polygon3D_Geo::updateGeometry()
{
    updateOSGNode();
}

std::vector<FeatureType> Polygon3D_Geo::getSupportedFeatureTypes() const
{
    return {FeatureType::FACE, FeatureType::EDGE, FeatureType::VERTEX};
}

osg::ref_ptr<osg::Geometry> Polygon3D_Geo::createGeometry()
{
    if (m_controlPoints.size() < 2)
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    if (isStateComplete() && m_controlPoints.size() >= 3)
    {
        // 绘制完成的多边形
        for (const Point3D& point : m_controlPoints)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
            normals->push_back(osg::Vec3(m_normal.x, m_normal.y, m_normal.z));
            colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                       m_parameters.fillColor.b, m_parameters.fillColor.a));
        }
        
        // 使用三角化的索引
        if (!m_triangleIndices.empty())
        {
            osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
            for (unsigned int index : m_triangleIndices)
            {
                indices->push_back(index);
            }
            geometry->addPrimitiveSet(indices.get());
        }
        else
        {
            // 简单扇形三角化
            for (size_t i = 1; i < m_controlPoints.size() - 1; ++i)
            {
                osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
                indices->push_back(0);
                indices->push_back(i);
                indices->push_back(i + 1);
                geometry->addPrimitiveSet(indices.get());
            }
        }
        
        // 边界线
        if (m_parameters.showBorder)
        {
            osg::ref_ptr<osg::DrawElementsUInt> borderIndices = new osg::DrawElementsUInt(GL_LINE_LOOP);
            for (size_t i = 0; i < m_controlPoints.size(); ++i)
            {
                borderIndices->push_back(i);
            }
            geometry->addPrimitiveSet(borderIndices.get());
        }
    }
    else
    {
        // 绘制过程中的辅助线
        for (const Point3D& point : m_controlPoints)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a));
            normals->push_back(osg::Vec3(0, 0, 1));
        }
        
        if (getTempPoint().position != glm::vec3(0))
        {
            vertices->push_back(osg::Vec3(getTempPoint().x(), getTempPoint().y(), getTempPoint().z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a * 0.5f));
            normals->push_back(osg::Vec3(0, 0, 1));
        }
        
        if (vertices->size() >= 2)
        {
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
        }
        else
        {
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    if (!normals->empty())
    {
        geometry->setNormalArray(normals.get());
        geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    
    return geometry;
}

std::vector<PickingFeature> Polygon3D_Geo::extractFaceFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 3)
    {
        // 多边形面Feature
        PickingFeature feature(FeatureType::FACE, 0);
        
        // 计算面中心点
        glm::vec3 center(0);
        for (const Point3D& point : m_controlPoints)
        {
            center += point.position;
        }
        center /= m_controlPoints.size();
        
        feature.center = osg::Vec3(center.x, center.y, center.z);
        feature.size = 0.1f; // 面指示器大小
        features.push_back(feature);
    }
    
    return features;
}

std::vector<PickingFeature> Polygon3D_Geo::extractEdgeFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 3)
    {
        // 多边形各边Feature
        for (size_t i = 0; i < m_controlPoints.size(); ++i)
        {
            size_t next = (i + 1) % m_controlPoints.size();
            PickingFeature feature(FeatureType::EDGE, static_cast<uint32_t>(i));
            
            // 计算边中心点
            glm::vec3 center = (m_controlPoints[i].position + m_controlPoints[next].position) * 0.5f;
            feature.center = osg::Vec3(center.x, center.y, center.z);
            feature.size = 0.08f; // 边指示器大小
            features.push_back(feature);
        }
    }
    
    return features;
}

std::vector<PickingFeature> Polygon3D_Geo::extractVertexFeatures() const
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

void Polygon3D_Geo::calculateNormal()
{
    if (m_controlPoints.size() >= 3)
    {
        glm::vec3 v1 = m_controlPoints[1].position - m_controlPoints[0].position;
        glm::vec3 v2 = m_controlPoints[2].position - m_controlPoints[0].position;
        m_normal = glm::normalize(glm::cross(v1, v2));
    }
}

void Polygon3D_Geo::triangulatePolygon()
{
    m_triangleIndices.clear();
    
    if (m_controlPoints.size() < 3)
        return;
    
    // 简单的耳切三角化算法
    // 这里使用简单的扇形三角化，更复杂的多边形可能需要更高级的算法
    for (size_t i = 1; i < m_controlPoints.size() - 1; ++i)
    {
        m_triangleIndices.push_back(0);
        m_triangleIndices.push_back(i);
        m_triangleIndices.push_back(i + 1);
    }
} 