#include "Prism3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include "../../util/MathUtils.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Prism3D_Geo::Prism3D_Geo()
{
    m_geoType = Geo_Prism3D;
    m_sides = 6;  // 默认六棱柱
    m_height = 1.0f;
    m_radius = 0.5f;
    initialize();
}

void Prism3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
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

void Prism3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 多棱柱的鼠标移动事件处理
    if (!mm_state()->isStateComplete() && mm_controlPoint()->hasControlPoints())
    {
        // 可以在这里实现实时预览
    }
}

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void Prism3D_Geo::buildVertexGeometries()
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
        const Point3D& heightPoint = controlPoints[1];
        
        // 计算高度
        m_height = glm::length(heightPoint.position - center.position);
        
        // 生成底面顶点
        for (int i = 0; i < m_sides; ++i)
        {
            float angle = 2.0f * M_PI * i / m_sides;
            float x = center.x() + m_radius * cos(angle);
            float y = center.y() + m_radius * sin(angle);
            float z = center.z();
            vertices->push_back(osg::Vec3(x, y, z));
        }
        
        // 生成顶面顶点
        for (int i = 0; i < m_sides; ++i)
        {
            float angle = 2.0f * M_PI * i / m_sides;
            float x = center.x() + m_radius * cos(angle);
            float y = center.y() + m_radius * sin(angle);
            float z = center.z() + m_height;
            vertices->push_back(osg::Vec3(x, y, z));
        }
    }
    
    geometry->setVertexArray(vertices);
    
    // 点绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void Prism3D_Geo::buildEdgeGeometries()
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
        
        // 生成底面顶点
        for (int i = 0; i < m_sides; ++i)
        {
            float angle = 2.0f * M_PI * i / m_sides;
            float x = center.x() + m_radius * cos(angle);
            float y = center.y() + m_radius * sin(angle);
            float z = center.z();
            vertices->push_back(osg::Vec3(x, y, z));
        }
        
        // 生成顶面顶点
        for (int i = 0; i < m_sides; ++i)
        {
            float angle = 2.0f * M_PI * i / m_sides;
            float x = center.x() + m_radius * cos(angle);
            float y = center.y() + m_radius * sin(angle);
            float z = center.z() + m_height;
            vertices->push_back(osg::Vec3(x, y, z));
        }
        
        geometry->setVertexArray(vertices);
        
        // 绘制底面边
        for (int i = 0; i < m_sides; ++i)
        {
            int next = (i + 1) % m_sides;
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, i, 2);
            geometry->addPrimitiveSet(drawArrays);
        }
        
        // 绘制顶面边
        for (int i = 0; i < m_sides; ++i)
        {
            int next = (i + 1) % m_sides;
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, m_sides + i, 2);
            geometry->addPrimitiveSet(drawArrays);
        }
        
        // 绘制连接边
        for (int i = 0; i < m_sides; ++i)
        {
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, i, 2);
            geometry->addPrimitiveSet(drawArrays);
        }
    }
}

void Prism3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    if (!mm_controlPoint()->hasControlPoints())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getFaceGeometry();
    if (!geometry.valid())
        return;
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() >= 2)
    {
        const Point3D& center = controlPoints[0];
        
        // 生成底面顶点
        for (int i = 0; i < m_sides; ++i)
        {
            float angle = 2.0f * M_PI * i / m_sides;
            float x = center.x() + m_radius * cos(angle);
            float y = center.y() + m_radius * sin(angle);
            float z = center.z();
            vertices->push_back(osg::Vec3(x, y, z));
            normals->push_back(osg::Vec3(0, 0, -1));  // 底面法向量
        }
        
        // 生成顶面顶点
        for (int i = 0; i < m_sides; ++i)
        {
            float angle = 2.0f * M_PI * i / m_sides;
            float x = center.x() + m_radius * cos(angle);
            float y = center.y() + m_radius * sin(angle);
            float z = center.z() + m_height;
            vertices->push_back(osg::Vec3(x, y, z));
            normals->push_back(osg::Vec3(0, 0, 1));  // 顶面法向量
        }
        
        geometry->setVertexArray(vertices);
        geometry->setNormalArray(normals);
        geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
        
        // 绘制底面
        osg::ref_ptr<osg::DrawArrays> bottomFace = new osg::DrawArrays(osg::PrimitiveSet::POLYGON, 0, m_sides);
        geometry->addPrimitiveSet(bottomFace);
        
        // 绘制顶面
        osg::ref_ptr<osg::DrawArrays> topFace = new osg::DrawArrays(osg::PrimitiveSet::POLYGON, m_sides, m_sides);
        geometry->addPrimitiveSet(topFace);
        
        // 绘制侧面
        for (int i = 0; i < m_sides; ++i)
        {
            int next = (i + 1) % m_sides;
            osg::ref_ptr<osg::DrawArrays> sideFace = new osg::DrawArrays(osg::PrimitiveSet::QUADS, i, 4);
            geometry->addPrimitiveSet(sideFace);
        }
    }
}

// ==================== 绘制完成检查和控制点验证 ====================

bool Prism3D_Geo::isDrawingComplete() const
{
    // 多棱柱需要2个控制点：中心点和高度点
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    return controlPoints.size() >= 2;
}

bool Prism3D_Geo::areControlPointsValid() const
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
    
    // 检查高度是否有效
    if (controlPoints.size() >= 2) {
        float height = glm::length(controlPoints[1].position - controlPoints[0].position);
        if (height <= 0.0f || std::isnan(height) || std::isinf(height)) {
            return false;
        }
    }
    
    return true;
} 