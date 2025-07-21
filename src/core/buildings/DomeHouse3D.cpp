#include "DomeHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include "../../util/MathUtils.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

DomeHouse3D_Geo::DomeHouse3D_Geo()
{
    m_geoType = Geo_UndefinedGeo3D;  // 使用未定义类型，因为这是特殊建筑
    m_size = glm::vec3(1.0f, 1.0f, 1.0f);
    m_domeHeight = 0.6f;
    m_domeRadius = 0.5f;
    m_segments = 16;  // 穹顶细分段数
    initialize();
}

void DomeHouse3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
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

void DomeHouse3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 穹顶房屋的鼠标移动事件处理
    if (!mm_state()->isStateComplete() && mm_controlPoint()->hasControlPoints())
    {
        // 可以在这里实现实时预览
    }
}

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void DomeHouse3D_Geo::buildVertexGeometries()
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
    if (controlPoints.size() >= 3)
    {
        const Point3D& basePoint = controlPoints[0];
        const Point3D& sizePoint = controlPoints[1];
        const Point3D& heightPoint = controlPoints[2];
        
        // 计算房屋尺寸
        m_size.x = abs(sizePoint.x() - basePoint.x());
        m_size.y = abs(sizePoint.y() - basePoint.y());
        m_size.z = abs(heightPoint.z() - basePoint.z());
        m_domeHeight = m_size.z * 0.5f;  // 穹顶高度为房屋高度的50%
        m_domeRadius = std::min(m_size.x, m_size.y) * 0.5f;  // 穹顶半径为较小边长的50%
        
        // 生成房屋顶点
        float x = basePoint.x();
        float y = basePoint.y();
        float z = basePoint.z();
        
        // 底面四个顶点
        vertices->push_back(osg::Vec3(x, y, z));
        vertices->push_back(osg::Vec3(x + m_size.x, y, z));
        vertices->push_back(osg::Vec3(x + m_size.x, y + m_size.y, z));
        vertices->push_back(osg::Vec3(x, y + m_size.y, z));
        
        // 顶面四个顶点
        vertices->push_back(osg::Vec3(x, y, z + m_size.z));
        vertices->push_back(osg::Vec3(x + m_size.x, y, z + m_size.z));
        vertices->push_back(osg::Vec3(x + m_size.x, y + m_size.y, z + m_size.z));
        vertices->push_back(osg::Vec3(x, y + m_size.y, z + m_size.z));
        
        // 穹顶顶点
        float domeCenterX = x + m_size.x * 0.5f;
        float domeCenterY = y + m_size.y * 0.5f;
        float domeCenterZ = z + m_size.z;
        
        // 生成穹顶顶点
        for (int i = 0; i <= m_segments; ++i)
        {
            float phi = M_PI * i / (2 * m_segments);  // 从0到π/2
            for (int j = 0; j <= m_segments; ++j)
            {
                float theta = 2 * M_PI * j / m_segments;  // 从0到2π
                
                float domeX = domeCenterX + m_domeRadius * sin(phi) * cos(theta);
                float domeY = domeCenterY + m_domeRadius * sin(phi) * sin(theta);
                float domeZ = domeCenterZ + m_domeRadius * cos(phi);
                
                vertices->push_back(osg::Vec3(domeX, domeY, domeZ));
            }
        }
    }
    
    geometry->setVertexArray(vertices);
    
    // 点绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void DomeHouse3D_Geo::buildEdgeGeometries()
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
    if (controlPoints.size() >= 3)
    {
        const Point3D& basePoint = controlPoints[0];
        float x = basePoint.x();
        float y = basePoint.y();
        float z = basePoint.z();
        
        // 生成房屋顶点
        // 底面四个顶点
        vertices->push_back(osg::Vec3(x, y, z));
        vertices->push_back(osg::Vec3(x + m_size.x, y, z));
        vertices->push_back(osg::Vec3(x + m_size.x, y + m_size.y, z));
        vertices->push_back(osg::Vec3(x, y + m_size.y, z));
        
        // 顶面四个顶点
        vertices->push_back(osg::Vec3(x, y, z + m_size.z));
        vertices->push_back(osg::Vec3(x + m_size.x, y, z + m_size.z));
        vertices->push_back(osg::Vec3(x + m_size.x, y + m_size.y, z + m_size.z));
        vertices->push_back(osg::Vec3(x, y + m_size.y, z + m_size.z));
        
        // 穹顶顶点
        float domeCenterX = x + m_size.x * 0.5f;
        float domeCenterY = y + m_size.y * 0.5f;
        float domeCenterZ = z + m_size.z;
        
        // 生成穹顶顶点
        for (int i = 0; i <= m_segments; ++i)
        {
            float phi = M_PI * i / (2 * m_segments);
            for (int j = 0; j <= m_segments; ++j)
            {
                float theta = 2 * M_PI * j / m_segments;
                
                float domeX = domeCenterX + m_domeRadius * sin(phi) * cos(theta);
                float domeY = domeCenterY + m_domeRadius * sin(phi) * sin(theta);
                float domeZ = domeCenterZ + m_domeRadius * cos(phi);
                
                vertices->push_back(osg::Vec3(domeX, domeY, domeZ));
            }
        }
        
        geometry->setVertexArray(vertices);
        
        // 绘制底面边
        for (int i = 0; i < 4; ++i)
        {
            int next = (i + 1) % 4;
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, i, 2);
            geometry->addPrimitiveSet(drawArrays);
        }
        
        // 绘制顶面边
        for (int i = 0; i < 4; ++i)
        {
            int next = (i + 1) % 4;
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 4 + i, 2);
            geometry->addPrimitiveSet(drawArrays);
        }
        
        // 绘制连接边
        for (int i = 0; i < 4; ++i)
        {
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, i, 2);
            geometry->addPrimitiveSet(drawArrays);
        }
        
        // 绘制穹顶边
        int domeStartIndex = 8;
        // 绘制经线
        for (int j = 0; j <= m_segments; ++j)
        {
            for (int i = 0; i < m_segments; ++i)
            {
                int index1 = domeStartIndex + i * (m_segments + 1) + j;
                int index2 = domeStartIndex + (i + 1) * (m_segments + 1) + j;
                osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, index1, 2);
                geometry->addPrimitiveSet(drawArrays);
            }
        }
        
        // 绘制纬线
        for (int i = 0; i <= m_segments; ++i)
        {
            for (int j = 0; j < m_segments; ++j)
            {
                int index1 = domeStartIndex + i * (m_segments + 1) + j;
                int index2 = domeStartIndex + i * (m_segments + 1) + j + 1;
                osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, index1, 2);
                geometry->addPrimitiveSet(drawArrays);
            }
        }
    }
}

