#include "GeometryPickingSystem.h"
#include "../GeometryBase.h"
#include "../managers/GeoNodeManager.h"
#include "../../util/LogManager.h"
#include <osg/Timer>
#include <osgViewer/Viewer>
#include <osgGA/GUIEventAdapter>
#include <osgUtil/IntersectionVisitor>
#include <cmath>
#include <algorithm>
#include <limits>

// ============================================================================
// GeometryPickingSystem Implementation
// ============================================================================

GeometryPickingSystem::GeometryPickingSystem()
{
    LOG_INFO("创建拾取系统", "拾取");
}

GeometryPickingSystem::~GeometryPickingSystem()
{
    shutdown();
}

bool GeometryPickingSystem::initialize(osg::Camera* camera, osg::Group* sceneRoot)
{
    if (!camera || !sceneRoot) {
        LOG_ERROR("初始化参数无效", "拾取");
        return false;
    }
    
    m_camera = camera;
    m_sceneRoot = sceneRoot;
    m_initialized = true;
    
    LOG_SUCCESS("拾取系统初始化成功", "拾取");
    return true;
}

void GeometryPickingSystem::shutdown()
{
    if (!m_initialized) return;
    
    m_camera = nullptr;
    m_sceneRoot = nullptr;
    m_initialized = false;
    
    LOG_INFO("拾取系统已关闭", "拾取");
}

PickResult GeometryPickingSystem::pickGeometry(int mouseX, int mouseY)
{
    if (!m_initialized) {
        LOG_ERROR("拾取系统未初始化", "拾取");
        return PickResult();
    }
    
    // 清空之前的结果
    m_singleResults.clear();
    
    // 1. 面拾取 - 单独遍历
    if (m_config.enableFacePicking) {
        osg::ref_ptr<osgUtil::LineSegmentIntersector> rayIntersector = 
            new osgUtil::LineSegmentIntersector(osgUtil::Intersector::WINDOW, mouseX, mouseY);
        rayIntersector->setPrecisionHint(osgUtil::Intersector::USE_DOUBLE_CALCULATIONS);
        rayIntersector->setIntersectionLimit(osgUtil::Intersector::LIMIT_NEAREST);  // 只获取最近的交点
        
        osg::ref_ptr<osgUtil::IntersectionVisitor> faceVisitor = 
            new osgUtil::IntersectionVisitor(rayIntersector.get());
        faceVisitor->setTraversalMask(NODE_MASK_FACE);  // 只访问面几何体
        
        // 单独遍历面几何体
        m_camera->accept(*faceVisitor);
        
        // 收集面拾取结果（只有一个最近的）
        if (rayIntersector->containsIntersections()) {
            m_singleResults.faceIntersection = rayIntersector->getFirstIntersection();
            m_singleResults.hasFaceResult = true;
        }
    }
    
    // 2. 顶点拾取 - 单独遍历
    if (m_config.enableVertexPicking) {
        double radius = m_config.cylinderRadius;
        osg::ref_ptr<osgUtil::PolytopeIntersector> cylinderIntersector = 
            new osgUtil::PolytopeIntersector(osgUtil::Intersector::WINDOW, 
                                            mouseX - radius, mouseY - radius,
                                            mouseX + radius, mouseY + radius);
        cylinderIntersector->setPrecisionHint(osgUtil::Intersector::USE_DOUBLE_CALCULATIONS);
        cylinderIntersector->setIntersectionLimit(osgUtil::Intersector::LIMIT_NEAREST);  // 只获取最近的交点
        
        osg::ref_ptr<osgUtil::IntersectionVisitor> vertexVisitor = 
            new osgUtil::IntersectionVisitor(cylinderIntersector.get());
        vertexVisitor->setTraversalMask(NODE_MASK_VERTEX|NODE_MASK_CONTROL_POINTS);  // 只访问顶点(和控制点)几何体
        
        // 单独遍历顶点几何体
        m_camera->accept(*vertexVisitor);
        
        // 收集顶点拾取结果（只有一个最近的）
        if (cylinderIntersector->containsIntersections()) {
            m_singleResults.vertexIntersection = cylinderIntersector->getFirstIntersection();
            m_singleResults.hasVertexResult = true;
        }
    }
    
    // 3. 边拾取 - 单独遍历
    if (m_config.enableEdgePicking) {
        double radius = m_config.cylinderRadius;
        osg::ref_ptr<osgUtil::PolytopeIntersector> cylinderIntersector = 
            new osgUtil::PolytopeIntersector(osgUtil::Intersector::WINDOW, 
                                            mouseX - radius, mouseY - radius,
                                            mouseX + radius, mouseY + radius);
        cylinderIntersector->setPrecisionHint(osgUtil::Intersector::USE_DOUBLE_CALCULATIONS);
        cylinderIntersector->setIntersectionLimit(osgUtil::Intersector::LIMIT_NEAREST);  // 只获取最近的交点
        
        osg::ref_ptr<osgUtil::IntersectionVisitor> edgeVisitor = 
            new osgUtil::IntersectionVisitor(cylinderIntersector.get());
        edgeVisitor->setTraversalMask(NODE_MASK_EDGE);  // 只访问边几何体
        
        // 单独遍历边几何体
        m_camera->accept(*edgeVisitor);
        
        // 收集边拾取结果（只有一个最近的）
        if (cylinderIntersector->containsIntersections()) {
            m_singleResults.edgeIntersection = cylinderIntersector->getFirstIntersection();
            m_singleResults.hasEdgeResult = true;
        }
    }
    
    // 4. 比较距离和优先级，选择最佳结果
    PickResult result = selectBestSingleResult();
    result.screenX = mouseX;
    result.screenY = mouseY;
    
    // 调用回调
    if (result.hasResult && m_pickingCallback) {
        m_pickingCallback(result);
    }
    
    return result;
}

