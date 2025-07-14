#include "CPUPickingSystem.h"
#include "../GeometryBase.h"
#include "../../util/LogManager.h"
#include <osg/NodeVisitor>
#include <osg/UserDataContainer>
#include <osg/Timer>
#include <osgUtil/IntersectionVisitor>
#include <osgViewer/Viewer>
#include <osg/LineSegment>
#include <osg/Geode>
#include <algorithm>
#include <cmath>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// CPUPickingSystem 实现
// ============================================================================

CPUPickingSystem::CPUPickingSystem()
    : m_camera(nullptr)
    , m_sceneRoot(nullptr)
    , m_indicatorManager(nullptr)
    , m_highlightSystem(nullptr)
    , m_totalSnapPoints(0)
    , m_lastPickTime(0.0)
    , m_initialized(false)
{
    // 设置默认配置
    m_config.pickingRadius = 5;
    m_config.rayCount = 8;
    m_config.snapThreshold = 0.1f;
    m_config.enableSnapping = true;
    m_config.enableIndicator = true;
    m_config.enableHighlight = true;
    m_config.indicatorSize = 0.2f;
}

CPUPickingSystem::~CPUPickingSystem()
{
    clearAllGeometries();
}

bool CPUPickingSystem::initialize(osg::Camera* camera, osg::Group* sceneRoot)
{
    if (!camera || !sceneRoot) {
        LOG_ERROR("Invalid camera or scene root for CPU picking system", "拾取");
        return false;
    }
    
    m_camera = camera;
    m_sceneRoot = sceneRoot;
    m_initialized = true;
    
    // 验证系统状态
    LOG_INFO(QString("CPU picking system initialized - Camera: %1, SceneRoot: %2")
        .arg(m_camera ? "Valid" : "Invalid")
        .arg(m_sceneRoot ? "Valid" : "Invalid"), "拾取");
    
    LOG_SUCCESS("CPU picking system initialized successfully", "拾取");
    return true;
}

void CPUPickingSystem::setConfig(const CPUPickingConfig& config)
{
    m_config = config;
    LOG_INFO(QString("CPU picking config updated - Radius: %1, Rays: %2, Threshold: %3")
        .arg(config.pickingRadius).arg(config.rayCount).arg(config.snapThreshold), "拾取");
}

void CPUPickingSystem::addGeometry(Geo3D* geometry)
{
    if (!geometry || !m_initialized) return;
    
    if (m_geometryData.find(geometry) != m_geometryData.end()) {
        LOG_DEBUG("Geometry already exists in CPU picking system", "拾取");
        return;
    }
    
    auto data = std::make_unique<GeometryPickingData>(geometry);
    buildGeometryData(geometry);
    updateSnapPoints(*data);
    
    m_geometryData[geometry] = std::move(data);
    
    LOG_DEBUG(QString("Added geometry to CPU picking system - Total geometries: %1")
        .arg(m_geometryData.size()), "拾取");
}

void CPUPickingSystem::removeGeometry(Geo3D* geometry)
{
    if (!geometry) return;
    
    auto it = m_geometryData.find(geometry);
    if (it != m_geometryData.end()) {
        m_totalSnapPoints -= it->second->snapPoints.size();
        m_geometryData.erase(it);
        
        // 如果当前高亮的是这个几何体，清除高亮
        if (m_lastResult.geometry == geometry) {
            clearHighlight();
            hideIndicator();
            m_lastResult = CPUPickingResult();
        }
        
        LOG_DEBUG(QString("Removed geometry from CPU picking system - Remaining: %1")
            .arg(m_geometryData.size()), "拾取");
    }
}

void CPUPickingSystem::updateGeometry(Geo3D* geometry)
{
    if (!geometry) return;
    
    auto it = m_geometryData.find(geometry);
    if (it != m_geometryData.end()) {
        updateSnapPoints(*it->second);
        LOG_DEBUG("Updated geometry in CPU picking system", "拾取");
    } else {
        // 如果几何体不存在，则添加它
        addGeometry(geometry);
    }
}

void CPUPickingSystem::clearAllGeometries()
{
    m_geometryData.clear();
    m_totalSnapPoints = 0;
    clearHighlight();
    hideIndicator();
    m_lastResult = CPUPickingResult();
    
    LOG_INFO("Cleared all geometries from CPU picking system", "拾取");
}

