#include "Cone3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Cone3D_Geo::Cone3D_Geo()
    : m_radius(1.0f)
    , m_height(2.0f)
    , m_segments(16)
    , m_axis(0, 0, 1)
{
    m_geoType = Geo_Cone3D;
    
    // 初始化圆锥几何计算缓存
    m_baseCenter = glm::vec3(0);
    m_apexPoint = glm::vec3(0, 0, 2);
    m_baseNormal = glm::vec3(0, 0, 1);
    m_calculatedRadius = 1.0f;
    m_calculatedHeight = 2.0f;
    
    // 确保基类正确初始化
    initialize();
}

// ==================== 多阶段绘制支持实现 ====================

std::vector<StageDescriptor> Cone3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    
    // 第一阶段：绘制底面直径（需要2个点）
    descriptors.emplace_back("绘制圆锥底面直径", 2, 2);
    
    // 第二阶段：选择高度点确定圆锥顶点（需要1个点）
    descriptors.emplace_back("选择圆锥顶点", 1, 1);
    
    return descriptors;
}

void Cone3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
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
                qDebug() << "圆锥: 进入阶段" << getCurrentStage() + 1;
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
            // 第二阶段：计算中垂线上的投影点
            Point3D projectedPoint = calculatePerpendicularHeightPoint(Point3D(worldPos));
            success = mm_controlPoint()->addControlPointToCurrentStage(projectedPoint);
        }
        
        if (success) {
            // 检查是否所有阶段都完成
            if (isAllStagesComplete() && areControlPointsValid()) {
                calculateConeParameters();
                mm_state()->setStateComplete();
                qDebug() << "圆锥: 绘制完成";
            }
        }
    }
}

void Cone3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
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
        // 第二阶段：计算中垂线上的投影点作为临时点
        Point3D projectedPoint = calculatePerpendicularHeightPoint(Point3D(worldPos));
        mm_controlPoint()->setCurrentStageTempPoint(projectedPoint);
    }
}

void Cone3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // 回车键完成当前阶段
        if (isCurrentStageComplete() && canAdvanceToNextStage()) {
            nextStage();
            qDebug() << "圆锥: 回车键进入阶段" << getCurrentStage() + 1;
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        // ESC键撤销当前阶段的最后一个控制点
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
    }
}

// ==================== 多阶段几何构建方法实现 ====================

void Cone3D_Geo::buildStageVertexGeometries(int stage)
{
    // 阶段特定的顶点几何体构建
    // 这里可以根据不同阶段绘制不同的控制点样式
}

void Cone3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 0) {
        buildBaseDiameterStageGeometry();
    }
    else if (stage == 1) {
        buildConeStageGeometry();
    }
}

void Cone3D_Geo::buildStageFaceGeometries(int stage)
{
    // 只有当所有阶段完成后才绘制面
    if (isAllStagesComplete()) {
        // 可以调用原有的面绘制方法
    }
}

void Cone3D_Geo::buildCurrentStagePreviewGeometries()
{
    int currentStage = getCurrentStage();
    
    if (currentStage == 0) {
        buildBaseDiameterPreview();
    }
    else if (currentStage == 1) {
        buildConePreview();
    }
}

// ==================== 阶段特定的辅助方法实现 ====================

void Cone3D_Geo::buildBaseDiameterStageGeometry()
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
    
    // 计算圆心和半径，绘制底面圆
    glm::vec3 center = (p1.position + p2.position) * 0.5f;
    float radius = glm::length(p2.position - p1.position) * 0.5f;
    glm::vec3 direction = glm::normalize(p2.position - p1.position);
    
    // 创建垂直于直径的两个向量
    glm::vec3 up = glm::vec3(0, 0, 1);
    if (abs(glm::dot(direction, up)) > 0.9f) {
        up = glm::vec3(0, 1, 0);
    }
    glm::vec3 u = glm::normalize(glm::cross(direction, up));
    glm::vec3 v = glm::normalize(glm::cross(direction, u));
    
    // 生成圆周点
    int segments = 16;
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float cos_a = cos(angle);
        float sin_a = sin(angle);
        
        glm::vec3 point = center + radius * (cos_a * u + sin_a * v);
        vertices->push_back(osg::Vec3(point.x, point.y, point.z));
    }
    
    geometry->setVertexArray(vertices);
    
    // 绘制直径线
    osg::ref_ptr<osg::DrawArrays> diameterLine = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2);
    geometry->addPrimitiveSet(diameterLine);
    
    // 绘制底面圆
    osg::ref_ptr<osg::DrawArrays> circle = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 2, segments + 1);
    geometry->addPrimitiveSet(circle);
}

