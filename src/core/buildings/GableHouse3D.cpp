#include "GableHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"
#include <QKeyEvent>

GableHouse3D_Geo::GableHouse3D_Geo()
    : m_width(4.0f)
    , m_length(6.0f)
    , m_height(3.0f)
    , m_gableHeight(1.5f)
    , m_baseCorner1(0.0f)
    , m_baseCorner2(0.0f)
    , m_calculatedWidth(0.0f)
    , m_calculatedLength(0.0f)
    , m_calculatedHeight(0.0f)
    , m_calculatedGableHeight(0.0f)
{
    m_geoType = Geo_GableHouse3D;
    initialize();
}

std::vector<StageDescriptor> GableHouse3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    descriptors.emplace_back("绘制房屋底面", 2, 2);
    descriptors.emplace_back("定义房屋和山墙高度", 1, 1);
    return descriptors;
}

void GableHouse3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    
    if (event->button() == Qt::RightButton) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) {
                    calculateGableHouseParameters();
                    qDebug() << "山墙房屋: 进入阶段" << getCurrentStage() + 1;
                }
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateGableHouseParameters();
                mm_state()->setStateComplete();
                qDebug() << "山墙房屋: 绘制完成";
            }
        }
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        bool success = mm_controlPoint()->addControlPointToCurrentStage(Point3D(worldPos));
        if (success) {
            calculateGableHouseParameters();
            if (isCurrentStageComplete() && canAdvanceToNextStage()) {
                nextStage();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                mm_state()->setStateComplete();
            }
        }
    }
}

void GableHouse3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
}

void GableHouse3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) calculateGableHouseParameters();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateGableHouseParameters();
                mm_state()->setStateComplete();
            }
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
        calculateGableHouseParameters();
    }
}

void GableHouse3D_Geo::buildStageVertexGeometries(int stage)
{
    if (stage == 0) buildBaseStageGeometry();
}

void GableHouse3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 0) buildBaseStageGeometry();
    else if (stage == 1) buildGableHouseStageGeometry();
}

void GableHouse3D_Geo::buildStageFaceGeometries(int stage)
{
    if (stage == 1 && isAllStagesComplete()) buildGableHouseStageGeometry();
}

void GableHouse3D_Geo::buildCurrentStagePreviewGeometries()
{
    int currentStage = getCurrentStage();
    if (currentStage == 0) buildBaseStageGeometry();
    else if (currentStage == 1) buildGableHouseStageGeometry();
}

void GableHouse3D_Geo::buildBaseStageGeometry()
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

void GableHouse3D_Geo::buildGableHouseStageGeometry()
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
    
    // 房屋墙顶4个顶点
    glm::vec3 wallTopVerts[4];
    for (int i = 0; i < 4; ++i) {
        wallTopVerts[i] = bottomVerts[i] + glm::vec3(0, 0, m_calculatedHeight);
    }
    
    // 山墙顶部中点
    glm::vec3 gableTop1 = (wallTopVerts[0] + wallTopVerts[1]) * 0.5f + glm::vec3(0, 0, m_calculatedGableHeight);
    glm::vec3 gableTop2 = (wallTopVerts[2] + wallTopVerts[3]) * 0.5f + glm::vec3(0, 0, m_calculatedGableHeight);
    
    // 添加房屋主体顶点
    for (int i = 0; i < 4; ++i) {
        vertices->push_back(osg::Vec3(bottomVerts[i].x, bottomVerts[i].y, bottomVerts[i].z));
        normals->push_back(osg::Vec3(0, 0, -1)); // 底面法向量
    }
    for (int i = 0; i < 4; ++i) {
        vertices->push_back(osg::Vec3(wallTopVerts[i].x, wallTopVerts[i].y, wallTopVerts[i].z));
        normals->push_back(osg::Vec3(0, 0, 0)); // 侧面法向量（暂时设为0）
    }
    
    // 添加山墙顶部顶点
    vertices->push_back(osg::Vec3(gableTop1.x, gableTop1.y, gableTop1.z));
    normals->push_back(osg::Vec3(0, 0, 1));
    vertices->push_back(osg::Vec3(gableTop2.x, gableTop2.y, gableTop2.z));
    normals->push_back(osg::Vec3(0, 0, 1));
    
    // 房屋主体索引
    // 底面
    indices->push_back(0); indices->push_back(1); indices->push_back(2);
    indices->push_back(0); indices->push_back(2); indices->push_back(3);
    
    // 侧面（不包括山墙）
    indices->push_back(1); indices->push_back(5); indices->push_back(2);
    indices->push_back(2); indices->push_back(5); indices->push_back(6);
    
    indices->push_back(3); indices->push_back(2); indices->push_back(7);
    indices->push_back(2); indices->push_back(6); indices->push_back(7);
    
    // 山墙三角形面
    indices->push_back(4); indices->push_back(8); indices->push_back(5);  // 山墙1
    indices->push_back(0); indices->push_back(4); indices->push_back(8);
    
    indices->push_back(6); indices->push_back(9); indices->push_back(7);  // 山墙2
    indices->push_back(3); indices->push_back(7); indices->push_back(9);
    
    // 屋顶面
    indices->push_back(8); indices->push_back(4); indices->push_back(9);
    indices->push_back(4); indices->push_back(7); indices->push_back(9);
    
    indices->push_back(5); indices->push_back(8); indices->push_back(6);
    indices->push_back(8); indices->push_back(9); indices->push_back(6);
    
    faceGeometry->setVertexArray(vertices);
    faceGeometry->setNormalArray(normals);
    faceGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    faceGeometry->addPrimitiveSet(indices);
}

void GableHouse3D_Geo::calculateGableHouseParameters()
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
    
    // 第二阶段：计算房屋高度和山墙高度
    if (allStages.size() >= 2 && !allStages[1].empty()) {
        const Point3D& heightPoint = allStages[1][0];
        float totalHeight = heightPoint.z() - m_baseCorner1.z;
        if (totalHeight < 0) totalHeight = -totalHeight;
        
        m_calculatedHeight = totalHeight * 0.7f; // 墙高度占70%
        m_calculatedGableHeight = totalHeight * 0.3f; // 山墙高度占30%
    }
}

bool GableHouse3D_Geo::isValidGableHouseConfiguration() const
{
    return m_calculatedWidth > 0.001f && m_calculatedLength > 0.001f && 
           m_calculatedHeight > 0.001f && m_calculatedGableHeight > 0.001f;
}

void GableHouse3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    buildStageVertexGeometries(getCurrentStage());
}

void GableHouse3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    buildStageEdgeGeometries(getCurrentStage());
}

void GableHouse3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    buildStageFaceGeometries(getCurrentStage());
}

bool GableHouse3D_Geo::isDrawingComplete() const
{
    return isAllStagesComplete();
}

bool GableHouse3D_Geo::areControlPointsValid() const
{
    return isValidGableHouseConfiguration();
} 