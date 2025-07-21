#include "OSGIndexPickingSystem.h"
#include "../GeometryBase.h"
#include "../../util/LogManager.h"
#include <osg/Timer>
#include <osg/ShapeDrawable>
#include <osg/Shape>
#include <osg/PolygonOffset>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/CullFace>
#include <osg/Point>
#include <osg/PrimitiveSet>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/Billboard>
#include <osgViewer/Viewer>
#include <osgGA/GUIEventAdapter>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/PolytopeIntersector>
#include <osgUtil/PlaneIntersector>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// OSGIndexPickingSystem Implementation
// ============================================================================

OSGIndexPickingSystem::OSGIndexPickingSystem()
{
    // OSGIndexPickingSystem创建完成（移除调试日志）
}

OSGIndexPickingSystem::~OSGIndexPickingSystem()
{
    shutdown();
}

bool OSGIndexPickingSystem::initialize(osg::Camera* camera, osg::Group* sceneRoot)
{
    if (!camera || !sceneRoot) {
        LOG_ERROR("初始化参数无效", "拾取");
        return false;
    }
    
    m_camera = camera;
    m_sceneRoot = sceneRoot;
    
    // 初始化指示器管理器
    if (!GlobalPickingIndicatorManager::getInstance().initialize(camera)) {
        LOG_ERROR("指示器管理器初始化失败", "拾取");
        return false;
    }
    
    m_initialized = true;
    
    LOG_SUCCESS("OSGIndexPickingSystem初始化成功", "拾取");
    return true;
}

void OSGIndexPickingSystem::shutdown()
{
    if (!m_initialized) return;
    
    clearAllGeometries();
    
    // 关闭指示器管理器
    GlobalPickingIndicatorManager::getInstance().shutdown();
    
    m_camera = nullptr;
    m_sceneRoot = nullptr;
    
    m_initialized = false;
    
    LOG_INFO("OSGIndexPickingSystem已关闭", "拾取");
}

void OSGIndexPickingSystem::addGeometry(Geo3D* geometry)
{
    if (!geometry || !m_initialized) return;
    
    // 检查是否已存在
    auto it = std::find_if(m_geometries.begin(), m_geometries.end(),
        [geometry](const osg::ref_ptr<Geo3D>& ref) { return ref.get() == geometry; });
    
    if (it != m_geometries.end()) {
        // 几何体已存在于拾取系统中（移除调试日志）
        return;
    }
    
    m_geometries.push_back(geometry);
    
    // 预计算捕捉点
    std::vector<glm::vec3> snapPoints = extractSnapPoints(geometry);
    m_snapPointsCache[geometry] = snapPoints;
    
    // 标记场景发生变化
    markSceneChanged();
    
    // 添加几何体到OSG索引拾取系统（移除调试日志）
}

void OSGIndexPickingSystem::removeGeometry(Geo3D* geometry)
{
    if (!geometry) return;
    
    auto it = std::find_if(m_geometries.begin(), m_geometries.end(),
        [geometry](const osg::ref_ptr<Geo3D>& ref) { return ref.get() == geometry; });
    
    if (it != m_geometries.end()) {
        m_geometries.erase(it);
        m_snapPointsCache.erase(geometry);
        
        // 标记场景发生变化
        markSceneChanged();
        
        // 从OSG索引拾取系统移除几何体（移除调试日志）
    }
}

void OSGIndexPickingSystem::updateGeometry(Geo3D* geometry)
{
    if (!geometry) return;
    
    // 检查几何体是否已经在拾取系统中
    auto it = std::find_if(m_geometries.begin(), m_geometries.end(),
        [geometry](const osg::ref_ptr<Geo3D>& ref) { return ref.get() == geometry; });
    
    if (it == m_geometries.end()) {
        // 如果几何体不在拾取系统中，先添加它
        addGeometry(geometry);
        // 几何体不在拾取系统中，已添加（移除调试日志）
    } else {
        // 如果几何体已经在拾取系统中，更新捕捉点缓存
        std::vector<glm::vec3> snapPoints = extractSnapPoints(geometry);
        m_snapPointsCache[geometry] = snapPoints;
        
        // 标记场景发生变化
        markSceneChanged();
        
        // 更新几何体（移除调试日志）
    }
}

void OSGIndexPickingSystem::clearAllGeometries()
{
    m_geometries.clear();
    m_snapPointsCache.clear();
    m_lastResult = OSGIndexPickResult();
    
    // 标记场景发生变化
    markSceneChanged();
    
    // 清除所有几何体（移除调试日志）
}

