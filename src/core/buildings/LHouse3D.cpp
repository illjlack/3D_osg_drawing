#include "LHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include "../../util/MathUtils.h"
#include <cmath>

LHouse3D_Geo::LHouse3D_Geo()
{
    m_geoType = Geo_UndefinedGeo3D;  // 使用未定义类型，因为这是特殊建筑
    m_mainSize = glm::vec3(1.0f, 1.0f, 1.0f);
    m_wingSize = glm::vec3(0.6f, 0.6f, 1.0f);
    m_height = 1.0f;
    initialize();
}

void LHouse3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
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

void LHouse3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // L型房屋的鼠标移动事件处理
    if (!mm_state()->isStateComplete() && mm_controlPoint()->hasControlPoints())
    {
        // 可以在这里实现实时预览
    }
}

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void LHouse3D_Geo::buildVertexGeometries()
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
        const Point3D& basePoint = controlPoints[0];
        const Point3D& mainSizePoint = controlPoints[1];
        const Point3D& wingSizePoint = controlPoints[2];
        const Point3D& heightPoint = controlPoints[3];
        
        // 计算房屋尺寸
        m_mainSize.x = abs(mainSizePoint.x() - basePoint.x());
        m_mainSize.y = abs(mainSizePoint.y() - basePoint.y());
        m_wingSize.x = abs(wingSizePoint.x() - basePoint.x());
        m_wingSize.y = abs(wingSizePoint.y() - basePoint.y());
        m_height = abs(heightPoint.z() - basePoint.z());
        
        // 生成L型房屋顶点
        float x = basePoint.x();
        float y = basePoint.y();
        float z = basePoint.z();
        
        // 主体部分顶点
        // 底面四个顶点
        vertices->push_back(osg::Vec3(x, y, z));
        vertices->push_back(osg::Vec3(x + m_mainSize.x, y, z));
        vertices->push_back(osg::Vec3(x + m_mainSize.x, y + m_mainSize.y, z));
        vertices->push_back(osg::Vec3(x, y + m_mainSize.y, z));
        
        // 顶面四个顶点
        vertices->push_back(osg::Vec3(x, y, z + m_height));
        vertices->push_back(osg::Vec3(x + m_mainSize.x, y, z + m_height));
        vertices->push_back(osg::Vec3(x + m_mainSize.x, y + m_mainSize.y, z + m_height));
        vertices->push_back(osg::Vec3(x, y + m_mainSize.y, z + m_height));
        
        // 翼部顶点
        // 翼部底面四个顶点
        vertices->push_back(osg::Vec3(x, y, z));
        vertices->push_back(osg::Vec3(x + m_wingSize.x, y, z));
        vertices->push_back(osg::Vec3(x + m_wingSize.x, y + m_wingSize.y, z));
        vertices->push_back(osg::Vec3(x, y + m_wingSize.y, z));
        
        // 翼部顶面四个顶点
        vertices->push_back(osg::Vec3(x, y, z + m_height));
        vertices->push_back(osg::Vec3(x + m_wingSize.x, y, z + m_height));
        vertices->push_back(osg::Vec3(x + m_wingSize.x, y + m_wingSize.y, z + m_height));
        vertices->push_back(osg::Vec3(x, y + m_wingSize.y, z + m_height));
    }
    
    geometry->setVertexArray(vertices);
    
    // 点绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void LHouse3D_Geo::buildEdgeGeometries()
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
        const Point3D& basePoint = controlPoints[0];
        float x = basePoint.x();
        float y = basePoint.y();
        float z = basePoint.z();
        
        // 生成L型房屋顶点
        // 主体部分顶点
        // 底面四个顶点
        vertices->push_back(osg::Vec3(x, y, z));
        vertices->push_back(osg::Vec3(x + m_mainSize.x, y, z));
        vertices->push_back(osg::Vec3(x + m_mainSize.x, y + m_mainSize.y, z));
        vertices->push_back(osg::Vec3(x, y + m_mainSize.y, z));
        
        // 顶面四个顶点
        vertices->push_back(osg::Vec3(x, y, z + m_height));
        vertices->push_back(osg::Vec3(x + m_mainSize.x, y, z + m_height));
        vertices->push_back(osg::Vec3(x + m_mainSize.x, y + m_mainSize.y, z + m_height));
        vertices->push_back(osg::Vec3(x, y + m_mainSize.y, z + m_height));
        
        // 翼部顶点
        // 翼部底面四个顶点
        vertices->push_back(osg::Vec3(x, y, z));
        vertices->push_back(osg::Vec3(x + m_wingSize.x, y, z));
        vertices->push_back(osg::Vec3(x + m_wingSize.x, y + m_wingSize.y, z));
        vertices->push_back(osg::Vec3(x, y + m_wingSize.y, z));
        
        // 翼部顶面四个顶点
        vertices->push_back(osg::Vec3(x, y, z + m_height));
        vertices->push_back(osg::Vec3(x + m_wingSize.x, y, z + m_height));
        vertices->push_back(osg::Vec3(x + m_wingSize.x, y + m_wingSize.y, z + m_height));
        vertices->push_back(osg::Vec3(x, y + m_wingSize.y, z + m_height));
        
        geometry->setVertexArray(vertices);
        
        // 绘制主体底面边
        for (int i = 0; i < 4; ++i)
        {
            int next = (i + 1) % 4;
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, i, 2);
            geometry->addPrimitiveSet(drawArrays);
        }
        
        // 绘制主体顶面边
        for (int i = 0; i < 4; ++i)
        {
            int next = (i + 1) % 4;
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 4 + i, 2);
            geometry->addPrimitiveSet(drawArrays);
        }
        
        // 绘制主体连接边
        for (int i = 0; i < 4; ++i)
        {
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, i, 2);
            geometry->addPrimitiveSet(drawArrays);
        }
        
        // 绘制翼部底面边
        for (int i = 0; i < 4; ++i)
        {
            int next = (i + 1) % 4;
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 8 + i, 2);
            geometry->addPrimitiveSet(drawArrays);
        }
        
        // 绘制翼部顶面边
        for (int i = 0; i < 4; ++i)
        {
            int next = (i + 1) % 4;
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 12 + i, 2);
            geometry->addPrimitiveSet(drawArrays);
        }
        
        // 绘制翼部连接边
        for (int i = 0; i < 4; ++i)
        {
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 8 + i, 2);
            geometry->addPrimitiveSet(drawArrays);
        }
    }
}

