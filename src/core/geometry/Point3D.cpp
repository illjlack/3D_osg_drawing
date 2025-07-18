﻿#include "Point3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include "../../util/MathUtils.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Point3D_Geo::Point3D_Geo()
{
    m_geoType = Geo_Point3D;
    // 确保基类正确初始化
    initialize();
}

void Point3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
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

void Point3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 点对象的鼠标移动事件 - 默认实现
    // 点对象不需要处理鼠标移动事件
}

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void Point3D_Geo::buildVertexGeometries()
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
    const Point3D& point = controlPoints[0];
    
    // 添加控制点
    vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
    
    geometry->setVertexArray(vertices);
    
    // 点绘制 - 控制点
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void Point3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    // 点对象没有边
}

void Point3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    // 点对象没有面
}

// ==================== 绘制完成检查和控制点验证 ====================

bool Point3D_Geo::isDrawingComplete() const
{
    // 点几何体只需要1个控制点就能完成绘制
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    return controlPoints.size() >= 1;
}

bool Point3D_Geo::areControlPointsValid() const
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
