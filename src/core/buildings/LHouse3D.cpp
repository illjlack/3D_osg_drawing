#include "LHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"
#include <QKeyEvent>

LHouse3D_Geo::LHouse3D_Geo()
    : m_mainWidth(4.0f)
    , m_mainLength(6.0f)
    , m_extensionWidth(3.0f)
    , m_extensionLength(4.0f)
    , m_height(3.0f)
    , m_baseCorner1(0.0f)
    , m_baseCorner2(0.0f)
    , m_extensionCorner(0.0f)
    , m_calculatedMainWidth(0.0f)
    , m_calculatedMainLength(0.0f)
    , m_calculatedExtensionWidth(0.0f)
    , m_calculatedExtensionLength(0.0f)
    , m_calculatedHeight(0.0f)
{
    m_geoType = Geo_LHouse3D;
    initialize();
}

std::vector<StageDescriptor> LHouse3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    descriptors.emplace_back("绘制主体底面", 2, 2);
    descriptors.emplace_back("定义L型扩展部分", 1, 1);
    descriptors.emplace_back("定义房屋高度", 1, 1);
    return descriptors;
}

void LHouse3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    
    if (event->button() == Qt::RightButton) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) {
                    calculateLHouseParameters();
                    qDebug() << "L型房屋: 进入阶段" << getCurrentStage() + 1;
                }
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateLHouseParameters();
                mm_state()->setStateComplete();
                qDebug() << "L型房屋: 绘制完成";
            }
        }
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        bool success = mm_controlPoint()->addControlPointToCurrentStage(Point3D(worldPos));
        if (success) {
            calculateLHouseParameters();
            if (isCurrentStageComplete() && canAdvanceToNextStage()) {
                nextStage();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
            mm_state()->setStateComplete();
            }
        }
    }
}

void LHouse3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
}

void LHouse3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (isCurrentStageComplete()) {
            if (canAdvanceToNextStage()) {
                if (nextStage()) calculateLHouseParameters();
            }
            else if (isAllStagesComplete() && areControlPointsValid()) {
                calculateLHouseParameters();
                mm_state()->setStateComplete();
            }
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
        calculateLHouseParameters();
    }
}

void LHouse3D_Geo::buildStageVertexGeometries(int stage)
{
    if (stage == 0) buildBaseStageGeometry();
    else if (stage == 1) buildExtensionStageGeometry();
}

void LHouse3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 0) buildBaseStageGeometry();
    else if (stage == 1) buildExtensionStageGeometry();
    else if (stage == 2) buildLHouseStageGeometry();
}

void LHouse3D_Geo::buildStageFaceGeometries(int stage)
{
    if (stage == 2 && isAllStagesComplete()) buildLHouseStageGeometry();
}

void LHouse3D_Geo::buildCurrentStagePreviewGeometries()
{
    int currentStage = getCurrentStage();
    if (currentStage == 0) buildBaseStageGeometry();
    else if (currentStage == 1) buildExtensionStageGeometry();
    else if (currentStage == 2) buildLHouseStageGeometry();
}