CPUPickingResult CPUPickingSystem::pick(int mouseX, int mouseY)
{
    if (!m_initialized) {
        LOG_ERROR("CPU picking system not initialized", "拾取");
        return CPUPickingResult();
    }
    
    osg::Timer_t startTime = osg::Timer::instance()->tick();
    
    // 生成多条射线
    std::vector<PickingRay> rays = generateRays(mouseX, mouseY);
    
    // 执行射线相交检测
    std::vector<osgUtil::LineSegmentIntersector::Intersection> intersections = 
        performRayIntersection(rays);
    
    // 处理候选结果
    std::vector<CPUPickingResult> candidates = processCandidates(intersections);
    
    // 选择最佳结果
    CPUPickingResult result = selectBestResult(candidates);
    
    // 计算捕捉
    if (result.hasResult && m_config.enableSnapping) {
        result = calculateSnapping(result);
    }
    
    // 处理拾取结果（指示器和高亮）
    processPickingResult(result);
    
    osg::Timer_t endTime = osg::Timer::instance()->tick();
    double pickTime = osg::Timer::instance()->delta_s(startTime, endTime);
    
    LOG_DEBUG(QString("CPU picking completed in %1ms - Result: %2")
        .arg(pickTime * 1000, 0, 'f', 2)
        .arg(result.hasResult ? "Hit" : "Miss"), "拾取");
    
    return result;
}

std::vector<PickingRay> CPUPickingSystem::generateRays(int mouseX, int mouseY)
{
    std::vector<PickingRay> rays;
    
    if (!m_camera) return rays;
    
    int radius = m_config.pickingRadius;
    int rayCount = std::max(m_config.rayCount, 16); // 至少16条射线
    
    // 添加中心射线
    glm::vec3 centerNear = screenToWorld(mouseX, mouseY, 0.0f);
    glm::vec3 centerFar = screenToWorld(mouseX, mouseY, 1.0f);
    
    if (centerNear != centerFar) {
        rays.emplace_back(centerNear, glm::normalize(centerFar - centerNear), mouseX, mouseY);
    }
    
    // 生成同心圆分布的射线，提高覆盖率
    int ringsCount = 3; // 3个圆环
    int raysPerRing = (rayCount - 1) / ringsCount;
    
    for (int ring = 1; ring <= ringsCount; ++ring) {
        float ringRadius = radius * ring / float(ringsCount);
        
        for (int i = 0; i < raysPerRing; ++i) {
            float angle = 2.0f * M_PI * i / raysPerRing;
            
            int offsetX = static_cast<int>(ringRadius * std::cos(angle));
            int offsetY = static_cast<int>(ringRadius * std::sin(angle));
            
            int rayX = mouseX + offsetX;
            int rayY = mouseY + offsetY;
            
            glm::vec3 rayNear = screenToWorld(rayX, rayY, 0.0f);
            glm::vec3 rayFar = screenToWorld(rayX, rayY, 1.0f);
            
            if (rayNear != rayFar) {
                rays.emplace_back(rayNear, glm::normalize(rayFar - rayNear), rayX, rayY);
            }
        }
    }
    
    // 如果还有剩余射线，在矩形网格中添加
    int remainingRays = rayCount - rays.size();
    if (remainingRays > 0) {
        int gridSize = static_cast<int>(std::sqrt(remainingRays)) + 1;
        float step = (2.0f * radius) / gridSize;
        
        for (int i = 0; i < gridSize && rays.size() < rayCount; ++i) {
            for (int j = 0; j < gridSize && rays.size() < rayCount; ++j) {
                int offsetX = static_cast<int>(-radius + i * step);
                int offsetY = static_cast<int>(-radius + j * step);
                
                if (offsetX == 0 && offsetY == 0) continue; // 跳过中心点
                
                int rayX = mouseX + offsetX;
                int rayY = mouseY + offsetY;
                
                glm::vec3 rayNear = screenToWorld(rayX, rayY, 0.0f);
                glm::vec3 rayFar = screenToWorld(rayX, rayY, 1.0f);
                
                if (rayNear != rayFar) {
                    rays.emplace_back(rayNear, glm::normalize(rayFar - rayNear), rayX, rayY);
                }
            }
        }
    }
    
    LOG_DEBUG(QString("Generated %1 picking rays for mouse position (%2, %3)")
        .arg(rays.size()).arg(mouseX).arg(mouseY), "拾取");
    
    return rays;
}

