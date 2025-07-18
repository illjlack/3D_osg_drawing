#include "Torus3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"

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
        // 添加控制点
        mm_controlPoint()->addControlPoint(Point3D(worldPos));
        
        // 使用新的检查方法
        if (isDrawingComplete() && areControlPointsValid())
        {
            // 计算环面参数
            const auto& controlPoints = mm_controlPoint()->getControlPoints();
            float distance = glm::length(controlPoints[1].position - controlPoints[0].position);
            m_majorRadius = distance * 0.7f; // 主半径
            m_minorRadius = distance * 0.3f; // 副半径
            mm_state()->setStateComplete();
        }
    }
}

void Torus3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (!mm_state()->isStateComplete() && controlPoints.size() < 2)
    {
        // 设置临时点用于预览
        mm_controlPoint()->setTempPoint(Point3D(worldPos));
    }
}

void Torus3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getVertexGeometry();
    if (!geometry.valid())
        return;
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 添加控制点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
    }
    
    // 控制点已包含临时点，无需单独处理
    
    geometry->setVertexArray(vertices);
    
    // 点绘制 - 控制点
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void Torus3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
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
    lineWidth->setWidth(m_parameters.lineWidth);
    stateSet->setAttribute(lineWidth);
}

void Torus3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getFaceGeometry();
    if (!geometry.valid())
        return;
    
    // 创建圆环面几何体
    float majorRadius = m_majorRadius;
    float minorRadius = m_minorRadius;
    glm::vec3 center = controlPoints[0].position;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    int majorSegments = static_cast<int>(m_parameters.subdivisionLevel);
    int minorSegments = majorSegments / 2;
    if (majorSegments < 8) majorSegments = 16;
    if (minorSegments < 4) minorSegments = 8;
    
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
    
    Color3D color = mm_state()->isStateComplete() ? m_parameters.fillColor : 
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
}

// ==================== 绘制完成检查和控制点验证 ====================

bool Torus3D_Geo::isDrawingComplete() const
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    return controlPoints.size() >= 2;
}

bool Torus3D_Geo::areControlPointsValid() const
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    
    // 检查控制点数量
    if (controlPoints.empty()) {
        return false;
    }
    
    // 检查控制点坐标是否有效（不是NaN或无穷大）
    for (const auto& point : controlPoints) {
        if (std::isnan(point.x()) || std::isnan(point.y()) || std::isnan(point.z()) ||
            std::isinf(point.x()) || std::isinf(point.y()) || std::isinf(point.z())) {
            return false;
        }
    }
    
    // 检查半径是否合理
    if (m_majorRadius <= 0.0f || m_minorRadius <= 0.0f) {
        return false;
    }
    
    // 检查次半径不能大于主半径
    if (m_minorRadius >= m_majorRadius) {
        return false;
    }
    
    return true;
}