#include "Sphere3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Sphere3D_Geo::Sphere3D_Geo()
    : m_radius(1.0f)
{
    m_geoType = Geo_Sphere3D;
}

void Sphere3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 2)
        {
            // 计算球的半径
            m_radius = glm::length(m_controlPoints[1].position - m_controlPoints[0].position);
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Sphere3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && m_controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void Sphere3D_Geo::updateGeometry()
{
    updateOSGNode();
}

std::vector<FeatureType> Sphere3D_Geo::getSupportedFeatureTypes() const
{
    return {FeatureType::FACE, FeatureType::VERTEX};
}

osg::ref_ptr<osg::Geometry> Sphere3D_Geo::createGeometry()
{
    if (m_controlPoints.empty())
        return nullptr;
    
    float radius = m_radius;
    glm::vec3 center = m_controlPoints[0].position;
    
    if (m_controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        radius = glm::length(getTempPoint().position - center);
    }
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    int latSegments = static_cast<int>(m_parameters.subdivisionLevel);
    int lonSegments = latSegments * 2;
    
    Color3D color = isStateComplete() ? m_parameters.fillColor : 
                   Color3D(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                          m_parameters.fillColor.b, m_parameters.fillColor.a * 0.5f);
    
    // 生成球面顶点
    for (int lat = 0; lat <= latSegments; ++lat)
    {
        float theta = static_cast<float>(M_PI * lat / latSegments); // 纬度角 0 到 π
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (int lon = 0; lon <= lonSegments; ++lon)
        {
            float phi = static_cast<float>(2.0 * M_PI * lon / lonSegments); // 经度角 0 到 2π
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            // 球面坐标到笛卡尔坐标
            glm::vec3 normal(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
            glm::vec3 point = center + radius * normal;
            
            vertices->push_back(osg::Vec3(point.x, point.y, point.z));
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
    
    // 生成三角形索引
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    
    for (int lat = 0; lat < latSegments; ++lat)
    {
        for (int lon = 0; lon < lonSegments; ++lon)
        {
            int current = lat * (lonSegments + 1) + lon;
            int next = current + lonSegments + 1;
            
            // 第一个三角形
            indices->push_back(current);
            indices->push_back(next);
            indices->push_back(current + 1);
            
            // 第二个三角形
            indices->push_back(current + 1);
            indices->push_back(next);
            indices->push_back(next + 1);
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalArray(normals.get());
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    geometry->addPrimitiveSet(indices.get());
    
    return geometry;
}

std::vector<PickingFeature> Sphere3D_Geo::extractFaceFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        // 球面作为一个整体面Feature
        PickingFeature feature(FeatureType::FACE, 0);
        
        glm::vec3 center = m_controlPoints[0].position;
        feature.center = osg::Vec3(center.x, center.y, center.z);
        feature.size = 0.1f; // 面指示器大小
        
        features.push_back(feature);
    }
    
    return features;
}

std::vector<PickingFeature> Sphere3D_Geo::extractVertexFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        // 球心作为顶点Feature
        PickingFeature centerFeature(FeatureType::VERTEX, 0);
        glm::vec3 center = m_controlPoints[0].position;
        centerFeature.center = osg::Vec3(center.x, center.y, center.z);
        centerFeature.size = 0.05f;
        features.push_back(centerFeature);
        
        // 球面上的关键点（如果有第二个控制点，添加表面点）
        if (m_controlPoints.size() >= 2)
        {
            PickingFeature surfaceFeature(FeatureType::VERTEX, 1);
            surfaceFeature.center = osg::Vec3(m_controlPoints[1].x(), m_controlPoints[1].y(), m_controlPoints[1].z());
            surfaceFeature.size = 0.05f;
            features.push_back(surfaceFeature);
        }
    }
    
    return features;
} 