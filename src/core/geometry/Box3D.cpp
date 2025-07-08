#include "Box3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include <algorithm>

Box3D_Geo::Box3D_Geo()
    : m_size(1.0f, 1.0f, 1.0f)
{
    m_geoType = Geo_Box3D;
}

void Box3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 2)
        {
            // 计算长方体尺寸
            glm::vec3 diff = m_controlPoints[1].position - m_controlPoints[0].position;
            m_size = glm::abs(diff);
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Box3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && m_controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void Box3D_Geo::updateGeometry()
{
    updateOSGNode();
}

std::vector<FeatureType> Box3D_Geo::getSupportedFeatureTypes() const
{
    return {FeatureType::FACE, FeatureType::EDGE, FeatureType::VERTEX};
}

osg::ref_ptr<osg::Geometry> Box3D_Geo::createGeometry()
{
    if (m_controlPoints.empty())
        return nullptr;
    
    glm::vec3 size = m_size;
    glm::vec3 center = m_controlPoints[0].position;
    
    if (m_controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        glm::vec3 diff = getTempPoint().position - center;
        size = glm::abs(diff);
    }
    else if (m_controlPoints.size() == 2)
    {
        center = (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
    }
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    float sx = size.x * 0.5f;
    float sy = size.y * 0.5f;
    float sz = size.z * 0.5f;
    
    // 定义长方体的8个顶点
    std::vector<glm::vec3> boxVertices = {
        center + glm::vec3(-sx, -sy, -sz), // 0
        center + glm::vec3( sx, -sy, -sz), // 1
        center + glm::vec3( sx,  sy, -sz), // 2
        center + glm::vec3(-sx,  sy, -sz), // 3
        center + glm::vec3(-sx, -sy,  sz), // 4
        center + glm::vec3( sx, -sy,  sz), // 5
        center + glm::vec3( sx,  sy,  sz), // 6
        center + glm::vec3(-sx,  sy,  sz)  // 7
    };
    
    // 定义6个面的顶点索引和法向量
    std::vector<std::vector<int>> faces = {
        {0, 1, 2, 3}, // 底面 (z = -sz)
        {4, 7, 6, 5}, // 顶面 (z = sz)
        {0, 4, 5, 1}, // 前面 (y = -sy)
        {2, 6, 7, 3}, // 后面 (y = sy)
        {0, 3, 7, 4}, // 左面 (x = -sx)
        {1, 5, 6, 2}  // 右面 (x = sx)
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
        
        // 三角化每个面 (四边形 -> 两个三角形)
        // 第一个三角形
        vertices->push_back(osg::Vec3(boxVertices[face[0]].x, boxVertices[face[0]].y, boxVertices[face[0]].z));
        vertices->push_back(osg::Vec3(boxVertices[face[1]].x, boxVertices[face[1]].y, boxVertices[face[1]].z));
        vertices->push_back(osg::Vec3(boxVertices[face[2]].x, boxVertices[face[2]].y, boxVertices[face[2]].z));
        
        // 第二个三角形
        vertices->push_back(osg::Vec3(boxVertices[face[0]].x, boxVertices[face[0]].y, boxVertices[face[0]].z));
        vertices->push_back(osg::Vec3(boxVertices[face[2]].x, boxVertices[face[2]].y, boxVertices[face[2]].z));
        vertices->push_back(osg::Vec3(boxVertices[face[3]].x, boxVertices[face[3]].y, boxVertices[face[3]].z));
        
        // 法向量和颜色
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

std::vector<PickingFeature> Box3D_Geo::extractFaceFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        glm::vec3 center = m_controlPoints[0].position;
        if (m_controlPoints.size() == 2)
        {
            center = (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
        }
        
        // 长方体有6个面
        std::vector<glm::vec3> faceNormals = {
            glm::vec3(0, 0, -1), // 底面
            glm::vec3(0, 0,  1), // 顶面
            glm::vec3(0, -1, 0), // 前面
            glm::vec3(0,  1, 0), // 后面
            glm::vec3(-1, 0, 0), // 左面
            glm::vec3( 1, 0, 0)  // 右面
        };
        
        float sx = m_size.x * 0.5f;
        float sy = m_size.y * 0.5f;
        float sz = m_size.z * 0.5f;
        
        std::vector<glm::vec3> faceOffsets = {
            glm::vec3(0, 0, -sz), // 底面中心
            glm::vec3(0, 0,  sz), // 顶面中心
            glm::vec3(0, -sy, 0), // 前面中心
            glm::vec3(0,  sy, 0), // 后面中心
            glm::vec3(-sx, 0, 0), // 左面中心
            glm::vec3( sx, 0, 0)  // 右面中心
        };
        
        for (size_t i = 0; i < 6; ++i)
        {
            PickingFeature feature(FeatureType::FACE, static_cast<uint32_t>(i));
            glm::vec3 faceCenter = center + faceOffsets[i];
            feature.center = osg::Vec3(faceCenter.x, faceCenter.y, faceCenter.z);
            feature.size = 0.1f;
            features.push_back(feature);
        }
    }
    
    return features;
}

std::vector<PickingFeature> Box3D_Geo::extractEdgeFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        glm::vec3 center = m_controlPoints[0].position;
        if (m_controlPoints.size() == 2)
        {
            center = (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
        }
        
        float sx = m_size.x * 0.5f;
        float sy = m_size.y * 0.5f;
        float sz = m_size.z * 0.5f;
        
        // 长方体有12条边，为简化只提取主要的4条边
        std::vector<glm::vec3> edgeCenters = {
            center + glm::vec3(0, -sy, -sz), // 底面前边中心
            center + glm::vec3(0,  sy, -sz), // 底面后边中心
            center + glm::vec3(-sx, 0, -sz), // 底面左边中心
            center + glm::vec3( sx, 0, -sz), // 底面右边中心
        };
        
        for (size_t i = 0; i < edgeCenters.size(); ++i)
        {
            PickingFeature feature(FeatureType::EDGE, static_cast<uint32_t>(i));
            feature.center = osg::Vec3(edgeCenters[i].x, edgeCenters[i].y, edgeCenters[i].z);
            feature.size = 0.08f;
            features.push_back(feature);
        }
    }
    
    return features;
}

std::vector<PickingFeature> Box3D_Geo::extractVertexFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (m_controlPoints.size() >= 1)
    {
        glm::vec3 center = m_controlPoints[0].position;
        if (m_controlPoints.size() == 2)
        {
            center = (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
        }
        
        float sx = m_size.x * 0.5f;
        float sy = m_size.y * 0.5f;
        float sz = m_size.z * 0.5f;
        
        // 长方体的8个顶点
        std::vector<glm::vec3> boxVertices = {
            center + glm::vec3(-sx, -sy, -sz), // 0
            center + glm::vec3( sx, -sy, -sz), // 1
            center + glm::vec3( sx,  sy, -sz), // 2
            center + glm::vec3(-sx,  sy, -sz), // 3
            center + glm::vec3(-sx, -sy,  sz), // 4
            center + glm::vec3( sx, -sy,  sz), // 5
            center + glm::vec3( sx,  sy,  sz), // 6
            center + glm::vec3(-sx,  sy,  sz)  // 7
        };
        
        for (size_t i = 0; i < boxVertices.size(); ++i)
        {
            PickingFeature feature(FeatureType::VERTEX, static_cast<uint32_t>(i));
            feature.center = osg::Vec3(boxVertices[i].x, boxVertices[i].y, boxVertices[i].z);
            feature.size = 0.05f;
            features.push_back(feature);
        }
    }
    
    return features;
} 