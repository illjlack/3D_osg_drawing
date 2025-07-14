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
    , m_segments(16)
    , m_axis(0, 0, 1)
{
    m_geoType = Geo_Cone3D;
    // 确保基类正确初始化
    initialize();
}

void Cone3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        const auto& controlPoints = getControlPoints();
        
        if (controlPoints.size() == 2)
        {
            // 计算圆锥参数
            glm::vec3 diff = controlPoints[1].position - controlPoints[0].position;
            m_height = glm::length(diff);
            if (m_height > 0)
                m_axis = glm::normalize(diff);
            m_radius = m_height * 0.3f; // 默认半径为高度的30%
            completeDrawing();
        }
        
        updateGeometry();
        emit stateChanged(this);
    }
}

void Cone3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    const auto& controlPoints = getControlPoints();
    if (!isStateComplete() && controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void Cone3D_Geo::updateGeometry()
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



osg::ref_ptr<osg::Geometry> Cone3D_Geo::createGeometry()
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return nullptr;
    
    float radius = m_radius;
    float height = m_height;
    glm::vec3 axis = m_axis;
    glm::vec3 base = controlPoints[0].position;
    
    if (controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        glm::vec3 diff = getTempPoint().position - base;
        height = glm::length(diff);
        if (height > 0)
            axis = glm::normalize(diff);
        radius = height * 0.3f;
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

 

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void Cone3D_Geo::buildVertexGeometries()
{
    clearVertexGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    glm::vec3 center = controlPoints[0].position;
    float radius = m_radius;
    float height = m_height;
    int segments = m_segments;
    
    // 创建圆锥体顶点的几何体
    osg::ref_ptr<osg::Geometry> vertexGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 生成圆锥体顶点
    for (int i = 0; i <= segments; ++i)
    {
        float theta = 2.0f * M_PI * i / segments;
        float x = center.x + radius * cos(theta);
        float y = center.y + radius * sin(theta);
        
        // 底面顶点
        vertices->push_back(osg::Vec3(x, y, center.z - height * 0.5f));
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a));
    }
    
    // 顶点
    vertices->push_back(osg::Vec3(center.x, center.y, center.z + height * 0.5f));
    colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                               m_parameters.pointColor.b, m_parameters.pointColor.a));
    
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

void Cone3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    glm::vec3 center = controlPoints[0].position;
    float radius = m_radius;
    float height = m_height;
    int segments = m_segments;
    
    // 创建圆锥体边的几何体
    osg::ref_ptr<osg::Geometry> edgeGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 生成圆锥体边
    for (int i = 0; i < segments; ++i)
    {
        float theta1 = 2.0f * M_PI * i / segments;
        float theta2 = 2.0f * M_PI * (i + 1) / segments;
        
        float x1 = center.x + radius * cos(theta1);
        float y1 = center.y + radius * sin(theta1);
        float x2 = center.x + radius * cos(theta2);
        float y2 = center.y + radius * sin(theta2);
        
        // 底面边
        vertices->push_back(osg::Vec3(x1, y1, center.z - height * 0.5f));
        vertices->push_back(osg::Vec3(x2, y2, center.z - height * 0.5f));
        
        // 侧边
        vertices->push_back(osg::Vec3(x1, y1, center.z - height * 0.5f));
        vertices->push_back(osg::Vec3(center.x, center.y, center.z + height * 0.5f));
        
        for (int j = 0; j < 4; ++j)
        {
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

void Cone3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    glm::vec3 center = controlPoints[0].position;
    float radius = m_radius;
    float height = m_height;
    int segments = m_segments;
    
    // 创建圆锥体面的几何体
    osg::ref_ptr<osg::Geometry> faceGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    // 生成圆锥体面
    for (int i = 0; i < segments; ++i)
    {
        float theta1 = 2.0f * M_PI * i / segments;
        float theta2 = 2.0f * M_PI * (i + 1) / segments;
        
        float x1 = center.x + radius * cos(theta1);
        float y1 = center.y + radius * sin(theta1);
        float x2 = center.x + radius * cos(theta2);
        float y2 = center.y + radius * sin(theta2);
        
        float z1 = center.z - height * 0.5f;
        float z2 = center.z + height * 0.5f;
        
        // 侧面三角形
        vertices->push_back(osg::Vec3(x1, y1, z1));
        vertices->push_back(osg::Vec3(x2, y2, z1));
        vertices->push_back(osg::Vec3(center.x, center.y, z2));
        
        // 计算法向量
        glm::vec3 v1(x1 - center.x, y1 - center.y, z1 - center.z);
        glm::vec3 v2(x2 - center.x, y2 - center.y, z1 - center.z);
        glm::vec3 v3(0, 0, z2 - center.z);
        glm::vec3 normal = glm::normalize(glm::cross(v2 - v1, v3 - v1));
        
        for (int j = 0; j < 3; ++j)
        {
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
            colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                       m_parameters.fillColor.b, m_parameters.fillColor.a));
        }
    }
    
    // 底面
    for (int i = 0; i < segments; ++i)
    {
        float theta1 = 2.0f * M_PI * i / segments;
        float theta2 = 2.0f * M_PI * (i + 1) / segments;
        
        float x1 = center.x + radius * cos(theta1);
        float y1 = center.y + radius * sin(theta1);
        float x2 = center.x + radius * cos(theta2);
        float y2 = center.y + radius * sin(theta2);
        
        // 底面三角形
        vertices->push_back(osg::Vec3(center.x, center.y, center.z - height * 0.5f));
        vertices->push_back(osg::Vec3(x1, y1, center.z - height * 0.5f));
        vertices->push_back(osg::Vec3(x2, y2, center.z - height * 0.5f));
        
        for (int j = 0; j < 3; ++j)
        {
            normals->push_back(osg::Vec3(0, 0, -1)); // 底面法向量
            colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                       m_parameters.fillColor.b, m_parameters.fillColor.a));
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