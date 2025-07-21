#include "Prism3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"
#include <QKeyEvent>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Prism3D_Geo::Prism3D_Geo()
    : m_height(2.0f)
    , m_baseCenter(0.0f)
    , m_normal(0.0f, 0.0f, 1.0f)
    , m_calculatedHeight(0.0f)
{
    m_geoType = Geo_Prism3D;
    initialize();
}

std::vector<StageDescriptor> Prism3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    descriptors.emplace_back("绘制底面多边形", 3, -1);
    descriptors.emplace_back("定义棱柱高度", 1, 1);
    return descriptors;
}

void Prism3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    
    if (event->button() == Qt::RightButton) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) {
                    calculatePrismParameters();
                    qDebug() << "棱柱: 进入阶段" << getCurrentStage() + 1;
                }
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculatePrismParameters();
                mm_state()->setStateComplete();
                qDebug() << "棱柱: 绘制完成";
            }
        }
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        bool success = mm_controlPoint()->addControlPointToCurrentStage(Point3D(worldPos));
        if (success) {
            calculatePrismParameters();
            if (isCurrentStageComplete() && canAdvanceToNextStage()) {
                nextStage();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                mm_state()->setStateComplete();
            }
        }
    }
}

void Prism3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
}

void Prism3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) calculatePrismParameters();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculatePrismParameters();
                mm_state()->setStateComplete();
            }
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
        calculatePrismParameters();
    }
}

void Prism3D_Geo::buildStageVertexGeometries(int stage)
{
    if (stage == 0) buildBaseStageGeometry();
}

void Prism3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 0) buildBaseStageGeometry();
    else if (stage == 1) buildPrismStageGeometry();
}

void Prism3D_Geo::buildStageFaceGeometries(int stage)
{
    if (stage == 1 && isAllStagesComplete()) buildPrismStageGeometry();
}

void Prism3D_Geo::buildCurrentStagePreviewGeometries()
{
    int currentStage = getCurrentStage();
    if (currentStage == 0) buildBaseStageGeometry();
    else if (currentStage == 1) buildPrismStageGeometry();
}

void Prism3D_Geo::buildBaseStageGeometry()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].size() < 3) return;
    
    osg::ref_ptr<osg::Geometry> edgeGeometry = mm_node()->getEdgeGeometry();
    if (!edgeGeometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    const auto& basePoints = allStages[0];
    
    // 绘制底面多边形边界
    for (size_t i = 0; i < basePoints.size(); ++i) {
        vertices->push_back(osg::Vec3(basePoints[i].x(), basePoints[i].y(), basePoints[i].z()));
        
        size_t next = (i + 1) % basePoints.size();
        vertices->push_back(osg::Vec3(basePoints[next].x(), basePoints[next].y(), basePoints[next].z()));
    }
    
    edgeGeometry->setVertexArray(vertices);
    
    if (vertices->size() >= 2) {
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
        edgeGeometry->addPrimitiveSet(drawArrays);
    }
}

void Prism3D_Geo::buildPrismStageGeometry()
{
    if (m_calculatedHeight <= 0.0f || m_baseVertices.empty()) return;
    
    osg::ref_ptr<osg::Geometry> edgeGeometry = mm_node()->getEdgeGeometry();
    if (!edgeGeometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 添加底面和顶面顶点
    for (const auto& baseVert : m_baseVertices) {
        vertices->push_back(osg::Vec3(baseVert.x, baseVert.y, baseVert.z));
    }
    for (const auto& topVert : m_topVertices) {
        vertices->push_back(osg::Vec3(topVert.x, topVert.y, topVert.z));
    }
    
    edgeGeometry->setVertexArray(vertices);
    
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES);
    
    int baseCount = m_baseVertices.size();
    
    // 底面边
    for (int i = 0; i < baseCount; ++i) {
        int next = (i + 1) % baseCount;
        indices->push_back(i);
        indices->push_back(next);
    }
    
    // 顶面边
    for (int i = 0; i < baseCount; ++i) {
        int next = (i + 1) % baseCount;
        indices->push_back(baseCount + i);
        indices->push_back(baseCount + next);
    }
    
    // 竖直边
    for (int i = 0; i < baseCount; ++i) {
        indices->push_back(i);
        indices->push_back(baseCount + i);
    }
    
    edgeGeometry->addPrimitiveSet(indices);
}

void Prism3D_Geo::calculatePrismParameters()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    
    // 第一阶段：计算底面参数
    if (!allStages.empty() && allStages[0].size() >= 3) {
        m_baseVertices.clear();
        glm::vec3 centroid(0.0f);
        
        for (const auto& point : allStages[0]) {
            m_baseVertices.push_back(point.position);
            centroid += point.position;
        }
        
        m_baseCenter = centroid / static_cast<float>(allStages[0].size());
    }
    
    // 第二阶段：计算高度和顶面
    if (allStages.size() >= 2 && !allStages[1].empty()) {
        const Point3D& heightPoint = allStages[1][0];
        glm::vec3 heightVec = heightPoint.position - m_baseCenter;
        m_calculatedHeight = glm::length(heightVec);
        
        if (m_calculatedHeight > 0.0f) {
            m_normal = glm::normalize(heightVec);
            
            // 生成顶面顶点
            m_topVertices.clear();
            for (const auto& baseVert : m_baseVertices) {
                m_topVertices.push_back(baseVert + m_normal * m_calculatedHeight);
            }
        }
    }
}

bool Prism3D_Geo::isValidPrismConfiguration() const
{
    return m_calculatedHeight > 0.001f && m_baseVertices.size() >= 3;
}

void Prism3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    buildStageVertexGeometries(getCurrentStage());
}

void Prism3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    buildStageEdgeGeometries(getCurrentStage());
}

void Prism3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    buildStageFaceGeometries(getCurrentStage());
}

bool Prism3D_Geo::isDrawingComplete() const
{
    return isAllStagesComplete();
}

bool Prism3D_Geo::areControlPointsValid() const
{
    return isValidPrismConfiguration();
} 