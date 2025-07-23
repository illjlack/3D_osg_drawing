#include "Cylinder3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"
#include <QKeyEvent>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Cylinder3D_Geo::Cylinder3D_Geo()
    : m_radius(1.0f)
    , m_height(2.0f)
    , m_segments(16)
    , m_baseCenter(0.0f)
    , m_topCenter(0.0f)
    , m_calculatedRadius(0.0f)
    , m_calculatedHeight(0.0f)
{
    m_geoType = Geo_Cylinder3D;
    initialize();
}

std::vector<StageDescriptor> Cylinder3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    descriptors.emplace_back("绘制底面圆心和半径", 2, 2);
    descriptors.emplace_back("定义圆柱高度", 1, 1);
    return descriptors;
}

void Cylinder3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    
    if (event->button() == Qt::RightButton) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) {
                    calculateCylinderParameters();
                    qDebug() << "圆柱体: 进入阶段" << getCurrentStage() + 1;
                }
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateCylinderParameters();
                mm_state()->setStateComplete();
                qDebug() << "圆柱体: 绘制完成";
            }
        }
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        bool success = mm_controlPoint()->addControlPointToCurrentStage(Point3D(worldPos));
        if (success) {
            calculateCylinderParameters();
            if (isCurrentStageComplete() && canAdvanceToNextStage()) {
                nextStage();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                mm_state()->setStateComplete();
            }
        }
    }
}

void Cylinder3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
}

void Cylinder3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) calculateCylinderParameters();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateCylinderParameters();
                mm_state()->setStateComplete();
            }
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
        calculateCylinderParameters();
    }
}

void Cylinder3D_Geo::buildStageVertexGeometries(int stage)
{
    if (stage == 0) buildBaseStageGeometry();
}

void Cylinder3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 0) buildBaseStageGeometry();
    else if (stage == 1) buildCylinderStageGeometry();
}

void Cylinder3D_Geo::buildStageFaceGeometries(int stage)
{
    if (stage == 1 && isAllStagesComplete()) buildCylinderStageGeometry();
}

void Cylinder3D_Geo::buildCurrentStagePreviewGeometries()
{
    int currentStage = getCurrentStage();
    if (currentStage == 0) buildBaseStageGeometry();
    else if (currentStage == 1) buildCylinderStageGeometry();
}

void Cylinder3D_Geo::buildBaseStageGeometry()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].size() < 2) return;
    
    osg::ref_ptr<osg::Geometry> edgeGeometry = mm_node()->getEdgeGeometry();
    if (!edgeGeometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    const auto& stage0Points = allStages[0];
    const Point3D& center = stage0Points[0];
    const Point3D& radiusPoint = stage0Points[1];
    
    // 绘制半径线
    vertices->push_back(osg::Vec3(center.x(), center.y(), center.z()));
    vertices->push_back(osg::Vec3(radiusPoint.x(), radiusPoint.y(), radiusPoint.z()));
    
    // 使用细分级别参数而不是固定的m_segments
    int segments = static_cast<int>(m_parameters.subdivisionLevel);
    
    // 绘制底面圆
    if (m_calculatedRadius > 0.0f) {
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * M_PI * i / segments;
            float x = center.x() + m_calculatedRadius * cos(angle);
            float y = center.y() + m_calculatedRadius * sin(angle);
            float z = center.z();
            vertices->push_back(osg::Vec3(x, y, z));
        }
    }
    
    edgeGeometry->setVertexArray(vertices);
    
    // 半径线
    osg::ref_ptr<osg::DrawArrays> radiusLine = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2);
    edgeGeometry->addPrimitiveSet(radiusLine);
    
    // 底面圆
    if (vertices->size() > 2) {
        osg::ref_ptr<osg::DrawArrays> circle = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 2, vertices->size() - 2);
        edgeGeometry->addPrimitiveSet(circle);
    }
}

void Cylinder3D_Geo::buildCylinderStageGeometry()
{
    if (m_calculatedRadius <= 0.0f || m_calculatedHeight <= 0.0f) return;
    
    osg::ref_ptr<osg::Geometry> faceGeometry = mm_node()->getFaceGeometry();
    if (!faceGeometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    
    // 使用细分级别参数而不是固定的m_segments
    int segments = static_cast<int>(m_parameters.subdivisionLevel);
    
    // 生成圆柱体顶点
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cos(angle);
        float y = sin(angle);
        
        // 底面顶点
        glm::vec3 bottomPos = m_baseCenter + m_calculatedRadius * glm::vec3(x, y, 0);
        vertices->push_back(osg::Vec3(bottomPos.x, bottomPos.y, bottomPos.z));
        normals->push_back(osg::Vec3(x, y, 0));
        
        // 顶面顶点
        glm::vec3 topPos = m_topCenter + m_calculatedRadius * glm::vec3(x, y, 0);
        vertices->push_back(osg::Vec3(topPos.x, topPos.y, topPos.z));
        normals->push_back(osg::Vec3(x, y, 0));
    }
    
    // 生成侧面索引
    for (int i = 0; i < segments; ++i) {
        int bottom1 = i * 2;
        int top1 = i * 2 + 1;
        int bottom2 = (i + 1) * 2;
        int top2 = (i + 1) * 2 + 1;
        
        // 第一个三角形
        indices->push_back(bottom1);
        indices->push_back(top1);
        indices->push_back(bottom2);
        
        // 第二个三角形
        indices->push_back(bottom2);
        indices->push_back(top1);
        indices->push_back(top2);
    }
    
    faceGeometry->setVertexArray(vertices);
    faceGeometry->setNormalArray(normals);
    faceGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    faceGeometry->addPrimitiveSet(indices);
}

void Cylinder3D_Geo::calculateCylinderParameters()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    
    // 第一阶段：计算底面中心和半径
    if (!allStages.empty() && allStages[0].size() >= 2) {
        m_baseCenter = allStages[0][0].position;
        const Point3D& radiusPoint = allStages[0][1];
        m_calculatedRadius = glm::length(radiusPoint.position - m_baseCenter);
    }
    
    // 第二阶段：计算高度
    if (allStages.size() >= 2 && !allStages[1].empty()) {
        const Point3D& heightPoint = allStages[1][0];
        glm::vec3 heightVec = heightPoint.position - m_baseCenter;
        m_calculatedHeight = glm::length(heightVec);
        m_topCenter = m_baseCenter + glm::vec3(0, 0, m_calculatedHeight);
    }
}

bool Cylinder3D_Geo::isValidCylinderConfiguration() const
{
    return m_calculatedRadius > 0.001f && m_calculatedHeight > 0.001f;
}

void Cylinder3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    buildStageVertexGeometries(getCurrentStage());
}

void Cylinder3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    buildStageEdgeGeometries(getCurrentStage());
}

void Cylinder3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    buildStageFaceGeometries(getCurrentStage());
}

bool Cylinder3D_Geo::isDrawingComplete() const
{
    return isAllStagesComplete();
}

bool Cylinder3D_Geo::areControlPointsValid() const
{
    return isValidCylinderConfiguration();
} 