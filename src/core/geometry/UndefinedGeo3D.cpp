#include "UndefinedGeo3D.h"
#include "../Common3D.h"
#include "../../util/LogManager.h"
#include "../../util/MathUtils.h"
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/Material>
#include <osg/StateSet>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/PolygonMode>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/MatrixTransform>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>
#include <GL/gl.h>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>
#include <cmath>

UndefinedGeo3D_Geo::UndefinedGeo3D_Geo()
{
    m_geoType = Geo_UndefinedGeo3D;
    initialize();
}

std::vector<StageDescriptor> UndefinedGeo3D_Geo::getStageDescriptors() const
{
    std::vector<StageDescriptor> descriptors;
    // 通用的单阶段点收集
    descriptors.emplace_back("收集控制点", 1, -1);
    return descriptors;
}

void UndefinedGeo3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    
    if (event->button() == Qt::RightButton) {
        if (isCurrentStageComplete()) {
            if (areControlPointsValid()) {
                mm_state()->setStateComplete();
                qDebug() << "未定义几何体: 绘制完成";
            }
        }
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        bool success = mm_controlPoint()->addControlPointToCurrentStage(Point3D(worldPos));
        if (success) {
            qDebug() << "未定义几何体: 添加控制点，当前数量:" << mm_controlPoint()->getCurrentStageControlPointCount();
        }
    }
}

void UndefinedGeo3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (mm_state()->isStateComplete()) return;
    mm_controlPoint()->setCurrentStageTempPoint(Point3D(worldPos));
}

void UndefinedGeo3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (isCurrentStageComplete() && areControlPointsValid()) {
            mm_state()->setStateComplete();
            qDebug() << "未定义几何体: 回车完成绘制";
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        mm_controlPoint()->removeLastControlPointFromCurrentStage();
    }
}

void UndefinedGeo3D_Geo::buildStageVertexGeometries(int stage)
{
    if (stage == 0) buildGenericGeometry();
}

void UndefinedGeo3D_Geo::buildStageEdgeGeometries(int stage)
{
    if (stage == 0) buildGenericGeometry();
}

void UndefinedGeo3D_Geo::buildStageFaceGeometries(int stage)
{
    // 对于未定义几何体，面几何体在文件导入时直接挂载，这里不需要额外构建
    // 主要用途是保持与其他几何体一致的状态管理
}

void UndefinedGeo3D_Geo::buildCurrentStagePreviewGeometries()
{
    buildGenericGeometry(); // 只构建点预览
}

void UndefinedGeo3D_Geo::buildGenericGeometry()
{
    const auto& allStages = mm_controlPoint()->getAllStageControlPoints();
    if (allStages.empty() || allStages[0].empty()) return;
    
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getVertexGeometry();
    if (!geometry.valid()) return;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 添加所有控制点
    for (const auto& point : allStages[0]) {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
    }
    
    geometry->setVertexArray(vertices);
    
    // 绘制为点
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void UndefinedGeo3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    buildStageVertexGeometries(getCurrentStage());
}

void UndefinedGeo3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    buildStageEdgeGeometries(getCurrentStage());
}

void UndefinedGeo3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    buildStageFaceGeometries(getCurrentStage());
}

bool UndefinedGeo3D_Geo::isDrawingComplete() const
{
    return isCurrentStageComplete();
}

bool UndefinedGeo3D_Geo::areControlPointsValid() const
{
    const auto& stagePoints = mm_controlPoint()->getCurrentStageControlPoints();
    
    // 至少需要一个有效的控制点
    if (stagePoints.empty()) {
        return false;
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

// ==================== 导入节点处理实现 ====================

void UndefinedGeo3D_Geo::setImportedFaceNode(osg::Node* node)
{
    if (!node) {
        LOG_WARNING("导入节点为空，无法设置", "UndefinedGeo3D");
        return;
    }
    
    // 设置节点的NodeMask为面拾取，使其可以被选中
    node->setNodeMask(NODE_MASK_FACE);
    
    // 设置用户数据，指向这个几何体对象，供拾取系统使用
    node->setUserData(this);
    
    // 将导入的节点挂载到变换节点下，与其他面几何体在同一层级
    auto transformNode = mm_node()->getTransformNode();
    if (transformNode.valid()) {
        transformNode->addChild(node);
        LOG_INFO("成功将导入节点设置为面节点，可支持选中拾取", "UndefinedGeo3D");
    } else {
        LOG_ERROR("变换节点无效，无法挂载导入节点", "UndefinedGeo3D");
    }
}

