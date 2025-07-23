#include "Ellipsoid3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include "../../util/MathUtils.h"
#include <cmath>
#include <QKeyEvent>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Ellipsoid3D_Geo::Ellipsoid3D_Geo()
{
    m_geoType = Geo_Ellipsoid3D;
    m_radii = glm::vec3(1.0f, 0.8f, 0.6f);  // 默认椭球半径
    m_segments = 16;  // 默认细分段数
    
    // 初始化椭圆参数
    m_center = glm::vec3(0);
    m_majorAxis = glm::vec3(1, 0, 0);
    m_minorAxis = glm::vec3(0, 1, 0);
    m_majorRadius = 1.0f;
    m_minorRadius = 0.5f;
    
    initialize();
}

// ==================== 多阶段绘制支持实现 ====================

std::vector<StageDescriptor> Ellipsoid3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    
    // 第一阶段：绘制直径（需要2个点）
    descriptors.emplace_back("绘制椭圆直径", 2, 2);
    
    // 第二阶段：选择垂直点确定椭圆形状（需要1个点）
    descriptors.emplace_back("选择椭圆高度点", 1, 1);
    
    return descriptors;
}

void Ellipsoid3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) {
        return;
    }
    
    int currentStage = getCurrentStage();
    const StageDescriptor* desc = getCurrentStageDescriptor();
    
    if (!desc) {
        return;
    }
    
    // 处理右键切换阶段
    if (event->button() == Qt::RightButton) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                nextStage();
                qDebug() << "椭圆: 进入阶段" << getCurrentStage() + 1;
            }
        }
        return;
    }
    
    // 处理左键添加控制点
    if (event->button() == Qt::LeftButton) {
        bool success = false;
        
        if (currentStage == 0) {
            // 第一阶段：直接添加控制点
            success = mm_controlPoint()->addControlPointToCurrentStage(Point3D(worldPos));
        }
        else if (currentStage == 1) {
            // 第二阶段：计算垂直投影点
            Point3D projectedPoint = calculatePerpendicularPoint(Point3D(worldPos));
            success = mm_controlPoint()->addControlPointToCurrentStage(projectedPoint);
        }
        
        if (success) {
            // 检查是否所有阶段都完成
            if (isAllStagesComplete() && areControlPointsValid()) {
                calculateEllipseParameters();
                mm_state()->setStateComplete();
                qDebug() << "椭圆: 绘制完成";
            }
        }
    }
}

void Ellipsoid3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) {
        return;
    }
    
    int currentStage = getCurrentStage();
    
    if (currentStage == 0) {
        // 第一阶段：直接设置临时点
        mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
    }
    else if (currentStage == 1) {
        // 第二阶段：计算垂直投影点作为临时点
        Point3D projectedPoint = calculatePerpendicularPoint(Point3D(worldPos));
        mm_controlPoint()->setCurrentStageTempPoint(projectedPoint);
    }
}

void Ellipsoid3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // 回车键完成当前阶段
        if (isCurrentStageComplete() && canAdvanceToNextStage()) {
            nextStage();
            qDebug() << "椭圆: 回车键进入阶段" << getCurrentStage() + 1;
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        // ESC键撤销当前阶段的最后一个控制点
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
    }
}

// ==================== 多阶段几何构建方法实现 ====================

void Ellipsoid3D_Geo::buildStageVertexGeometries(int stage)
{
    // 阶段特定的顶点几何体构建
    // 这里可以根据不同阶段绘制不同的控制点样式
}

void Ellipsoid3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 0) {
        buildDiameterStageGeometry();
    }
    else if (stage == 1) {
        buildEllipseStageGeometry();
    }
}

void Ellipsoid3D_Geo::buildStageFaceGeometries(int stage)
{
    // 只有当所有阶段完成后才绘制面
    if (isAllStagesComplete()) {
        // 可以调用原有的面绘制方法
    }
}