OSGIndexPickResult OSGIndexPickingSystem::pick(int mouseX, int mouseY)
{
    if (!m_initialized) {
        LOG_ERROR("拾取系统未初始化", "拾取");
        return OSGIndexPickResult();
    }
    
    // 增强缓存机制：检查缓存是否有效
    if (isCacheValid(mouseX, mouseY)) {
        // 缓存有效，直接返回上次结果
        return m_lastResult;
    }
    
    osg::Timer_t startTime = osg::Timer::instance()->tick();
    
    // 执行OSG索引拾取
    OSGIndexPickResult result = performOSGIndexPicking(mouseX, mouseY);
    
    // 计算捕捉
    if (result.hasResult && m_config.enableSnapping) {
        result = calculateSnapping(result);
    }
    
    // 更新指示器 - 委托给指示器管理器
    if (result.hasResult) {
        if (m_config.enableIndicator) {
            GlobalPickingIndicatorManager::getInstance().showIndicator(
                result.indicatorPosition, result.featureType);
        }
    } else {
        GlobalPickingIndicatorManager::getInstance().hideIndicator();
    }
    
    // 调用回调
    if (m_pickingCallback) {
        m_pickingCallback(result);
    }
    
    // 更新缓存状态
    m_lastResult = result;
    m_lastPickTime = osg::Timer::instance()->time_s();
    m_lastMouseX = mouseX;
    m_lastMouseY = mouseY;
    m_lastSceneVersion = m_sceneVersionNumber;
    updateCameraState();
    
    osg::Timer_t endTime = osg::Timer::instance()->tick();
    double elapsedTime = osg::Timer::instance()->delta_s(startTime, endTime);
    
    if (m_debugMode) {
        // OSG索引拾取耗时（移除调试日志）
    }
    
    return result;
}

OSGIndexPickResult OSGIndexPickingSystem::performOSGIndexPicking(int mouseX, int mouseY)
{
    OSGIndexPickResult result;
    
    if (!m_camera || !m_sceneRoot) return result;
    
    // 按优先级顺序进行拾取：顶点 -> 边 -> 面
    if (m_config.pickVertexFirst) {
        result = pickVertex(mouseX, mouseY);
        if (result.hasResult) {
            // 拾取到顶点（移除调试日志）
            return result;
        }
    }
    
    if (m_config.pickEdgeSecond) {
        result = pickEdge(mouseX, mouseY);
        if (result.hasResult) {
            // 拾取到边（移除调试日志）
            return result;
        }
    }
    
    if (m_config.pickFaceLast) {
        result = pickFace(mouseX, mouseY);
        if (result.hasResult) {
            // 拾取到面（移除调试日志）
            return result;
        }
    }
    
    return result;
}

OSGIndexPickResult OSGIndexPickingSystem::pickVertex(int mouseX, int mouseY)
{
    OSGIndexPickResult result;
    
    if (!m_camera || !m_sceneRoot) return result;
    
    // 创建射线
    osg::Viewport* viewport = m_camera->getViewport();
    if (!viewport) return result;
    
    // 修复问题1：鼠标Y坐标翻转
    int winX = mouseX;
    int winY = viewport->height() - mouseY;
    
    // 使用OSG的射线拾取器
    osg::ref_ptr<osgUtil::LineSegmentIntersector> picker = 
        new osgUtil::LineSegmentIntersector(osgUtil::Intersector::WINDOW, winX, winY);
    
    // 修复问题2：使用Camera驱动IntersectionVisitor，获取正确的MVP矩阵
    osgUtil::IntersectionVisitor iv(picker.get());
    m_camera->accept(iv);
    
    // 处理拾取结果，但只保留geoNode下的物体
    osgUtil::LineSegmentIntersector::Intersections intersections = picker->getIntersections();
    
    // 过滤出属于geoNode的最近交点
    const osgUtil::LineSegmentIntersector::Intersection* validIntersection = nullptr;
    for (const auto& intersection : intersections) {
        // 检查nodePath是否包含m_sceneRoot
        bool belongsToGeoNode = false;
        for (const auto& node : intersection.nodePath) {
            if (node == m_sceneRoot.get()) {
                belongsToGeoNode = true;
                break;
            }
        }
        
        if (belongsToGeoNode) {
            validIntersection = &intersection;
            break;  // 取第一个（最近的）属于geoNode的交点
        }
    }
    
    if (validIntersection) {
        // 获取最近的交点
        const osgUtil::LineSegmentIntersector::Intersection& intersection = *validIntersection;
        
        // 修复问题3：改进几何体匹配逻辑
        Geo3D* pickedGeometry = findGeometryFromIntersection(intersection);
        
        if (pickedGeometry) {
            // 检查是否为顶点
            const auto& controlPoints = pickedGeometry->mm_controlPoint()->getControlPoints();
            glm::vec3 intersectPoint(intersection.getWorldIntersectPoint().x(),
                                   intersection.getWorldIntersectPoint().y(),
                                   intersection.getWorldIntersectPoint().z());
            
            // 查找最近的顶点
            float minDistance = FLT_MAX;
            int closestVertexIndex = -1;
            
            for (size_t i = 0; i < controlPoints.size(); ++i) {
                glm::vec3 vertexPos(controlPoints[i].x(), controlPoints[i].y(), controlPoints[i].z());
                float distance = glm::distance(intersectPoint, vertexPos);
                
                if (distance < m_config.snapThreshold && distance < minDistance) {
                    minDistance = distance;
                    closestVertexIndex = static_cast<int>(i);
                }
            }
            
            if (closestVertexIndex >= 0) {
                result.hasResult = true;
                result.geometry = pickedGeometry;
                result.featureType = PickFeatureType::VERTEX;
                result.vertexIndex = closestVertexIndex;
                result.worldPosition = glm::vec3(controlPoints[closestVertexIndex].x(),
                                               controlPoints[closestVertexIndex].y(),
                                               controlPoints[closestVertexIndex].z());
                result.distance = minDistance;
                result.screenX = mouseX;
                result.screenY = mouseY;
                result.indicatorPosition = result.worldPosition;
                result.indicatorSize = m_config.indicatorSize;
            }
        }
    }
    
    return result;
}

