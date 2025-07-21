#include "Ellipsoid3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include "../../util/MathUtils.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Ellipsoid3D_Geo::Ellipsoid3D_Geo()
{
    m_geoType = Geo_Ellipsoid3D;
    m_radii = glm::vec3(1.0f, 0.8f, 0.6f);  // 默认椭球半径
    m_segments = 16;  // 默认细分段数
    initialize();
}

void Ellipsoid3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
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

void Ellipsoid3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 椭球的鼠标移动事件处理
    if (!mm_state()->isStateComplete() && mm_controlPoint()->hasControlPoints())
    {
        // 可以在这里实现实时预览
    }
}

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void Ellipsoid3D_Geo::buildVertexGeometries()
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
    if (controlPoints.size() >= 4)
    {
        const Point3D& center = controlPoints[0];
        const Point3D& xAxis = controlPoints[1];
        const Point3D& yAxis = controlPoints[2];
        const Point3D& zAxis = controlPoints[3];
        
        // 计算三个轴的半径
        m_radii.x = glm::length(xAxis.position - center.position);
        m_radii.y = glm::length(yAxis.position - center.position);
        m_radii.z = glm::length(zAxis.position - center.position);
        
        // 生成椭球顶点
        for (int i = 0; i <= m_segments; ++i)
        {
            float phi = M_PI * i / m_segments;  // 从0到π
            for (int j = 0; j <= m_segments; ++j)
            {
                float theta = 2 * M_PI * j / m_segments;  // 从0到2π
                
                float x = center.x() + m_radii.x * sin(phi) * cos(theta);
                float y = center.y() + m_radii.y * sin(phi) * sin(theta);
                float z = center.z() + m_radii.z * cos(phi);
                
                vertices->push_back(osg::Vec3(x, y, z));
            }
        }
    }
    
    geometry->setVertexArray(vertices);
    
    // 点绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void Ellipsoid3D_Geo::buildEdgeGeometries()
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
    if (controlPoints.size() >= 4)
    {
        const Point3D& center = controlPoints[0];
        
        // 生成椭球顶点
        for (int i = 0; i <= m_segments; ++i)
        {
            float phi = M_PI * i / m_segments;
            for (int j = 0; j <= m_segments; ++j)
            {
                float theta = 2 * M_PI * j / m_segments;
                
                float x = center.x() + m_radii.x * sin(phi) * cos(theta);
                float y = center.y() + m_radii.y * sin(phi) * sin(theta);
                float z = center.z() + m_radii.z * cos(phi);
                
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

void Ellipsoid3D_Geo::buildFaceGeometries()
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
    if (controlPoints.size() >= 4)
    {
        const Point3D& center = controlPoints[0];
        
        // 生成椭球顶点和法向量
        for (int i = 0; i <= m_segments; ++i)
        {
            float phi = M_PI * i / m_segments;
            for (int j = 0; j <= m_segments; ++j)
            {
                float theta = 2 * M_PI * j / m_segments;
                
                float x = center.x() + m_radii.x * sin(phi) * cos(theta);
                float y = center.y() + m_radii.y * sin(phi) * sin(theta);
                float z = center.z() + m_radii.z * cos(phi);
                
                vertices->push_back(osg::Vec3(x, y, z));
                
                // 计算法向量（椭球表面的法向量）
                float nx = 2 * (x - center.x()) / (m_radii.x * m_radii.x);
                float ny = 2 * (y - center.y()) / (m_radii.y * m_radii.y);
                float nz = 2 * (z - center.z()) / (m_radii.z * m_radii.z);
                float length = sqrt(nx * nx + ny * ny + nz * nz);
                normals->push_back(osg::Vec3(nx / length, ny / length, nz / length));
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

bool Ellipsoid3D_Geo::isDrawingComplete() const
{
    // 椭球需要4个控制点：中心点和三个轴点
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    return controlPoints.size() >= 4;
}

bool Ellipsoid3D_Geo::areControlPointsValid() const
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    
    // 检查控制点数量
    if (controlPoints.size() < 4) {
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
    if (controlPoints.size() >= 4) {
        const Point3D& center = controlPoints[0];
        float rx = glm::length(controlPoints[1].position - center.position);
        float ry = glm::length(controlPoints[2].position - center.position);
        float rz = glm::length(controlPoints[3].position - center.position);
        
        if (rx <= 0.0f || ry <= 0.0f || rz <= 0.0f ||
            std::isnan(rx) || std::isnan(ry) || std::isnan(rz) ||
            std::isinf(rx) || std::isinf(ry) || std::isinf(rz)) {
            return false;
        }
    }
    
    return true;
} 