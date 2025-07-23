#include "Triangle3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Triangle3D_Geo::Triangle3D_Geo()
{
    m_geoType = Geo_Triangle3D;
    m_area = 0.0f;
    m_normal = glm::vec3(0.0f);
    // 确保基类正确初始化
    initialize();
}

// ==================== 多阶段绘制支持实现 ====================

std::vector<StageDescriptor> Triangle3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    
    // 三角形只有一个阶段：选择三个顶点（需要3个点）
    descriptors.emplace_back("绘制三角形顶点", 3, 3);
    
    return descriptors;
}

void Triangle3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
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
                calculateTriangleParameters();
                mm_state()->setStateComplete();
                qDebug() << "三角形: 绘制完成";
            }
        }
    }
}

void Triangle3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) {
        return;
    }
    
    // 设置临时点用于预览
    mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
}

// ==================== 多阶段几何构建方法实现 ====================

void Triangle3D_Geo::buildStageVertexGeometries(int stage)
{
    if (stage == 0) {
        buildTriangleStageGeometry();
    }
}

void Triangle3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 0) {
        buildTriangleStageGeometry();
    }
}

void Triangle3D_Geo::buildStageFaceGeometries(int stage)
{
    if (stage == 0 && isAllStagesComplete()) {
        buildTriangleStageGeometry();
    }
}

void Triangle3D_Geo::buildCurrentStagePreviewGeometries()
{
    buildTrianglePreview();
}

// ==================== 阶段特定的辅助方法实现 ====================

void Triangle3D_Geo::buildTriangleStageGeometry()
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    if (stagePoints.size() < 2) {
        return;
    }
    
    // 获取边几何体
    osg::ref_ptr<osg::Geometry> edgeGeometry = mm_node()->getEdgeGeometry();
    if (edgeGeometry.valid()) {
        // 创建顶点数组
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
        
        // 添加三角形的边
        if (stagePoints.size() >= 2) {
            // 添加已有的边
            for (size_t i = 0; i < stagePoints.size(); ++i) {
                vertices->push_back(osg::Vec3(stagePoints[i].x(), stagePoints[i].y(), stagePoints[i].z()));
                if (i > 0) {
                    vertices->push_back(osg::Vec3(stagePoints[i].x(), stagePoints[i].y(), stagePoints[i].z()));
                }
            }
            
            // 如果有三个点，闭合三角形
            if (stagePoints.size() >= 3) {
                vertices->push_back(osg::Vec3(stagePoints[0].x(), stagePoints[0].y(), stagePoints[0].z()));
            }
        }
        
        edgeGeometry->setVertexArray(vertices);
        
        // 线绘制
        if (vertices->size() >= 2) {
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
            edgeGeometry->addPrimitiveSet(drawArrays);
        }
    }
    
    // 如果三角形完成，绘制面
    if (stagePoints.size() >= 3 && isAllStagesComplete()) {
        osg::ref_ptr<osg::Geometry> faceGeometry = mm_node()->getFaceGeometry();
        if (faceGeometry.valid()) {
            // 创建面的顶点数组
            osg::ref_ptr<osg::Vec3Array> faceVertices = new osg::Vec3Array;
            osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
            
            // 添加三角形顶点
            for (const Point3D& point : stagePoints) {
                faceVertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
                normals->push_back(osg::Vec3(m_normal.x, m_normal.y, m_normal.z));
            }
            
            faceGeometry->setVertexArray(faceVertices);
            faceGeometry->setNormalArray(normals);
            faceGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
            
            // 三角形面绘制
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 3);
            faceGeometry->addPrimitiveSet(drawArrays);
        }
    }
}

void Triangle3D_Geo::buildTrianglePreview()
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    if (stagePoints.size() < 2) {
        return;
    }
    
    // 如果有临时点，绘制预览三角形
    buildTriangleStageGeometry();
}

void Triangle3D_Geo::calculateTriangleParameters()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].size() < 3) {
        return;
    }
    
    const Point3D& p1 = allStages[0][0];
    const Point3D& p2 = allStages[0][1];
    const Point3D& p3 = allStages[0][2];
    
    // 计算三角形法向量
    glm::vec3 edge1 = p2.position - p1.position;
    glm::vec3 edge2 = p3.position - p1.position;
    m_normal = glm::normalize(glm::cross(edge1, edge2));
    
    // 计算三角形面积
    glm::vec3 crossProduct = glm::cross(edge1, edge2);
    m_area = 0.5f * glm::length(crossProduct);
    
    qDebug() << "三角形参数: 面积:" << m_area << " 法向量:(" << m_normal.x << "," << m_normal.y << "," << m_normal.z << ")";
}

bool Triangle3D_Geo::isValidTriangleConfiguration() const
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    
    if (stagePoints.size() < 3) {
        return false;
    }
    
    // 检查三点是否共线
    const Point3D& p1 = stagePoints[0];
    const Point3D& p2 = stagePoints[1];
    const Point3D& p3 = stagePoints[2];
    
    glm::vec3 edge1 = p2.position - p1.position;
    glm::vec3 edge2 = p3.position - p1.position;
    glm::vec3 crossProduct = glm::cross(edge1, edge2);
    
    // 如果叉积长度接近0，说明三点共线
    const float epsilon = 0.001f;
    if (glm::length(crossProduct) < epsilon) {
        return false; // 三点共线，无法构成三角形
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

void Triangle3D_Geo::buildVertexGeometries()
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

void Triangle3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    // 调用阶段特定的绘制方法
    buildStageEdgeGeometries(getCurrentStage());
}

void Triangle3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    // 调用阶段特定的绘制方法
    buildStageFaceGeometries(getCurrentStage());
}

// ==================== 绘制完成检查和控制点验证 ====================

bool Triangle3D_Geo::isDrawingComplete() const
{
    // 三角形需要所有阶段都完成
    return isAllStagesComplete();
}

bool Triangle3D_Geo::areControlPointsValid() const
{
    if (!isAllStagesComplete()) {
        return false;
    }
    
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.size() < 1 || allStages[0].size() < 3) {
        return false;
    }
    
    // 检查三角形配置是否有效
    return isValidTriangleConfiguration();
}