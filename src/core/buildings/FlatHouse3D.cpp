#include "FlatHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include "../../util/MathUtils.h"
#include <cmath>

FlatHouse3D_Geo::FlatHouse3D_Geo()
{
    m_geoType = Geo_UndefinedGeo3D;  // 使用未定义类型，因为这是特殊建筑
    m_size = glm::vec3(1.0f, 1.0f, 1.0f);
    initialize();
}

void FlatHouse3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
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

void FlatHouse3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 平顶房屋的鼠标移动事件处理
    if (!mm_state()->isStateComplete() && mm_controlPoint()->hasControlPoints())
    {
        // 可以在这里实现实时预览
    }
}

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void FlatHouse3D_Geo::buildVertexGeometries()
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
    }
    
    geometry->setVertexArray(vertices);
    
    // 点绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void FlatHouse3D_Geo::buildEdgeGeometries()
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
    }
}

void FlatHouse3D_Geo::buildFaceGeometries()
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
        
        // 添加法向量
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
        
        // 绘制顶面
        osg::ref_ptr<osg::DrawArrays> topFace = new osg::DrawArrays(osg::PrimitiveSet::QUADS, 4, 4);
        geometry->addPrimitiveSet(topFace);
        
        // 绘制侧面
        for (int i = 0; i < 4; ++i)
        {
            int next = (i + 1) % 4;
            osg::ref_ptr<osg::DrawArrays> sideFace = new osg::DrawArrays(osg::PrimitiveSet::QUADS, i, 4);
            geometry->addPrimitiveSet(sideFace);
        }
    }
}

// ==================== 绘制完成检查和控制点验证 ====================

bool FlatHouse3D_Geo::isDrawingComplete() const
{
    // 平顶房屋需要3个控制点：基点、尺寸点、高度点
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    return controlPoints.size() >= 3;
}

bool FlatHouse3D_Geo::areControlPointsValid() const
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