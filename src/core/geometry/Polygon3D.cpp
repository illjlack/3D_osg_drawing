#include "Polygon3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Polygon3D_Geo::Polygon3D_Geo()
    : m_normal(0, 0, 1)
    , m_area(0.0f)
{
    m_geoType = Geo_Polygon3D;
    // 确保基类正确初始化
    initialize();
}

// ==================== 多阶段绘制支持实现 ====================

std::vector<StageDescriptor> Polygon3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    
    // 多边形只有一个阶段：连续添加顶点（最少3个，最多无限制）
    descriptors.emplace_back("绘制多边形顶点", 3, -1);
    
    return descriptors;
}

void Polygon3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{

    
    
    
}

void Polygon3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) {
        return;
    }
    
    // 设置临时点用于预览
    mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
}

void Polygon3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // 回车键完成绘制
        if (isCurrentStageComplete() && areControlPointsValid()) {
            calculatePolygonParameters();
            mm_state()->setStateComplete();
            qDebug() << "多边形: 回车键完成绘制";
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        // ESC键撤销最后一个控制点
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
    }
}

// ==================== 多阶段几何构建方法实现 ====================

void Polygon3D_Geo::buildStageVertexGeometries(int stage)
{
    if (stage == 0) {
        buildPolygonStageGeometry();
    }
}

void Polygon3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 0) {
        buildPolygonStageGeometry();
    }
}

void Polygon3D_Geo::buildStageFaceGeometries(int stage)
{
    if (stage == 0 && isAllStagesComplete()) {
        buildPolygonStageGeometry();
    }
}

void Polygon3D_Geo::buildCurrentStagePreviewGeometries()
{
    buildPolygonPreview();
}

// ==================== 阶段特定的辅助方法实现 ====================

void Polygon3D_Geo::buildPolygonStageGeometry()
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    if (stagePoints.size() < 2) {
        return;
    }
    
    // 绘制边
    osg::ref_ptr<osg::Geometry> edgeGeometry = mm_node()->getEdgeGeometry();
    if (edgeGeometry.valid()) {
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
        
        // 添加多边形的边
        for (size_t i = 0; i < stagePoints.size(); ++i) {
            vertices->push_back(osg::Vec3(stagePoints[i].x(), stagePoints[i].y(), stagePoints[i].z()));
            if (i > 0) {
                vertices->push_back(osg::Vec3(stagePoints[i].x(), stagePoints[i].y(), stagePoints[i].z()));
            }
        }
        
        // 如果有足够多的点，闭合多边形
        if (stagePoints.size() >= 3) {
            vertices->push_back(osg::Vec3(stagePoints[0].x(), stagePoints[0].y(), stagePoints[0].z()));
        }
        
        edgeGeometry->setVertexArray(vertices);
        
        if (vertices->size() >= 2) {
            osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
            edgeGeometry->addPrimitiveSet(drawArrays);
        }
    }
    
    // 如果多边形完成，绘制面
    if (stagePoints.size() >= 3 && isAllStagesComplete()) {
        osg::ref_ptr<osg::Geometry> faceGeometry = mm_node()->getFaceGeometry();
        if (faceGeometry.valid()) {
            osg::ref_ptr<osg::Vec3Array> faceVertices = new osg::Vec3Array;
            osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
            
            // 三角化多边形并添加顶点
            triangulatePolygon();
            
            for (unsigned int index : m_triangleIndices) {
                if (index < stagePoints.size()) {
                    const Point3D& point = stagePoints[index];
                    faceVertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
                    normals->push_back(osg::Vec3(m_normal.x, m_normal.y, m_normal.z));
                }
            }
            
            faceGeometry->setVertexArray(faceVertices);
            faceGeometry->setNormalArray(normals);
            faceGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
            
            // 绘制三角形
            if (faceVertices->size() >= 3) {
                osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, faceVertices->size());
                faceGeometry->addPrimitiveSet(drawArrays);
            }
        }
    }
}

