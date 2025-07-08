#include "Triangle3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>

Triangle3D_Geo::Triangle3D_Geo()
    : m_normal(0, 0, 1)
{
    m_geoType = Geo_Triangle3D;
}

void Triangle3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 3)
        {
            calculateNormal();
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Triangle3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void Triangle3D_Geo::updateGeometry()
{
    updateOSGNode();
}

std::vector<FeatureType> Triangle3D_Geo::getSupportedFeatureTypes() const
{
    return {FeatureType::FACE, FeatureType::EDGE, FeatureType::VERTEX};
}

osg::ref_ptr<osg::Geometry> Triangle3D_Geo::createGeometry()
{
    if (m_controlPoints.size() < 2)
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    // 根据控制点数量决定绘制内容
    if (m_controlPoints.size() == 3 || (m_controlPoints.size() == 2 && getTempPoint().position != glm::vec3(0)))
    {
        // 三个点：绘制完整三角形
        Point3D p1 = m_controlPoints[0];
        Point3D p2 = m_controlPoints[1];
        Point3D p3 = (m_controlPoints.size() == 3) ? m_controlPoints[2] : getTempPoint();
        
        vertices->push_back(osg::Vec3(p1.x(), p1.y(), p1.z()));
        vertices->push_back(osg::Vec3(p2.x(), p2.y(), p2.z()));
        vertices->push_back(osg::Vec3(p3.x(), p3.y(), p3.z()));
        
        // 计算法向量
        glm::vec3 v1 = p2.position - p1.position;
        glm::vec3 v2 = p3.position - p1.position;
        glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
        
        for (int i = 0; i < 3; ++i)
        {
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
        }
        
        // 设置颜色
        Color3D color = isStateComplete() ? m_parameters.fillColor : 
                       Color3D(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                              m_parameters.fillColor.b, m_parameters.fillColor.a * 0.5f);
        
        for (int i = 0; i < 3; ++i)
        {
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
        
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 3));
        
        // 如果需要显示边界
        if (m_parameters.showBorder)
        {
            osg::ref_ptr<osg::Vec4Array> borderColors = new osg::Vec4Array();
            for (int i = 0; i < 3; ++i)
            {
                borderColors->push_back(osg::Vec4(m_parameters.borderColor.r, m_parameters.borderColor.g, 
                                                 m_parameters.borderColor.b, m_parameters.borderColor.a));
            }
            
            // 创建边界线
            osg::ref_ptr<osg::DrawElementsUInt> borderIndices = new osg::DrawElementsUInt(GL_LINE_LOOP);
            borderIndices->push_back(0);
            borderIndices->push_back(1);
            borderIndices->push_back(2);
            
            geometry->addPrimitiveSet(borderIndices.get());
        }
    }
    else if (m_controlPoints.size() >= 1)
    {
        // 一个或两个点：绘制辅助线
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

std::vector<PickingFeature> Triangle3D_Geo::extractFaceFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() == 3)
    {
        PickingFeature feature(FeatureType::FACE, 0);
        
        // 计算三角形中心点
        glm::vec3 center = (m_controlPoints[0].position + 
                           m_controlPoints[1].position + 
                           m_controlPoints[2].position) / 3.0f;
        feature.center = osg::Vec3(center.x, center.y, center.z);
        feature.size = 0.1f; // 面指示器大小
        
        features.push_back(feature);
    }
    
    return features;
}

std::vector<PickingFeature> Triangle3D_Geo::extractEdgeFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 2)
    {
        // 每条边作为一个边Feature
        size_t edgeCount = (m_controlPoints.size() == 3) ? 3 : m_controlPoints.size() - 1;
        
        for (size_t i = 0; i < edgeCount; ++i)
        {
            PickingFeature feature(FeatureType::EDGE, static_cast<uint32_t>(i));
            
            size_t nextIdx = (i + 1) % m_controlPoints.size();
            if (nextIdx >= m_controlPoints.size()) nextIdx = 0;
            
            // 计算边中心点
            glm::vec3 center = (m_controlPoints[i].position + m_controlPoints[nextIdx].position) * 0.5f;
            feature.center = osg::Vec3(center.x, center.y, center.z);
            feature.size = 0.08f; // 边指示器大小
            
            features.push_back(feature);
        }
    }
    
    return features;
}

std::vector<PickingFeature> Triangle3D_Geo::extractVertexFeatures() const
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

void Triangle3D_Geo::calculateNormal()
{
    if (m_controlPoints.size() >= 3)
    {
        glm::vec3 v1 = m_controlPoints[1].position - m_controlPoints[0].position;
        glm::vec3 v2 = m_controlPoints[2].position - m_controlPoints[0].position;
        m_normal = glm::normalize(glm::cross(v1, v2));
    }
}