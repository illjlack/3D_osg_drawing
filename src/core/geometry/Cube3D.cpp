#include "Cube3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <algorithm>

Cube3D_Geo::Cube3D_Geo()
    : m_size(1.0f)
{
    m_geoType = Geo_Cube3D;
}

void Cube3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 2)
        {
            // 计算正方体边长（取最大边）
            glm::vec3 diff = m_controlPoints[1].position - m_controlPoints[0].position;
            m_size = std::max({abs(diff.x), abs(diff.y), abs(diff.z)});
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Cube3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && m_controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void Cube3D_Geo::updateGeometry()
{
    updateOSGNode();
}

std::vector<FeatureType> Cube3D_Geo::getSupportedFeatureTypes() const
{
    return {FeatureType::FACE, FeatureType::EDGE, FeatureType::VERTEX};
}

osg::ref_ptr<osg::Geometry> Cube3D_Geo::createGeometry()
{
    if (m_controlPoints.empty())
        return nullptr;
    
    float size = m_size;
    glm::vec3 center = m_controlPoints[0].position;
    
    if (m_controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        glm::vec3 diff = getTempPoint().position - center;
        size = std::max({abs(diff.x), abs(diff.y), abs(diff.z)});
    }
    else if (m_controlPoints.size() == 2)
    {
        center = (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
    }
    
    // 创建正方体几何体（复用长方体代码，但用相同的尺寸）
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    float s = size * 0.5f;
    
    // 定义正方体的8个顶点
    std::vector<glm::vec3> cubeVertices = {
        center + glm::vec3(-s, -s, -s), // 0
        center + glm::vec3( s, -s, -s), // 1
        center + glm::vec3( s,  s, -s), // 2
        center + glm::vec3(-s,  s, -s), // 3
        center + glm::vec3(-s, -s,  s), // 4
        center + glm::vec3( s, -s,  s), // 5
        center + glm::vec3( s,  s,  s), // 6
        center + glm::vec3(-s,  s,  s)  // 7
    };
    
    // 定义6个面的顶点索引和法向量
    std::vector<std::vector<int>> faces = {
        {0, 1, 2, 3}, // 底面
        {4, 7, 6, 5}, // 顶面
        {0, 4, 5, 1}, // 前面
        {2, 6, 7, 3}, // 后面
        {0, 3, 7, 4}, // 左面
        {1, 5, 6, 2}  // 右面
    };
    
    std::vector<glm::vec3> faceNormals = {
        glm::vec3(0, 0, -1), // 底面
        glm::vec3(0, 0,  1), // 顶面
        glm::vec3(0, -1, 0), // 前面
        glm::vec3(0,  1, 0), // 后面
        glm::vec3(-1, 0, 0), // 左面
        glm::vec3( 1, 0, 0)  // 右面
    };
    
    Color3D color = isStateComplete() ? m_parameters.fillColor : 
                   Color3D(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                          m_parameters.fillColor.b, m_parameters.fillColor.a * 0.5f);
    
    // 添加每个面的顶点
    for (size_t f = 0; f < faces.size(); ++f)
    {
        const std::vector<int>& face = faces[f];
        const glm::vec3& normal = faceNormals[f];
        
        // 三角化每个面
        vertices->push_back(osg::Vec3(cubeVertices[face[0]].x, cubeVertices[face[0]].y, cubeVertices[face[0]].z));
        vertices->push_back(osg::Vec3(cubeVertices[face[1]].x, cubeVertices[face[1]].y, cubeVertices[face[1]].z));
        vertices->push_back(osg::Vec3(cubeVertices[face[2]].x, cubeVertices[face[2]].y, cubeVertices[face[2]].z));
        
        vertices->push_back(osg::Vec3(cubeVertices[face[0]].x, cubeVertices[face[0]].y, cubeVertices[face[0]].z));
        vertices->push_back(osg::Vec3(cubeVertices[face[2]].x, cubeVertices[face[2]].y, cubeVertices[face[2]].z));
        vertices->push_back(osg::Vec3(cubeVertices[face[3]].x, cubeVertices[face[3]].y, cubeVertices[face[3]].z));
        
        for (int i = 0; i < 6; ++i)
        {
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
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

std::vector<PickingFeature> Cube3D_Geo::extractFaceFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        glm::vec3 center = (m_controlPoints.size() == 1) ? m_controlPoints[0].position : 
                          (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
        float s = m_size * 0.5f;
        
        // 6个面Feature
        std::vector<glm::vec3> facePositions = {
            center + glm::vec3(0, 0, -s), // 底面
            center + glm::vec3(0, 0,  s), // 顶面
            center + glm::vec3(0, -s, 0), // 前面
            center + glm::vec3(0,  s, 0), // 后面
            center + glm::vec3(-s, 0, 0), // 左面
            center + glm::vec3( s, 0, 0)  // 右面
        };
        
        for (size_t i = 0; i < 6; ++i)
        {
            PickingFeature feature(FeatureType::FACE, static_cast<uint32_t>(i));
            feature.center = osg::Vec3(facePositions[i].x, facePositions[i].y, facePositions[i].z);
            feature.size = 0.1f;
            features.push_back(feature);
        }
    }
    
    return features;
}

std::vector<PickingFeature> Cube3D_Geo::extractEdgeFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        glm::vec3 center = (m_controlPoints.size() == 1) ? m_controlPoints[0].position : 
                          (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
        float s = m_size * 0.5f;
        
        // 12条边Feature
        std::vector<glm::vec3> edgePositions = {
            // 底面4条边
            center + glm::vec3(0, -s, -s), // 前底边
            center + glm::vec3(s, 0, -s),  // 右底边
            center + glm::vec3(0, s, -s),  // 后底边
            center + glm::vec3(-s, 0, -s), // 左底边
            // 顶面4条边
            center + glm::vec3(0, -s, s),  // 前顶边
            center + glm::vec3(s, 0, s),   // 右顶边
            center + glm::vec3(0, s, s),   // 后顶边
            center + glm::vec3(-s, 0, s),  // 左顶边
            // 垂直4条边
            center + glm::vec3(-s, -s, 0), // 左前边
            center + glm::vec3(s, -s, 0),  // 右前边
            center + glm::vec3(s, s, 0),   // 右后边
            center + glm::vec3(-s, s, 0)   // 左后边
        };
        
        for (size_t i = 0; i < 12; ++i)
        {
            PickingFeature feature(FeatureType::EDGE, static_cast<uint32_t>(i));
            feature.center = osg::Vec3(edgePositions[i].x, edgePositions[i].y, edgePositions[i].z);
            feature.size = 0.08f;
            features.push_back(feature);
        }
    }
    
    return features;
}

std::vector<PickingFeature> Cube3D_Geo::extractVertexFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        glm::vec3 center = (m_controlPoints.size() == 1) ? m_controlPoints[0].position : 
                          (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
        float s = m_size * 0.5f;
        
        // 8个顶点Feature
        std::vector<glm::vec3> vertexPositions = {
            center + glm::vec3(-s, -s, -s), // 0
            center + glm::vec3( s, -s, -s), // 1
            center + glm::vec3( s,  s, -s), // 2
            center + glm::vec3(-s,  s, -s), // 3
            center + glm::vec3(-s, -s,  s), // 4
            center + glm::vec3( s, -s,  s), // 5
            center + glm::vec3( s,  s,  s), // 6
            center + glm::vec3(-s,  s,  s)  // 7
        };
        
        for (size_t i = 0; i < 8; ++i)
        {
            PickingFeature feature(FeatureType::VERTEX, static_cast<uint32_t>(i));
            feature.center = osg::Vec3(vertexPositions[i].x, vertexPositions[i].y, vertexPositions[i].z);
            feature.size = 0.05f;
            features.push_back(feature);
        }
    }
    
    return features;
} 