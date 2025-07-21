#include "Hemisphere3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include "../../util/MathUtils.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Hemisphere3D_Geo::Hemisphere3D_Geo()
{
    m_geoType = Geo_Hemisphere3D;
    m_radius = 1.0f;
    m_segments = 16;  // 默认细分段数
    initialize();
}

void Hemisphere3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!mm_state()->isStateComplete())
    {
        // 添加控制点
        mm_controlPoint()->addControlPoint(Point3D(worldPos));
        
        // 使用新的检查方法
        if (isDrawingComplete() && areControlPointsValid())
        {
            mm_state()->setStateComplete();
        }
    }
}

void Hemisphere3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 半球的鼠标移动事件处理
    if (!mm_state()->isStateComplete() && mm_controlPoint()->hasControlPoints())
    {
        // 可以在这里实现实时预览
    }
}

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void Hemisphere3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
    if (!mm_controlPoint()->hasControlPoints())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getVertexGeometry();
    if (!geometry.valid())
        return;
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() >= 2)
    {
        const Point3D& center = controlPoints[0];
        const Point3D& radiusPoint = controlPoints[1];
        
        // 计算半径
        m_radius = glm::length(radiusPoint.position - center.position);
        
        // 生成半球顶点
        for (int i = 0; i <= m_segments; ++i)
        {
            float phi = M_PI * i / (2 * m_segments);  // 从0到π/2
            for (int j = 0; j <= m_segments; ++j)
            {
                float theta = 2 * M_PI * j / m_segments;  // 从0到2π
                
                float x = center.x() + m_radius * sin(phi) * cos(theta);
                float y = center.y() + m_radius * sin(phi) * sin(theta);
                float z = center.z() + m_radius * cos(phi);
                
                vertices->push_back(osg::Vec3(x, y, z));
            }
        }
    }
    
    geometry->setVertexArray(vertices);
    
    // 点绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void Hemisphere3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    if (!mm_controlPoint()->hasControlPoints())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid())
        return;
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() >= 2)
    {
        const Point3D& center = controlPoints[0];
        
        // 生成半球顶点
        for (int i = 0; i <= m_segments; ++i)
        {
            float phi = M_PI * i / (2 * m_segments);
            for (int j = 0; j <= m_segments; ++j)
            {
                float theta = 2 * M_PI * j / m_segments;
                
                float x = center.x() + m_radius * sin(phi) * cos(theta);
                float y = center.y() + m_radius * sin(phi) * sin(theta);
                float z = center.z() + m_radius * cos(phi);
                
                vertices->push_back(osg::Vec3(x, y, z));
            }
        }
        
        geometry->setVertexArray(vertices);
        
        // 绘制经线
        for (int j = 0; j <= m_segments; ++j)
        {
            for (int i = 0; i < m_segments; ++i)
            {
                int index1 = i * (m_segments + 1) + j;
                int index2 = (i + 1) * (m_segments + 1) + j;
                osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, index1, 2);
                geometry->addPrimitiveSet(drawArrays);
            }
        }
        
        // 绘制纬线
        for (int i = 0; i <= m_segments; ++i)
        {
            for (int j = 0; j < m_segments; ++j)
            {
                int index1 = i * (m_segments + 1) + j;
                int index2 = i * (m_segments + 1) + j + 1;
                osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, index1, 2);
                geometry->addPrimitiveSet(drawArrays);
            }
        }
    }
}

void Hemisphere3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    if (!mm_controlPoint()->hasControlPoints())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getFaceGeometry();
    if (!geometry.valid())
        return;
    
    // 创建顶点数组和法向量数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() >= 2)
    {
        const Point3D& center = controlPoints[0];
        
        // 生成半球顶点和法向量
        for (int i = 0; i <= m_segments; ++i)
        {
            float phi = M_PI * i / (2 * m_segments);
            for (int j = 0; j <= m_segments; ++j)
            {
                float theta = 2 * M_PI * j / m_segments;
                
                float x = center.x() + m_radius * sin(phi) * cos(theta);
                float y = center.y() + m_radius * sin(phi) * sin(theta);
                float z = center.z() + m_radius * cos(phi);
                
                vertices->push_back(osg::Vec3(x, y, z));
                
                // 计算法向量（指向球心）
                float nx = (x - center.x()) / m_radius;
                float ny = (y - center.y()) / m_radius;
                float nz = (z - center.z()) / m_radius;
                normals->push_back(osg::Vec3(nx, ny, nz));
            }
        }
        
        geometry->setVertexArray(vertices);
        geometry->setNormalArray(normals);
        geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
        
        // 绘制三角形面片
        for (int i = 0; i < m_segments; ++i)
        {
            for (int j = 0; j < m_segments; ++j)
            {
                int index1 = i * (m_segments + 1) + j;
                int index2 = (i + 1) * (m_segments + 1) + j;
                int index3 = (i + 1) * (m_segments + 1) + j + 1;
                int index4 = i * (m_segments + 1) + j + 1;
                
                // 第一个三角形
                osg::ref_ptr<osg::DrawArrays> triangle1 = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, index1, 3);
                geometry->addPrimitiveSet(triangle1);
                
                // 第二个三角形
                osg::ref_ptr<osg::DrawArrays> triangle2 = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, index1, 3);
                geometry->addPrimitiveSet(triangle2);
            }
        }
    }
}

// ==================== 绘制完成检查和控制点验证 ====================

bool Hemisphere3D_Geo::isDrawingComplete() const
{
    // 半球需要2个控制点：中心点和半径点
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    return controlPoints.size() >= 2;
}

bool Hemisphere3D_Geo::areControlPointsValid() const
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    
    // 检查控制点数量
    if (controlPoints.size() < 2) {
        return false;
    }
    
    // 检查控制点坐标是否有效
    for (const auto& point : controlPoints) {
        if (std::isnan(point.x()) || std::isnan(point.y()) || std::isnan(point.z()) ||
            std::isinf(point.x()) || std::isinf(point.y()) || std::isinf(point.z())) {
            return false;
        }
    }
    
    // 检查半径是否有效
    if (controlPoints.size() >= 2) {
        float radius = glm::length(controlPoints[1].position - controlPoints[0].position);
        if (radius <= 0.0f || std::isnan(radius) || std::isinf(radius)) {
            return false;
        }
    }
    
    return true;
} 