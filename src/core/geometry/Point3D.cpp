#include "Point3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
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

// ==================== 多阶段绘制支持实现 ====================

std::vector<StageDescriptor> Point3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    
    // 点只有一个阶段：选择位置（需要1个点）
    descriptors.emplace_back("选择点位置", 1, 1);
    
    return descriptors;
}

void Point3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) {
        return;
    }
    
    const StageDescriptor* desc = getCurrentStageDescriptor();
    if (!desc) {
        return;
    }
    
    // 处理左键添加控制点
    if (event->button() == Qt::LeftButton) {
        bool success = mm_controlPoint()->addControlPointToCurrentStage(Point3D(worldPos));
        
        if (success) {
            // 点只需要一个控制点就完成
            if (isAllStagesComplete() && areControlPointsValid()) {
                mm_state()->setStateComplete();
                qDebug() << "点: 绘制完成";
            }
        }
    }
}

void Point3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) {
        return;
    }
    
    // 设置临时点用于预览
    if (mm_controlPoint()->getCurrentStageControlPointCount() == 0) {
        mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
    }
}

void Point3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        // ESC键撤销当前阶段的最后一个控制点
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
    }
}

// ==================== 多阶段几何构建方法实现 ====================

void Point3D_Geo::buildStageVertexGeometries(int stage)
{
    if (stage == 0) {
        buildPointStageGeometry();
    }
}

void Point3D_Geo::buildStageEdgeGeometries(int stage)
{
    // 点没有边
}

void Point3D_Geo::buildStageFaceGeometries(int stage)
{
    // 点没有面
}

void Point3D_Geo::buildCurrentStagePreviewGeometries()
{
    buildPointPreview();
}

// ==================== 阶段特定的辅助方法实现 ====================

void Point3D_Geo::buildPointStageGeometry()
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    if (stagePoints.empty()) {
        return;
    }
    
    // 获取顶点几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getVertexGeometry();
    if (!geometry.valid()) {
        return;
    }
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 添加点
    for (const Point3D& point : stagePoints) {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
    }
    
    geometry->setVertexArray(vertices);
    
    // 点绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void Point3D_Geo::buildPointPreview()
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    if (stagePoints.empty()) {
        return;
    }
    
    // 如果有临时点，绘制预览点
    buildPointStageGeometry();
}

bool Point3D_Geo::isValidPointConfiguration() const
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    
    if (stagePoints.empty()) {
        return false;
    }
    
    // 检查控制点坐标是否有效
    for (const auto& point : stagePoints) {
        if (std::isnan(point.x()) || std::isnan(point.y()) || std::isnan(point.z()) ||
            std::isinf(point.x()) || std::isinf(point.y()) || std::isinf(point.z())) {
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// 传统几何体构建实现（保持兼容性）
// ============================================================================

void Point3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
    // 调用阶段特定的绘制方法
    buildStageVertexGeometries(getCurrentStage());
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
    // 点需要所有阶段都完成
    return isAllStagesComplete();
}

bool Point3D_Geo::areControlPointsValid() const
{
    if (!isAllStagesComplete()) {
        return false;
    }
    
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].empty()) {
        return false;
    }
    
    // 检查点配置是否有效
    return isValidPointConfiguration();
}