void Ellipsoid3D_Geo::buildCurrentStagePreviewGeometries()
{
    int currentStage = getCurrentStage();
    
    if (currentStage == 0) {
        buildDiameterPreview();
    }
    else if (currentStage == 1) {
        buildEllipsePreview();
    }
}

// ==================== 阶段特定的辅助方法实现 ====================

void Ellipsoid3D_Geo::buildDiameterStageGeometry()
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
    
    // 添加直径线
    const Point3D& p1 = stagePoints[0];
    const Point3D& p2 = stagePoints[1];
    
    vertices->push_back(osg::Vec3(p1.x(), p1.y(), p1.z()));
    vertices->push_back(osg::Vec3(p2.x(), p2.y(), p2.z()));
    
    geometry->setVertexArray(vertices);
    
    // 绘制直径线
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2);
    geometry->addPrimitiveSet(drawArrays);
}

void Ellipsoid3D_Geo::buildDiameterPreview()
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    if (stagePoints.empty()) {
        return;
    }
    
    // 如果有临时点，绘制预览线
    if (stagePoints.size() == 2) { // 包含临时点
        buildDiameterStageGeometry();
    }
}

void Ellipsoid3D_Geo::buildEllipseStageGeometry()
{
    if (!isAllStagesComplete()) {
        return;
    }
    
    // 获取所有阶段的控制点
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.size() < 2 || allStages[0].size() < 2 || allStages[1].size() < 1) {
        return;
    }
    
    // 计算椭圆参数
    calculateEllipseParameters();
    
    // 获取边几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid()) {
        return;
    }
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 生成椭圆边界点
    // 使用细分级别参数而不是固定值
    int segments = static_cast<int>(m_parameters.subdivisionLevel);
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float cos_a = cos(angle);
        float sin_a = sin(angle);
        
        glm::vec3 point = m_center + 
                         cos_a * m_majorRadius * m_majorAxis + 
                         sin_a * m_minorRadius * m_minorAxis;
        
        vertices->push_back(osg::Vec3(point.x, point.y, point.z));
    }
    
    geometry->setVertexArray(vertices);
    
    // 绘制椭圆
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void Ellipsoid3D_Geo::buildEllipsePreview()
{
    if (getCurrentStage() != 1) {
        return;
    }
    
    const auto& currentStagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    if (currentStagePoints.empty()) {
        return;
    }
    
    // 如果有临时点，绘制预览椭圆
    if (currentStagePoints.size() == 1) { // 包含临时点
        buildEllipseStageGeometry();
    }
}

Point3D Ellipsoid3D_Geo::calculatePerpendicularPoint(const Point3D& mousePos) const
{
    // 获取第一阶段的两个点（直径端点）
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].size() < 2) {
        return mousePos; // 如果没有足够的点，返回原始位置
    }
    
    const Point3D& p1 = allStages[0][0];
    const Point3D& p2 = allStages[0][1];
    
    // 计算直径的中点
    glm::vec3 center = (p1.position + p2.position) * 0.5f;
    
    // 计算直径的方向向量
    glm::vec3 diameterDir = glm::normalize(p2.position - p1.position);
    
    // 计算从中心到鼠标位置的向量
    glm::vec3 toMouse = mousePos.position - center;
    
    // 计算投影到直径方向的分量
    float projLength = glm::dot(toMouse, diameterDir);
    
    // 计算垂直分量
    glm::vec3 perpComponent = toMouse - projLength * diameterDir;
    
    // 返回在中垂线上的投影点
    glm::vec3 projectedPos = center + perpComponent;
    
    return Point3D(projectedPos);
}

