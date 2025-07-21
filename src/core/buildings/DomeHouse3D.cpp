#include "DomeHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"
#include <QKeyEvent>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

DomeHouse3D_Geo::DomeHouse3D_Geo()
    : m_radius(3.0f)
    , m_height(4.0f)
    , m_domeHeight(2.0f)
    , m_segments(16)
    , m_center(0.0f)
    , m_calculatedRadius(0.0f)
    , m_calculatedHeight(0.0f)
    , m_calculatedDomeHeight(0.0f)
{
    m_geoType = Geo_DomeHouse3D;
    initialize();
}

std::vector<StageDescriptor> DomeHouse3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    descriptors.emplace_back("绘制房屋底面圆", 2, 2);
    descriptors.emplace_back("定义房屋和圆顶高度", 1, 1);
    return descriptors;
}

void DomeHouse3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    
    if (event->button() == Qt::RightButton) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) {
                    calculateDomeHouseParameters();
                    qDebug() << "圆顶房屋: 进入阶段" << getCurrentStage() + 1;
                }
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateDomeHouseParameters();
                mm_state()->setStateComplete();
                qDebug() << "圆顶房屋: 绘制完成";
            }
        }
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        bool success = mm_controlPoint()->addControlPointToCurrentStage(Point3D(worldPos));
        if (success) {
            calculateDomeHouseParameters();
            if (isCurrentStageComplete() && canAdvanceToNextStage()) {
                nextStage();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                mm_state()->setStateComplete();
            }
        }
    }
}

void DomeHouse3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
}

void DomeHouse3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) calculateDomeHouseParameters();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateDomeHouseParameters();
                mm_state()->setStateComplete();
            }
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
        calculateDomeHouseParameters();
    }
}

void DomeHouse3D_Geo::buildStageVertexGeometries(int stage)
{
    if (stage == 0) buildBaseStageGeometry();
}

void DomeHouse3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 0) buildBaseStageGeometry();
    else if (stage == 1) buildDomeHouseStageGeometry();
}

void DomeHouse3D_Geo::buildStageFaceGeometries(int stage)
{
    if (stage == 1 && isAllStagesComplete()) buildDomeHouseStageGeometry();
}

void DomeHouse3D_Geo::buildCurrentStagePreviewGeometries()
{
    int currentStage = getCurrentStage();
    if (currentStage == 0) buildBaseStageGeometry();
    else if (currentStage == 1) buildDomeHouseStageGeometry();
}

void DomeHouse3D_Geo::buildBaseStageGeometry()
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
    
    // 绘制底面圆
    if (m_calculatedRadius > 0.0f) {
        for (int i = 0; i <= m_segments; ++i) {
            float angle = 2.0f * M_PI * i / m_segments;
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

void DomeHouse3D_Geo::buildDomeHouseStageGeometry()
{
    if (m_calculatedRadius <= 0.0f || m_calculatedHeight <= 0.0f) return;
    
    osg::ref_ptr<osg::Geometry> faceGeometry = mm_node()->getFaceGeometry();
    if (!faceGeometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    
    // 生成圆柱形房屋主体
    for (int i = 0; i <= m_segments; ++i) {
        float angle = 2.0f * M_PI * i / m_segments;
        float x = cos(angle);
        float y = sin(angle);
        
        // 底面顶点
        glm::vec3 bottomPos = m_center + m_calculatedRadius * glm::vec3(x, y, 0);
        vertices->push_back(osg::Vec3(bottomPos.x, bottomPos.y, bottomPos.z));
        normals->push_back(osg::Vec3(x, y, 0));
        
        // 主体顶部顶点
        glm::vec3 topPos = m_center + m_calculatedRadius * glm::vec3(x, y, 0) + glm::vec3(0, 0, m_calculatedHeight);
        vertices->push_back(osg::Vec3(topPos.x, topPos.y, topPos.z));
        normals->push_back(osg::Vec3(x, y, 0));
    }
    
    // 添加圆顶顶点
    for (int i = 0; i <= m_segments/2; ++i) {
        float phi = M_PI * i / (m_segments/2); // 从0到π/2
        for (int j = 0; j <= m_segments; ++j) {
            float theta = 2.0f * M_PI * j / m_segments;
            
            float x = m_calculatedRadius * sin(phi) * cos(theta);
            float y = m_calculatedRadius * sin(phi) * sin(theta);
            float z = m_calculatedHeight + m_calculatedDomeHeight * cos(phi);
            
            glm::vec3 pos = m_center + glm::vec3(x, y, z);
            vertices->push_back(osg::Vec3(pos.x, pos.y, pos.z));
            
            glm::vec3 normal = glm::normalize(glm::vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi)));
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
        }
    }
    
    // 生成房屋主体侧面索引
    for (int i = 0; i < m_segments; ++i) {
        int bottom1 = i * 2;
        int top1 = i * 2 + 1;
        int bottom2 = (i + 1) * 2;
        int top2 = (i + 1) * 2 + 1;
        
        indices->push_back(bottom1); indices->push_back(top1); indices->push_back(bottom2);
        indices->push_back(bottom2); indices->push_back(top1); indices->push_back(top2);
    }
    
    faceGeometry->setVertexArray(vertices);
    faceGeometry->setNormalArray(normals);
    faceGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    faceGeometry->addPrimitiveSet(indices);
}

void DomeHouse3D_Geo::calculateDomeHouseParameters()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    
    // 第一阶段：计算底面中心和半径
    if (!allStages.empty() && allStages[0].size() >= 2) {
        m_center = allStages[0][0].position;
        const Point3D& radiusPoint = allStages[0][1];
        m_calculatedRadius = glm::length(radiusPoint.position - m_center);
    }
    
    // 第二阶段：计算高度
    if (allStages.size() >= 2 && !allStages[1].empty()) {
        const Point3D& heightPoint = allStages[1][0];
        float totalHeight = heightPoint.z() - m_center.z;
        if (totalHeight < 0) totalHeight = -totalHeight;
        
        m_calculatedHeight = totalHeight * 0.7f; // 主体高度占70%
        m_calculatedDomeHeight = totalHeight * 0.3f; // 圆顶高度占30%
    }
}

bool DomeHouse3D_Geo::isValidDomeHouseConfiguration() const
{
    return m_calculatedRadius > 0.001f && m_calculatedHeight > 0.001f && m_calculatedDomeHeight > 0.001f;
}

void DomeHouse3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    buildStageVertexGeometries(getCurrentStage());
}

void DomeHouse3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    buildStageEdgeGeometries(getCurrentStage());
}

void DomeHouse3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    buildStageFaceGeometries(getCurrentStage());
}

bool DomeHouse3D_Geo::isDrawingComplete() const
{
    return isAllStagesComplete();
}

bool DomeHouse3D_Geo::areControlPointsValid() const
{
    return isValidDomeHouseConfiguration();
} 