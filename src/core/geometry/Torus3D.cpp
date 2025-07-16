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
    if (!mm_state()->isStateComplete())
    {
        mm_controlPoint()->addControlPoint(Point3D(worldPos));
        const auto& controlPoints = mm_controlPoint()->getControlPoints();
        
        if (controlPoints.size() == 2)
        {
            // 计算环面参数
            float distance = glm::length(controlPoints[1].position - controlPoints[0].position);
            m_majorRadius = distance * 0.7f; // 主半径
            m_minorRadius = distance * 0.3f; // 副半径
            mm_state()->setStateComplete();
        }
        
        updateGeometry();
        emit stateChanged(this);
    }
}

void Torus3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (!mm_state()->isStateComplete() && controlPoints.size() == 1)
    {
        // 设置临时点用于预览
        // 这里需要实现临时点机制
        updateGeometry();
    }
}

void Torus3D_Geo::updateGeometry()
{
    // 清除点线面节点
    mm_node()->clearAllGeometries();
    
    // 构建点线面几何体
    buildVertexGeometries();
    buildEdgeGeometries();
    buildFaceGeometries();
    
    // 更新OSG节点
    updateOSGNode();
    
    // 更新捕捉点
    mm_snapPoint()->updateSnapPoints();
    
    // 更新包围盒
    mm_boundingBox()->updateBoundingBox();
    
    // 更新空间索引
    mm_node()->updateSpatialIndex();
}



osg::ref_ptr<osg::Geometry> Torus3D_Geo::createGeometry()
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.empty())
        return nullptr;
    
    float majorRadius = m_majorRadius;
    float minorRadius = m_minorRadius;
    glm::vec3 center = controlPoints[0].position;
    
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
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = getVertexGeometry();
    if (!geometry.valid())
        return;
    
    // 创建顶点数组
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

void Torus3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = getEdgeGeometry();
    if (!geometry.valid())
        return;
    
    // 创建圆环边界线几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 生成圆环边界线
    glm::vec3 center = controlPoints[0].position;
    int majorSegments = static_cast<int>(m_parameters.subdivisionLevel);
    int minorSegments = majorSegments / 2;
    if (majorSegments < 8) majorSegments = 16;
    if (minorSegments < 4) minorSegments = 8;
    
    // 主圆环的边（只显示4条主要的）
    for (int i = 0; i < majorSegments; i += majorSegments / 4)
    {
        float majorAngle1 = 2.0f * M_PI * i / majorSegments;
        float majorAngle2 = 2.0f * M_PI * (i + 1) / majorSegments;
        
        for (int j = 0; j < minorSegments; j += minorSegments / 4)
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

void Torus3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = getFaceGeometry();
    if (!geometry.valid())
        return;
    
    // 创建圆环面几何体
    osg::ref_ptr<osg::Geometry> newGeometry = createGeometry();
    if (newGeometry.valid())
    {
        // 复制新几何体的数据到现有几何体
        geometry->setVertexArray(newGeometry->getVertexArray());
        geometry->setColorArray(newGeometry->getColorArray());
        geometry->setNormalArray(newGeometry->getNormalArray());
        geometry->removePrimitiveSet(0, geometry->getNumPrimitiveSets());
        for (unsigned int i = 0; i < newGeometry->getNumPrimitiveSets(); ++i)
        {
            geometry->addPrimitiveSet(newGeometry->getPrimitiveSet(i));
        }
    }
}

bool Torus3D_Geo::hitTest(const Ray3D& ray, PickResult3D& result) const
{
    // 获取环面的参数
    float majorRadius = getMajorRadius();
    float minorRadius = getMinorRadius();
    
    // 射线-环面相交测试（简化版本，使用包围盒）
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
            result.userData = const_cast<Torus3D_Geo*>(this);
            result.point = ray.origin + t * rayDir;
            return true;
        }
    }
    
    return false;
}

 