OSGIndexPickResult OSGIndexPickingSystem::pickEdge(int mouseX, int mouseY)
{
    OSGIndexPickResult result;
    
    if (!m_camera || !m_sceneRoot) return result;
    
    // 创建射线
    osg::Viewport* viewport = m_camera->getViewport();
    if (!viewport) return result;
    
    // 修复问题1：鼠标Y坐标翻转
    int winX = mouseX;
    int winY = viewport->height() - mouseY;
    
    // 使用OSG的射线拾取器
    osg::ref_ptr<osgUtil::LineSegmentIntersector> picker = 
        new osgUtil::LineSegmentIntersector(osgUtil::Intersector::WINDOW, winX, winY);
    
    // 修复问题2：使用Camera驱动IntersectionVisitor，获取正确的MVP矩阵
    osgUtil::IntersectionVisitor iv(picker.get());
    m_camera->accept(iv);
    
    // 处理拾取结果，但只保留geoNode下的物体
    osgUtil::LineSegmentIntersector::Intersections intersections = picker->getIntersections();
    
    // 过滤出属于geoNode的最近交点
    const osgUtil::LineSegmentIntersector::Intersection* validIntersection = nullptr;
    for (const auto& intersection : intersections) {
        // 检查nodePath是否包含m_sceneRoot
        bool belongsToGeoNode = false;
        for (const auto& node : intersection.nodePath) {
            if (node == m_sceneRoot.get()) {
                belongsToGeoNode = true;
                break;
            }
        }
        
        if (belongsToGeoNode) {
            validIntersection = &intersection;
            break;  // 取第一个（最近的）属于geoNode的交点
        }
    }
    
    if (validIntersection) {
        // 获取最近的交点
        const osgUtil::LineSegmentIntersector::Intersection& intersection = *validIntersection;
        
        // 修复问题3：改进几何体匹配逻辑
        Geo3D* pickedGeometry = findGeometryFromIntersection(intersection);
        
        if (pickedGeometry) {
            const auto& controlPoints = pickedGeometry->mm_controlPoint()->getControlPoints();
            glm::vec3 intersectPoint(intersection.getWorldIntersectPoint().x(),
                                   intersection.getWorldIntersectPoint().y(),
                                   intersection.getWorldIntersectPoint().z());
            
            // 查找最近的边
            float minDistance = FLT_MAX;
            int closestEdgeIndex = -1;
            
            for (size_t i = 0; i < controlPoints.size() - 1; ++i) {
                glm::vec3 p1(controlPoints[i].x(), controlPoints[i].y(), controlPoints[i].z());
                glm::vec3 p2(controlPoints[i + 1].x(), controlPoints[i + 1].y(), controlPoints[i + 1].z());
                
                // 计算点到线段的距离
                glm::vec3 edge = p2 - p1;
                glm::vec3 toPoint = intersectPoint - p1;
                
                float edgeLength = glm::length(edge);
                if (edgeLength > 0.0f) {
                    float t = glm::dot(toPoint, edge) / (edgeLength * edgeLength);
                    t = std::max(0.0f, std::min(1.0f, t));
                    
                    glm::vec3 closestPoint = p1 + t * edge;
                    float distance = glm::distance(intersectPoint, closestPoint);
                    
                    if (distance < m_config.snapThreshold && distance < minDistance) {
                        minDistance = distance;
                        closestEdgeIndex = static_cast<int>(i);
                    }
                }
            }
            
            if (closestEdgeIndex >= 0) {
                result.hasResult = true;
                result.geometry = pickedGeometry;
                result.featureType = PickFeatureType::EDGE;
                result.edgeIndex = closestEdgeIndex;
                
                // 计算边上的最近点
                glm::vec3 p1(controlPoints[closestEdgeIndex].x(), controlPoints[closestEdgeIndex].y(), controlPoints[closestEdgeIndex].z());
                glm::vec3 p2(controlPoints[closestEdgeIndex + 1].x(), controlPoints[closestEdgeIndex + 1].y(), controlPoints[closestEdgeIndex + 1].z());
                glm::vec3 edge = p2 - p1;
                glm::vec3 toPoint = intersectPoint - p1;
                
                float edgeLength = glm::length(edge);
                float t = glm::dot(toPoint, edge) / (edgeLength * edgeLength);
                t = std::max(0.0f, std::min(1.0f, t));
                
                result.worldPosition = p1 + t * edge;
                result.distance = minDistance;
                result.screenX = mouseX;
                result.screenY = mouseY;
                result.indicatorPosition = result.worldPosition;
                result.indicatorSize = m_config.indicatorSize;
            }
        }
    }
    
    return result;
}