Geo3D* GeometryPickingSystem::findGeometryFromNodePath(const osg::NodePath& nodePath)
{
    // 从最后一个节点开始查找，通常几何体信息存储在叶子节点中
    
    for (auto it = nodePath.rbegin(); it != nodePath.rend(); ++it) {
        osg::Node* node = *it;
        
        // 方法1：通过用户数据查找
        if (node->getUserData()) {
            // 尝试将用户数据转换为Geo3D指针
            if (Geo3D* geometry = dynamic_cast<Geo3D*>(node->getUserData())) {
                return geometry;
            }
        }
    }
    
    return nullptr;
}

PickResult GeometryPickingSystem::selectBestResult(const std::vector<PickResult>& results)
{
    if (results.empty()) return PickResult();
    
    // 按优先级排序：顶点 > 边 > 面
    std::vector<PickResult> sorted = results;
    
    std::sort(sorted.begin(), sorted.end(), [](const PickResult& a, const PickResult& b) {
        // 优先级：顶点=3, 边=2, 面=1
        int priorityA = static_cast<int>(a.featureType);
        int priorityB = static_cast<int>(b.featureType);
        
        if (priorityA != priorityB) {
            return priorityA < priorityB; // 数值越小优先级越高（VERTEX=1最高）
        }
        
        // 相同优先级时，选择距离更近的
        return a.distance < b.distance;
    });
    
    return sorted.front();
}

PickResult GeometryPickingSystem::selectBestSingleResult()
{
    std::vector<PickResult> candidates;
    
    // 分析面拾取结果
    if (m_singleResults.hasFaceResult) {
        PickResult faceResult = analyzeFaceIntersection(m_singleResults.faceIntersection);
        if (faceResult.hasResult) {
            candidates.push_back(faceResult);
        }
    }
    
    // 分析顶点拾取结果
    if (m_singleResults.hasVertexResult) {
        PickResult vertexResult = analyzePolytopeIntersection(m_singleResults.vertexIntersection, PickFeatureType::VERTEX);
        if (vertexResult.hasResult) {
            candidates.push_back(vertexResult);
        }
    }
    
    // 分析边拾取结果
    if (m_singleResults.hasEdgeResult) {
        PickResult edgeResult = analyzePolytopeIntersection(m_singleResults.edgeIntersection, PickFeatureType::EDGE);
        if (edgeResult.hasResult) {
            candidates.push_back(edgeResult);
        }
    }
    
    // 先按距离排序，再按优先级排序
    if (candidates.empty()) {
        return PickResult();
    }
    
    std::sort(candidates.begin(), candidates.end(), [](const PickResult& a, const PickResult& b) {
        // 先比较距离（保留一定的容差用于距离相等的判断）
        const double tolerance = 1e-6;
        if (std::abs(a.distance - b.distance) > tolerance) {
            return a.distance < b.distance;
        }
        
        // 距离相等时，按优先级：顶点(1) > 边(2) > 面(3)
        return static_cast<int>(a.featureType) < static_cast<int>(b.featureType);
    });
    
    return candidates.front();
}