glm::vec3 CPUPickingSystem::screenToWorld(int x, int y, float depth)
{
    if (!m_camera) return glm::vec3(0.0f);
    
    osg::Viewport* viewport = m_camera->getViewport();
    if (!viewport) return glm::vec3(0.0f);
    
    // 翻转Y坐标
    int screenY = viewport->height() - y;
    
    // 构建视口变换矩阵
    osg::Matrix VPW = m_camera->getViewMatrix() * 
                      m_camera->getProjectionMatrix() * 
                      viewport->computeWindowMatrix();
    
    osg::Matrix invVPW;
    if (!invVPW.invert(VPW)) {
        LOG_WARNING("Failed to invert view-projection-window matrix", "拾取");
        return glm::vec3(0.0f);
    }
    
    // 转换屏幕坐标到世界坐标
    osg::Vec3 worldPos = osg::Vec3(x, screenY, depth) * invVPW;
    
    return glm::vec3(worldPos.x(), worldPos.y(), worldPos.z());
}

std::vector<osgUtil::LineSegmentIntersector::Intersection> 
CPUPickingSystem::performRayIntersection(const std::vector<PickingRay>& rays)
{
    std::vector<osgUtil::LineSegmentIntersector::Intersection> allIntersections;
    
    if (!m_sceneRoot) return allIntersections;
    
    // 逐个处理每条射线
    for (const auto& ray : rays) {
        glm::vec3 farPoint = ray.origin + ray.direction * 1000.0f;  // 远端点
        
        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector = 
            new osgUtil::LineSegmentIntersector(
                osg::Vec3(ray.origin.x, ray.origin.y, ray.origin.z),
                osg::Vec3(farPoint.x, farPoint.y, farPoint.z)
            );
        
        // 配置相交检测器
        setupIntersectorSettings(intersector.get());
        
        // 执行相交检测
        osgUtil::IntersectionVisitor visitor(intersector.get());
        m_sceneRoot->accept(visitor);
        
        // 收集相交结果
        if (intersector->containsIntersections()) {
            const auto& intersections = intersector->getIntersections();
            allIntersections.insert(allIntersections.end(), intersections.begin(), intersections.end());
        }
    }
    
    return allIntersections;
}

void CPUPickingSystem::setupIntersectorSettings(osgUtil::LineSegmentIntersector* intersector)
{
    if (!intersector) return;
    
    // 设置相交检测模式 - 获取所有相交而不是只获取最近的
    intersector->setIntersectionLimit(osgUtil::Intersector::LIMIT_ONE_PER_DRAWABLE);
}

std::vector<CPUPickingResult> CPUPickingSystem::processCandidates(
    const std::vector<osgUtil::LineSegmentIntersector::Intersection>& intersections)
{
    std::vector<CPUPickingResult> candidates;
    
    for (const auto& intersection : intersections) {
        CPUPickingResult candidate;
        candidate.hasResult = true;
        
        // 获取相交点
        osg::Vec3 wp = intersection.getWorldIntersectPoint();
        candidate.worldPosition = glm::vec3(wp.x(), wp.y(), wp.z());
        
        // 计算到相机的距离
        glm::vec3 cameraPos = glm::vec3(0.0f);
        if (m_camera) {
            osg::Vec3 camPos = m_camera->getInverseViewMatrix().getTrans();
            cameraPos = glm::vec3(camPos.x(), camPos.y(), camPos.z());
        }
        candidate.distance = glm::length(candidate.worldPosition - cameraPos);
        
        // 查找对应的几何体
        candidate.geometry = findGeometryFromNode(intersection.drawable->getParent(0));
        
        if (candidate.geometry) {
            // 确定特征类型
            candidate.featureType = determineFeatureType(intersection.drawable->getParent(0));
        }
        
        candidates.push_back(candidate);
    }
    
    return candidates;
}

