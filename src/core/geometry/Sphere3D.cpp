#include "Sphere3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"
#include <QKeyEvent>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Sphere3D_Geo::Sphere3D_Geo()
    : m_radius(1.0f)
    , m_segments(16)
    , m_center(0.0f)
    , m_calculatedRadius(0.0f)
{
    m_geoType = Geo_Sphere3D;
    initialize();
}

std::vector<StageDescriptor> Sphere3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    descriptors.emplace_back("选择球心", 1, 1);
    descriptors.emplace_back("定义球体半径", 1, 1);
    return descriptors;
}

void Sphere3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    
    if (event->button() == Qt::RightButton) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) {
                    calculateSphereParameters();
                    qDebug() << "球体: 进入阶段" << getCurrentStage() + 1;
                }
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateSphereParameters();
                mm_state()->setStateComplete();
                qDebug() << "球体: 绘制完成";
            }
        }
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        bool success = mm_controlPoint()->addControlPointToCurrentStage(Point3D(worldPos));
        if (success) {
            calculateSphereParameters();
            if (isCurrentStageComplete() && canAdvanceToNextStage()) {
                nextStage();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                mm_state()->setStateComplete();
            }
        }
    }
}

void Sphere3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
}

void Sphere3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) calculateSphereParameters();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateSphereParameters();
                mm_state()->setStateComplete();
            }
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
        calculateSphereParameters();
    }
}

void Sphere3D_Geo::buildStageVertexGeometries(int stage)
{
    if (stage == 0) buildCenterStageGeometry();
}

void Sphere3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 1) buildSphereStageGeometry();
}

void Sphere3D_Geo::buildStageFaceGeometries(int stage)
{
    if (stage == 1 && isAllStagesComplete()) buildSphereStageGeometry();
}

void Sphere3D_Geo::buildCurrentStagePreviewGeometries()
{
    int currentStage = getCurrentStage();
    if (currentStage == 0) buildCenterPreview();
    else if (currentStage == 1) buildSpherePreview();
}

void Sphere3D_Geo::buildCenterStageGeometry()
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

void Sphere3D_Geo::buildCenterPreview()
{
    buildCenterStageGeometry();
}

void Sphere3D_Geo::buildSphereStageGeometry()
{
    if (m_calculatedRadius <= 0.0f) return;
    
    osg::ref_ptr<osg::Geometry> faceGeometry = mm_node()->getFaceGeometry();
    if (!faceGeometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    
    // 生成球体顶点
    for (int i = 0; i <= m_segments; ++i) {
        float lat = M_PI * i / m_segments - M_PI / 2;
        for (int j = 0; j <= m_segments; ++j) {
            float lon = 2 * M_PI * j / m_segments;
            
            float x = cos(lat) * cos(lon);
            float y = cos(lat) * sin(lon);
            float z = sin(lat);
            
            glm::vec3 position = m_center + m_calculatedRadius * glm::vec3(x, y, z);
            vertices->push_back(osg::Vec3(position.x, position.y, position.z));
            normals->push_back(osg::Vec3(x, y, z));
        }
    }
    
    // 生成球体索引
    for (int i = 0; i < m_segments; ++i) {
        for (int j = 0; j < m_segments; ++j) {
            int curr = i * (m_segments + 1) + j;
            int next = curr + m_segments + 1;
            
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

void Sphere3D_Geo::buildSpherePreview()
{
    buildSphereStageGeometry();
}

void Sphere3D_Geo::calculateSphereParameters()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    
    if (!allStages.empty() && !allStages[0].empty()) {
        m_center = allStages[0][0].position;
    }
    
    if (allStages.size() >= 2 && !allStages[1].empty()) {
        const Point3D& radiusPoint = allStages[1][0];
        m_calculatedRadius = glm::length(radiusPoint.position - m_center);
    }
}

bool Sphere3D_Geo::isValidSphereConfiguration() const
{
    return m_calculatedRadius > 0.001f;
}

void Sphere3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    buildStageVertexGeometries(getCurrentStage());
}

void Sphere3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    buildStageEdgeGeometries(getCurrentStage());
}

void Sphere3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    buildStageFaceGeometries(getCurrentStage());
}

bool Sphere3D_Geo::isDrawingComplete() const
{
    return isAllStagesComplete();
}

bool Sphere3D_Geo::areControlPointsValid() const
{
    return isValidSphereConfiguration();
}
