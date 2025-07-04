#include "Cylinder3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Cylinder3D_Geo::Cylinder3D_Geo()
    : m_radius(1.0f)
    , m_height(2.0f)
    , m_axis(0, 0, 1)
{
    m_geoType = Geo_Cylinder3D;
}

void Cylinder3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 2)
        {
            // 计算圆柱参数
            glm::vec3 diff = m_controlPoints[1].position - m_controlPoints[0].position;
            m_height = glm::length(diff);
            if (m_height > 0)
                m_axis = glm::normalize(diff);
            m_radius = m_height * 0.3f; // 默认半径为高度的30%
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Cylinder3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && m_controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        updateGeometry();
    }
}

void Cylinder3D_Geo::updateGeometry()
{
    updateOSGNode();
}

std::vector<FeatureType> Cylinder3D_Geo::getSupportedFeatureTypes() const
{
    return {FeatureType::FACE, FeatureType::EDGE, FeatureType::VERTEX};
}

osg::ref_ptr<osg::Geometry> Cylinder3D_Geo::createGeometry()
{
    if (m_controlPoints.empty())
        return nullptr;
    
    float radius = m_radius;
    float height = m_height;
    glm::vec3 axis = m_axis;
    glm::vec3 center = m_controlPoints[0].position;
    
    if (m_controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        glm::vec3 diff = getTempPoint().position - center;
        height = glm::length(diff);
        if (height > 0)
            axis = glm::normalize(diff);
        radius = height * 0.3f;
    }
    else if (m_controlPoints.size() == 2)
    {
        center = (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
    }
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    int segments = static_cast<int>(m_parameters.subdivisionLevel);
    if (segments < 8) segments = 16; // 默认16段
    
    // 计算圆柱的两个圆面中心
    glm::vec3 bottom = center - axis * (height * 0.5f);
    glm::vec3 top = center + axis * (height * 0.5f);
    
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
    
    // 生成圆柱侧面
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1 = static_cast<float>(cos(angle1)) * u + static_cast<float>(sin(angle1)) * v;
        glm::vec3 dir2 = static_cast<float>(cos(angle2)) * u + static_cast<float>(sin(angle2)) * v;
        
        glm::vec3 p1_bottom = bottom + radius * dir1;
        glm::vec3 p2_bottom = bottom + radius * dir2;
        glm::vec3 p1_top = top + radius * dir1;
        glm::vec3 p2_top = top + radius * dir2;
        
        // 第一个三角形
        vertices->push_back(osg::Vec3(p1_bottom.x, p1_bottom.y, p1_bottom.z));
        vertices->push_back(osg::Vec3(p2_bottom.x, p2_bottom.y, p2_bottom.z));
        vertices->push_back(osg::Vec3(p1_top.x, p1_top.y, p1_top.z));
        
        normals->push_back(osg::Vec3(dir1.x, dir1.y, dir1.z));
        normals->push_back(osg::Vec3(dir2.x, dir2.y, dir2.z));
        normals->push_back(osg::Vec3(dir1.x, dir1.y, dir1.z));
        
        // 第二个三角形
        vertices->push_back(osg::Vec3(p2_bottom.x, p2_bottom.y, p2_bottom.z));
        vertices->push_back(osg::Vec3(p2_top.x, p2_top.y, p2_top.z));
        vertices->push_back(osg::Vec3(p1_top.x, p1_top.y, p1_top.z));
        
        normals->push_back(osg::Vec3(dir2.x, dir2.y, dir2.z));
        normals->push_back(osg::Vec3(dir2.x, dir2.y, dir2.z));
        normals->push_back(osg::Vec3(dir1.x, dir1.y, dir1.z));
        
        // 添加颜色
        for (int j = 0; j < 6; ++j)
        {
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
    
    // 生成底面和顶面
    // 底面
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1 = static_cast<float>(cos(angle1)) * u + static_cast<float>(sin(angle1)) * v;
        glm::vec3 dir2 = static_cast<float>(cos(angle2)) * u + static_cast<float>(sin(angle2)) * v;
        
        glm::vec3 p1 = bottom + radius * dir1;
        glm::vec3 p2 = bottom + radius * dir2;
        
        vertices->push_back(osg::Vec3(bottom.x, bottom.y, bottom.z));
        vertices->push_back(osg::Vec3(p2.x, p2.y, p2.z));
        vertices->push_back(osg::Vec3(p1.x, p1.y, p1.z));
        
        for (int j = 0; j < 3; ++j)
        {
            normals->push_back(osg::Vec3(-axis.x, -axis.y, -axis.z));
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
    
    // 顶面
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1 = static_cast<float>(cos(angle1)) * u + static_cast<float>(sin(angle1)) * v;
        glm::vec3 dir2 = static_cast<float>(cos(angle2)) * u + static_cast<float>(sin(angle2)) * v;
        
        glm::vec3 p1 = top + radius * dir1;
        glm::vec3 p2 = top + radius * dir2;
        
        vertices->push_back(osg::Vec3(top.x, top.y, top.z));
        vertices->push_back(osg::Vec3(p1.x, p1.y, p1.z));
        vertices->push_back(osg::Vec3(p2.x, p2.y, p2.z));
        
        for (int j = 0; j < 3; ++j)
        {
            normals->push_back(osg::Vec3(axis.x, axis.y, axis.z));
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

std::vector<PickingFeature> Cylinder3D_Geo::extractFaceFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        glm::vec3 center = (m_controlPoints.size() == 1) ? m_controlPoints[0].position : 
                          (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
        
        // 侧面 - 圆柱体中心
        PickingFeature sideFeature(FeatureType::FACE, 0);
        sideFeature.center = osg::Vec3(center.x, center.y, center.z);
        sideFeature.size = 0.1f;
        features.push_back(sideFeature);
        
        // 底面
        glm::vec3 bottom = center - m_axis * (m_height * 0.5f);
        PickingFeature bottomFeature(FeatureType::FACE, 1);
        bottomFeature.center = osg::Vec3(bottom.x, bottom.y, bottom.z);
        bottomFeature.size = 0.1f;
        features.push_back(bottomFeature);
        
        // 顶面
        glm::vec3 top = center + m_axis * (m_height * 0.5f);
        PickingFeature topFeature(FeatureType::FACE, 2);
        topFeature.center = osg::Vec3(top.x, top.y, top.z);
        topFeature.size = 0.1f;
        features.push_back(topFeature);
    }
    
    return features;
}

std::vector<PickingFeature> Cylinder3D_Geo::extractEdgeFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        glm::vec3 center = (m_controlPoints.size() == 1) ? m_controlPoints[0].position : 
                          (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
        
        glm::vec3 bottom = center - m_axis * (m_height * 0.5f);
        glm::vec3 top = center + m_axis * (m_height * 0.5f);
        
        // 底面边缘
        PickingFeature bottomEdge(FeatureType::EDGE, 0);
        bottomEdge.center = osg::Vec3(bottom.x, bottom.y, bottom.z);
        bottomEdge.size = 0.08f;
        features.push_back(bottomEdge);
        
        // 顶面边缘
        PickingFeature topEdge(FeatureType::EDGE, 1);
        topEdge.center = osg::Vec3(top.x, top.y, top.z);
        topEdge.size = 0.08f;
        features.push_back(topEdge);
    }
    
    return features;
}

std::vector<PickingFeature> Cylinder3D_Geo::extractVertexFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        glm::vec3 center = (m_controlPoints.size() == 1) ? m_controlPoints[0].position : 
                          (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
        
        // 圆柱体的"顶点"是底面和顶面的中心点
        glm::vec3 bottom = center - m_axis * (m_height * 0.5f);
        glm::vec3 top = center + m_axis * (m_height * 0.5f);
        
        // 底面中心
        PickingFeature bottomVertex(FeatureType::VERTEX, 0);
        bottomVertex.center = osg::Vec3(bottom.x, bottom.y, bottom.z);
        bottomVertex.size = 0.05f;
        features.push_back(bottomVertex);
        
        // 顶面中心
        PickingFeature topVertex(FeatureType::VERTEX, 1);
        topVertex.center = osg::Vec3(top.x, top.y, top.z);
        topVertex.size = 0.05f;
        features.push_back(topVertex);
    }
    
    return features;
} 