void Polygon3D_Geo::buildPolygonPreview()
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    if (stagePoints.size() < 2) {
        return;
    }
    
    // 如果有临时点，绘制预览多边形
    buildPolygonStageGeometry();
}

void Polygon3D_Geo::calculatePolygonParameters()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].size() < 3) {
        return;
    }
    
    const auto& points = allStages[0];
    
    // 计算多边形法向量（使用前三个点）
    glm::vec3 edge1 = points[1].position - points[0].position;
    glm::vec3 edge2 = points[2].position - points[0].position;
    m_normal = glm::normalize(glm::cross(edge1, edge2));
    
    // 计算多边形面积（通过三角化）
    triangulatePolygon();
    m_area = 0.0f;
    for (size_t i = 0; i < m_triangleIndices.size(); i += 3) {
        if (i + 2 < m_triangleIndices.size()) {
            const auto& p1 = points[m_triangleIndices[i]].position;
            const auto& p2 = points[m_triangleIndices[i + 1]].position;
            const auto& p3 = points[m_triangleIndices[i + 2]].position;
            
            glm::vec3 cross = glm::cross(p2 - p1, p3 - p1);
            m_area += 0.5f * glm::length(cross);
        }
    }
    
    qDebug() << "多边形参数: 顶点数:" << points.size() << " 面积:" << m_area << " 法向量:(" << m_normal.x << "," << m_normal.y << "," << m_normal.z << ")";
}

void Polygon3D_Geo::triangulatePolygon()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].size() < 3) {
        return;
    }
    
    const auto& points = allStages[0];
    m_triangleIndices.clear();
    
    // 简单的扇形三角化（适用于凸多边形）
    for (size_t i = 1; i < points.size() - 1; ++i) {
        m_triangleIndices.push_back(0);
        m_triangleIndices.push_back(i);
        m_triangleIndices.push_back(i + 1);
    }
}

bool Polygon3D_Geo::isValidPolygonConfiguration() const
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    
    if (stagePoints.size() < 3) {
        return false;
    }
    
    // 检查是否所有点都共线
    if (stagePoints.size() >= 3) {
        glm::vec3 edge1 = stagePoints[1].position - stagePoints[0].position;
        glm::vec3 edge2 = stagePoints[2].position - stagePoints[0].position;
        glm::vec3 crossProduct = glm::cross(edge1, edge2);
        
        const float epsilon = 0.001f;
        if (glm::length(crossProduct) < epsilon) {
            // 前三点共线，检查是否有其他点不共线
            bool hasNonCollinearPoint = false;
            for (size_t i = 3; i < stagePoints.size(); ++i) {
                glm::vec3 edge3 = stagePoints[i].position - stagePoints[0].position;
                glm::vec3 crossProduct2 = glm::cross(edge1, edge3);
                if (glm::length(crossProduct2) >= epsilon) {
                    hasNonCollinearPoint = true;
                    break;
                }
            }
            if (!hasNonCollinearPoint) {
                return false; // 所有点都共线，无法构成多边形
            }
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

void Polygon3D_Geo::buildVertexGeometries()
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

void Polygon3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    // 调用阶段特定的绘制方法
    buildStageEdgeGeometries(getCurrentStage());
}

void Polygon3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    // 调用阶段特定的绘制方法
    buildStageFaceGeometries(getCurrentStage());
}

// ==================== 绘制完成检查和控制点验证 ====================

bool Polygon3D_Geo::isDrawingComplete() const
{
    // 多边形的绘制完成由用户决定（右键或回车），而不是自动完成
    // 但是需要满足最少控制点数量
    return isCurrentStageComplete();
}

bool Polygon3D_Geo::areControlPointsValid() const
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    
    // 检查控制点数量
    if (stagePoints.size() < 3) {
        return false;
    }
    
    // 检查多边形配置是否有效
    return isValidPolygonConfiguration();
}
