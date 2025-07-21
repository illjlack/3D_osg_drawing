#include "Arc3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"
#include <QKeyEvent>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Arc3D_Geo::Arc3D_Geo()
    : m_radius(0.0f)
    , m_startAngle(0.0f)
    , m_endAngle(0.0f)
    , m_sweepAngle(0.0f)
    , m_normal(0, 0, 1)
    , m_center(0.0f)
    , m_uAxis(1, 0, 0)
    , m_vAxis(0, 1, 0)
{
    m_geoType = Geo_Arc3D;
    // 确保基类正确初始化
    initialize();
}

// ==================== 多阶段绘制支持实现 ====================

std::vector<StageDescriptor> Arc3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    
    // 第一阶段：确定圆心和半径（2个点）
    descriptors.emplace_back("绘制圆心和半径点", 2, 2);
    
    // 第二阶段：确定圆弧的角度范围（2个点）
    descriptors.emplace_back("定义圆弧角度范围", 2, 2);
    
    return descriptors;
}

void Arc3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) {
        return;
    }
    
    const StageDescriptor* desc = getCurrentStageDescriptor();
    if (!desc) {
        return;
    }
    
    // 处理右键完成绘制或进入下一阶段
    if (event->button() == Qt::RightButton) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                // 进入下一阶段
                if (nextStage()) {
                    calculateArcParameters(); // 每阶段完成后重新计算参数
                    qDebug() << "圆弧: 进入阶段" << getCurrentStage() + 1;
                }
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                // 所有阶段完成，结束绘制
                calculateArcParameters();
                mm_state()->setStateComplete();
                qDebug() << "圆弧: 绘制完成";
            }
        }
        return;
    }
    
    // 处理左键添加控制点
    if (event->button() == Qt::LeftButton) {
        Point3D adjustedPos(worldPos);
        
        // 第二阶段需要将点投影到圆上
        if (getCurrentStage() == 1) {
            adjustedPos = calculateArcPoint(Point3D(worldPos));
        }
        
        bool success = mm_controlPoint()->addControlPointToCurrentStage(adjustedPos);
        
        if (success) {
            qDebug() << "圆弧: 阶段" << getCurrentStage() + 1 << "添加控制点，当前数量:" << mm_controlPoint()->getCurrentStageControlPointCount();
            
            // 每次添加点后重新计算参数
            calculateArcParameters();
            
            // 检查是否自动进入下一阶段
            if (isCurrentStageComplete() && canAdvanceToNextStage()) {
                nextStage();
                qDebug() << "圆弧: 自动进入阶段" << getCurrentStage() + 1;
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                mm_state()->setStateComplete();
                qDebug() << "圆弧: 自动完成绘制";
            }
        }
    }
}

void Arc3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) {
        return;
    }
    
    Point3D tempPoint(worldPos);
    
    // 第二阶段需要将临时点投影到圆上
    if (getCurrentStage() == 1 && mm_controlPoint()->getCurrentStageControlPointCount() > 0) {
        tempPoint = calculateArcPoint(Point3D(worldPos));
    }
    
    // 设置临时点用于预览
    mm_controlPoint()->setCurrentStageTempPoint(tempPoint);
}

void Arc3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // 回车键完成当前阶段或整个绘制
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) {
                    calculateArcParameters();
                    qDebug() << "圆弧: 回车进入阶段" << getCurrentStage() + 1;
                }
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateArcParameters();
                mm_state()->setStateComplete();
                qDebug() << "圆弧: 回车完成绘制";
            }
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        // ESC键撤销最后一个控制点
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
        calculateArcParameters(); // 重新计算参数
    }
}

// ==================== 多阶段几何构建方法实现 ====================

void Arc3D_Geo::buildStageVertexGeometries(int stage)
{
    // 圆弧主要以边形式展现，顶点几何体不是主要的
}

void Arc3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 0) {
        buildCenterRadiusStageGeometry();
    }
    else if (stage == 1) {
        buildArcRangeStageGeometry();
    }
}