OSGIndexPickResult OSGIndexPickingSystem::pickFace(int mouseX, int mouseY)
{
    OSGIndexPickResult result;
    
    if (!m_camera || !m_sceneRoot) return result;
    
    // 创建射线
    osg::Viewport* viewport = m_camera->getViewport();
    if (!viewport) return result;
    
    // 修复问题1：鼠标Y坐标翻转
    int winX = mouseX;
    int winY = viewport->height() - mouseY;
    
    // 使用OSG的射线拾取器
    osg::ref_ptr<osgUtil::LineSegmentIntersector> picker = 
        new osgUtil::LineSegmentIntersector(osgUtil::Intersector::WINDOW, winX, winY);
    
    // 修复问题2：使用Camera驱动IntersectionVisitor，获取正确的MVP矩阵
    osgUtil::IntersectionVisitor iv(picker.get());
    m_camera->accept(iv);
    
    // 处理拾取结果，但只保留geoNode下的物体
    osgUtil::LineSegmentIntersector::Intersections intersections = picker->getIntersections();
    
    // 过滤出属于geoNode的最近交点
    const osgUtil::LineSegmentIntersector::Intersection* validIntersection = nullptr;
    for (const auto& intersection : intersections) {
        // 检查nodePath是否包含m_sceneRoot
        bool belongsToGeoNode = false;
        for (const auto& node : intersection.nodePath) {
            if (node == m_sceneRoot.get()) {
                belongsToGeoNode = true;
                break;
            }
        }
        
        if (belongsToGeoNode) {
            validIntersection = &intersection;
            break;  // 取第一个（最近的）属于geoNode的交点
        }
    }
    
    if (validIntersection) {
        // 获取最近的交点
        const osgUtil::LineSegmentIntersector::Intersection& intersection = *validIntersection;
        
        // 修复问题3：改进几何体匹配逻辑
        Geo3D* pickedGeometry = findGeometryFromIntersection(intersection);
        
        if (pickedGeometry) {
            result.hasResult = true;
            result.geometry = pickedGeometry;
            result.featureType = PickFeatureType::FACE;
            result.worldPosition = glm::vec3(intersection.getWorldIntersectPoint().x(),
                                           intersection.getWorldIntersectPoint().y(),
                                           intersection.getWorldIntersectPoint().z());
            result.surfaceNormal = glm::vec3(intersection.getWorldIntersectNormal().x(),
                                           intersection.getWorldIntersectNormal().y(),
                                           intersection.getWorldIntersectNormal().z());
            result.distance = intersection.ratio;
            result.screenX = mouseX;
            result.screenY = mouseY;
            result.indicatorPosition = result.worldPosition;
            result.indicatorSize = m_config.indicatorSize;
        }
    }
    
    return result;
}

