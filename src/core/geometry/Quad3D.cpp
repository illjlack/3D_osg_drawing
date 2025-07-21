#include "Quad3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Quad3D_Geo::Quad3D_Geo()
    : m_area(0.0f)
    , m_normal(0, 0, 1)
{
    m_geoType = Geo_Quad3D;
    // 确保基类正确初始化
    initialize();
}

// ==================== 多阶段绘制支持实现 ====================

std::vector<StageDescriptor> Quad3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    
    // 四边形只有一个阶段：选择四个顶点（需要4个点）
    descriptors.emplace_back("绘制四边形顶点", 4, 4);
    
    return descriptors;
}

void Quad3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
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
                calculateQuadParameters();
                mm_state()->setStateComplete();
                qDebug() << "四边形: 绘制完成";
            }
        }
    }
}

void Quad3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) {
        return;
    }
    
    // 设置临时点用于预览
    mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
}

void Quad3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // 回车键完成绘制
        if (isCurrentStageComplete() && areControlPointsValid()) {
            calculateQuadParameters();
            mm_state()->setStateComplete();
            qDebug() << "四边形: 回车键完成绘制";
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        // ESC键撤销最后一个控制点
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
    }
}

// ==================== 多阶段几何构建方法实现 ====================

void Quad3D_Geo::buildStageVertexGeometries(int stage)
{
    if (stage == 0) {
        buildQuadStageGeometry();
    }
}

void Quad3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 0) {
        buildQuadStageGeometry();
    }
}

void Quad3D_Geo::buildStageFaceGeometries(int stage)
{
    if (stage == 0 && isAllStagesComplete()) {
        buildQuadStageGeometry();
    }
}

void Quad3D_Geo::buildCurrentStagePreviewGeometries()
{
    buildQuadPreview();
}

// ==================== 阶段特定的辅助方法实现 ====================

void Quad3D_Geo::buildQuadStageGeometry()
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    if (stagePoints.size() < 2) {
        return;
    }
    
    // 绘制边
    osg::ref_ptr<osg::Geometry> edgeGeometry = mm_node()->getEdgeGeometry();
    if (edgeGeometry.valid()) {
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
        
        // 添加已有的点和连线
        for (size_t i = 0; i < stagePoints.size(); ++i) {
            vertices->push_back(osg::Vec3(stagePoints[i].x(), stagePoints[i].y(), stagePoints[i].z()));
            if (i > 0) {
                vertices->push_back(osg::Vec3(stagePoints[i].x(), stagePoints[i].y(), stagePoints[i].z()));
            }
        }
        
        // 如果有四个点，闭合四边形
        if (stagePoints.size() >= 4) {
            vertices->push_back(osg::Vec3(stagePoints[0].x(), stagePoints[0].y(), stagePoints[0].z()));
        }
        
        edgeGeometry->setVertexArray(vertices);
        
        if (vertices->size() >= 2) {
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
            edgeGeometry->addPrimitiveSet(drawArrays);
        }
    }
    
    // 如果四边形完成，绘制面
    if (stagePoints.size() >= 4 && isAllStagesComplete()) {
        osg::ref_ptr<osg::Geometry> faceGeometry = mm_node()->getFaceGeometry();
        if (faceGeometry.valid()) {
            osg::ref_ptr<osg::Vec3Array> faceVertices = new osg::Vec3Array;
            osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
            
            // 添加四边形顶点（分为两个三角形）
            // 第一个三角形: 0, 1, 2
            faceVertices->push_back(osg::Vec3(stagePoints[0].x(), stagePoints[0].y(), stagePoints[0].z()));
            faceVertices->push_back(osg::Vec3(stagePoints[1].x(), stagePoints[1].y(), stagePoints[1].z()));
            faceVertices->push_back(osg::Vec3(stagePoints[2].x(), stagePoints[2].y(), stagePoints[2].z()));
            
            // 第二个三角形: 0, 2, 3
            faceVertices->push_back(osg::Vec3(stagePoints[0].x(), stagePoints[0].y(), stagePoints[0].z()));
            faceVertices->push_back(osg::Vec3(stagePoints[2].x(), stagePoints[2].y(), stagePoints[2].z()));
            faceVertices->push_back(osg::Vec3(stagePoints[3].x(), stagePoints[3].y(), stagePoints[3].z()));
            
            // 为每个顶点添加法向量
            for (int i = 0; i < 6; ++i) {
                normals->push_back(osg::Vec3(m_normal.x, m_normal.y, m_normal.z));
            }
            
            faceGeometry->setVertexArray(faceVertices);
            faceGeometry->setNormalArray(normals);
            faceGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
            
            // 绘制两个三角形
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 6);
            faceGeometry->addPrimitiveSet(drawArrays);
        }
    }
}