void Arc3D_Geo::buildStageFaceGeometries(int stage)
{
    // 圆弧没有面
}

void Arc3D_Geo::buildCurrentStagePreviewGeometries()
{
    int currentStage = getCurrentStage();
    if (currentStage == 0) {
        buildCenterRadiusPreview();
    }
    else if (currentStage == 1) {
        buildArcRangePreview();
    }
}

// ==================== 阶段特定的辅助方法实现 ====================

void Arc3D_Geo::buildCenterRadiusStageGeometry()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].size() < 1) {
        return;
    }
    
    const auto& stage0Points = allStages[0];
    
    // 获取边几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid()) {
        return;
    }
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 如果有两个点，绘制半径线和预览圆
    if (stage0Points.size() >= 2) {
        const Point3D& center = stage0Points[0];
        const Point3D& radiusPoint = stage0Points[1];
        
        // 绘制半径线
        vertices->push_back(osg::Vec3(center.x(), center.y(), center.z()));
        vertices->push_back(osg::Vec3(radiusPoint.x(), radiusPoint.y(), radiusPoint.z()));
        
        // 绘制完整圆作为预览
        if (m_radius > 0.0f) {
            const int segments = 32;
            for (int i = 0; i <= segments; ++i) {
                float angle = 2.0f * M_PI * i / segments;
                glm::vec3 point = m_center + m_radius * (cos(angle) * m_uAxis + sin(angle) * m_vAxis);
                vertices->push_back(osg::Vec3(point.x, point.y, point.z));
            }
        }
    }
    
    geometry->setVertexArray(vertices);
    
    if (vertices->size() >= 2) {
        // 半径线
        osg::ref_ptr<osg::DrawArrays> radiusLine = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2);
        geometry->addPrimitiveSet(radiusLine);
        
        // 预览圆
        if (vertices->size() > 2) {
            osg::ref_ptr<osg::DrawArrays> circle = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 2, vertices->size() - 2);
            geometry->addPrimitiveSet(circle);
        }
    }
}

void Arc3D_Geo::buildCenterRadiusPreview()
{
    buildCenterRadiusStageGeometry();
}

void Arc3D_Geo::buildArcRangeStageGeometry()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.size() < 2 || allStages[0].size() < 2) {
        return;
    }
    
    // 获取边几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid()) {
        return;
    }
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 绘制圆弧
    if (m_radius > 0.0f && !m_arcPoints.empty()) {
        for (const auto& point : m_arcPoints) {
            vertices->push_back(osg::Vec3(point.x, point.y, point.z));
        }
    }
    
    geometry->setVertexArray(vertices);
    
    if (vertices->size() >= 2) {
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size());
        geometry->addPrimitiveSet(drawArrays);
    }
}

void Arc3D_Geo::buildArcRangePreview()
{
    buildArcRangeStageGeometry();
}

Point3D Arc3D_Geo::calculateArcPoint(const Point3D& mousePos) const
{
    if (m_radius <= 0.0f) {
        return mousePos;
    }
    
    // 将鼠标位置投影到圆上
    glm::vec3 mouseVec = mousePos.position - m_center;
    
    // 投影到圆弧平面
    glm::vec3 projected = mouseVec - glm::dot(mouseVec, m_normal) * m_normal;
    
    if (glm::length(projected) < 0.001f) {
        // 如果投影长度太小，使用u轴方向
        projected = m_uAxis;
    }
    else {
        projected = glm::normalize(projected);
    }
    
    // 计算圆上的点
    glm::vec3 arcPoint = m_center + m_radius * projected;
    
    return Point3D(arcPoint);
}