PickingFeatureType CPUPickingSystem::determineFeatureType(osg::Node* node)
{
    if (!node) return PickingFeatureType::NONE;
    
    // 检查节点名称
    std::string nodeName = node->getName();
    
    // 检查父节点的名称
    osg::Node* parent = node->getParent(0);
    if (parent) {
        std::string parentName = parent->getName();
        if (parentName.find("vertex") != std::string::npos || 
            parentName.find("Vertex") != std::string::npos ||
            parentName.find("point") != std::string::npos ||
            parentName.find("Point") != std::string::npos) {
            return PickingFeatureType::VERTEX;
        }
        if (parentName.find("edge") != std::string::npos || 
            parentName.find("Edge") != std::string::npos ||
            parentName.find("line") != std::string::npos ||
            parentName.find("Line") != std::string::npos) {
            return PickingFeatureType::EDGE;
        }
        if (parentName.find("face") != std::string::npos || 
            parentName.find("Face") != std::string::npos) {
            return PickingFeatureType::FACE;
        }
    }
    
    // 检查当前节点名称
    if (nodeName.find("vertex") != std::string::npos || 
        nodeName.find("Vertex") != std::string::npos ||
        nodeName.find("point") != std::string::npos ||
        nodeName.find("Point") != std::string::npos) {
        return PickingFeatureType::VERTEX;
    }
    if (nodeName.find("edge") != std::string::npos || 
        nodeName.find("Edge") != std::string::npos ||
        nodeName.find("line") != std::string::npos ||
        nodeName.find("Line") != std::string::npos) {
        return PickingFeatureType::EDGE;
    }
    if (nodeName.find("face") != std::string::npos || 
        nodeName.find("Face") != std::string::npos) {
        return PickingFeatureType::FACE;
    }
    
    // 默认为面
    return PickingFeatureType::FACE;
}

CPUPickingResult CPUPickingSystem::calculateSnapping(const CPUPickingResult& candidate)
{
    CPUPickingResult result = candidate;
    
    if (!candidate.geometry) return result;
    
    auto it = m_geometryData.find(candidate.geometry);
    if (it == m_geometryData.end()) return result;
    
    // 计算屏幕空间的捕捉阈值（更直观）
    float worldThreshold = m_config.snapThreshold;
    
    // 如果相机距离较远，增加世界空间阈值
    if (m_camera) {
        osg::Vec3 camPos = m_camera->getInverseViewMatrix().getTrans();
        glm::vec3 cameraPos = glm::vec3(camPos.x(), camPos.y(), camPos.z());
        float distance = glm::length(candidate.worldPosition - cameraPos);
        worldThreshold = m_config.snapThreshold * (distance / 10.0f); // 距离调整系数
        worldThreshold = std::min(worldThreshold, 2.0f); // 最大阈值限制
    }
    
    // 使用简单的距离检测查找最近的捕捉点
    float bestDistance = worldThreshold;
    SnapPoint bestSnap(glm::vec3(0), PickingFeatureType::NONE, FLT_MAX);
    
    for (const auto& snap : it->second->snapPoints) {
        float distance = glm::length(snap.position - candidate.worldPosition);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestSnap = snap;
        }
    }
    
    if (bestSnap.priority < FLT_MAX && bestDistance < worldThreshold) {
        result.snapPosition = bestSnap.position;
        result.isSnapped = true;
        result.featureType = bestSnap.type;
        
        // 更新屏幕坐标到捕捉位置
        glm::vec2 snapScreenPos = worldToScreen(bestSnap.position);
        result.screenX = static_cast<int>(snapScreenPos.x);
        result.screenY = static_cast<int>(snapScreenPos.y);
        
        LOG_DEBUG(QString("Snapped to point - Distance: %1, Threshold: %2")
            .arg(bestDistance).arg(worldThreshold), "拾取");
    } else {
        result.snapPosition = candidate.worldPosition;
        result.isSnapped = false;
    }
    
    return result;
}

// 添加世界坐标到屏幕坐标的转换方法
glm::vec2 CPUPickingSystem::worldToScreen(const glm::vec3& worldPos)
{
    if (!m_camera) return glm::vec2(0, 0);
    
    osg::Viewport* viewport = m_camera->getViewport();
    if (!viewport) return glm::vec2(0, 0);
    
    // 构建变换矩阵
    osg::Matrix VPW = m_camera->getViewMatrix() * 
                      m_camera->getProjectionMatrix() * 
                      viewport->computeWindowMatrix();
    
    osg::Vec3 screenPos = osg::Vec3(worldPos.x, worldPos.y, worldPos.z) * VPW;
    
    // 翻转Y坐标
    float screenY = viewport->height() - screenPos.y();
    
    return glm::vec2(screenPos.x(), screenY);
}