OSGIndexPickResult OSGIndexPickingSystem::calculateSnapping(const OSGIndexPickResult& result)
{
    OSGIndexPickResult snappedResult = result;
    
    if (!result.hasResult || !m_config.enableSnapping) {
        return snappedResult;
    }
    
    float bestDistance = FLT_MAX;
    glm::vec3 bestSnapPoint;
    bool foundSnap = false;
    
    // 搜索所有几何体的捕捉点
    for (const auto& pair : m_snapPointsCache) {
        const std::vector<glm::vec3>& snapPoints = pair.second;
        
        for (const glm::vec3& snapPoint : snapPoints) {
            float distance = glm::distance(result.worldPosition, snapPoint);
            if (distance < m_config.snapThreshold && distance < bestDistance) {
                bestDistance = distance;
                bestSnapPoint = snapPoint;
                foundSnap = true;
            }
        }
    }
    
    if (foundSnap) {
        snappedResult.isSnapped = true;
        snappedResult.snapPosition = bestSnapPoint;
        snappedResult.worldPosition = bestSnapPoint;
        snappedResult.featureType = PickFeatureType::VERTEX;
        
        // 更新屏幕坐标
        glm::vec2 screenPos = worldToScreen(bestSnapPoint);
        snappedResult.screenX = static_cast<int>(screenPos.x);
        snappedResult.screenY = static_cast<int>(screenPos.y);
        
        if (m_debugMode) {
            // 捕捉到点（移除调试日志）
        }
    }
    
    return snappedResult;
}

std::vector<glm::vec3> OSGIndexPickingSystem::extractSnapPoints(Geo3D* geometry)
{
    std::vector<glm::vec3> snapPoints;
    
    if (!geometry) return snapPoints;
    
    // 获取几何体的控制点
    const auto& controlPoints = geometry->mm_controlPoint()->getControlPoints();
    for (const auto& point : controlPoints) {
        // 将Point3D转换为glm::vec3
        glm::vec3 glmPoint(point.x(), point.y(), point.z());
        snapPoints.push_back(glmPoint);
    }
    
    // 如果是线段，添加中点
    if (controlPoints.size() >= 2) {
        for (size_t i = 0; i < controlPoints.size() - 1; ++i) {
            // 将Point3D转换为glm::vec3进行计算
            glm::vec3 p1(controlPoints[i].x(), controlPoints[i].y(), controlPoints[i].z());
            glm::vec3 p2(controlPoints[i + 1].x(), controlPoints[i + 1].y(), controlPoints[i + 1].z());
            glm::vec3 midPoint = (p1 + p2) * 0.5f;
            snapPoints.push_back(midPoint);
        }
    }
    
    return snapPoints;
}

// ============================================================================
// OSGIndexPickingSystem 委托方法 - 委托给指示器管理器
// ============================================================================

osg::Group* OSGIndexPickingSystem::getIndicatorRoot() const
{
    return GlobalPickingIndicatorManager::getInstance().getIndicatorRoot();
}

void OSGIndexPickingSystem::showHighlight(Geo3D* geometry)
{
    GlobalPickingIndicatorManager::getInstance().showHighlight(geometry);
}

void OSGIndexPickingSystem::hideHighlight()
{
    GlobalPickingIndicatorManager::getInstance().hideHighlight();
}

void OSGIndexPickingSystem::showSelectionHighlight(Geo3D* geometry)
{
    GlobalPickingIndicatorManager::getInstance().showSelectionHighlight(geometry);
}

void OSGIndexPickingSystem::hideSelectionHighlight()
{
    GlobalPickingIndicatorManager::getInstance().hideSelectionHighlight();
}





void OSGIndexPickingSystem::setPickingCallback(std::function<void(const OSGIndexPickResult&)> callback)
{
    m_pickingCallback = callback;
}

// ============================================================================
// OSGIndexPickingSystem 坐标转换辅助方法
// ============================================================================

glm::vec2 OSGIndexPickingSystem::worldToScreen(const glm::vec3& worldPos)
{
    if (!m_camera) return glm::vec2(0.0f, 0.0f);
    
    // 获取相机矩阵
    osg::Matrix viewMatrix = m_camera->getViewMatrix();
    osg::Matrix projectionMatrix = m_camera->getProjectionMatrix();
    osg::Matrix viewportMatrix = m_camera->getViewport()->computeWindowMatrix();
    
    // 组合矩阵
    osg::Matrix modelViewProjectionMatrix = viewMatrix * projectionMatrix * viewportMatrix;
    
    // 转换世界坐标到屏幕坐标
    osg::Vec3 screenPos = osg::Vec3(worldPos.x, worldPos.y, worldPos.z) * modelViewProjectionMatrix;
    
    return glm::vec2(screenPos.x(), screenPos.y());
}

