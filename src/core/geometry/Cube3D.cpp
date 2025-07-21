#include "Cube3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Cube3D_Geo::Cube3D_Geo()
    : m_size(1.0f)
    , m_corner1(0.0f)
    , m_corner2(0.0f)
    , m_calculatedSize(0.0f)
{
    m_geoType = Geo_Cube3D;
    initialize();
}

std::vector<StageDescriptor> Cube3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    descriptors.emplace_back("选择起始角点", 1, 1);
    descriptors.emplace_back("选择对角点", 1, 1);
    return descriptors;
}

void Cube3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    
    if (event->button() == Qt::RightButton) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) {
                    calculateCubeParameters();
                    qDebug() << "立方体: 进入阶段" << getCurrentStage() + 1;
                }
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateCubeParameters();
                mm_state()->setStateComplete();
                qDebug() << "立方体: 绘制完成";
            }
        }
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        bool success = mm_controlPoint()->addControlPointToCurrentStage(Point3D(worldPos));
        if (success) {
            calculateCubeParameters();
            if (isCurrentStageComplete() && canAdvanceToNextStage()) {
                nextStage();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                mm_state()->setStateComplete();
            }
        }
    }
}

void Cube3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
}

void Cube3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) calculateCubeParameters();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateCubeParameters();
                mm_state()->setStateComplete();
            }
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
        calculateCubeParameters();
    }
}

void Cube3D_Geo::buildStageVertexGeometries(int stage)
{
    if (stage == 0) buildCornerStageGeometry();
}

void Cube3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 1) buildCubeStageGeometry();
}

void Cube3D_Geo::buildStageFaceGeometries(int stage)
{
    if (stage == 1 && isAllStagesComplete()) buildCubeStageGeometry();
}

void Cube3D_Geo::buildCurrentStagePreviewGeometries()
{
    int currentStage = getCurrentStage();
    if (currentStage == 0) buildCornerStageGeometry();
    else if (currentStage == 1) buildCubeStageGeometry();
}

void Cube3D_Geo::buildCornerStageGeometry()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].empty()) return;
    
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getVertexGeometry();
    if (!geometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    const Point3D& corner = allStages[0][0];
    vertices->push_back(osg::Vec3(corner.x(), corner.y(), corner.z()));
    
    geometry->setVertexArray(vertices);
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void Cube3D_Geo::buildCubeStageGeometry()
{
    if (m_calculatedSize <= 0.0f) return;
    
    osg::ref_ptr<osg::Geometry> edgeGeometry = mm_node()->getEdgeGeometry();
    if (!edgeGeometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 立方体的8个顶点
    glm::vec3 min = glm::min(m_corner1, m_corner2);
    glm::vec3 max = glm::max(m_corner1, m_corner2);
    
    std::vector<glm::vec3> cubeVertices = {
        {min.x, min.y, min.z}, {max.x, min.y, min.z},
        {max.x, max.y, min.z}, {min.x, max.y, min.z},
        {min.x, min.y, max.z}, {max.x, min.y, max.z},
        {max.x, max.y, max.z}, {min.x, max.y, max.z}
    };
    
    for (const auto& vertex : cubeVertices) {
        vertices->push_back(osg::Vec3(vertex.x, vertex.y, vertex.z));
    }
    
    edgeGeometry->setVertexArray(vertices);
    
    // 立方体的12条边
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES);
    std::vector<unsigned int> edgeIndices = {
        0,1, 1,2, 2,3, 3,0,  // 底面
        4,5, 5,6, 6,7, 7,4,  // 顶面
        0,4, 1,5, 2,6, 3,7   // 竖边
    };
    
    for (unsigned int index : edgeIndices) {
        indices->push_back(index);
    }
    
    edgeGeometry->addPrimitiveSet(indices);
}

void Cube3D_Geo::calculateCubeParameters()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    
    if (!allStages.empty() && !allStages[0].empty()) {
        m_corner1 = allStages[0][0].position;
    }
    
    if (allStages.size() >= 2 && !allStages[1].empty()) {
        m_corner2 = allStages[1][0].position;
        glm::vec3 size = glm::abs(m_corner2 - m_corner1);
        m_calculatedSize = (size.x + size.y + size.z) / 3.0f; // 平均边长
    }
}

bool Cube3D_Geo::isValidCubeConfiguration() const
{
    return m_calculatedSize > 0.001f;
}

void Cube3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    buildStageVertexGeometries(getCurrentStage());
}

void Cube3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    buildStageEdgeGeometries(getCurrentStage());
}

void Cube3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    buildStageFaceGeometries(getCurrentStage());
}

bool Cube3D_Geo::isDrawingComplete() const
{
    return isAllStagesComplete();
}

bool Cube3D_Geo::areControlPointsValid() const
{
    return isValidCubeConfiguration();
}
