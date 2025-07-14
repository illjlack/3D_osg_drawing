#include "Box3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include <algorithm>

Box3D_Geo::Box3D_Geo()
    : m_size(1.0f, 1.0f, 1.0f)
{
    m_geoType = Geo_Box3D;
    // 确保基类正确初始化
    initialize();
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





 

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void Box3D_Geo::buildVertexGeometries()
{
    clearVertexGeometries();
    
    if (m_controlPoints.empty())
        return;
    
    glm::vec3 center = (m_controlPoints.size() == 1) ? m_controlPoints[0].position : 
                      (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
    
    glm::vec3 size = m_size;
    if (size.x <= 0.001f || size.y <= 0.001f || size.z <= 0.001f)
        size = glm::vec3(1.0f);
    
    glm::vec3 halfSize = size * 0.5f;
    
    // 创建长方体8个顶点的几何体
    osg::ref_ptr<osg::Geometry> vertexGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 长方体的8个顶点
    std::vector<glm::vec3> boxVertices = {
        center + glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z), // 0
        center + glm::vec3( halfSize.x, -halfSize.y, -halfSize.z), // 1
        center + glm::vec3( halfSize.x,  halfSize.y, -halfSize.z), // 2
        center + glm::vec3(-halfSize.x,  halfSize.y, -halfSize.z), // 3
        center + glm::vec3(-halfSize.x, -halfSize.y,  halfSize.z), // 4
        center + glm::vec3( halfSize.x, -halfSize.y,  halfSize.z), // 5
        center + glm::vec3( halfSize.x,  halfSize.y,  halfSize.z), // 6
        center + glm::vec3(-halfSize.x,  halfSize.y,  halfSize.z)  // 7
    };
    
    // 添加顶点和颜色
    for (const auto& vertex : boxVertices)
    {
        vertices->push_back(osg::Vec3(vertex.x, vertex.y, vertex.z));
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a));
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

void Box3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    if (m_controlPoints.empty())
        return;
    
    glm::vec3 center = (m_controlPoints.size() == 1) ? m_controlPoints[0].position : 
                      (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
    
    glm::vec3 size = m_size;
    if (size.x <= 0.001f || size.y <= 0.001f || size.z <= 0.001f)
        size = glm::vec3(1.0f);
    
    glm::vec3 halfSize = size * 0.5f;
    
    // 创建长方体12条边的几何体
    osg::ref_ptr<osg::Geometry> edgeGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 长方体的8个顶点
    std::vector<glm::vec3> boxVertices = {
        center + glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z), // 0
        center + glm::vec3( halfSize.x, -halfSize.y, -halfSize.z), // 1
        center + glm::vec3( halfSize.x,  halfSize.y, -halfSize.z), // 2
        center + glm::vec3(-halfSize.x,  halfSize.y, -halfSize.z), // 3
        center + glm::vec3(-halfSize.x, -halfSize.y,  halfSize.z), // 4
        center + glm::vec3( halfSize.x, -halfSize.y,  halfSize.z), // 5
        center + glm::vec3( halfSize.x,  halfSize.y,  halfSize.z), // 6
        center + glm::vec3(-halfSize.x,  halfSize.y,  halfSize.z)  // 7
    };
    
    // 12条边的顶点索引
    std::vector<std::pair<int, int>> edges = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, // 底面4条边
        {4, 5}, {5, 6}, {6, 7}, {7, 4}, // 顶面4条边
        {0, 4}, {1, 5}, {2, 6}, {3, 7}  // 垂直4条边
    };
    
    // 添加每条边的顶点
    for (const auto& edge : edges)
    {
        const glm::vec3& v1 = boxVertices[edge.first];
        const glm::vec3& v2 = boxVertices[edge.second];
        
        vertices->push_back(osg::Vec3(v1.x, v1.y, v1.z));
        vertices->push_back(osg::Vec3(v2.x, v2.y, v2.z));
        
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
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

void Box3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    
    if (m_controlPoints.empty())
        return;
    
    glm::vec3 center = (m_controlPoints.size() == 1) ? m_controlPoints[0].position : 
                      (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
    
    glm::vec3 size = m_size;
    if (size.x <= 0.001f || size.y <= 0.001f || size.z <= 0.001f)
        size = glm::vec3(1.0f);
    
    glm::vec3 halfSize = size * 0.5f;
    
    // 创建长方体6个面的几何体
    osg::ref_ptr<osg::Geometry> faceGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    // 长方体的8个顶点
    std::vector<glm::vec3> boxVertices = {
        center + glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z), // 0
        center + glm::vec3( halfSize.x, -halfSize.y, -halfSize.z), // 1
        center + glm::vec3( halfSize.x,  halfSize.y, -halfSize.z), // 2
        center + glm::vec3(-halfSize.x,  halfSize.y, -halfSize.z), // 3
        center + glm::vec3(-halfSize.x, -halfSize.y,  halfSize.z), // 4
        center + glm::vec3( halfSize.x, -halfSize.y,  halfSize.z), // 5
        center + glm::vec3( halfSize.x,  halfSize.y,  halfSize.z), // 6
        center + glm::vec3(-halfSize.x,  halfSize.y,  halfSize.z)  // 7
    };
    
    // 6个面的顶点索引和法向量
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
    
    // 添加每个面的顶点
    for (size_t f = 0; f < faces.size(); ++f)
    {
        const std::vector<int>& face = faces[f];
        const glm::vec3& normal = faceNormals[f];
        
        // 三角化每个面
        vertices->push_back(osg::Vec3(boxVertices[face[0]].x, boxVertices[face[0]].y, boxVertices[face[0]].z));
        vertices->push_back(osg::Vec3(boxVertices[face[1]].x, boxVertices[face[1]].y, boxVertices[face[1]].z));
        vertices->push_back(osg::Vec3(boxVertices[face[2]].x, boxVertices[face[2]].y, boxVertices[face[2]].z));
        
        vertices->push_back(osg::Vec3(boxVertices[face[0]].x, boxVertices[face[0]].y, boxVertices[face[0]].z));
        vertices->push_back(osg::Vec3(boxVertices[face[2]].x, boxVertices[face[2]].y, boxVertices[face[2]].z));
        vertices->push_back(osg::Vec3(boxVertices[face[3]].x, boxVertices[face[3]].y, boxVertices[face[3]].z));
        
        for (int i = 0; i < 6; ++i)
        {
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
            colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                       m_parameters.fillColor.b, m_parameters.fillColor.a));
        }
    }
    
    faceGeometry->setVertexArray(vertices.get());
    faceGeometry->setColorArray(colors.get());
    faceGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    faceGeometry->setNormalArray(normals.get());
    faceGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    faceGeometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vertices->size()));
    
    addFaceGeometry(faceGeometry.get());
} 