glm::vec3 OSGIndexPickingSystem::screenToWorld(int screenX, int screenY, float depth)
{
    if (!m_camera) return glm::vec3(0.0f, 0.0f, 0.0f);
    
    // 获取相机矩阵
    osg::Matrix viewMatrix = m_camera->getViewMatrix();
    osg::Matrix projectionMatrix = m_camera->getProjectionMatrix();
    osg::Matrix viewportMatrix = m_camera->getViewport()->computeWindowMatrix();
    
    // 计算逆矩阵
    osg::Matrix inverseMatrix = osg::Matrix::inverse(viewMatrix * projectionMatrix * viewportMatrix);
    
    // 转换屏幕坐标到世界坐标
    osg::Vec3 worldPos = osg::Vec3(screenX, screenY, depth) * inverseMatrix;
    
    return glm::vec3(worldPos.x(), worldPos.y(), worldPos.z());
}

// 新增：改进的几何体匹配方法
Geo3D* OSGIndexPickingSystem::findGeometryFromIntersection(const osgUtil::LineSegmentIntersector::Intersection& intersection)
{
    // 方法1：直接通过Drawable匹配
    osg::Drawable* drawable = intersection.drawable.get();
    if (drawable) {
        for (const osg::ref_ptr<Geo3D>& geo : m_geometries) {
            if (!geo) continue;
            
            // 检查是否匹配面几何体
            if (geo->mm_node()->getFaceGeometry() == drawable) {
                return geo.get();
            }
            // 检查是否匹配边几何体
            if (geo->mm_node()->getEdgeGeometry() == drawable) {
                return geo.get();
            }
            // 检查是否匹配顶点几何体
            if (geo->mm_node()->getVertexGeometry() == drawable) {
                return geo.get();
            }
        }
    }
    
    // 方法2：通过节点路径匹配
    for (const osg::ref_ptr<Geo3D>& geo : m_geometries) {
        if (!geo) continue;
        
        osg::ref_ptr<osg::Group> geoNode = geo->mm_node()->getOSGNode();
        if (!geoNode) continue;
        
        // 检查节点路径中是否包含几何体的节点
        for (const auto& node : intersection.nodePath) {
            if (node == geoNode || geoNode->containsNode(node)) {
                return geo.get();
            }
        }
    }
    
    // 方法3：通过节点名称匹配（备用方案）
    for (const osg::ref_ptr<Geo3D>& geo : m_geometries) {
        if (!geo) continue;
        
        osg::ref_ptr<osg::Group> geoNode = geo->mm_node()->getOSGNode();
        if (!geoNode) continue;
        
        // 检查节点名称是否匹配
        std::string geoNodeName = geoNode->getName();
        for (const auto& node : intersection.nodePath) {
            if (node->getName() == geoNodeName) {
                return geo.get();
            }
        }
    }
    
    return nullptr;
}

// ============================================================================
// OSGIndexPickingEventHandler Implementation
// ============================================================================

OSGIndexPickingEventHandler::OSGIndexPickingEventHandler(OSGIndexPickingSystem* pickingSystem)
    : m_pickingSystem(pickingSystem)
{
    if (!m_pickingSystem) {
        LOG_ERROR("事件处理器初始化失败 - 拾取系统为空", "拾取");
    }
}

OSGIndexPickingEventHandler::~OSGIndexPickingEventHandler()
{
    m_pickingSystem = nullptr;
}

bool OSGIndexPickingEventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    if (!m_enabled || !m_pickingSystem) return false;
    
    switch (ea.getEventType()) {
        case osgGA::GUIEventAdapter::MOVE:
        case osgGA::GUIEventAdapter::DRAG:
            processPicking(static_cast<int>(ea.getX()), static_cast<int>(ea.getY()));
            break;
            
        case osgGA::GUIEventAdapter::PUSH:
            if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON) {
                processPicking(static_cast<int>(ea.getX()), static_cast<int>(ea.getY()));
            }
            break;
            
        default:
            break;
    }
    
    return false; // 继续处理其他事件
}

void OSGIndexPickingEventHandler::processPicking(int x, int y)
{
    if (!m_pickingSystem || !m_enabled) return;
    
    // 频率限制
    double currentTime = osg::Timer::instance()->time_s();
    if (currentTime - m_lastPickTime < 1.0 / 60.0) { // 60Hz限制
        return;
    }
    
    // 检查鼠标位置是否发生变化
    if (x == m_lastX && y == m_lastY) {
        return;
    }
    
    m_lastX = x;
    m_lastY = y;
    m_lastPickTime = currentTime;
    
    // 执行拾取
    OSGIndexPickResult result = m_pickingSystem->pick(x, y);
    
    // 调用回调
    if (m_pickingCallback) {
        m_pickingCallback(result);
    }
}

