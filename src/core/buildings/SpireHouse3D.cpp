#include "SpireHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"
#include <QKeyEvent>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SpireHouse3D_Geo::SpireHouse3D_Geo()
    : m_width(4.0f)
    , m_length(6.0f)
    , m_height(3.0f)
    , m_spireHeight(2.0f)
    , m_baseCorner1(0.0f)
    , m_baseCorner2(0.0f)
    , m_calculatedWidth(0.0f)
    , m_calculatedLength(0.0f)
    , m_calculatedHeight(0.0f)
    , m_calculatedSpireHeight(0.0f)
{
    m_geoType = Geo_SpireHouse3D;
    initialize();
}

std::vector<StageDescriptor> SpireHouse3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    descriptors.emplace_back("绘制房屋底面", 2, 2);
    descriptors.emplace_back("定义房屋高度", 1, 1);
    descriptors.emplace_back("定义尖顶高度", 1, 1);
    return descriptors;
}

void SpireHouse3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    
    if (event->button() == Qt::RightButton) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) {
                    calculateSpireHouseParameters();
                    qDebug() << "尖顶房屋: 进入阶段" << getCurrentStage() + 1;
                }
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateSpireHouseParameters();
                mm_state()->setStateComplete();
                qDebug() << "尖顶房屋: 绘制完成";
            }
        }
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        bool success = mm_controlPoint()->addControlPointToCurrentStage(Point3D(worldPos));
        if (success) {
            calculateSpireHouseParameters();
            if (isCurrentStageComplete() && canAdvanceToNextStage()) {
                nextStage();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                mm_state()->setStateComplete();
            }
        }
    }
}

void SpireHouse3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
}

void SpireHouse3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) calculateSpireHouseParameters();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateSpireHouseParameters();
                mm_state()->setStateComplete();
            }
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
        calculateSpireHouseParameters();
    }
}

void SpireHouse3D_Geo::buildStageVertexGeometries(int stage)
{
    if (stage == 0) buildBaseStageGeometry();
}

void SpireHouse3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 0) buildBaseStageGeometry();
    else if (stage >= 1) buildSpireHouseStageGeometry();
}

void SpireHouse3D_Geo::buildStageFaceGeometries(int stage)
{
    if (stage >= 2 && isAllStagesComplete()) buildSpireHouseStageGeometry();
}

void SpireHouse3D_Geo::buildCurrentStagePreviewGeometries()
{
    int currentStage = getCurrentStage();
    if (currentStage == 0) buildBaseStageGeometry();
    else buildSpireHouseStageGeometry();
}

void SpireHouse3D_Geo::buildBaseStageGeometry()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].size() < 2) return;
    
    osg::ref_ptr<osg::Geometry> edgeGeometry = mm_node()->getEdgeGeometry();
    if (!edgeGeometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    const auto& stage0Points = allStages[0];
    const Point3D& corner1 = stage0Points[0];
    const Point3D& corner2 = stage0Points[1];
    
    // 绘制底面矩形对角线
    vertices->push_back(osg::Vec3(corner1.x(), corner1.y(), corner1.z()));
    vertices->push_back(osg::Vec3(corner2.x(), corner2.y(), corner2.z()));
    
    // 计算矩形的四个角点并绘制边框
    if (m_calculatedWidth > 0.0f && m_calculatedLength > 0.0f) {
        glm::vec3 dir = corner2.position - corner1.position;
        glm::vec3 perpDir = glm::normalize(glm::cross(dir, glm::vec3(0, 0, 1)));
        
        glm::vec3 corner3 = corner1.position + perpDir * m_calculatedWidth;
        glm::vec3 corner4 = corner2.position + perpDir * m_calculatedWidth;
        
        // 添加矩形边界
        vertices->push_back(osg::Vec3(corner1.x(), corner1.y(), corner1.z()));
        vertices->push_back(osg::Vec3(corner3.x, corner3.y, corner3.z));
        
        vertices->push_back(osg::Vec3(corner3.x, corner3.y, corner3.z));
        vertices->push_back(osg::Vec3(corner4.x, corner4.y, corner4.z));
        
        vertices->push_back(osg::Vec3(corner4.x, corner4.y, corner4.z));
        vertices->push_back(osg::Vec3(corner2.x(), corner2.y(), corner2.z()));
        
        vertices->push_back(osg::Vec3(corner2.x(), corner2.y(), corner2.z()));
        vertices->push_back(osg::Vec3(corner1.x(), corner1.y(), corner1.z()));
    }
    
    edgeGeometry->setVertexArray(vertices);
    
    if (vertices->size() >= 2) {
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
        edgeGeometry->addPrimitiveSet(drawArrays);
    }
}