PickResult GeometryPickingSystem::analyzeFaceIntersection(const osgUtil::LineSegmentIntersector::Intersection& intersection)
{
    PickResult result;
    
    Geo3D* geometry = findGeometryFromNodePath(intersection.nodePath);
    if (!geometry) return result;
    
    result.hasResult = true;
    result.geometry = geometry;
    result.featureType = PickFeatureType::FACE;
    result.worldPosition = glm::vec3(intersection.getWorldIntersectPoint().x(),
                                   intersection.getWorldIntersectPoint().y(),
                                   intersection.getWorldIntersectPoint().z());
    result.surfaceNormal = glm::vec3(intersection.getWorldIntersectNormal().x(),
                                   intersection.getWorldIntersectNormal().y(),
                                   intersection.getWorldIntersectNormal().z());
    
    // 计算距离
    osg::Vec3 cameraPos = m_camera->getInverseViewMatrix().getTrans();
    result.distance = (intersection.getWorldIntersectPoint() - cameraPos).length();
    
    // 获取图元索引
    if (!intersection.indexList.empty()) {
        result.primitiveIndex = intersection.indexList[0];
    }

    // 提取OSG几何体信息
    if (intersection.drawable.valid()) {
        osg::Geometry* osgGeom = intersection.drawable->asGeometry();
        if (osgGeom) {
            result.osgGeometry = osgGeom;
            result.osgPrimitiveIndex = intersection.primitiveIndex;
        }
    }

    return result;
}

PickResult GeometryPickingSystem::analyzePolytopeIntersection(const osgUtil::PolytopeIntersector::Intersection& intersection, PickFeatureType featureType)
{
    PickResult result;
    
    Geo3D* geometry = findGeometryFromNodePath(intersection.nodePath);
    if (!geometry) return result;
    
    result.hasResult = true;
    result.geometry = geometry;
    result.featureType = featureType;
    result.worldPosition = glm::vec3(intersection.localIntersectionPoint.x(),
                                   intersection.localIntersectionPoint.y(),
                                   intersection.localIntersectionPoint.z());
    
    // 计算距离
    osg::Vec3 cameraPos = m_camera->getInverseViewMatrix().getTrans();
    result.distance = (intersection.localIntersectionPoint - cameraPos).length();
    
    // 提取OSG几何体信息
    if (intersection.drawable.valid()) {
        osg::Geometry* osgGeom = intersection.drawable->asGeometry();
        if (osgGeom) {
            result.osgGeometry = osgGeom;
            result.osgPrimitiveIndex = intersection.primitiveIndex;
        }
    }
    
    // 节点路径已通过findGeometryFromNodePath处理，无需保存整个路径
    
    return result;
}

unsigned int GeometryPickingSystem::getPickingMask() const
{
    unsigned int mask = 0;
    if (m_config.enableVertexPicking) mask |= NODE_MASK_VERTEX;
    if (m_config.enableEdgePicking) mask |= NODE_MASK_EDGE;
    if (m_config.enableFacePicking) mask |= NODE_MASK_FACE;
    return mask != 0 ? mask : NODE_MASK_ALL_GEOMETRY;
}

glm::vec2 GeometryPickingSystem::worldToScreen(const glm::vec3& worldPos)
{
    if (!m_camera) return glm::vec2(0.0f, 0.0f);
    
    osg::Matrix VPW = m_camera->getViewMatrix() * 
                      m_camera->getProjectionMatrix() * 
                      m_camera->getViewport()->computeWindowMatrix();
    
    osg::Vec3 screenPos = osg::Vec3(worldPos.x, worldPos.y, worldPos.z) * VPW;
    return glm::vec2(screenPos.x(), screenPos.y());
}

void GeometryPickingSystem::setPickingCallback(std::function<void(const PickResult&)> callback)
{
    m_pickingCallback = callback;
}