void OSGIndexPickingEventHandler::setPickingCallback(std::function<void(const OSGIndexPickResult&)> callback)
{
    m_pickingCallback = callback;
}

// ============================================================================
// OSGIndexPickingSystemManager Implementation
// ============================================================================

OSGIndexPickingSystemManager::OSGIndexPickingSystemManager()
{
    m_pickingSystem = new OSGIndexPickingSystem();
    m_eventHandler = new OSGIndexPickingEventHandler(m_pickingSystem.get());
}

OSGIndexPickingSystemManager& OSGIndexPickingSystemManager::getInstance()
{
    static OSGIndexPickingSystemManager instance;
    return instance;
}

bool OSGIndexPickingSystemManager::initialize(osg::Camera* camera, osg::Group* sceneRoot)
{
    if (!m_pickingSystem) {
        LOG_ERROR("拾取系统管理器初始化失败 - 拾取系统为空", "拾取");
        return false;
    }
    
    bool success = m_pickingSystem->initialize(camera, sceneRoot);
    if (success) {
        LOG_SUCCESS("OSG索引拾取系统管理器初始化成功", "拾取");
    } else {
        LOG_ERROR("OSG索引拾取系统管理器初始化失败", "拾取");
    }
    
    return success;
}

void OSGIndexPickingSystemManager::shutdown()
{
    if (m_pickingSystem) {
        m_pickingSystem->shutdown();
    }
    
    m_eventHandler = nullptr;
    m_pickingSystem = nullptr;
    
    LOG_INFO("OSG索引拾取系统管理器已关闭", "拾取");
}

void OSGIndexPickingSystemManager::setConfig(const OSGIndexPickConfig& config)
{
    if (m_pickingSystem) {
        m_pickingSystem->setConfig(config);
    }
}

const OSGIndexPickConfig& OSGIndexPickingSystemManager::getConfig() const
{
    if (m_pickingSystem) {
        return m_pickingSystem->getConfig();
    }
    
    static OSGIndexPickConfig defaultConfig;
    return defaultConfig;
}

void OSGIndexPickingSystemManager::addGeometry(Geo3D* geometry)
{
    if (m_pickingSystem) {
        m_pickingSystem->addGeometry(geometry);
    }
}

void OSGIndexPickingSystemManager::removeGeometry(Geo3D* geometry)
{
    if (m_pickingSystem) {
        m_pickingSystem->removeGeometry(geometry);
    }
}

void OSGIndexPickingSystemManager::updateGeometry(Geo3D* geometry)
{
    if (m_pickingSystem) {
        m_pickingSystem->updateGeometry(geometry);
    }
}

void OSGIndexPickingSystemManager::clearAllGeometries()
{
    if (m_pickingSystem) {
        m_pickingSystem->clearAllGeometries();
    }
}

OSGIndexPickResult OSGIndexPickingSystemManager::pick(int mouseX, int mouseY)
{
    if (m_pickingSystem) {
        return m_pickingSystem->pick(mouseX, mouseY);
    }
    
    return OSGIndexPickResult();
}

void OSGIndexPickingSystemManager::showSelectionHighlight(Geo3D* geometry)
{
    if (m_pickingSystem) {
        m_pickingSystem->showSelectionHighlight(geometry);
    }
}

void OSGIndexPickingSystemManager::hideSelectionHighlight()
{
    if (m_pickingSystem) {
        m_pickingSystem->hideSelectionHighlight();
    }
}

void OSGIndexPickingSystemManager::setPickingCallback(std::function<void(const OSGIndexPickResult&)> callback)
{
    if (m_pickingSystem) {
        m_pickingSystem->setPickingCallback(callback);
    }
    
    if (m_eventHandler) {
        m_eventHandler->setPickingCallback(callback);
    }
}

bool OSGIndexPickingSystemManager::isInitialized() const
{
    return m_pickingSystem && m_pickingSystem->isInitialized();
}