void Quad3D_Geo::buildQuadPreview()
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    if (stagePoints.size() < 2) {
        return;
    }
    
    // 如果有临时点，绘制预览四边形
    buildQuadStageGeometry();
}

void Quad3D_Geo::calculateQuadParameters()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].size() < 4) {
        return;
    }
    
    const Point3D& p1 = allStages[0][0];
    const Point3D& p2 = allStages[0][1];
    const Point3D& p3 = allStages[0][2];
    const Point3D& p4 = allStages[0][3];
    
    // 计算四边形法向量（使用前三个点）
    glm::vec3 edge1 = p2.position - p1.position;
    glm::vec3 edge2 = p3.position - p1.position;
    m_normal = glm::normalize(glm::cross(edge1, edge2));
    
    // 计算四边形面积（分为两个三角形）
    glm::vec3 cross1 = glm::cross(p2.position - p1.position, p3.position - p1.position);
    glm::vec3 cross2 = glm::cross(p3.position - p1.position, p4.position - p1.position);
    m_area = 0.5f * (glm::length(cross1) + glm::length(cross2));
    
    qDebug() << "四边形参数: 面积:" << m_area << " 法向量:(" << m_normal.x << "," << m_normal.y << "," << m_normal.z << ")";
}

bool Quad3D_Geo::isValidQuadConfiguration() const
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    
    if (stagePoints.size() < 4) {
        return false;
    }
    
    // 检查四个点不能都共线
    const Point3D& p1 = stagePoints[0];
    const Point3D& p2 = stagePoints[1];
    const Point3D& p3 = stagePoints[2];
    const Point3D& p4 = stagePoints[3];
    
    // 检查至少有一组三点不共线
    glm::vec3 edge1 = p2.position - p1.position;
    glm::vec3 edge2 = p3.position - p1.position;
    glm::vec3 crossProduct = glm::cross(edge1, edge2);
    
    const float epsilon = 0.001f;
    if (glm::length(crossProduct) < epsilon) {
        // 前三点共线，检查第四点
        glm::vec3 edge3 = p4.position - p1.position;
        glm::vec3 crossProduct2 = glm::cross(edge1, edge3);
        if (glm::length(crossProduct2) < epsilon) {
            return false; // 四点都共线，无效
        }
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

void Quad3D_Geo::buildVertexGeometries()
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

void Quad3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    // 调用阶段特定的绘制方法
    buildStageEdgeGeometries(getCurrentStage());
}

void Quad3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    // 调用阶段特定的绘制方法
    buildStageFaceGeometries(getCurrentStage());
}

// ==================== 绘制完成检查和控制点验证 ====================

bool Quad3D_Geo::isDrawingComplete() const
{
    // 四边形需要所有阶段都完成
    return isAllStagesComplete();
}

bool Quad3D_Geo::areControlPointsValid() const
{
    if (!isAllStagesComplete()) {
        return false;
    }
    
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.size() < 1 || allStages[0].size() < 4) {
        return false;
    }
    
    // 检查四边形配置是否有效
    return isValidQuadConfiguration();
} 