void Arc3D_Geo::calculateArcParameters()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    
    // 第一阶段：计算圆心、半径和坐标系
    if (!allStages.empty() && allStages[0].size() >= 2) {
        const Point3D& center = allStages[0][0];
        const Point3D& radiusPoint = allStages[0][1];
        
        m_center = center.position;
        glm::vec3 radiusVec = radiusPoint.position - center.position;
        m_radius = glm::length(radiusVec);
        
        if (m_radius > 0.0f) {
            // 建立局部坐标系
            m_uAxis = glm::normalize(radiusVec);
            m_vAxis = glm::normalize(glm::cross(m_normal, m_uAxis));
            
            // 确保坐标系正交
            m_normal = glm::normalize(glm::cross(m_uAxis, m_vAxis));
        }
    }
    
    // 第二阶段：计算角度范围
    if (allStages.size() >= 2 && allStages[1].size() >= 2 && m_radius > 0.0f) {
        const Point3D& startPoint = allStages[1][0];
        const Point3D& endPoint = allStages[1][1];
        
        // 计算起始和结束角度
        glm::vec3 startVec = startPoint.position - m_center;
        glm::vec3 endVec = endPoint.position - m_center;
        
        // 投影到圆弧平面并归一化
        startVec = startVec - glm::dot(startVec, m_normal) * m_normal;
        endVec = endVec - glm::dot(endVec, m_normal) * m_normal;
        
        if (glm::length(startVec) > 0.001f) {
            startVec = glm::normalize(startVec);
        }
        if (glm::length(endVec) > 0.001f) {
            endVec = glm::normalize(endVec);
        }
        
        // 计算角度
        m_startAngle = atan2(glm::dot(startVec, m_vAxis), glm::dot(startVec, m_uAxis));
        m_endAngle = atan2(glm::dot(endVec, m_vAxis), glm::dot(endVec, m_uAxis));
        
        // 确保角度在[0, 2π]范围内
        if (m_startAngle < 0) m_startAngle += 2.0f * M_PI;
        if (m_endAngle < 0) m_endAngle += 2.0f * M_PI;
        
        // 计算扫描角度
        m_sweepAngle = m_endAngle - m_startAngle;
        if (m_sweepAngle < 0) m_sweepAngle += 2.0f * M_PI;
        
        // 生成圆弧点
        m_arcPoints.clear();
        const int segments = std::max(8, static_cast<int>(32 * m_sweepAngle / (2.0f * M_PI)));
        
        for (int i = 0; i <= segments; ++i) {
            float t = static_cast<float>(i) / segments;
            float angle = m_startAngle + t * m_sweepAngle;
            glm::vec3 point = m_center + m_radius * (cos(angle) * m_uAxis + sin(angle) * m_vAxis);
            m_arcPoints.push_back(point);
        }
        
        qDebug() << "圆弧参数: 半径:" << m_radius << " 起始角度:" << m_startAngle * 180.0f / M_PI << "° 结束角度:" << m_endAngle * 180.0f / M_PI << "° 扫描角度:" << m_sweepAngle * 180.0f / M_PI << "°";
    }
}

bool Arc3D_Geo::isValidArcConfiguration() const
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    
    // 检查第一阶段：圆心和半径
    if (allStages.empty() || allStages[0].size() < 2) {
        return false;
    }
    
    if (m_radius <= 0.001f) {
        return false; // 半径太小
    }
    
    // 检查第二阶段：角度范围
    if (allStages.size() >= 2 && allStages[1].size() >= 2) {
        if (m_sweepAngle <= 0.001f || m_sweepAngle >= 2.0f * M_PI - 0.001f) {
            return false; // 扫描角度无效
        }
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
    
    return true;
}

// ============================================================================
// 传统几何体构建实现（保持兼容性）
// ============================================================================

void Arc3D_Geo::buildVertexGeometries()
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

void Arc3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    // 调用阶段特定的绘制方法
    buildStageEdgeGeometries(getCurrentStage());
}

void Arc3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    // 圆弧没有面
}

// ==================== 绘制完成检查和控制点验证 ====================

bool Arc3D_Geo::isDrawingComplete() const
{
    // 圆弧需要完成所有阶段才能绘制完成
    return isAllStagesComplete();
}

bool Arc3D_Geo::areControlPointsValid() const
{
    // 检查圆弧配置是否有效
    return isValidArcConfiguration();
}