void DomeHouse3D_Geo::buildFaceGeometries()
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
    if (controlPoints.size() >= 3)
    {
        const Point3D& basePoint = controlPoints[0];
        float x = basePoint.x();
        float y = basePoint.y();
        float z = basePoint.z();
        
        // 生成房屋顶点和法向量
        // 底面四个顶点
        vertices->push_back(osg::Vec3(x, y, z));
        vertices->push_back(osg::Vec3(x + m_size.x, y, z));
        vertices->push_back(osg::Vec3(x + m_size.x, y + m_size.y, z));
        vertices->push_back(osg::Vec3(x, y + m_size.y, z));
        
        // 顶面四个顶点
        vertices->push_back(osg::Vec3(x, y, z + m_size.z));
        vertices->push_back(osg::Vec3(x + m_size.x, y, z + m_size.z));
        vertices->push_back(osg::Vec3(x + m_size.x, y + m_size.y, z + m_size.z));
        vertices->push_back(osg::Vec3(x, y + m_size.y, z + m_size.z));
        
        // 穹顶顶点
        float domeCenterX = x + m_size.x * 0.5f;
        float domeCenterY = y + m_size.y * 0.5f;
        float domeCenterZ = z + m_size.z;
        
        // 生成穹顶顶点和法向量
        for (int i = 0; i <= m_segments; ++i)
        {
            float phi = M_PI * i / (2 * m_segments);
            for (int j = 0; j <= m_segments; ++j)
            {
                float theta = 2 * M_PI * j / m_segments;
                
                float domeX = domeCenterX + m_domeRadius * sin(phi) * cos(theta);
                float domeY = domeCenterY + m_domeRadius * sin(phi) * sin(theta);
                float domeZ = domeCenterZ + m_domeRadius * cos(phi);
                
                vertices->push_back(osg::Vec3(domeX, domeY, domeZ));
                
                // 计算法向量（指向穹顶中心）
                float nx = (domeX - domeCenterX) / m_domeRadius;
                float ny = (domeY - domeCenterY) / m_domeRadius;
                float nz = (domeZ - domeCenterZ) / m_domeRadius;
                normals->push_back(osg::Vec3(nx, ny, nz));
            }
        }
        
        // 添加房屋部分的法向量
        for (int i = 0; i < 8; ++i)
        {
            normals->push_back(osg::Vec3(0, 0, 1));
        }
        
        geometry->setVertexArray(vertices);
        geometry->setNormalArray(normals);
        geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
        
        // 绘制底面
        osg::ref_ptr<osg::DrawArrays> bottomFace = new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4);
        geometry->addPrimitiveSet(bottomFace);
        
        // 绘制侧面
        for (int i = 0; i < 4; ++i)
        {
            int next = (i + 1) % 4;
            osg::ref_ptr<osg::DrawArrays> sideFace = new osg::DrawArrays(osg::PrimitiveSet::QUADS, i, 4);
            geometry->addPrimitiveSet(sideFace);
        }
        
        // 绘制穹顶面
        int domeStartIndex = 8;
        for (int i = 0; i < m_segments; ++i)
        {
            for (int j = 0; j < m_segments; ++j)
            {
                int index1 = domeStartIndex + i * (m_segments + 1) + j;
                int index2 = domeStartIndex + (i + 1) * (m_segments + 1) + j;
                int index3 = domeStartIndex + (i + 1) * (m_segments + 1) + j + 1;
                int index4 = domeStartIndex + i * (m_segments + 1) + j + 1;
                
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

bool DomeHouse3D_Geo::isDrawingComplete() const
{
    // 穹顶房屋需要3个控制点：基点、尺寸点、高度点
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    return controlPoints.size() >= 3;
}

bool DomeHouse3D_Geo::areControlPointsValid() const
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    
    // 检查控制点数量
    if (controlPoints.size() < 3) {
        return false;
    }
    
    // 检查控制点坐标是否有效
    for (const auto& point : controlPoints) {
        if (std::isnan(point.x()) || std::isnan(point.y()) || std::isnan(point.z()) ||
            std::isinf(point.x()) || std::isinf(point.y()) || std::isinf(point.z())) {
            return false;
        }
    }
    
    // 检查尺寸是否有效
    if (controlPoints.size() >= 3) {
        const Point3D& basePoint = controlPoints[0];
        const Point3D& sizePoint = controlPoints[1];
        const Point3D& heightPoint = controlPoints[2];
        
        float width = abs(sizePoint.x() - basePoint.x());
        float length = abs(sizePoint.y() - basePoint.y());
        float height = abs(heightPoint.z() - basePoint.z());
        
        if (width <= 0.0f || length <= 0.0f || height <= 0.0f ||
            std::isnan(width) || std::isnan(length) || std::isnan(height) ||
            std::isinf(width) || std::isinf(length) || std::isinf(height)) {
            return false;
        }
    }
    
    return true;
} 