void LHouse3D_Geo::buildFaceGeometries()
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
        const Point3D& basePoint = controlPoints[0];
        float x = basePoint.x();
        float y = basePoint.y();
        float z = basePoint.z();
        
        // 生成L型房屋顶点和法向量
        // 主体部分顶点
        // 底面四个顶点
        vertices->push_back(osg::Vec3(x, y, z));
        vertices->push_back(osg::Vec3(x + m_mainSize.x, y, z));
        vertices->push_back(osg::Vec3(x + m_mainSize.x, y + m_mainSize.y, z));
        vertices->push_back(osg::Vec3(x, y + m_mainSize.y, z));
        
        // 顶面四个顶点
        vertices->push_back(osg::Vec3(x, y, z + m_height));
        vertices->push_back(osg::Vec3(x + m_mainSize.x, y, z + m_height));
        vertices->push_back(osg::Vec3(x + m_mainSize.x, y + m_mainSize.y, z + m_height));
        vertices->push_back(osg::Vec3(x, y + m_mainSize.y, z + m_height));
        
        // 翼部顶点
        // 翼部底面四个顶点
        vertices->push_back(osg::Vec3(x, y, z));
        vertices->push_back(osg::Vec3(x + m_wingSize.x, y, z));
        vertices->push_back(osg::Vec3(x + m_wingSize.x, y + m_wingSize.y, z));
        vertices->push_back(osg::Vec3(x, y + m_wingSize.y, z));
        
        // 翼部顶面四个顶点
        vertices->push_back(osg::Vec3(x, y, z + m_height));
        vertices->push_back(osg::Vec3(x + m_wingSize.x, y, z + m_height));
        vertices->push_back(osg::Vec3(x + m_wingSize.x, y + m_wingSize.y, z + m_height));
        vertices->push_back(osg::Vec3(x, y + m_wingSize.y, z + m_height));
        
        // 添加法向量
        for (int i = 0; i < 16; ++i)
        {
            normals->push_back(osg::Vec3(0, 0, 1));
        }
        
        geometry->setVertexArray(vertices);
        geometry->setNormalArray(normals);
        geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
        
        // 绘制主体底面
        osg::ref_ptr<osg::DrawArrays> mainBottomFace = new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4);
        geometry->addPrimitiveSet(mainBottomFace);
        
        // 绘制主体顶面
        osg::ref_ptr<osg::DrawArrays> mainTopFace = new osg::DrawArrays(osg::PrimitiveSet::QUADS, 4, 4);
        geometry->addPrimitiveSet(mainTopFace);
        
        // 绘制主体侧面
        for (int i = 0; i < 4; ++i)
        {
            int next = (i + 1) % 4;
            osg::ref_ptr<osg::DrawArrays> sideFace = new osg::DrawArrays(osg::PrimitiveSet::QUADS, i, 4);
            geometry->addPrimitiveSet(sideFace);
        }
        
        // 绘制翼部底面
        osg::ref_ptr<osg::DrawArrays> wingBottomFace = new osg::DrawArrays(osg::PrimitiveSet::QUADS, 8, 4);
        geometry->addPrimitiveSet(wingBottomFace);
        
        // 绘制翼部顶面
        osg::ref_ptr<osg::DrawArrays> wingTopFace = new osg::DrawArrays(osg::PrimitiveSet::QUADS, 12, 4);
        geometry->addPrimitiveSet(wingTopFace);
        
        // 绘制翼部侧面
        for (int i = 0; i < 4; ++i)
        {
            int next = (i + 1) % 4;
            osg::ref_ptr<osg::DrawArrays> sideFace = new osg::DrawArrays(osg::PrimitiveSet::QUADS, 8 + i, 4);
            geometry->addPrimitiveSet(sideFace);
        }
    }
}

