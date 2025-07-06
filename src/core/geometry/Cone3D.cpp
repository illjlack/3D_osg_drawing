#include "Cone3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Cone3D_Geo::Cone3D_Geo()
    : m_radius(1.0f)
    , m_height(2.0f)
    , m_axis(0, 0, 1)
{
    m_geoType = Geo_Cone3D;
}

void Cone3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 2)
        {
            // 计算圆锥参数
            glm::vec3 diff = m_controlPoints[1].position - m_controlPoints[0].position;
            m_height = glm::length(diff);
            if (m_height > 0)
                m_axis = glm::normalize(diff);
            m_radius = m_height * 0.4f; // 默认半径为高度的40%
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Cone3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && m_controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void Cone3D_Geo::updateGeometry()
{
    updateOSGNode();
}

std::vector<FeatureType> Cone3D_Geo::getSupportedFeatureTypes() const
{
    return {FeatureType::FACE, FeatureType::EDGE, FeatureType::VERTEX};
}

osg::ref_ptr<osg::Geometry> Cone3D_Geo::createGeometry()
{
    if (m_controlPoints.empty())
        return nullptr;
    
    float radius = m_radius;
    float height = m_height;
    glm::vec3 axis = m_axis;
    glm::vec3 base = m_controlPoints[0].position;
    
    if (m_controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        glm::vec3 diff = getTempPoint().position - base;
        height = glm::length(diff);
        if (height > 0)
            axis = glm::normalize(diff);
        radius = height * 0.4f;
    }
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    int segments = static_cast<int>(m_parameters.subdivisionLevel);
    if (segments < 8) segments = 16; // 默认16段
    
    glm::vec3 apex = base + axis * height;
    
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
    
    // 生成圆锥侧面
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1 = static_cast<float>(cos(angle1)) * u + static_cast<float>(sin(angle1)) * v;
        glm::vec3 dir2 = static_cast<float>(cos(angle2)) * u + static_cast<float>(sin(angle2)) * v;
        
        glm::vec3 p1 = base + radius * dir1;
        glm::vec3 p2 = base + radius * dir2;
        
        // 计算侧面法向量（朝外）
        glm::vec3 edge1 = apex - p1;
        glm::vec3 edge2 = p2 - p1;
        glm::vec3 normal = glm::normalize(glm::cross(edge2, edge1));
        
        vertices->push_back(osg::Vec3(p1.x, p1.y, p1.z));
        vertices->push_back(osg::Vec3(p2.x, p2.y, p2.z));
        vertices->push_back(osg::Vec3(apex.x, apex.y, apex.z));
        
        for (int j = 0; j < 3; ++j)
        {
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
    
    // 生成底面
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1 = static_cast<float>(cos(angle1)) * u + static_cast<float>(sin(angle1)) * v;
        glm::vec3 dir2 = static_cast<float>(cos(angle2)) * u + static_cast<float>(sin(angle2)) * v;
        
        glm::vec3 p1 = base + radius * dir1;
        glm::vec3 p2 = base + radius * dir2;
        
        vertices->push_back(osg::Vec3(base.x, base.y, base.z));
        vertices->push_back(osg::Vec3(p2.x, p2.y, p2.z));
        vertices->push_back(osg::Vec3(p1.x, p1.y, p1.z));
        
        for (int j = 0; j < 3; ++j)
        {
            normals->push_back(osg::Vec3(-axis.x, -axis.y, -axis.z));
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalArray(normals.get());
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vertices->size()));
    
    return geometry;
}

std::vector<PickingFeature> Cone3D_Geo::extractFaceFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        glm::vec3 base = m_controlPoints[0].position;
        glm::vec3 apex = base + m_axis * m_height;
        
        // 侧面 - 圆锥侧面中心
        glm::vec3 sideCenter = (base + apex) * 0.5f;
        PickingFeature sideFeature(FeatureType::FACE, 0);
        sideFeature.center = osg::Vec3(sideCenter.x, sideCenter.y, sideCenter.z);
        sideFeature.size = 0.1f;
        features.push_back(sideFeature);
        
        // 底面
        PickingFeature bottomFeature(FeatureType::FACE, 1);
        bottomFeature.center = osg::Vec3(base.x, base.y, base.z);
        bottomFeature.size = 0.1f;
        features.push_back(bottomFeature);
    }
    
    return features;
}

std::vector<PickingFeature> Cone3D_Geo::extractEdgeFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        glm::vec3 base = m_controlPoints[0].position;
        
        // 底面边缘
        PickingFeature bottomEdge(FeatureType::EDGE, 0);
        bottomEdge.center = osg::Vec3(base.x, base.y, base.z);
        bottomEdge.size = 0.08f;
        features.push_back(bottomEdge);
    }
    
    return features;
}

std::vector<PickingFeature> Cone3D_Geo::extractVertexFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        glm::vec3 base = m_controlPoints[0].position;
        glm::vec3 apex = base + m_axis * m_height;
        
        // 底面中心
        PickingFeature baseVertex(FeatureType::VERTEX, 0);
        baseVertex.center = osg::Vec3(base.x, base.y, base.z);
        baseVertex.size = 0.05f;
        features.push_back(baseVertex);
        
        // 顶点
        PickingFeature apexVertex(FeatureType::VERTEX, 1);
        apexVertex.center = osg::Vec3(apex.x, apex.y, apex.z);
        apexVertex.size = 0.05f;
        features.push_back(apexVertex);
    }
    
    return features;
} 