void Ellipsoid3D_Geo::calculateEllipseParameters()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.size() < 2 || allStages[0].size() < 2 || allStages[1].size() < 1) {
        return;
    }
    
    // 第一阶段的两个点定义长轴
    const Point3D& p1 = allStages[0][0];
    const Point3D& p2 = allStages[0][1];
    
    // 第二阶段的点定义短轴长度
    const Point3D& heightPoint = allStages[1][0];
    
    // 计算椭圆中心（长轴中点）
    m_center = (p1.position + p2.position) * 0.5f;
    
    // 计算长轴方向和长度
    m_majorAxis = glm::normalize(p2.position - p1.position);
    m_majorRadius = glm::length(p2.position - p1.position) * 0.5f;
    
    // 计算短轴方向和长度
    glm::vec3 toHeight = heightPoint.position - m_center;
    float projLength = glm::dot(toHeight, m_majorAxis);
    glm::vec3 perpComponent = toHeight - projLength * m_majorAxis;
    m_minorRadius = glm::length(perpComponent);
    
    if (m_minorRadius > 0.0f) {
        m_minorAxis = glm::normalize(perpComponent);
    }
    
    qDebug() << "椭圆参数: 中心(" << m_center.x << "," << m_center.y << "," << m_center.z 
             << ") 长轴半径:" << m_majorRadius << " 短轴半径:" << m_minorRadius;
}

bool Ellipsoid3D_Geo::isValidEllipseConfiguration() const
{
    return m_majorRadius > 0.0f && m_minorRadius > 0.0f && 
           glm::length(m_majorAxis) > 0.9f && glm::length(m_minorAxis) > 0.9f;
}

// ============================================================================
// 传统几何体构建实现（保持兼容性）
// ============================================================================

void Ellipsoid3D_Geo::buildVertexGeometries()
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
    
    // 点绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void Ellipsoid3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    // 根据当前阶段调用不同的绘制方法
    int currentStage = getCurrentStage();
    buildStageEdgeGeometries(currentStage);
}

void Ellipsoid3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    // 只有所有阶段完成后才绘制面
    if (!isAllStagesComplete()) {
        return;
    }
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getFaceGeometry();
    if (!geometry.valid()) {
        return;
    }
    
    if (!isValidEllipseConfiguration()) {
        return;
    }
    
    // 创建顶点数组和法向量数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    // 生成椭圆面（这里简化为一个平面椭圆）
    // 使用细分级别参数而不是固定值
    int segments = static_cast<int>(m_parameters.subdivisionLevel);
    
    // 添加中心点
    vertices->push_back(osg::Vec3(m_center.x, m_center.y, m_center.z));
    glm::vec3 faceNormal = glm::cross(m_majorAxis, m_minorAxis);
    if (glm::length(faceNormal) > 0.0f) {
        faceNormal = glm::normalize(faceNormal);
    }
    normals->push_back(osg::Vec3(faceNormal.x, faceNormal.y, faceNormal.z));
    
    // 生成椭圆边界点
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float cos_a = cos(angle);
        float sin_a = sin(angle);
        
        glm::vec3 point = m_center + 
                         cos_a * m_majorRadius * m_majorAxis + 
                         sin_a * m_minorRadius * m_minorAxis;
        
        vertices->push_back(osg::Vec3(point.x, point.y, point.z));
        normals->push_back(osg::Vec3(faceNormal.x, faceNormal.y, faceNormal.z));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 绘制三角形扇形
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

// ==================== 绘制完成检查和控制点验证 ====================

bool Ellipsoid3D_Geo::isDrawingComplete() const
{
    // 椭圆需要所有阶段都完成
    return isAllStagesComplete();
}

bool Ellipsoid3D_Geo::areControlPointsValid() const
{
    if (!isAllStagesComplete()) {
        return false;
    }
    
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.size() < 2 || allStages[0].size() < 2 || allStages[1].size() < 1) {
        return false;
    }
    
    // 检查控制点坐标是否有效
    for (const auto& stage : allStages) {
        for (const auto& point : stage) {
            if (std::isnan(point.x()) || std::isnan(point.y()) || std::isnan(point.z()) ||
                std::isinf(point.x()) || std::isinf(point.y()) || std::isinf(point.z())) {
                return false;
            }
        }
    }
    
    // 检查椭圆配置是否有效
    return isValidEllipseConfiguration();
} 