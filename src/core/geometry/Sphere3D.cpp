#include "Sphere3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"

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
    if (!mm_state()->isStateComplete())
    {
        // 添加控制点
        mm_controlPoint()->addControlPoint(Point3D(worldPos));
        
        // 使用新的检查方法
        if (isDrawingComplete() && areControlPointsValid())
        {
            // 计算球体半径
            const auto& controlPoints = mm_controlPoint()->getControlPoints();
            if (controlPoints.size() >= 2) {
                m_radius = glm::length(controlPoints[1].position - controlPoints[0].position);
            }
            mm_state()->setStateComplete();
        }
    }
}

void Sphere3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (!mm_state()->isStateComplete() && controlPoints.size() == 1)
    {
        // 设置临时点用于预览
        mm_controlPoint()->setTempPoint(Point3D(worldPos));
    }
}

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void Sphere3D_Geo::buildVertexGeometries()
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
    
    geometry->setVertexArray(vertices);
    
    // 点绘制 - 控制点使用较大的点大小以便拾取
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    

}

void Sphere3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid())
        return;
    
    // 创建边的几何体（球体网格线）
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    float radius = m_radius;
    glm::vec3 center = controlPoints[0].position;
    
    int latSegments = static_cast<int>(m_parameters.subdivisionLevel);
    int lonSegments = latSegments * 2;
    
    // 生成网格线顶点
    for (int lat = 0; lat <= latSegments; ++lat)
    {
        float theta = static_cast<float>(M_PI * lat / latSegments);
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (int lon = 0; lon <= lonSegments; ++lon)
        {
            float phi = static_cast<float>(2.0 * M_PI * lon / lonSegments);
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            glm::vec3 normal(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
            glm::vec3 point = center + radius * normal;
            
            vertices->push_back(osg::Vec3(point.x, point.y, point.z));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a));
        }
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 绘制网格线
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES);
    
    // 经线
    for (int lat = 0; lat <= latSegments; ++lat)
    {
        for (int lon = 0; lon < lonSegments; ++lon)
        {
            int current = lat * (lonSegments + 1) + lon;
            int next = current + 1;
            indices->push_back(current);
            indices->push_back(next);
        }
    }
    
    // 纬线
    for (int lat = 0; lat < latSegments; ++lat)
    {
        for (int lon = 0; lon <= lonSegments; ++lon)
        {
            int current = lat * (lonSegments + 1) + lon;
            int next = current + lonSegments + 1;
            indices->push_back(current);
            indices->push_back(next);
        }
    }
    
    geometry->addPrimitiveSet(indices);
    
    // 设置线的宽度
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth;
    lineWidth->setWidth(m_parameters.lineWidth);
    stateSet->setAttribute(lineWidth);
}

void Sphere3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getFaceGeometry();
    if (!geometry.valid())
        return;
    
    // 创建面的几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    float radius = m_radius;
    glm::vec3 center = controlPoints[0].position;
    
    int latSegments = static_cast<int>(m_parameters.subdivisionLevel);
    int lonSegments = latSegments * 2;
    
    // 生成球面顶点
    for (int lat = 0; lat <= latSegments; ++lat)
    {
        float theta = static_cast<float>(M_PI * lat / latSegments);
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (int lon = 0; lon <= lonSegments; ++lon)
        {
            float phi = static_cast<float>(2.0 * M_PI * lon / lonSegments);
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            glm::vec3 normal(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
            glm::vec3 point = center + radius * normal;
            
            vertices->push_back(osg::Vec3(point.x, point.y, point.z));
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
            colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                            m_parameters.fillColor.b, m_parameters.fillColor.a));
        }
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
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
    
    geometry->addPrimitiveSet(indices);
}

// hitTest方法已移除，使用OSG内置拾取系统 

// ==================== 绘制完成检查和控制点验证 ====================

bool Sphere3D_Geo::isDrawingComplete() const
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    return controlPoints.size() >= 2;
}

bool Sphere3D_Geo::areControlPointsValid() const
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
    
    return true;
} 