void SpireHouse3D_Geo::buildSpireHouseStageGeometry()
{
    if (m_calculatedHeight <= 0.0f) return;
    
    osg::ref_ptr<osg::Geometry> faceGeometry = mm_node()->getFaceGeometry();
    if (!faceGeometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    
    // 计算房屋的基本顶点
    glm::vec3 dir = m_baseCorner2 - m_baseCorner1;
    glm::vec3 perpDir = glm::normalize(glm::cross(dir, glm::vec3(0, 0, 1))) * m_calculatedWidth;
    
    // 底面4个顶点
    glm::vec3 bottomVerts[4] = {
        m_baseCorner1,
        m_baseCorner2,
        m_baseCorner2 + perpDir,
        m_baseCorner1 + perpDir
    };
    
    // 房屋顶部4个顶点
    glm::vec3 houseTopVerts[4];
    for (int i = 0; i < 4; ++i) {
        houseTopVerts[i] = bottomVerts[i] + glm::vec3(0, 0, m_calculatedHeight);
    }
    
    // 添加房屋主体顶点
    for (int i = 0; i < 4; ++i) {
        vertices->push_back(osg::Vec3(bottomVerts[i].x, bottomVerts[i].y, bottomVerts[i].z));
        normals->push_back(osg::Vec3(0, 0, -1)); // 底面法向量
    }
    for (int i = 0; i < 4; ++i) {
        vertices->push_back(osg::Vec3(houseTopVerts[i].x, houseTopVerts[i].y, houseTopVerts[i].z));
        normals->push_back(osg::Vec3(0, 0, 1)); // 顶面法向量
    }
    
    // 添加尖顶顶点
    if (m_calculatedSpireHeight > 0.0f) {
        glm::vec3 spireTop = (houseTopVerts[0] + houseTopVerts[1] + houseTopVerts[2] + houseTopVerts[3]) * 0.25f;
        spireTop.z += m_calculatedSpireHeight;
        vertices->push_back(osg::Vec3(spireTop.x, spireTop.y, spireTop.z));
        normals->push_back(osg::Vec3(0, 0, 1)); // 尖顶法向量
    }
    
    // 房屋主体索引
    // 底面三角形
    indices->push_back(0); indices->push_back(1); indices->push_back(2);
    indices->push_back(0); indices->push_back(2); indices->push_back(3);
    
    // 侧面三角形
    for (int i = 0; i < 4; ++i) {
        int next = (i + 1) % 4;
        indices->push_back(i); indices->push_back(i + 4); indices->push_back(next);
        indices->push_back(next); indices->push_back(i + 4); indices->push_back(next + 4);
    }
    
    // 尖顶三角形
    if (m_calculatedSpireHeight > 0.0f) {
        int spireTopIndex = 8;
        for (int i = 0; i < 4; ++i) {
            int next = (i + 1) % 4;
            indices->push_back(4 + i); indices->push_back(spireTopIndex); indices->push_back(4 + next);
        }
    }
    
    faceGeometry->setVertexArray(vertices);
    faceGeometry->setNormalArray(normals);
    faceGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    faceGeometry->addPrimitiveSet(indices);
}

void SpireHouse3D_Geo::calculateSpireHouseParameters()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    
    // 第一阶段：计算底面参数
    if (!allStages.empty() && allStages[0].size() >= 2) {
        m_baseCorner1 = allStages[0][0].position;
        m_baseCorner2 = allStages[0][1].position;
        
        glm::vec3 diagonal = m_baseCorner2 - m_baseCorner1;
        m_calculatedLength = glm::length(diagonal);
        m_calculatedWidth = m_calculatedLength * 0.6f; // 默认宽高比
    }
    
    // 第二阶段：计算房屋高度
    if (allStages.size() >= 2 && !allStages[1].empty()) {
        const Point3D& heightPoint = allStages[1][0];
        m_calculatedHeight = heightPoint.z() - m_baseCorner1.z;
        if (m_calculatedHeight < 0) m_calculatedHeight = -m_calculatedHeight;
    }
    
    // 第三阶段：计算尖顶高度
    if (allStages.size() >= 3 && !allStages[2].empty()) {
        const Point3D& spirePoint = allStages[2][0];
        float totalHeight = spirePoint.z() - m_baseCorner1.z;
        if (totalHeight < 0) totalHeight = -totalHeight;
        m_calculatedSpireHeight = totalHeight - m_calculatedHeight;
        if (m_calculatedSpireHeight < 0) m_calculatedSpireHeight = 0.5f; // 最小尖顶高度
    }
}

bool SpireHouse3D_Geo::isValidSpireHouseConfiguration() const
{
    return m_calculatedWidth > 0.001f && m_calculatedLength > 0.001f && 
           m_calculatedHeight > 0.001f && m_calculatedSpireHeight > 0.001f;
}

void SpireHouse3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    buildStageVertexGeometries(getCurrentStage());
}

void SpireHouse3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    buildStageEdgeGeometries(getCurrentStage());
}

void SpireHouse3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    buildStageFaceGeometries(getCurrentStage());
}

bool SpireHouse3D_Geo::isDrawingComplete() const
{
    return isAllStagesComplete();
}

bool SpireHouse3D_Geo::areControlPointsValid() const
{
    return isValidSpireHouseConfiguration();
} 