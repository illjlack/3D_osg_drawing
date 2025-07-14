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
    , m_segments(16)
    , m_axis(0, 0, 1)
{
    m_geoType = Geo_Cylinder3D;
    // 确保基类正确初始化
    initialize();
}

void Cylinder3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        const auto& controlPoints = getControlPoints();
        
        if (controlPoints.size() == 2)
        {
            // 计算圆柱参数
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

void Cylinder3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    const auto& controlPoints = getControlPoints();
    if (!isStateComplete() && controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void Cylinder3D_Geo::updateGeometry()
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



osg::ref_ptr<osg::Geometry> Cylinder3D_Geo::createGeometry()
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return nullptr;
    
    float radius = m_radius;
    float height = m_height;
    glm::vec3 axis = m_axis;
    glm::vec3 center = controlPoints[0].position;
    
    if (controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        glm::vec3 diff = getTempPoint().position - center;
        height = glm::length(diff);
        if (height > 0)
            axis = glm::normalize(diff);
        radius = height * 0.3f;
    }
    else if (controlPoints.size() == 2)
    {
        center = (controlPoints[0].position + controlPoints[1].position) * 0.5f;
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

 

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void Cylinder3D_Geo::buildVertexGeometries()
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

void Cylinder3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 创建圆柱体边界线几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    glm::vec3 center = controlPoints[0].position;
    float radius = m_radius;
    float height = m_height;
    int segments = static_cast<int>(m_parameters.subdivisionLevel);
    if (segments < 8) segments = 16;
    
    // 生成圆柱体边界线
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
        
        // 顶面边
        vertices->push_back(osg::Vec3(x1, y1, center.z + height * 0.5f));
        vertices->push_back(osg::Vec3(x2, y2, center.z + height * 0.5f));
        
        // 垂直边（只显示4条主要的）
        if (i % (segments / 4) == 0)
        {
            vertices->push_back(osg::Vec3(x1, y1, center.z - height * 0.5f));
            vertices->push_back(osg::Vec3(x1, y1, center.z + height * 0.5f));
        }
        
        for (int j = 0; j < 4; ++j)
        {
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a));
        }
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 线绘制 - 边界线
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置线的宽度
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth;
    lineWidth->setWidth(2.0f);  // 边界线宽度
    stateSet->setAttribute(lineWidth);
    
    addEdgeGeometry(geometry);
}

void Cylinder3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 创建圆柱体面几何体
    osg::ref_ptr<osg::Geometry> geometry = createGeometry();
    if (geometry.valid())
    {
        addFaceGeometry(geometry);
    }
} 

bool Cylinder3D_Geo::hitTest(const Ray3D& ray, PickResult3D& result) const
{
    // 获取圆柱体的参数
    glm::vec3 center = getCenter();
    float radius = getRadius();
    float height = getHeight();
    
    // 射线-圆柱体相交测试（简化版本，使用包围盒）
    const BoundingBox3D& bbox = getBoundingBox();
    if (!bbox.isValid())
    {
        return false;
    }
    
    // 使用包围盒进行相交测试
    glm::vec3 rayDir = glm::normalize(ray.direction);
    glm::vec3 invDir = 1.0f / rayDir;
    
    // 计算与包围盒各面的交点参数
    float t1 = (bbox.min.x - ray.origin.x) * invDir.x;
    float t2 = (bbox.max.x - ray.origin.x) * invDir.x;
    float t3 = (bbox.min.y - ray.origin.y) * invDir.y;
    float t4 = (bbox.max.y - ray.origin.y) * invDir.y;
    float t5 = (bbox.min.z - ray.origin.z) * invDir.z;
    float t6 = (bbox.max.z - ray.origin.z) * invDir.z;
    
    // 计算进入和退出参数
    float tmin = glm::max(glm::max(glm::min(t1, t2), glm::min(t3, t4)), glm::min(t5, t6));
    float tmax = glm::min(glm::min(glm::max(t1, t2), glm::max(t3, t4)), glm::max(t5, t6));
    
    // 检查是否相交
    if (tmax >= 0 && tmin <= tmax)
    {
        float t = (tmin >= 0) ? tmin : tmax;
        if (t >= 0)
        {
            result.hit = true;
            result.distance = t;
            result.userData = const_cast<Cylinder3D_Geo*>(this);
            result.point = ray.origin + t * rayDir;
            return true;
        }
    }
    
    return false;
} 
