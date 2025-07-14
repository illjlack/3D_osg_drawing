#include "Sphere3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Sphere3D_Geo::Sphere3D_Geo()
    : m_radius(1.0f)
    , m_segments(16)
{
    m_geoType = Geo_Sphere3D;
    // 确保基类正确初始化
    initialize();
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
    // 清除点线面节点
    clearVertexGeometries();
    clearEdgeGeometries();
    clearFaceGeometries();
    
    updateOSGNode();
    
    // 构建点线面几何体
    buildVertexGeometries();
    buildEdgeGeometries();
    buildFaceGeometries();
    
    // 更新可见性
    updateFeatureVisibility();
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







 

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void Sphere3D_Geo::buildVertexGeometries()
{
    clearVertexGeometries();
    
    if (m_controlPoints.empty())
        return;
    
    glm::vec3 center = m_controlPoints[0].position;
    float radius = m_radius;
    int segments = m_segments;
    
    // 创建球体顶点的几何体
    osg::ref_ptr<osg::Geometry> vertexGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 生成球体顶点
    for (int i = 0; i <= segments; ++i)
    {
        float phi = M_PI * i / segments;
        for (int j = 0; j <= segments; ++j)
        {
            float theta = 2.0f * M_PI * j / segments;
            
            float x = center.x + radius * sin(phi) * cos(theta);
            float y = center.y + radius * sin(phi) * sin(theta);
            float z = center.z + radius * cos(phi);
            
            vertices->push_back(osg::Vec3(x, y, z));
            colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                       m_parameters.pointColor.b, m_parameters.pointColor.a));
        }
    }
    
    vertexGeometry->setVertexArray(vertices.get());
    vertexGeometry->setColorArray(colors.get());
    vertexGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用点绘制
    vertexGeometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));
    
    // 设置点大小
    osg::ref_ptr<osg::StateSet> stateSet = vertexGeometry->getOrCreateStateSet();
    osg::ref_ptr<osg::Point> point = new osg::Point();
    point->setSize(m_parameters.pointSize);
    stateSet->setAttribute(point.get());
    
    addVertexGeometry(vertexGeometry.get());
}

void Sphere3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    if (m_controlPoints.empty())
        return;
    
    glm::vec3 center = m_controlPoints[0].position;
    float radius = m_radius;
    int segments = m_segments;
    
    // 创建球体边的几何体
    osg::ref_ptr<osg::Geometry> edgeGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 生成球体边（经纬线）
    for (int i = 0; i <= segments; ++i)
    {
        float phi = M_PI * i / segments;
        for (int j = 0; j < segments; ++j)
        {
            float theta1 = 2.0f * M_PI * j / segments;
            float theta2 = 2.0f * M_PI * (j + 1) / segments;
            
            // 经线
            float x1 = center.x + radius * sin(phi) * cos(theta1);
            float y1 = center.y + radius * sin(phi) * sin(theta1);
            float z1 = center.z + radius * cos(phi);
            
            float x2 = center.x + radius * sin(phi) * cos(theta2);
            float y2 = center.y + radius * sin(phi) * sin(theta2);
            float z2 = center.z + radius * cos(phi);
            
            vertices->push_back(osg::Vec3(x1, y1, z1));
            vertices->push_back(osg::Vec3(x2, y2, z2));
            
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a));
        }
    }
    
    // 纬线
    for (int j = 0; j < segments; ++j)
    {
        float theta = 2.0f * M_PI * j / segments;
        for (int i = 0; i < segments; ++i)
        {
            float phi1 = M_PI * i / segments;
            float phi2 = M_PI * (i + 1) / segments;
            
            float x1 = center.x + radius * sin(phi1) * cos(theta);
            float y1 = center.y + radius * sin(phi1) * sin(theta);
            float z1 = center.z + radius * cos(phi1);
            
            float x2 = center.x + radius * sin(phi2) * cos(theta);
            float y2 = center.y + radius * sin(phi2) * sin(theta);
            float z2 = center.z + radius * cos(phi2);
            
            vertices->push_back(osg::Vec3(x1, y1, z1));
            vertices->push_back(osg::Vec3(x2, y2, z2));
            
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a));
        }
    }
    
    edgeGeometry->setVertexArray(vertices.get());
    edgeGeometry->setColorArray(colors.get());
    edgeGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用线绘制
    edgeGeometry->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, vertices->size()));
    
    // 设置线宽
    osg::ref_ptr<osg::StateSet> stateSet = edgeGeometry->getOrCreateStateSet();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth();
    lineWidth->setWidth(m_parameters.lineWidth);
    stateSet->setAttribute(lineWidth.get());
    
    addEdgeGeometry(edgeGeometry.get());
}

void Sphere3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    
    if (m_controlPoints.empty())
        return;
    
    glm::vec3 center = m_controlPoints[0].position;
    float radius = m_radius;
    int segments = m_segments;
    
    // 创建球体面的几何体
    osg::ref_ptr<osg::Geometry> faceGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    // 生成球体面
    for (int i = 0; i < segments; ++i)
    {
        float phi1 = M_PI * i / segments;
        float phi2 = M_PI * (i + 1) / segments;
        
        for (int j = 0; j < segments; ++j)
        {
            float theta1 = 2.0f * M_PI * j / segments;
            float theta2 = 2.0f * M_PI * (j + 1) / segments;
            
            // 计算四个顶点
            float x1 = center.x + radius * sin(phi1) * cos(theta1);
            float y1 = center.y + radius * sin(phi1) * sin(theta1);
            float z1 = center.z + radius * cos(phi1);
            
            float x2 = center.x + radius * sin(phi1) * cos(theta2);
            float y2 = center.y + radius * sin(phi1) * sin(theta2);
            float z2 = center.z + radius * cos(phi1);
            
            float x3 = center.x + radius * sin(phi2) * cos(theta2);
            float y3 = center.y + radius * sin(phi2) * sin(theta2);
            float z3 = center.z + radius * cos(phi2);
            
            float x4 = center.x + radius * sin(phi2) * cos(theta1);
            float y4 = center.y + radius * sin(phi2) * sin(theta1);
            float z4 = center.z + radius * cos(phi2);
            
            // 添加两个三角形
            vertices->push_back(osg::Vec3(x1, y1, z1));
            vertices->push_back(osg::Vec3(x2, y2, z2));
            vertices->push_back(osg::Vec3(x3, y3, z3));
            
            vertices->push_back(osg::Vec3(x1, y1, z1));
            vertices->push_back(osg::Vec3(x3, y3, z3));
            vertices->push_back(osg::Vec3(x4, y4, z4));
            
            // 计算法向量
            glm::vec3 v1(x1 - center.x, y1 - center.y, z1 - center.z);
            glm::vec3 v2(x2 - center.x, y2 - center.y, z2 - center.z);
            glm::vec3 v3(x3 - center.x, y3 - center.y, z3 - center.z);
            glm::vec3 normal = glm::normalize(glm::cross(v2 - v1, v3 - v1));
            
            for (int k = 0; k < 6; ++k)
            {
                normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
                colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                           m_parameters.fillColor.b, m_parameters.fillColor.a));
            }
        }
    }
    
    faceGeometry->setVertexArray(vertices.get());
    faceGeometry->setColorArray(colors.get());
    faceGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    faceGeometry->setNormalArray(normals.get());
    faceGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用三角面绘制
    faceGeometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vertices->size()));
    
    addFaceGeometry(faceGeometry.get());
} 