void Cone3D_Geo::buildBaseDiameterPreview()
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    if (stagePoints.empty()) {
        return;
    }
    
    // 如果有临时点，绘制预览线和圆
    if (stagePoints.size() == 2) { // 包含临时点
        buildBaseDiameterStageGeometry();
    }
}

void Cone3D_Geo::buildConeStageGeometry()
{
    if (!isAllStagesComplete()) {
        return;
    }
    
    // 获取所有阶段的控制点
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.size() < 2 || allStages[0].size() < 2 || allStages[1].size() < 1) {
        return;
    }
    
    // 计算圆锥参数
    calculateConeParameters();
    
    // 获取边几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid()) {
        return;
    }
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 生成底面圆
    int segments = 16;
    glm::vec3 direction = glm::normalize(m_apexPoint - m_baseCenter);
    
    // 创建垂直于轴的两个向量
    glm::vec3 up = glm::vec3(0, 0, 1);
    if (abs(glm::dot(direction, up)) > 0.9f) {
        up = glm::vec3(0, 1, 0);
    }
    glm::vec3 u = glm::normalize(glm::cross(direction, up));
    glm::vec3 v = glm::normalize(glm::cross(direction, u));
    
    // 添加底面圆周点
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float cos_a = cos(angle);
        float sin_a = sin(angle);
        
        glm::vec3 point = m_baseCenter + m_calculatedRadius * (cos_a * u + sin_a * v);
        vertices->push_back(osg::Vec3(point.x, point.y, point.z));
    }
    
    // 添加顶点
    vertices->push_back(osg::Vec3(m_apexPoint.x, m_apexPoint.y, m_apexPoint.z));
    
    geometry->setVertexArray(vertices);
    
    // 绘制底面圆
    osg::ref_ptr<osg::DrawArrays> circle = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, segments + 1);
    geometry->addPrimitiveSet(circle);
    
    // 绘制侧边线（从底面到顶点）
    for (int i = 0; i < segments; i += segments / 4) { // 只显示4条主要的侧边线
        osg::ref_ptr<osg::DrawArrays> sideLine = new osg::DrawArrays(osg::PrimitiveSet::LINES, i, 1);
        geometry->addPrimitiveSet(sideLine);
        
        // 添加到顶点的线
        osg::ref_ptr<osg::DrawArrays> apexLine = new osg::DrawArrays(osg::PrimitiveSet::LINES, segments + 1, 1);
        geometry->addPrimitiveSet(apexLine);
    }
}

void Cone3D_Geo::buildConePreview()
{
    if (getCurrentStage() != 1) {
        return;
    }
    
    const auto& currentStagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    if (currentStagePoints.empty()) {
        return;
    }
    
    // 如果有临时点，绘制预览圆锥
    if (currentStagePoints.size() == 1) { // 包含临时点
        buildConeStageGeometry();
    }
}

Point3D Cone3D_Geo::calculatePerpendicularHeightPoint(const Point3D& mousePos) const
{
    // 获取第一阶段的两个点（底面直径端点）
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].size() < 2) {
        return mousePos; // 如果没有足够的点，返回原始位置
    }
    
    const Point3D& p1 = allStages[0][0];
    const Point3D& p2 = allStages[0][1];
    
    // 计算底面中心
    glm::vec3 baseCenter = (p1.position + p2.position) * 0.5f;
    
    // 计算底面法向量（假设底面垂直于直径所在平面的法向量）
    glm::vec3 diameterDir = glm::normalize(p2.position - p1.position);
    
    // 计算从底面中心到鼠标位置的向量
    glm::vec3 toMouse = mousePos.position - baseCenter;
    
    // 计算投影到直径方向的分量（应该是0，保持在中垂线上）
    float projLength = glm::dot(toMouse, diameterDir);
    
    // 计算垂直分量（这是我们想要的高度方向）
    glm::vec3 heightComponent = toMouse - projLength * diameterDir;
    
    // 返回在中垂线上的投影点
    glm::vec3 projectedPos = baseCenter + heightComponent;
    
    return Point3D(projectedPos);
}

void Cone3D_Geo::calculateConeParameters()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.size() < 2 || allStages[0].size() < 2 || allStages[1].size() < 1) {
        return;
    }
    
    // 第一阶段的两个点定义底面直径
    const Point3D& p1 = allStages[0][0];
    const Point3D& p2 = allStages[0][1];
    
    // 第二阶段的点定义顶点位置
    const Point3D& apexPoint = allStages[1][0];
    
    // 计算底面中心
    m_baseCenter = (p1.position + p2.position) * 0.5f;
    
    // 计算底面半径
    m_calculatedRadius = glm::length(p2.position - p1.position) * 0.5f;
    
    // 设置顶点位置
    m_apexPoint = apexPoint.position;
    
    // 计算高度和轴方向
    glm::vec3 axisVector = m_apexPoint - m_baseCenter;
    m_calculatedHeight = glm::length(axisVector);
    
    if (m_calculatedHeight > 0.0f) {
        m_baseNormal = glm::normalize(axisVector);
    }
    
    // 更新成员变量
    m_radius = m_calculatedRadius;
    m_height = m_calculatedHeight;
    m_axis = m_baseNormal;
    
    qDebug() << "圆锥参数: 底面中心(" << m_baseCenter.x << "," << m_baseCenter.y << "," << m_baseCenter.z 
             << ") 半径:" << m_calculatedRadius << " 高度:" << m_calculatedHeight;
}

