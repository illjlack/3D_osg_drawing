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
    // 确保基类正确初始化
    initialize();
}

void Torus3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        const auto& controlPoints = getControlPoints();
        
        if (controlPoints.size() == 2)
        {
            // 计算环面参数
            float distance = glm::length(controlPoints[1].position - controlPoints[0].position);
            m_majorRadius = distance * 0.7f; // 主半径
            m_minorRadius = distance * 0.3f; // 副半径
            completeDrawing();
        }
        
        updateGeometry();
        emit stateChanged(this);
    }
}

void Torus3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    const auto& controlPoints = getControlPoints();
    if (!isStateComplete() && controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void Torus3D_Geo::updateGeometry()
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



osg::ref_ptr<osg::Geometry> Torus3D_Geo::createGeometry()
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return nullptr;
    
    float majorRadius = m_majorRadius;
    float minorRadius = m_minorRadius;
    glm::vec3 center = controlPoints[0].position;
    
    if (controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        float distance = glm::length(getTempPoint().position - center);
        majorRadius = distance * 0.7f;
        minorRadius = distance * 0.3f;
    }
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    int majorSegments = static_cast<int>(m_parameters.subdivisionLevel);
    if (majorSegments < 8) majorSegments = 16; // 默认16段
    int minorSegments = majorSegments / 2;
    if (minorSegments < 4) minorSegments = 8; // 默认8段
    
    // 环面轴向量（默认为Z轴向上）
    glm::vec3 axis = m_axis;
    
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

void Torus3D_Geo::buildVertexGeometries()
{
    clearVertexGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 创建圆环顶点的几何体
    osg::ref_ptr<osg::Geometry> vertexGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 生成圆环顶点
    glm::vec3 center = controlPoints[0].position;
    int majorSegments = static_cast<int>(m_parameters.subdivisionLevel);
    int minorSegments = majorSegments / 2;
    if (majorSegments < 8) majorSegments = 16;
    if (minorSegments < 4) minorSegments = 8;
    
    for (int i = 0; i <= majorSegments; ++i)
    {
        float majorAngle = 2.0f * M_PI * i / majorSegments;
        for (int j = 0; j <= minorSegments; ++j)
        {
            float minorAngle = 2.0f * M_PI * j / minorSegments;
            
            float x = center.x + (m_majorRadius + m_minorRadius * cos(minorAngle)) * cos(majorAngle);
            float y = center.y + (m_majorRadius + m_minorRadius * cos(minorAngle)) * sin(majorAngle);
            float z = center.z + m_minorRadius * sin(minorAngle);
            
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

void Torus3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 创建圆环边的几何体
    osg::ref_ptr<osg::Geometry> edgeGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 生成圆环边
    glm::vec3 center = controlPoints[0].position;
    int majorSegments = static_cast<int>(m_parameters.subdivisionLevel);
    int minorSegments = majorSegments / 2;
    if (majorSegments < 8) majorSegments = 16;
    if (minorSegments < 4) minorSegments = 8;
    
    // 主圆环的边
    for (int i = 0; i < majorSegments; ++i)
    {
        float majorAngle1 = 2.0f * M_PI * i / majorSegments;
        float majorAngle2 = 2.0f * M_PI * (i + 1) / majorSegments;
        
        for (int j = 0; j < minorSegments; ++j)
        {
            float minorAngle = 2.0f * M_PI * j / minorSegments;
            
            float x1 = center.x + (m_majorRadius + m_minorRadius * cos(minorAngle)) * cos(majorAngle1);
            float y1 = center.y + (m_majorRadius + m_minorRadius * cos(minorAngle)) * sin(majorAngle1);
            float z1 = center.z + m_minorRadius * sin(minorAngle);
            
            float x2 = center.x + (m_majorRadius + m_minorRadius * cos(minorAngle)) * cos(majorAngle2);
            float y2 = center.y + (m_majorRadius + m_minorRadius * cos(minorAngle)) * sin(majorAngle2);
            float z2 = center.z + m_minorRadius * sin(minorAngle);
            
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

void Torus3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 创建圆环面的几何体
    osg::ref_ptr<osg::Geometry> faceGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 生成圆环面
    glm::vec3 center = controlPoints[0].position;
    int majorSegments = static_cast<int>(m_parameters.subdivisionLevel);
    int minorSegments = majorSegments / 2;
    if (majorSegments < 8) majorSegments = 16;
    if (minorSegments < 4) minorSegments = 8;
    
    for (int i = 0; i <= majorSegments; ++i)
    {
        float majorAngle = 2.0f * M_PI * i / majorSegments;
        for (int j = 0; j <= minorSegments; ++j)
        {
            float minorAngle = 2.0f * M_PI * j / minorSegments;
            
            float x = center.x + (m_majorRadius + m_minorRadius * cos(minorAngle)) * cos(majorAngle);
            float y = center.y + (m_majorRadius + m_minorRadius * cos(minorAngle)) * sin(majorAngle);
            float z = center.z + m_minorRadius * sin(minorAngle);
            
            vertices->push_back(osg::Vec3(x, y, z));
            colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                       m_parameters.fillColor.b, m_parameters.fillColor.a));
        }
    }
    
    faceGeometry->setVertexArray(vertices.get());
    faceGeometry->setColorArray(colors.get());
    faceGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用三角形绘制
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    for (int i = 0; i < majorSegments; ++i)
    {
        for (int j = 0; j < minorSegments; ++j)
        {
            int current = i * (minorSegments + 1) + j;
            int next = (i + 1) * (minorSegments + 1) + j;
            int currentNext = i * (minorSegments + 1) + j + 1;
            int nextNext = (i + 1) * (minorSegments + 1) + j + 1;
            
            // 第一个三角形
            indices->push_back(current);
            indices->push_back(next);
            indices->push_back(currentNext);
            
            // 第二个三角形
            indices->push_back(currentNext);
            indices->push_back(next);
            indices->push_back(nextNext);
        }
    }
    
    faceGeometry->addPrimitiveSet(indices.get());
    
    addFaceGeometry(faceGeometry.get());
}

 