#include "Torus3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Torus3D_Geo::Torus3D_Geo()
    : m_majorRadius(2.0f)
    , m_minorRadius(0.5f)
    , m_axis(0, 0, 1)
{
    m_geoType = Geo_Torus3D;
}

void Torus3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 2)
        {
            // 计算圆环参数
            float distance = glm::length(m_controlPoints[1].position - m_controlPoints[0].position);
            m_majorRadius = distance;
            m_minorRadius = distance * 0.2f; // 次半径为主半径的20%
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Torus3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && m_controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void Torus3D_Geo::updateGeometry()
{
    updateOSGNode();
}

std::vector<FeatureType> Torus3D_Geo::getSupportedFeatureTypes() const
{
    return {FeatureType::FACE, FeatureType::EDGE, FeatureType::VERTEX};
}

osg::ref_ptr<osg::Geometry> Torus3D_Geo::createGeometry()
{
    if (m_controlPoints.empty())
        return nullptr;
    
    float majorRadius = m_majorRadius;
    float minorRadius = m_minorRadius;
    glm::vec3 center = m_controlPoints[0].position;
    glm::vec3 axis = m_axis;
    
    if (m_controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        float distance = glm::length(getTempPoint().position - center);
        majorRadius = distance;
        minorRadius = distance * 0.2f;
    }
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    int majorSegments = static_cast<int>(m_parameters.subdivisionLevel);
    if (majorSegments < 8) majorSegments = 16; // 默认16段
    int minorSegments = majorSegments / 2;
    if (minorSegments < 4) minorSegments = 8; // 默认8段
    
    // 计算垂直于轴的两个正交向量
    glm::vec3 u, v;
    if (abs(axis.z) < 0.9f)
    {
        u = glm::normalize(glm::cross(axis, glm::vec3(0, 0, 1)));
    }
    else
    {
        u = glm::normalize(glm::cross(axis, glm::vec3(1, 0, 0)));
    }
    v = glm::normalize(glm::cross(axis, u));
    
    Color3D color = isStateComplete() ? m_parameters.fillColor : 
                   Color3D(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                          m_parameters.fillColor.b, m_parameters.fillColor.a * 0.5f);
    
    // 生成圆环顶点
    for (int i = 0; i <= majorSegments; ++i)
    {
        float majorAngle = 2.0f * M_PI * i / majorSegments;
        glm::vec3 majorCenter = center + majorRadius * (static_cast<float>(cos(majorAngle)) * u + static_cast<float>(sin(majorAngle)) * v);
        glm::vec3 majorTangent = glm::normalize(-static_cast<float>(sin(majorAngle)) * u + static_cast<float>(cos(majorAngle)) * v);
        
        for (int j = 0; j <= minorSegments; ++j)
        {
            float minorAngle = 2.0f * M_PI * j / minorSegments;
            
            // 在局部坐标系中计算次圆上的点
            glm::vec3 localPoint = minorRadius * (static_cast<float>(cos(minorAngle)) * glm::normalize(glm::cross(majorTangent, axis)) + 
                                                 static_cast<float>(sin(minorAngle)) * axis);
            glm::vec3 point = majorCenter + localPoint;
            
            // 计算法向量
            glm::vec3 normal = glm::normalize(localPoint);
            
            vertices->push_back(osg::Vec3(point.x, point.y, point.z));
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
    
    // 生成三角形索引
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    
    for (int i = 0; i < majorSegments; ++i)
    {
        for (int j = 0; j < minorSegments; ++j)
        {
            int current = i * (minorSegments + 1) + j;
            int next = ((i + 1) % (majorSegments + 1)) * (minorSegments + 1) + j;
            
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

std::vector<PickingFeature> Torus3D_Geo::extractFaceFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        // 圆环面Feature - 整个环形表面
        PickingFeature feature(FeatureType::FACE, 0);
        feature.center = osg::Vec3(m_controlPoints[0].x(), m_controlPoints[0].y(), m_controlPoints[0].z());
        feature.size = 0.1f;
        features.push_back(feature);
    }
    
    return features;
}

std::vector<PickingFeature> Torus3D_Geo::extractEdgeFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        glm::vec3 center = m_controlPoints[0].position;
        
        // 外边缘
        PickingFeature outerEdge(FeatureType::EDGE, 0);
        outerEdge.center = osg::Vec3(center.x, center.y, center.z);
        outerEdge.size = 0.08f;
        features.push_back(outerEdge);
        
        // 内边缘
        PickingFeature innerEdge(FeatureType::EDGE, 1);
        innerEdge.center = osg::Vec3(center.x, center.y, center.z);
        innerEdge.size = 0.08f;
        features.push_back(innerEdge);
    }
    
    return features;
}

std::vector<PickingFeature> Torus3D_Geo::extractVertexFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        // 圆环的"顶点"是中心点
        PickingFeature centerVertex(FeatureType::VERTEX, 0);
        centerVertex.center = osg::Vec3(m_controlPoints[0].x(), m_controlPoints[0].y(), m_controlPoints[0].z());
        centerVertex.size = 0.05f;
        features.push_back(centerVertex);
    }
    
    return features;
} 