QString OSGIndexPickingSystemManager::getSystemInfo() const
{
    if (!m_pickingSystem) {
        return "OSG索引拾取系统未初始化";
    }
    
    QString info = QString("OSG索引拾取系统状态:\n");
    info += QString("- 初始化状态: %1\n").arg(m_pickingSystem->isInitialized() ? "已初始化" : "未初始化");
    info += QString("- 几何体数量: %1\n").arg(m_pickingSystem->getGeometryCount());
    info += QString("- 调试模式: %1\n").arg(m_pickingSystem->isDebugMode() ? "启用" : "禁用");
    
    const auto& config = m_pickingSystem->getConfig();
    info += QString("- 拾取半径: %1 像素\n").arg(config.pickingRadius);
    info += QString("- 捕捉阈值: %1\n").arg(config.snapThreshold);
    info += QString("- 指示器大小: %1\n").arg(config.indicatorSize);
    info += QString("- 拾取频率: %1 Hz\n").arg(config.pickingFrequency);
    
    return info;
}

// ============================================================================
// OSGIndexPickingSystem Cache Helper Methods
// ============================================================================

bool OSGIndexPickingSystem::isCacheValid(int mouseX, int mouseY)
{
    if (!m_initialized || !m_camera) {
        return false;
    }
    
    // 检查频率限制
    double currentTime = osg::Timer::instance()->time_s();
    if (currentTime - m_lastPickTime < 1.0 / m_config.pickingFrequency) {
        return true; // 频率限制内，使用缓存
    }
    
    // 检查鼠标位置是否发生变化
    if (mouseX != m_lastMouseX || mouseY != m_lastMouseY) {
        return false; // 鼠标位置变化，缓存失效
    }
    
    // 检查场景内容是否发生变化
    if (m_sceneVersionNumber != m_lastSceneVersion) {
        return false; // 场景内容变化，缓存失效
    }
    
    // 检查相机状态是否发生变化
    if (!m_cameraStateValid) {
        return false; // 相机状态无效，缓存失效
    }
    
    // 获取当前相机状态
    osg::Matrix currentViewMatrix = m_camera->getViewMatrix();
    osg::Matrix currentProjectionMatrix = m_camera->getProjectionMatrix();
    osg::Vec4 currentViewport(0, 0, 0, 0);
    if (m_camera->getViewport()) {
        currentViewport = osg::Vec4(
            m_camera->getViewport()->x(),
            m_camera->getViewport()->y(),
            m_camera->getViewport()->width(),
            m_camera->getViewport()->height()
        );
    }
    
    // 比较相机矩阵和视口
    const double MATRIX_EPSILON = 1e-6;
    
    // 检查视图矩阵
    for (int i = 0; i < 16; ++i) {
        if (std::abs(currentViewMatrix.ptr()[i] - m_lastViewMatrix.ptr()[i]) > MATRIX_EPSILON) {
            return false; // 视图矩阵变化，缓存失效
        }
    }
    
    // 检查投影矩阵
    for (int i = 0; i < 16; ++i) {
        if (std::abs(currentProjectionMatrix.ptr()[i] - m_lastProjectionMatrix.ptr()[i]) > MATRIX_EPSILON) {
            return false; // 投影矩阵变化，缓存失效
        }
    }
    
    // 检查视口
    if (std::abs(currentViewport.x() - m_lastViewport.x()) > 1.0 ||
        std::abs(currentViewport.y() - m_lastViewport.y()) > 1.0 ||
        std::abs(currentViewport.z() - m_lastViewport.z()) > 1.0 ||
        std::abs(currentViewport.w() - m_lastViewport.w()) > 1.0) {
        return false; // 视口变化，缓存失效
    }
    
    // 所有检查通过，缓存有效
    return true;
}

void OSGIndexPickingSystem::updateCameraState()
{
    if (!m_camera) {
        m_cameraStateValid = false;
        return;
    }
    
    // 保存当前相机状态
    m_lastViewMatrix = m_camera->getViewMatrix();
    m_lastProjectionMatrix = m_camera->getProjectionMatrix();
    
    if (m_camera->getViewport()) {
        m_lastViewport = osg::Vec4(
            m_camera->getViewport()->x(),
            m_camera->getViewport()->y(),
            m_camera->getViewport()->width(),
            m_camera->getViewport()->height()
        );
    } else {
        m_lastViewport = osg::Vec4(0, 0, 0, 0);
    }
    
    m_cameraStateValid = true;
}

void OSGIndexPickingSystem::invalidateCache()
{
    m_cameraStateValid = false;
    m_lastMouseX = -1;
    m_lastMouseY = -1;
    m_lastPickTime = 0.0;
    m_lastResult = OSGIndexPickResult();
}

void OSGIndexPickingSystem::markSceneChanged()
{
    m_sceneVersionNumber++;
    // 场景变化会在下次 isCacheValid 检查时自动使缓存失效
}