bool Cone3D_Geo::isValidConeConfiguration() const
{
    return m_calculatedRadius > 0.0f && m_calculatedHeight > 0.0f;
}

// ============================================================================
// 传统几何体构建实现（保持兼容性）
// ============================================================================

void Cone3D_Geo::buildVertexGeometries()
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

void Cone3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    // 根据当前阶段调用不同的绘制方法
    int currentStage = getCurrentStage();
    buildStageEdgeGeometries(currentStage);
}

void Cone3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    // 只有所有阶段完成后才绘制面
    if (!isAllStagesComplete()) {
        return;
    }
    
    if (!isValidConeConfiguration()) {
        return;
    }
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getFaceGeometry();
    if (!geometry.valid()) {
        return;
    }
    
    // 创建圆锥底面和侧面几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    int segments = static_cast<int>(m_parameters.subdivisionLevel);
    // 确保最小细分级别，防止几何体过于简单
    if (segments < 8) segments = 8;
    
    // 使用lambda表达式计算圆锥体参数
    auto calculateConeParams = [&]() -> MathUtils::ConeParameters {
        return MathUtils::calculateConeParameters(m_baseCenter, m_apexPoint, m_calculatedRadius);
    };
    
    auto coneParams = calculateConeParams();
    glm::vec3 u = coneParams.uAxis;
    glm::vec3 v = coneParams.vAxis;
    glm::vec3 axis = coneParams.axis;
    
    // 生成底面三角形
    for (int i = 0; i < segments; ++i) {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1 = static_cast<float>(cos(angle1)) * u + static_cast<float>(sin(angle1)) * v;
        glm::vec3 dir2 = static_cast<float>(cos(angle2)) * u + static_cast<float>(sin(angle2)) * v;
        
        glm::vec3 p1 = m_baseCenter + m_calculatedRadius * dir1;
        glm::vec3 p2 = m_baseCenter + m_calculatedRadius * dir2;
        
        vertices->push_back(osg::Vec3(m_baseCenter.x, m_baseCenter.y, m_baseCenter.z));
        vertices->push_back(osg::Vec3(p1.x, p1.y, p1.z));
        vertices->push_back(osg::Vec3(p2.x, p2.y, p2.z));
        
        for (int j = 0; j < 3; ++j) {
            normals->push_back(osg::Vec3(-axis.x, -axis.y, -axis.z));  // 底面法向量
            colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                       m_parameters.fillColor.b, m_parameters.fillColor.a));
        }
    }
    
    // 生成侧面三角形
    for (int i = 0; i < segments; ++i) {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1 = static_cast<float>(cos(angle1)) * u + static_cast<float>(sin(angle1)) * v;
        glm::vec3 dir2 = static_cast<float>(cos(angle2)) * u + static_cast<float>(sin(angle2)) * v;
        
        glm::vec3 p1 = m_baseCenter + m_calculatedRadius * dir1;
        glm::vec3 p2 = m_baseCenter + m_calculatedRadius * dir2;
        
        // 第一个三角形：底面点到顶点到下一个底面点
        vertices->push_back(osg::Vec3(p1.x, p1.y, p1.z));
        vertices->push_back(osg::Vec3(m_apexPoint.x, m_apexPoint.y, m_apexPoint.z));
        vertices->push_back(osg::Vec3(p2.x, p2.y, p2.z));
        
        // 计算侧面法向量
        glm::vec3 sideNormal = glm::normalize(glm::cross(p2 - p1, m_apexPoint - p1));
        
        for (int j = 0; j < 3; ++j) {
            normals->push_back(osg::Vec3(sideNormal.x, sideNormal.y, sideNormal.z));
            colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                       m_parameters.fillColor.b, m_parameters.fillColor.a));
        }
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用三角面绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

// ==================== 绘制完成检查和控制点验证 ====================

bool Cone3D_Geo::isDrawingComplete() const
{
    // 圆锥需要所有阶段都完成
    return isAllStagesComplete();
}

bool Cone3D_Geo::areControlPointsValid() const
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
    
    // 检查圆锥配置是否有效
    return isValidConeConfiguration();
}
