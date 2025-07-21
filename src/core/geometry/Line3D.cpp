#include "Line3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Line3D_Geo::Line3D_Geo()
    : m_totalLength(0.0f)
{
    m_geoType = Geo_Line3D;
    // 确保基类正确初始化
    initialize();
}

// ==================== 多阶段绘制支持实现 ====================

std::vector<StageDescriptor> Line3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    
    // 线只有一个阶段：绘制起点和终点（需要2个点）
    descriptors.emplace_back("绘制线段", 2, 2);
    
    return descriptors;
}

void Line3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
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
            // 检查是否所有阶段都完成
            if (isAllStagesComplete() && areControlPointsValid()) {
                calculateLineParameters();
                mm_state()->setStateComplete();
                qDebug() << "线: 绘制完成";
            }
        }
    }
}

void Line3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) {
        return;
    }
    
    // 设置临时点用于预览
    mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
}

void Line3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // 回车键完成绘制
        if (isCurrentStageComplete() && areControlPointsValid()) {
            calculateLineParameters();
            mm_state()->setStateComplete();
            qDebug() << "线: 回车键完成绘制";
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        // ESC键撤销最后一个控制点
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
    }
}

// ==================== 多阶段几何构建方法实现 ====================

void Line3D_Geo::buildStageVertexGeometries(int stage)
{
    if (stage == 0) {
        buildLineStageGeometry();
    }
}

void Line3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 0) {
        buildLineStageGeometry();
    }
}

void Line3D_Geo::buildStageFaceGeometries(int stage)
{
    // 线没有面
}

void Line3D_Geo::buildCurrentStagePreviewGeometries()
{
    buildLinePreview();
}

// ==================== 阶段特定的辅助方法实现 ====================

void Line3D_Geo::buildLineStageGeometry()
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    if (stagePoints.size() < 2) {
        return;
    }
    
    // 获取边几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid()) {
        return;
    }
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 添加线段的两个端点
    for (const Point3D& point : stagePoints) {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
    }
    
    geometry->setVertexArray(vertices);
    
    // 线绘制
    if (vertices->size() >= 2) {
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
        geometry->addPrimitiveSet(drawArrays);
    }
}

void Line3D_Geo::buildLinePreview()
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    if (stagePoints.size() < 2) {
        return;
    }
    
    // 如果有临时点，绘制预览线
    buildLineStageGeometry();
}

void Line3D_Geo::calculateLineParameters()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].size() < 2) {
        return;
    }
    
    const Point3D& p1 = allStages[0][0];
    const Point3D& p2 = allStages[0][1];
    
    // 计算线段长度
    m_totalLength = glm::length(p2.position - p1.position);
    
    qDebug() << "线段参数: 起点(" << p1.x() << "," << p1.y() << "," << p1.z() 
             << ") 终点(" << p2.x() << "," << p2.y() << "," << p2.z() 
             << ") 长度:" << m_totalLength;
}

bool Line3D_Geo::isValidLineConfiguration() const
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    
    if (stagePoints.size() < 2) {
        return false;
    }
    
    // 检查两点是否重合
    const float epsilon = 0.001f;
    glm::vec3 diff = stagePoints[1].position - stagePoints[0].position;
    float distance = glm::length(diff);
    
    if (distance < epsilon) {
        return false; // 两点重合，无效
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

void Line3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
    // 获取所有控制点用于绘制
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.empty()) {
        return;
    }
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getVertexGeometry();
    if (!geometry.valid()) {
        return;
    }
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 添加所有控制点
    for (const Point3D& point : controlPoints) {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
    }
    
    geometry->setVertexArray(vertices);
    
    // 点绘制 - 控制点
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void Line3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    // 调用阶段特定的绘制方法
    buildStageEdgeGeometries(getCurrentStage());
}

void Line3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    // 线对象没有面
}

// ==================== 绘制完成检查和控制点验证 ====================

bool Line3D_Geo::isDrawingComplete() const
{
    // 线需要所有阶段都完成
    return isAllStagesComplete();
}

bool Line3D_Geo::areControlPointsValid() const
{
    if (!isAllStagesComplete()) {
        return false;
    }
    
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.size() < 1 || allStages[0].size() < 2) {
        return false;
    }
    
    // 检查线配置是否有效
    return isValidLineConfiguration();
}
