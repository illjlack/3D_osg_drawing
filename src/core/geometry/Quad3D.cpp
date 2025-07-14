#include "Quad3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Quad3D_Geo::Quad3D_Geo()
    : m_area(0.0f)
    , m_normal(0, 0, 1)
{
    m_geoType = Geo_Quad3D;
    // 确保基类正确初始化
    initialize();
}

void Quad3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        const auto& controlPoints = getControlPoints();
        if (controlPoints.size() == 4)
        {
            calculateNormal();
            calculateArea();
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Quad3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        setTempPoint(Point3D(worldPos));
        updateGeometry();
    }
}

void Quad3D_Geo::updateGeometry()
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
    
    // 更新KDTree
    if (getNodeManager()) {
        getNodeManager()->updateKdTree();
    }
}

osg::ref_ptr<osg::Geometry> Quad3D_Geo::createGeometry()
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 2)
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    if (controlPoints.size() == 4 || (controlPoints.size() == 3 && getTempPoint().position != glm::vec3(0)))
    {
        // 绘制四边形
        std::vector<Point3D> points = controlPoints;
        if (points.size() == 3)
            points.push_back(getTempPoint());
        
        // 添加顶点（按顺序）
        for (const Point3D& point : points)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        }
        
        // 计算法向量
        glm::vec3 v1 = points[1].position - points[0].position;
        glm::vec3 v2 = points[2].position - points[0].position;
        glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
        
        for (int i = 0; i < 4; ++i)
        {
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
        }
        
        // 设置颜色
        Color3D color = isStateComplete() ? m_parameters.fillColor : 
                       Color3D(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                              m_parameters.fillColor.b, m_parameters.fillColor.a * 0.5f);
        
        for (int i = 0; i < 4; ++i)
        {
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
        
        // 将四边形三角化
        osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
        indices->push_back(0); indices->push_back(1); indices->push_back(2);
        indices->push_back(0); indices->push_back(2); indices->push_back(3);
        
        geometry->addPrimitiveSet(indices.get());
    }
    else
    {
        // 绘制辅助线或点
        for (const Point3D& point : controlPoints)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a));
        }
        
        if (getTempPoint().position != glm::vec3(0))
        {
            vertices->push_back(osg::Vec3(getTempPoint().x(), getTempPoint().y(), getTempPoint().z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a * 0.5f));
        }
        
        if (vertices->size() >= 2)
        {
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
        }
        else
        {
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    if (!normals->empty())
    {
        geometry->setNormalArray(normals.get());
        geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    
    return geometry;
}

float Quad3D_Geo::calculateArea() const
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 4)
    {
        return 0.0f;
    }
    
    // 计算四边形面积（使用两个三角形）
    glm::vec3 v1 = controlPoints[1].position - controlPoints[0].position;
    glm::vec3 v2 = controlPoints[2].position - controlPoints[0].position;
    glm::vec3 v3 = controlPoints[3].position - controlPoints[0].position;
    
    float area1 = glm::length(glm::cross(v1, v2)) * 0.5f;
    float area2 = glm::length(glm::cross(v2, v3)) * 0.5f;
    
    return area1 + area2;
}

void Quad3D_Geo::calculateNormal()
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() >= 3)
    {
        glm::vec3 v1 = controlPoints[1].position - controlPoints[0].position;
        glm::vec3 v2 = controlPoints[2].position - controlPoints[0].position;
        m_normal = glm::normalize(glm::cross(v1, v2));
    }
} 

void Quad3D_Geo::buildVertexGeometries()
{
    clearVertexGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 只为控制点创建几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 添加控制点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a));
    }
    
    // 如果有临时点且几何体未完成，添加临时点
    if (!isStateComplete() && getTempPoint().position != glm::vec3(0))
    {
        vertices->push_back(osg::Vec3(getTempPoint().x(), getTempPoint().y(), getTempPoint().z()));
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a * 0.5f));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 点绘制 - 控制点使用较大的点大小以便拾取
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置点的大小
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::Point> point = new osg::Point;
    point->setSize(8.0f);  // 控制点大小
    stateSet->setAttribute(point);
    
    addVertexGeometry(geometry);
}