void CPUPickingSystem::buildGeometryData(Geo3D* geometry)
{
    if (!geometry) return;
    
    // 为几何体的OSG节点构建包围盒信息
    osg::ref_ptr<osg::Group> osgNode = geometry->getOSGNode();
    if (osgNode) {
        // 计算包围球
        osg::BoundingSphere bs = osgNode->getBound();
        auto it = m_geometryData.find(geometry);
        if (it != m_geometryData.end()) {
            it->second->boundingSphere = bs;
        }
    }
}

void CPUPickingSystem::updateSnapPoints(GeometryPickingData& data)
{
    if (!data.geometry) return;
    
    data.snapPoints.clear();
    
    // 添加控制点作为捕捉点
    const auto& controlPoints = data.geometry->getControlPoints();
    for (const auto& cp : controlPoints) {
        data.snapPoints.emplace_back(cp.position, PickingFeatureType::VERTEX, 0.0f);
    }
    
    // 从顶点节点提取捕捉点
    osg::ref_ptr<osg::Group> vertexNode = data.geometry->getVertexNode();
    if (vertexNode) {
        // 设置节点名称以便拾取识别
        vertexNode->setName("vertex_group");
        
        // 遍历顶点节点，提取所有顶点位置
        for (unsigned int i = 0; i < vertexNode->getNumChildren(); ++i) {
            osg::Node* child = vertexNode->getChild(i);
            child->setName("vertex_node");
            
            if (auto geode = dynamic_cast<osg::Geode*>(child)) {
                for (unsigned int j = 0; j < geode->getNumDrawables(); ++j) {
                    if (auto geometry = dynamic_cast<osg::Geometry*>(geode->getDrawable(j))) {
                        if (auto vertices = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray())) {
                            for (const auto& vertex : *vertices) {
                                data.snapPoints.emplace_back(
                                    glm::vec3(vertex.x(), vertex.y(), vertex.z()),
                                    PickingFeatureType::VERTEX, 0.0f);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 从边节点提取中点作为捕捉点
    osg::ref_ptr<osg::Group> edgeNode = data.geometry->getEdgeNode();
    if (edgeNode) {
        edgeNode->setName("edge_group");
        
        for (unsigned int i = 0; i < edgeNode->getNumChildren(); ++i) {
            osg::Node* child = edgeNode->getChild(i);
            child->setName("edge_node");
        }
    }
    
    // 设置面节点名称
    osg::ref_ptr<osg::Group> faceNode = data.geometry->getFaceNode();
    if (faceNode) {
        faceNode->setName("face_group");
        
        for (unsigned int i = 0; i < faceNode->getNumChildren(); ++i) {
            osg::Node* child = faceNode->getChild(i);
            child->setName("face_node");
        }
    }
    
    // 更新统计信息
    m_totalSnapPoints += data.snapPoints.size();
    
    LOG_DEBUG(QString("Updated snap points for geometry - Points: %1")
        .arg(data.snapPoints.size()), "拾取");
}

CPUPickingResult CPUPickingSystem::selectBestResult(const std::vector<CPUPickingResult>& candidates)
{
    if (candidates.empty()) return CPUPickingResult();
    
    // 按优先级排序：顶点 > 边 > 面，相同类型按距离排序
    std::vector<CPUPickingResult> vertices, edges, faces;
    
    for (const auto& candidate : candidates) {
        switch (candidate.featureType) {
        case PickingFeatureType::VERTEX:
            vertices.push_back(candidate);
            break;
        case PickingFeatureType::EDGE:
            edges.push_back(candidate);
            break;
        case PickingFeatureType::FACE:
            faces.push_back(candidate);
            break;
        }
    }
    
    // 选择最高优先级的类型中距离最近的
    if (!vertices.empty()) {
        return *std::min_element(vertices.begin(), vertices.end(),
            [](const CPUPickingResult& a, const CPUPickingResult& b) {
                return a.distance < b.distance;
            });
    }
    
    if (!edges.empty()) {
        return *std::min_element(edges.begin(), edges.end(),
            [](const CPUPickingResult& a, const CPUPickingResult& b) {
                return a.distance < b.distance;
            });
    }
    
    if (!faces.empty()) {
        return *std::min_element(faces.begin(), faces.end(),
            [](const CPUPickingResult& a, const CPUPickingResult& b) {
                return a.distance < b.distance;
            });
    }
    
    return CPUPickingResult();
}

void CPUPickingSystem::processPickingResult(const CPUPickingResult& result)
{
    // 详细的调试输出
    if (result.hasResult) {
        LOG_INFO(QString("Picking SUCCESS - Geometry: %1, Feature: %2, Position: (%3, %4, %5), Snapped: %6")
            .arg(result.geometry ? result.geometry->getGeoType() : -1)
            .arg(static_cast<int>(result.featureType))
            .arg(result.worldPosition.x, 0, 'f', 3)
            .arg(result.worldPosition.y, 0, 'f', 3)
            .arg(result.worldPosition.z, 0, 'f', 3)
            .arg(result.isSnapped ? "Yes" : "No"), "拾取");
    } else {
        LOG_DEBUG("Picking FAILED - No geometry hit", "拾取");
    }
    
    // 如果结果与上次相同，不需要更新
    if (result.hasResult == m_lastResult.hasResult &&
        result.geometry == m_lastResult.geometry &&
        result.featureType == m_lastResult.featureType) {
        return;
    }
    
    // 清除上次的高亮和指示器
    clearHighlight();
    hideIndicator();
    
    // 处理新结果
    if (result.hasResult) {
        // 显示指示器
        if (m_config.enableIndicator) {
            showIndicator(result);
            LOG_DEBUG("Indicator shown", "拾取");
        }
        
        // 高亮几何体
        if (m_config.enableHighlight) {
            highlightGeometry(result.geometry);
            LOG_DEBUG("Geometry highlighted", "拾取");
        }
    }
    
    // 保存当前结果
    m_lastResult = result;
}

void CPUPickingSystem::showIndicator(const CPUPickingResult& result)
{
    if (!m_indicatorManager || !result.hasResult) return;
    
    // 转换为统一拾取结果格式
    PickingResult unifiedResult;
    unifiedResult.hasResult = true;
    unifiedResult.geometry = result.geometry;
    unifiedResult.worldPos = result.isSnapped ? result.snapPosition : result.worldPosition;
    unifiedResult.depth = result.distance;
    unifiedResult.screenX = result.screenX;
    unifiedResult.screenY = result.screenY;
    
    // 设置ID信息
    switch (result.featureType) {
    case PickingFeatureType::VERTEX:
        unifiedResult.id.typeCode = PickingID64::TYPE_VERTEX;
        break;
    case PickingFeatureType::EDGE:
        unifiedResult.id.typeCode = PickingID64::TYPE_EDGE;
        break;
    case PickingFeatureType::FACE:
        unifiedResult.id.typeCode = PickingID64::TYPE_FACE;
        break;
    default:
        unifiedResult.id.typeCode = PickingID64::TYPE_INVALID;
        break;
    }
    
    m_indicatorManager->showIndicator(unifiedResult);
}

void CPUPickingSystem::hideIndicator()
{
    if (m_indicatorManager) {
        m_indicatorManager->hideIndicator();
    }
}

void CPUPickingSystem::highlightGeometry(Geo3D* geometry)
{
    if (!m_highlightSystem || !geometry) return;
    
    m_highlightSystem->highlightObject(geometry);
}

void CPUPickingSystem::clearHighlight()
{
    if (m_highlightSystem) {
        m_highlightSystem->clearHighlight();
    }
}

Geo3D* CPUPickingSystem::findGeometryFromNode(osg::Node* node)
{
    if (!node) return nullptr;
    
    // 向上遍历节点树，查找对应的几何体
    osg::Node* current = node;
    std::set<osg::Node*> visited; // 防止循环遍历
    
    while (current && visited.find(current) == visited.end()) {
        visited.insert(current);
        
        // 检查当前节点是否是某个几何体的节点
        for (auto& pair : m_geometryData) {
            Geo3D* geometry = pair.second->geometry.get();
            if (!geometry) continue;
            
            osg::ref_ptr<osg::Group> osgNode = geometry->getOSGNode();
            if (!osgNode) continue;
            
            // 检查是否是几何体的根节点或子节点
            if (osgNode == current) {
                return geometry;
            }
            
            // 检查是否在几何体的节点树中
            if (osgNode->containsNode(current)) {
                return geometry;
            }
            
            // 检查点线面节点
            if (geometry->getVertexNode() && 
                (geometry->getVertexNode() == current || geometry->getVertexNode()->containsNode(current))) {
                return geometry;
            }
            
            if (geometry->getEdgeNode() && 
                (geometry->getEdgeNode() == current || geometry->getEdgeNode()->containsNode(current))) {
                return geometry;
            }
            
            if (geometry->getFaceNode() && 
                (geometry->getFaceNode() == current || geometry->getFaceNode()->containsNode(current))) {
                return geometry;
            }
        }
        
        // 向上移动到父节点
        if (current->getNumParents() > 0) {
            current = current->getParent(0);
        } else {
            break;
        }
    }
    
    return nullptr;
}

// ============================================================================
// CPUPickingEventHandler 实现
// ============================================================================

CPUPickingEventHandler::CPUPickingEventHandler(CPUPickingSystem* pickingSystem)
    : m_pickingSystem(pickingSystem)
    , m_enabled(true)
    , m_pickingFrequency(60.0f)
    , m_lastPickTime(0.0)
    , m_lastX(-1)
    , m_lastY(-1)
{
}

CPUPickingEventHandler::~CPUPickingEventHandler()
{
}

bool CPUPickingEventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    if (!m_enabled || !m_pickingSystem) return false;
    
    switch (ea.getEventType()) {
    case osgGA::GUIEventAdapter::MOVE:
        if (ea.getX() != m_lastX || ea.getY() != m_lastY) {
            processPicking(static_cast<int>(ea.getX()), static_cast<int>(ea.getY()));
            m_lastX = static_cast<int>(ea.getX());
            m_lastY = static_cast<int>(ea.getY());
        }
        break;
    default:
        break;
    }
    
    return false;
}

void CPUPickingEventHandler::setPickingCallback(std::function<void(const CPUPickingResult&)> callback)
{
    m_pickingCallback = callback;
}

void CPUPickingEventHandler::processPicking(int x, int y)
{
    if (!m_pickingSystem) return;
    
    // 频率控制
    double currentTime = osg::Timer::instance()->time_s();
    if (currentTime - m_lastPickTime < 1.0 / m_pickingFrequency) {
        return;
    }
    m_lastPickTime = currentTime;
    
    // 执行拾取
    CPUPickingResult result = m_pickingSystem->pick(x, y);
    
    // 调用回调
    if (m_pickingCallback) {
        m_pickingCallback(result);
    }
}

// ============================================================================
// CPUPickingSystemManager 实现
// ============================================================================

CPUPickingSystemManager& CPUPickingSystemManager::getInstance()
{
    static CPUPickingSystemManager instance;
    return instance;
}

CPUPickingSystemManager::CPUPickingSystemManager()
    : m_cpuPickingEnabled(false)
{
    m_pickingSystem = new CPUPickingSystem();
    m_eventHandler = new CPUPickingEventHandler(m_pickingSystem.get());
}

CPUPickingSystemManager::~CPUPickingSystemManager()
{
}

bool CPUPickingSystemManager::initialize(osg::Camera* camera, osg::Group* sceneRoot)
{
    bool result = m_pickingSystem->initialize(camera, sceneRoot);
    if (result) {
        initializeIndicatorSystem();
    }
    return result;
}

void CPUPickingSystemManager::initializeIndicatorSystem()
{
    // 创建指示器管理器
    auto indicatorManager = new PickingIndicatorManager();
    if (indicatorManager->initialize()) {
        m_pickingSystem->setIndicatorManager(indicatorManager);
    }
    
    // 创建高亮系统
    auto highlightSystem = new HighlightSystem();
    if (highlightSystem->initialize()) {
        m_pickingSystem->setHighlightSystem(highlightSystem);
    }
}

void CPUPickingSystemManager::setConfig(const CPUPickingConfig& config)
{
    m_pickingSystem->setConfig(config);
}

void CPUPickingSystemManager::addGeometry(Geo3D* geometry)
{
    m_pickingSystem->addGeometry(geometry);
}

void CPUPickingSystemManager::removeGeometry(Geo3D* geometry)
{
    m_pickingSystem->removeGeometry(geometry);
}

void CPUPickingSystemManager::updateGeometry(Geo3D* geometry)
{
    m_pickingSystem->updateGeometry(geometry);
}

CPUPickingResult CPUPickingSystemManager::pick(int mouseX, int mouseY)
{
    return m_pickingSystem->pick(mouseX, mouseY);
} 