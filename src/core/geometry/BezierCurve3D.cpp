#include "BezierCurve3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

BezierCurve3D_Geo::BezierCurve3D_Geo()
{
    m_geoType = Geo_BezierCurve3D;
    // 确保基类正确初始化
    initialize();
}

// ==================== 多阶段绘制支持实现 ====================

std::vector<StageDescriptor> BezierCurve3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    
    // 贝塞尔曲线只有一个阶段：连续添加控制点（最少2个，最多无限制）
    descriptors.emplace_back("绘制贝塞尔曲线控制点", 2, -1);
    
    return descriptors;
}

void BezierCurve3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) {
        return;
    }
    
    const StageDescriptor* desc = getCurrentStageDescriptor();
    if (!desc) {
        return;
    }
    
    // 处理右键完成绘制
    if (event->button() == Qt::RightButton) {
        if (isCurrentStageComplete()) {
            // 右键完成绘制
            if (areControlPointsValid()) {
                mm_state()->setStateComplete();
                qDebug() << "贝塞尔曲线: 绘制完成";
            }
        }
        return;
    }
    
    // 处理左键添加控制点
    if (event->button() == Qt::LeftButton) {
        bool success = mm_controlPoint()->addControlPointToCurrentStage(Point3D(worldPos));
        
        if (success) {
            qDebug() << "贝塞尔曲线: 添加控制点，当前数量:" << mm_controlPoint()->getCurrentStageControlPointCount();
        }
    }
}

void BezierCurve3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) {
        return;
    }
    
    // 设置临时点用于预览
    mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
}

void BezierCurve3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // 回车键完成绘制
        if (isCurrentStageComplete() && areControlPointsValid()) {
            mm_state()->setStateComplete();
            qDebug() << "贝塞尔曲线: 回车键完成绘制";
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        // ESC键撤销最后一个控制点
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
    }
}

// ==================== 多阶段几何构建方法实现 ====================

void BezierCurve3D_Geo::buildStageVertexGeometries(int stage)
{
    // 贝塞尔曲线的控制点绘制
    if (stage == 0) {
        buildBezierCurveStageGeometry();
    }
}

void BezierCurve3D_Geo::buildStageEdgeGeometries(int stage)
{
    // 贝塞尔曲线的边绘制
    if (stage == 0) {
        buildBezierCurveStageGeometry();
    }
}

void BezierCurve3D_Geo::buildStageFaceGeometries(int stage)
{
    // 贝塞尔曲线没有面
}

void BezierCurve3D_Geo::buildCurrentStagePreviewGeometries()
{
    buildBezierCurvePreview();
}

// ==================== 阶段特定的辅助方法实现 ====================

void BezierCurve3D_Geo::buildBezierCurveStageGeometry()
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
    
    // 生成贝塞尔曲线点
    std::vector<glm::vec3> controlVecs;
    for (const Point3D& cp : stagePoints) {
        controlVecs.push_back(cp.position);
    }
    
    auto bezierPoints = MathUtils::generateBezierCurve(controlVecs, 50);
    
    // 更新成员变量
    m_bezierPoints.clear();
    for (const auto& point : bezierPoints) {
        m_bezierPoints.push_back(Point3D(point));
        vertices->push_back(osg::Vec3(point.x, point.y, point.z));
    }
    
    geometry->setVertexArray(vertices);
    
    // 绘制贝塞尔曲线
    if (vertices->size() > 1) {
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size());
        geometry->addPrimitiveSet(drawArrays);
    }
}

void BezierCurve3D_Geo::buildBezierCurvePreview()
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    if (stagePoints.size() < 2) {
        return;
    }
    
    // 如果有临时点，绘制预览贝塞尔曲线
    buildBezierCurveStageGeometry();
}

bool BezierCurve3D_Geo::isValidBezierConfiguration() const
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    
    if (stagePoints.size() < 2) {
        return false;
    }
    
    // 检查控制点是否重合
    const float epsilon = 0.001f;
    for (size_t i = 0; i < stagePoints.size() - 1; ++i) {
        for (size_t j = i + 1; j < stagePoints.size(); ++j) {
            glm::vec3 diff = stagePoints[j].position - stagePoints[i].position;
            float distance = glm::length(diff);
            if (distance < epsilon) {
                return false; // 有重复点，无效
            }
        }
    }
    
    return true;
}

// ============================================================================
// 传统几何体构建实现（保持兼容性）
// ============================================================================

void BezierCurve3D_Geo::buildVertexGeometries()
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

void BezierCurve3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    // 调用阶段特定的绘制方法
    buildStageEdgeGeometries(getCurrentStage());
}

void BezierCurve3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    // 贝塞尔曲线没有面
}

// ==================== 绘制完成检查和控制点验证 ====================

bool BezierCurve3D_Geo::isDrawingComplete() const
{
    // 贝塞尔曲线的绘制完成由用户决定（右键或回车），而不是自动完成
    // 但是需要满足最少控制点数量
    return isCurrentStageComplete();
}

bool BezierCurve3D_Geo::areControlPointsValid() const
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    
    // 检查控制点数量
    if (stagePoints.size() < 2) {
        return false;
    }
    
    // 检查控制点坐标是否有效（不是NaN或无穷大）
    for (const auto& point : stagePoints) {
        if (std::isnan(point.x()) || std::isnan(point.y()) || std::isnan(point.z()) ||
            std::isinf(point.x()) || std::isinf(point.y()) || std::isinf(point.z())) {
            return false;
        }
    }
    
    // 检查贝塞尔曲线配置是否有效
    return isValidBezierConfiguration();
}