// ==================== 绘制完成检查和控制点验证 ====================

bool LHouse3D_Geo::isDrawingComplete() const
{
    // L型房屋需要4个控制点：基点、主体尺寸点、翼部尺寸点、高度点
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    return controlPoints.size() >= 4;
}

bool LHouse3D_Geo::areControlPointsValid() const
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
    
    // 检查尺寸是否有效
    if (controlPoints.size() >= 4) {
        const Point3D& basePoint = controlPoints[0];
        const Point3D& mainSizePoint = controlPoints[1];
        const Point3D& wingSizePoint = controlPoints[2];
        const Point3D& heightPoint = controlPoints[3];
        
        float mainWidth = abs(mainSizePoint.x() - basePoint.x());
        float mainLength = abs(mainSizePoint.y() - basePoint.y());
        float wingWidth = abs(wingSizePoint.x() - basePoint.x());
        float wingLength = abs(wingSizePoint.y() - basePoint.y());
        float height = abs(heightPoint.z() - basePoint.z());
        
        if (mainWidth <= 0.0f || mainLength <= 0.0f || wingWidth <= 0.0f || wingLength <= 0.0f || height <= 0.0f ||
            std::isnan(mainWidth) || std::isnan(mainLength) || std::isnan(wingWidth) || std::isnan(wingLength) || std::isnan(height) ||
            std::isinf(mainWidth) || std::isinf(mainLength) || std::isinf(wingWidth) || std::isinf(wingLength) || std::isinf(height)) {
            return false;
        }
    }
    
    return true;
} 