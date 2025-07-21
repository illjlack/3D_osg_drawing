#include "Torus3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"
#include <QKeyEvent>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Torus3D_Geo::Torus3D_Geo()
    : m_majorRadius(2.0f)
    , m_minorRadius(0.5f)
    , m_majorSegments(16)
    , m_minorSegments(8)
    , m_center(0.0f)
    , m_normal(0.0f, 0.0f, 1.0f)
    , m_calculatedMajorRadius(0.0f)
    , m_calculatedMinorRadius(0.0f)
{
    m_geoType = Geo_Torus3D;
    initialize();
}

std::vector<StageDescriptor> Torus3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    descriptors.emplace_back("选择环面中心", 1, 1);
    descriptors.emplace_back("定义主半径", 1, 1);
    descriptors.emplace_back("定义次半径", 1, 1);
    return descriptors;
}

void Torus3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    
    if (event->button() == Qt::RightButton) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) {
                    calculateTorusParameters();
                    qDebug() << "环面: 进入阶段" << getCurrentStage() + 1;
                }
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateTorusParameters();
                mm_state()->setStateComplete();
                qDebug() << "环面: 绘制完成";
            }
        }
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        bool success = mm_controlPoint()->addControlPointToCurrentStage(Point3D(worldPos));
        if (success) {
            calculateTorusParameters();
            if (isCurrentStageComplete() && canAdvanceToNextStage()) {
                nextStage();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                mm_state()->setStateComplete();
            }
        }
    }
}

void Torus3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
}

void Torus3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) calculateTorusParameters();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateTorusParameters();
                mm_state()->setStateComplete();
            }
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
        calculateTorusParameters();
    }
}

void Torus3D_Geo::buildStageVertexGeometries(int stage)
{
    if (stage == 0) buildCenterStageGeometry();
}

void Torus3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 2) buildTorusStageGeometry();
}

void Torus3D_Geo::buildStageFaceGeometries(int stage)
{
    if (stage == 2 && isAllStagesComplete()) buildTorusStageGeometry();
}

void Torus3D_Geo::buildCurrentStagePreviewGeometries()
{
    int currentStage = getCurrentStage();
    if (currentStage == 0) buildCenterStageGeometry();
    else if (currentStage == 2) buildTorusStageGeometry();
}

void Torus3D_Geo::buildCenterStageGeometry()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].empty()) return;
    
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getVertexGeometry();
    if (!geometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    const Point3D& center = allStages[0][0];
    vertices->push_back(osg::Vec3(center.x(), center.y(), center.z()));
    
    geometry->setVertexArray(vertices);
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void Torus3D_Geo::buildTorusStageGeometry()
{
    if (m_calculatedMajorRadius <= 0.0f || m_calculatedMinorRadius <= 0.0f) return;
    
    osg::ref_ptr<osg::Geometry> faceGeometry = mm_node()->getFaceGeometry();
    if (!faceGeometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    
    // 生成环面顶点
    for (int i = 0; i <= m_majorSegments; ++i) {
        float u = 2.0f * M_PI * i / m_majorSegments;
        for (int j = 0; j <= m_minorSegments; ++j) {
            float v = 2.0f * M_PI * j / m_minorSegments;
            
            float x = (m_calculatedMajorRadius + m_calculatedMinorRadius * cos(v)) * cos(u);
            float y = (m_calculatedMajorRadius + m_calculatedMinorRadius * cos(v)) * sin(u);
            float z = m_calculatedMinorRadius * sin(v);
            
            glm::vec3 position = m_center + glm::vec3(x, y, z);
            vertices->push_back(osg::Vec3(position.x, position.y, position.z));
            
            // 计算法向量
            glm::vec3 normal = glm::normalize(glm::vec3(cos(v) * cos(u), cos(v) * sin(u), sin(v)));
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
        }
    }
    
    // 生成环面索引
    for (int i = 0; i < m_majorSegments; ++i) {
        for (int j = 0; j < m_minorSegments; ++j) {
            int curr = i * (m_minorSegments + 1) + j;
            int next = curr + m_minorSegments + 1;
            
            indices->push_back(curr);
            indices->push_back(next);
            indices->push_back(curr + 1);
            
            indices->push_back(curr + 1);
            indices->push_back(next);
            indices->push_back(next + 1);
        }
    }
    
    faceGeometry->setVertexArray(vertices);
    faceGeometry->setNormalArray(normals);
    faceGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    faceGeometry->addPrimitiveSet(indices);
}

void Torus3D_Geo::calculateTorusParameters()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    
    if (!allStages.empty() && !allStages[0].empty()) {
        m_center = allStages[0][0].position;
    }
    
    if (allStages.size() >= 2 && !allStages[1].empty()) {
        const Point3D& majorRadiusPoint = allStages[1][0];
        m_calculatedMajorRadius = glm::length(majorRadiusPoint.position - m_center);
    }
    
    if (allStages.size() >= 3 && !allStages[2].empty()) {
        const Point3D& minorRadiusPoint = allStages[2][0];
        m_calculatedMinorRadius = glm::length(minorRadiusPoint.position - m_center) - m_calculatedMajorRadius;
        if (m_calculatedMinorRadius < 0) m_calculatedMinorRadius = 0.1f; // 防止负值
    }
}

bool Torus3D_Geo::isValidTorusConfiguration() const
{
    return m_calculatedMajorRadius > 0.001f && m_calculatedMinorRadius > 0.001f;
}

void Torus3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    buildStageVertexGeometries(getCurrentStage());
}

void Torus3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    buildStageEdgeGeometries(getCurrentStage());
}

void Torus3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    buildStageFaceGeometries(getCurrentStage());
}

bool Torus3D_Geo::isDrawingComplete() const
{
    return isAllStagesComplete();
}

bool Torus3D_Geo::areControlPointsValid() const
{
    return isValidTorusConfiguration();
}