void LHouse3D_Geo::buildBaseStageGeometry()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].size() < 2) return;
    
    osg::ref_ptr<osg::Geometry> edgeGeometry = mm_node()->getEdgeGeometry();
    if (!edgeGeometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    const auto& stage0Points = allStages[0];
    const Point3D& corner1 = stage0Points[0];
    const Point3D& corner2 = stage0Points[1];
    
    // 绘制主体底面矩形对角线
    vertices->push_back(osg::Vec3(corner1.x(), corner1.y(), corner1.z()));
    vertices->push_back(osg::Vec3(corner2.x(), corner2.y(), corner2.z()));
    
    // 计算矩形的四个角点并绘制边框
    if (m_calculatedMainWidth > 0.0f && m_calculatedMainLength > 0.0f) {
        glm::vec3 dir = corner2.position - corner1.position;
        glm::vec3 perpDir = glm::normalize(glm::cross(dir, glm::vec3(0, 0, 1)));
        
        glm::vec3 corner3 = corner1.position + perpDir * m_calculatedMainWidth;
        glm::vec3 corner4 = corner2.position + perpDir * m_calculatedMainWidth;
        
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

void LHouse3D_Geo::buildExtensionStageGeometry()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.size() < 2 || allStages[1].empty()) return;
    
    // 先绘制主体
    buildBaseStageGeometry();
    
    osg::ref_ptr<osg::Geometry> edgeGeometry = mm_node()->getEdgeGeometry();
    if (!edgeGeometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = dynamic_cast<osg::Vec3Array*>(edgeGeometry->getVertexArray());
    if (!vertices) return;
    
    // 添加扩展部分
    if (m_calculatedExtensionWidth > 0.0f && m_calculatedExtensionLength > 0.0f) {
        glm::vec3 extensionCorner1 = m_extensionCorner;
        glm::vec3 extensionCorner2 = m_extensionCorner + glm::vec3(m_calculatedExtensionLength, 0, 0);
        glm::vec3 extensionCorner3 = m_extensionCorner + glm::vec3(m_calculatedExtensionLength, m_calculatedExtensionWidth, 0);
        glm::vec3 extensionCorner4 = m_extensionCorner + glm::vec3(0, m_calculatedExtensionWidth, 0);
        
        // 添加扩展部分边界
        int startIndex = vertices->size();
        vertices->push_back(osg::Vec3(extensionCorner1.x, extensionCorner1.y, extensionCorner1.z));
        vertices->push_back(osg::Vec3(extensionCorner2.x, extensionCorner2.y, extensionCorner2.z));
        
        vertices->push_back(osg::Vec3(extensionCorner2.x, extensionCorner2.y, extensionCorner2.z));
        vertices->push_back(osg::Vec3(extensionCorner3.x, extensionCorner3.y, extensionCorner3.z));
        
        vertices->push_back(osg::Vec3(extensionCorner3.x, extensionCorner3.y, extensionCorner3.z));
        vertices->push_back(osg::Vec3(extensionCorner4.x, extensionCorner4.y, extensionCorner4.z));
        
        vertices->push_back(osg::Vec3(extensionCorner4.x, extensionCorner4.y, extensionCorner4.z));
        vertices->push_back(osg::Vec3(extensionCorner1.x, extensionCorner1.y, extensionCorner1.z));
        
        edgeGeometry->removePrimitiveSet(0, edgeGeometry->getNumPrimitiveSets());
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
        edgeGeometry->addPrimitiveSet(drawArrays);
    }
}

void LHouse3D_Geo::buildLHouseStageGeometry()
{
    if (m_calculatedHeight <= 0.0f) return;
    
    osg::ref_ptr<osg::Geometry> faceGeometry = mm_node()->getFaceGeometry();
    if (!faceGeometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    
    // 计算主体部分的顶点
    glm::vec3 dir = m_baseCorner2 - m_baseCorner1;
    glm::vec3 perpDir = glm::normalize(glm::cross(dir, glm::vec3(0, 0, 1))) * m_calculatedMainWidth;
    
    // 主体底面4个顶点
    glm::vec3 mainBottomVerts[4] = {
        m_baseCorner1,
        m_baseCorner2,
        m_baseCorner2 + perpDir,
        m_baseCorner1 + perpDir
    };
    
    // 主体顶部4个顶点
    glm::vec3 mainTopVerts[4];
    for (int i = 0; i < 4; ++i) {
        mainTopVerts[i] = mainBottomVerts[i] + glm::vec3(0, 0, m_calculatedHeight);
    }
    
    // 扩展部分顶点
    glm::vec3 extBottomVerts[4] = {
        m_extensionCorner,
        m_extensionCorner + glm::vec3(m_calculatedExtensionLength, 0, 0),
        m_extensionCorner + glm::vec3(m_calculatedExtensionLength, m_calculatedExtensionWidth, 0),
        m_extensionCorner + glm::vec3(0, m_calculatedExtensionWidth, 0)
    };
    
    glm::vec3 extTopVerts[4];
    for (int i = 0; i < 4; ++i) {
        extTopVerts[i] = extBottomVerts[i] + glm::vec3(0, 0, m_calculatedHeight);
    }
    
    // 添加主体顶点
    for (int i = 0; i < 4; ++i) {
        vertices->push_back(osg::Vec3(mainBottomVerts[i].x, mainBottomVerts[i].y, mainBottomVerts[i].z));
        normals->push_back(osg::Vec3(0, 0, -1));
    }
    for (int i = 0; i < 4; ++i) {
        vertices->push_back(osg::Vec3(mainTopVerts[i].x, mainTopVerts[i].y, mainTopVerts[i].z));
        normals->push_back(osg::Vec3(0, 0, 1));
    }
    
    // 添加扩展部分顶点
    for (int i = 0; i < 4; ++i) {
        vertices->push_back(osg::Vec3(extBottomVerts[i].x, extBottomVerts[i].y, extBottomVerts[i].z));
        normals->push_back(osg::Vec3(0, 0, -1));
    }
    for (int i = 0; i < 4; ++i) {
        vertices->push_back(osg::Vec3(extTopVerts[i].x, extTopVerts[i].y, extTopVerts[i].z));
            normals->push_back(osg::Vec3(0, 0, 1));
        }
        
    // 主体部分索引
    // 底面
    indices->push_back(0); indices->push_back(1); indices->push_back(2);
    indices->push_back(0); indices->push_back(2); indices->push_back(3);
    
    // 顶面
    indices->push_back(4); indices->push_back(6); indices->push_back(5);
    indices->push_back(4); indices->push_back(7); indices->push_back(6);
    
    // 侧面
    for (int i = 0; i < 4; ++i) {
        int next = (i + 1) % 4;
        indices->push_back(i); indices->push_back(i + 4); indices->push_back(next);
        indices->push_back(next); indices->push_back(i + 4); indices->push_back(next + 4);
    }
    
    // 扩展部分索引
    // 底面
    indices->push_back(8); indices->push_back(9); indices->push_back(10);
    indices->push_back(8); indices->push_back(10); indices->push_back(11);
    
    // 顶面
    indices->push_back(12); indices->push_back(14); indices->push_back(13);
    indices->push_back(12); indices->push_back(15); indices->push_back(14);
    
    // 侧面
    for (int i = 0; i < 4; ++i) {
            int next = (i + 1) % 4;
        indices->push_back(8 + i); indices->push_back(8 + i + 4); indices->push_back(8 + next);
        indices->push_back(8 + next); indices->push_back(8 + i + 4); indices->push_back(8 + next + 4);
    }
    
    faceGeometry->setVertexArray(vertices);
    faceGeometry->setNormalArray(normals);
    faceGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    faceGeometry->addPrimitiveSet(indices);
}

void LHouse3D_Geo::calculateLHouseParameters()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    
    // 第一阶段：计算主体底面参数
    if (!allStages.empty() && allStages[0].size() >= 2) {
        m_baseCorner1 = allStages[0][0].position;
        m_baseCorner2 = allStages[0][1].position;
        
        glm::vec3 diagonal = m_baseCorner2 - m_baseCorner1;
        m_calculatedMainLength = glm::length(diagonal);
        m_calculatedMainWidth = m_calculatedMainLength * 0.6f; // 默认宽高比
    }
    
    // 第二阶段：计算扩展部分参数
    if (allStages.size() >= 2 && !allStages[1].empty()) {
        m_extensionCorner = allStages[1][0].position;
        
        // 扩展部分尺寸基于主体尺寸
        m_calculatedExtensionLength = m_calculatedMainLength * 0.6f;
        m_calculatedExtensionWidth = m_calculatedMainWidth * 0.8f;
    }
    
    // 第三阶段：计算房屋高度
    if (allStages.size() >= 3 && !allStages[2].empty()) {
        const Point3D& heightPoint = allStages[2][0];
        m_calculatedHeight = heightPoint.z() - m_baseCorner1.z;
        if (m_calculatedHeight < 0) m_calculatedHeight = -m_calculatedHeight;
    }
}

bool LHouse3D_Geo::isValidLHouseConfiguration() const
{
    return m_calculatedMainWidth > 0.001f && m_calculatedMainLength > 0.001f && 
           m_calculatedExtensionWidth > 0.001f && m_calculatedExtensionLength > 0.001f &&
           m_calculatedHeight > 0.001f;
}

void LHouse3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    buildStageVertexGeometries(getCurrentStage());
}

void LHouse3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    buildStageEdgeGeometries(getCurrentStage());
}

void LHouse3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    buildStageFaceGeometries(getCurrentStage());
}

bool LHouse3D_Geo::isDrawingComplete() const
{
    return isAllStagesComplete();
}

bool LHouse3D_Geo::areControlPointsValid() const
{
    return isValidLHouseConfiguration();
} 