void Quad3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 创建四边形边界线几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 构建所有点（包括临时点）
    std::vector<Point3D> allPoints = controlPoints;
    if (!isStateComplete() && getTempPoint().position != glm::vec3(0))
    {
        allPoints.push_back(getTempPoint());
    }
    
    // 添加边线
    for (size_t i = 0; i < allPoints.size(); ++i)
    {
        vertices->push_back(osg::Vec3(allPoints[i].x(), allPoints[i].y(), allPoints[i].z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    // 如果有4个或更多点，形成闭合四边形
    if (allPoints.size() >= 4)
    {
        vertices->push_back(osg::Vec3(allPoints[0].x(), allPoints[0].y(), allPoints[0].z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 线绘制 - 边界线
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置线的宽度
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth;
    lineWidth->setWidth(2.0f);  // 边界线宽度
    stateSet->setAttribute(lineWidth);
    
    addEdgeGeometry(geometry);
}

void Quad3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 3)
        return;
    
    // 创建四边形面几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    // 添加顶点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                   m_parameters.fillColor.b, m_parameters.fillColor.a));
    }
    
    // 计算法向量
    calculateNormal();
    for (int i = 0; i < controlPoints.size(); ++i)
    {
        normals->push_back(osg::Vec3(m_normal.x, m_normal.y, m_normal.z));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用三角形绘制（两个三角形组成四边形）
    if (controlPoints.size() >= 4)
    {
        osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 6);
        (*indices)[0] = 0; (*indices)[1] = 1; (*indices)[2] = 2;
        (*indices)[3] = 0; (*indices)[4] = 2; (*indices)[5] = 3;
        geometry->addPrimitiveSet(indices);
    }
    else if (controlPoints.size() >= 3)
    {
        // 如果只有3个点，绘制三角形
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 3);
        geometry->addPrimitiveSet(drawArrays);
    }
    
    addFaceGeometry(geometry);
} 

bool Quad3D_Geo::hitTest(const Ray3D& ray, PickResult3D& result) const
{
    // 获取四边形的四个顶点
    glm::vec3 v0 = getVertex(0);
    glm::vec3 v1 = getVertex(1);
    glm::vec3 v2 = getVertex(2);
    glm::vec3 v3 = getVertex(3);
    
    // 将四边形分解为两个三角形进行测试
    // 第一个三角形：v0, v1, v2
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    
    glm::vec3 rayDir = glm::normalize(ray.direction);
    glm::vec3 h = glm::cross(rayDir, edge2);
    float a = glm::dot(edge1, h);
    
    if (std::abs(a) >= 1e-6f)
    {
        float f = 1.0f / a;
        glm::vec3 s = ray.origin - v0;
        float u = f * glm::dot(s, h);
        
        if (u >= 0.0f && u <= 1.0f)
        {
            glm::vec3 q = glm::cross(s, edge1);
            float v = f * glm::dot(rayDir, q);
            
            if (v >= 0.0f && u + v <= 1.0f)
            {
                float t = f * glm::dot(edge2, q);
                if (t >= 0.0f)
                {
                    result.hit = true;
                    result.distance = t;
                    result.userData = const_cast<Quad3D_Geo*>(this);
                    result.point = ray.origin + t * rayDir;
                    return true;
                }
            }
        }
    }
    
    // 第二个三角形：v0, v2, v3
    edge1 = v2 - v0;
    edge2 = v3 - v0;
    
    h = glm::cross(rayDir, edge2);
    a = glm::dot(edge1, h);
    
    if (std::abs(a) >= 1e-6f)
    {
        float f = 1.0f / a;
        glm::vec3 s = ray.origin - v0;
        float u = f * glm::dot(s, h);
        
        if (u >= 0.0f && u <= 1.0f)
        {
            glm::vec3 q = glm::cross(s, edge1);
            float v = f * glm::dot(rayDir, q);
            
            if (v >= 0.0f && u + v <= 1.0f)
            {
                float t = f * glm::dot(edge2, q);
                if (t >= 0.0f)
                {
                    result.hit = true;
                    result.distance = t;
                    result.userData = const_cast<Quad3D_Geo*>(this);
                    result.point = ray.origin + t * rayDir;
                    return true;
                }
            }
        }
    }
    
    return false;
} 