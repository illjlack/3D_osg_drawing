#include "RayPickingSystem.h"
#include "../GeometryBase.h"
#include "../../util/LogManager.h"
#include <osg/Timer>
#include <osg/ShapeDrawable>
#include <osg/PolygonOffset>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/Point>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/LineWidth>
#include <osgViewer/Viewer>
#include <osgGA/GUIEventAdapter>
#include <osgUtil/IntersectionVisitor>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// SimplifiedPickingSystem Implementation
// ============================================================================

RayPickingSystem::RayPickingSystem()
{
    LOG_INFO("创建射线拾取系统", "拾取");
}

RayPickingSystem::~RayPickingSystem()
{
    shutdown();
}

bool RayPickingSystem::initialize(osg::Camera* camera, osg::Group* sceneRoot)
{
    if (!camera || !sceneRoot) {
        LOG_ERROR("初始化参数无效", "拾取");
        return false;
    }
    
    m_camera = camera;
    m_sceneRoot = sceneRoot;
    
    // 创建指示器根节点
    m_indicatorRoot = new osg::Group;
    m_indicatorRoot->setName("PayPickingIndicatorRoot");
    m_indicatorRoot->setNodeMask(0x80000000);
    
    // 创建当前指示器
    m_currentIndicator = new osg::MatrixTransform;
    m_currentIndicator->setName("CurrentIndicator");
    m_currentIndicator->setNodeMask(0);
    m_indicatorRoot->addChild(m_currentIndicator);
    
    // 预先创建三个指示器几何体
    createIndicators();
    
    m_initialized = true;
    
    LOG_SUCCESS("简化拾取系统初始化成功", "拾取");
    return true;
}

void RayPickingSystem::shutdown()
{
    if (!m_initialized) return;
    
    hideIndicator();
    clearAllGeometries();
    
    m_camera = nullptr;
    m_sceneRoot = nullptr;
    m_indicatorRoot = nullptr;
    m_currentIndicator = nullptr;
    
    m_vertexIndicator = nullptr;
    m_edgeIndicator = nullptr;
    m_faceIndicator = nullptr;
    
    m_initialized = false;
    
    LOG_INFO("射线拾取系统已关闭", "拾取");
}

void RayPickingSystem::addGeometry(Geo3D* geometry)
{
    if (!geometry) return;
    
    // 添加到几何体列表
    auto it = std::find(m_geometries.begin(), m_geometries.end(), geometry);
    if (it == m_geometries.end()) {
        m_geometries.push_back(geometry);
        
        // 建立节点到几何体的映射
        if (geometry->mm_node() && geometry->mm_node()->getOSGNode()) {
            osg::Node* node = geometry->mm_node()->getOSGNode().get();
            m_nodeToGeometry[node] = geometry;
            
            // 确保几何体节点可以被拾取：保持原有NodeMask，但确保最高位为0
            unsigned int currentMask = node->getNodeMask();
            if (currentMask == 0) {
                // 如果当前NodeMask为0（隐藏），设置为默认可见可拾取
                node->setNodeMask(0xFFFFFFFF);
            } else {
                // 如果已有NodeMask，确保最高位为0（可拾取）
                node->setNodeMask(currentMask & 0x7FFFFFFF);
            }
        }
        
        LOG_DEBUG(QString("添加几何体到拾取系统，当前几何体数量：%1").arg(m_geometries.size()), "拾取");
    }
}

void RayPickingSystem::removeGeometry(Geo3D* geometry)
{
    if (!geometry) return;
    
    // 从几何体列表移除
    auto it = std::find(m_geometries.begin(), m_geometries.end(), geometry);
    if (it != m_geometries.end()) {
        m_geometries.erase(it);
        
        // 从映射中移除
        if (geometry->mm_node() && geometry->mm_node()->getOSGNode()) {
            m_nodeToGeometry.erase(geometry->mm_node()->getOSGNode().get());
        }
        
        LOG_DEBUG(QString("从拾取系统移除几何体，当前几何体数量：%1").arg(m_geometries.size()), "拾取");
    }
}

void RayPickingSystem::clearAllGeometries()
{
    m_geometries.clear();
    m_nodeToGeometry.clear();
    
    LOG_DEBUG("清空拾取系统中的所有几何体", "拾取");
}

PickResult RayPickingSystem::pick(int mouseX, int mouseY)
{
    if (!m_initialized) {
        LOG_ERROR("拾取系统未初始化", "拾取");
        return PickResult();
    }
    
    if (m_geometries.empty()) {
        return PickResult();
    }
    
    // 执行射线拾取
    PickResult result = performRayPicking(mouseX, mouseY);
    
    // 计算捕捉
    if (result.hasResult && m_config.enableSnapping) {
        result = calculateSnapping(result);
    }
    
    // 更新指示器
    if (result.hasResult && m_config.enableIndicator) {
        showIndicator(result.worldPosition, result.featureType, result.surfaceNormal);
    } else {
        hideIndicator();
    }
    
    // 调用回调
    if (m_pickingCallback) {
        m_pickingCallback(result);
    }
    
    return result;
}

void RayPickingSystem::setPickingCallback(std::function<void(const PickResult&)> callback)
{
    m_pickingCallback = callback;
}

PickResult RayPickingSystem::performRayPicking(int mouseX, int mouseY)
{
    PickResult result;
    
    if (!m_camera || !m_sceneRoot) return result;
    
    // 获取视口
    osg::Viewport* viewport = m_camera->getViewport();
    if (!viewport) return result;

    int winX = mouseX;
    int winY = mouseY;
    
    // 创建射线拾取器 - 使用RayIntersector支持更精确的拾取
    osg::ref_ptr<osgUtil::RayIntersector> picker = 
        new osgUtil::RayIntersector(osgUtil::Intersector::WINDOW, winX, winY);
    
    // 设置精度和拾取模式
    picker->setPrecisionHint(osgUtil::Intersector::USE_DOUBLE_CALCULATIONS);
    
    // 创建相交访问器，设置遍历掩码排除指示器
    osgUtil::IntersectionVisitor iv(picker.get());
    // 排除指示器：不遍历NodeMask最高位为1的节点（0x80000000）
    iv.setTraversalMask(0x7FFFFFFF); // 只遍历前31位，排除最高位
    
    // 只在场景根节点中进行拾取
    m_sceneRoot->accept(iv);
    
    // 计算射线的起点和终点
    osg::Vec3 start, end;
    osg::Matrix VPW = m_camera->getViewMatrix() * 
                      m_camera->getProjectionMatrix() * 
                      m_camera->getViewport()->computeWindowMatrix();
    osg::Matrix inverseVPW;
    inverseVPW.invert(VPW);
    
    start = osg::Vec3(winX, winY, 0.0) * inverseVPW;
    end = osg::Vec3(winX, winY, 1.0) * inverseVPW;

    // 按优先级顺序进行拾取
    if (m_config.pickVertexFirst) {
        result = pickVertex(start, end, mouseX, mouseY);
        if (result.hasResult) {
            result.screenX = mouseX;
            result.screenY = mouseY;
            return result;
        }
    }
    
    if (m_config.pickEdgeSecond) {
        result = pickEdge(start, end, mouseX, mouseY);
        if (result.hasResult) {
            result.screenX = mouseX;
            result.screenY = mouseY;
            return result;
        }
    }
    
    if (m_config.pickFaceLast) {
        result = pickFace(start, end);
        if (result.hasResult) {
            result.screenX = mouseX;
            result.screenY = mouseY;
            return result;
        }
    }
    
    return result;
}

PickResult RayPickingSystem::pickVertex(const osg::Vec3& start, const osg::Vec3& end, int mouseX, int mouseY)
{
    PickResult result;
    
    // 首先使用IntersectionVisitor进行快速筛选
    osg::ref_ptr<osgUtil::RayIntersector> picker = 
        new osgUtil::RayIntersector(start, end);
    
    osgUtil::IntersectionVisitor iv(picker.get());
    // 确保排除指示器：不遍历NodeMask最高位为1的节点（0x80000000）
    iv.setTraversalMask(0x7FFFFFFF);
    m_sceneRoot->accept(iv);
    
    if (!picker->containsIntersections()) {
        return result; // 如果没有任何相交，直接返回
    }
    
    // 计算射线方向
    osg::Vec3 rayDir = end - start;
    rayDir.normalize();
    
    float minScreenDistance = FLT_MAX;
    Geo3D* closestGeometry = nullptr;
    int closestVertexIndex = -1;
    glm::vec3 closestPoint;
    
    // 获取鼠标屏幕坐标
    glm::vec2 mousePos(mouseX, mouseY);
    
    // 从相交结果中找到相关的几何体，然后精确检测顶点
    const osgUtil::RayIntersector::Intersections& intersections = picker->getIntersections();
    std::set<Geo3D*> candidateGeometries;
    
    for (const auto& intersection : intersections) {
        Geo3D* geometry = findGeometryFromNode(intersection.nodePath.back());
        if (geometry) {
            candidateGeometries.insert(geometry);
        }
    }
    
    // 对候选几何体进行精确的顶点检测
    for (Geo3D* geometry : candidateGeometries) {
        if (!geometry || !geometry->mm_controlPoint()) continue;
        
        const auto& controlPoints = geometry->mm_controlPoint()->getControlPoints();
        
        for (size_t i = 0; i < controlPoints.size(); ++i) {
            glm::vec3 vertexPos(controlPoints[i].x(), controlPoints[i].y(), controlPoints[i].z());
            
            // 转换到屏幕坐标系检查距离
            glm::vec2 screenPos = worldToScreen(vertexPos);
            float screenDistance = glm::distance(screenPos, mousePos);
            
            if (screenDistance < m_config.vertexPickRadius && screenDistance < minScreenDistance) {
                minScreenDistance = screenDistance;
                closestGeometry = geometry;
                closestVertexIndex = static_cast<int>(i);
                closestPoint = vertexPos;
            }
        }
    }
    
    if (closestGeometry) {
        result.hasResult = true;
        result.geometry = closestGeometry;
        result.featureType = PickFeatureType::VERTEX;
        result.primitiveIndex = closestVertexIndex;
        
        // 吸附功能：将世界位置精确设置为顶点位置
        result.worldPosition = closestPoint;
        result.isSnapped = true;
        result.snapPosition = closestPoint;
        
        // 计算从相机到顶点的距离
        osg::Vec3 cameraPos = m_camera->getInverseViewMatrix().getTrans();
        osg::Vec3 vertexOSG(closestPoint.x, closestPoint.y, closestPoint.z);
        result.distance = (vertexOSG - cameraPos).length();
        
        // 顶点的法向量设为从相机指向顶点的方向
        glm::vec3 viewDir = glm::normalize(closestPoint - glm::vec3(cameraPos.x(), cameraPos.y(), cameraPos.z()));
        result.surfaceNormal = viewDir;
    }
    
    return result;
}

PickResult RayPickingSystem::pickEdge(const osg::Vec3& start, const osg::Vec3& end, int mouseX, int mouseY)
{
    PickResult result;
    
    // 首先使用IntersectionVisitor进行快速筛选
    osg::ref_ptr<osgUtil::RayIntersector> picker = 
        new osgUtil::RayIntersector(start, end);
    
    osgUtil::IntersectionVisitor iv(picker.get());
    // 确保排除指示器：不遍历NodeMask最高位为1的节点（0x80000000）
    iv.setTraversalMask(0x7FFFFFFF);
    m_sceneRoot->accept(iv);
    
    if (!picker->containsIntersections()) {
        return result; // 如果没有任何相交，直接返回
    }
    
    // 计算射线方向
    osg::Vec3 rayDir = end - start;
    rayDir.normalize();
    
    float minScreenDistance = FLT_MAX;
    Geo3D* closestGeometry = nullptr;
    int closestEdgeIndex = -1;
    glm::vec3 closestPointOnEdge;
    glm::vec3 closestNormal;
    
    // 获取鼠标屏幕坐标
    glm::vec2 mousePos(mouseX, mouseY);
    
    // 从相交结果中找到相关的几何体，然后精确检测边
    const osgUtil::RayIntersector::Intersections& intersections = picker->getIntersections();
    std::set<Geo3D*> candidateGeometries;
    
    for (const auto& intersection : intersections) {
        Geo3D* geometry = findGeometryFromNode(intersection.nodePath.back());
        if (geometry) {
            candidateGeometries.insert(geometry);
        }
    }
    
    // 对候选几何体进行精确的边检测
    for (Geo3D* geometry : candidateGeometries) {
        if (!geometry || !geometry->mm_controlPoint()) continue;
        
        const auto& controlPoints = geometry->mm_controlPoint()->getControlPoints();
        
        // 对于线型几何体，相邻控制点构成边
        if (controlPoints.size() >= 2) {
            for (size_t i = 0; i < controlPoints.size() - 1; ++i) {
                glm::vec3 p1(controlPoints[i].x(), controlPoints[i].y(), controlPoints[i].z());
                glm::vec3 p2(controlPoints[i + 1].x(), controlPoints[i + 1].y(), controlPoints[i + 1].z());
                
                // 使用屏幕空间投影计算，更精确的吸附效果
                float screenDistance;
                glm::vec3 projectedPoint;
                if (calculateScreenLineProjection(mouseX, mouseY, p1, p2, screenDistance, projectedPoint)) {
                    if (screenDistance < m_config.edgePickRadius && screenDistance < minScreenDistance) {
                        minScreenDistance = screenDistance;
                        closestGeometry = geometry;
                        closestEdgeIndex = static_cast<int>(i);
                        closestPointOnEdge = projectedPoint;
                        
                        // 计算边的法向量（垂直于边的方向）
                        glm::vec3 edgeDir = glm::normalize(p2 - p1);
                        glm::vec3 viewDir = glm::normalize(glm::vec3(start.x(), start.y(), start.z()) - projectedPoint);
                        closestNormal = glm::normalize(glm::cross(edgeDir, viewDir));
                        
                        // 如果交叉积结果为零向量，使用默认法向量
                        if (glm::length(closestNormal) < 0.01f) {
                            closestNormal = glm::vec3(0.0f, 0.0f, 1.0f);
                        }
                    }
                }
            }
            
            // 对于闭合几何体，添加最后一个点到第一个点的边
            if (controlPoints.size() > 2) {
                glm::vec3 p1(controlPoints.back().x(), controlPoints.back().y(), controlPoints.back().z());
                glm::vec3 p2(controlPoints[0].x(), controlPoints[0].y(), controlPoints[0].z());
                
                float screenDistance;
                glm::vec3 projectedPoint;
                if (calculateScreenLineProjection(mouseX, mouseY, p1, p2, screenDistance, projectedPoint)) {
                    if (screenDistance < m_config.edgePickRadius && screenDistance < minScreenDistance) {
                        minScreenDistance = screenDistance;
                        closestGeometry = geometry;
                        closestEdgeIndex = static_cast<int>(controlPoints.size() - 1);
                        closestPointOnEdge = projectedPoint;
                        
                        glm::vec3 edgeDir = glm::normalize(p2 - p1);
                        glm::vec3 viewDir = glm::normalize(glm::vec3(start.x(), start.y(), start.z()) - projectedPoint);
                        closestNormal = glm::normalize(glm::cross(edgeDir, viewDir));
                        
                        // 如果交叉积结果为零向量，使用默认法向量
                        if (glm::length(closestNormal) < 0.01f) {
                            closestNormal = glm::vec3(0.0f, 0.0f, 1.0f);
                        }
                    }
                }
            }
        }
    }
    
    if (closestGeometry) {
        result.hasResult = true;
        result.geometry = closestGeometry;
        result.featureType = PickFeatureType::EDGE;
        result.primitiveIndex = closestEdgeIndex;
        
        // 吸附功能：将世界位置精确设置为线段上的垂点
        result.worldPosition = closestPointOnEdge;
        result.isSnapped = true;
        result.snapPosition = closestPointOnEdge;
        result.surfaceNormal = closestNormal;
        
        // 计算从相机到垂点的距离
        osg::Vec3 cameraPos = m_camera->getInverseViewMatrix().getTrans();
        osg::Vec3 pointOSG(closestPointOnEdge.x, closestPointOnEdge.y, closestPointOnEdge.z);
        result.distance = (pointOSG - cameraPos).length();
    }
    
    return result;
}

PickResult RayPickingSystem::pickFace(const osg::Vec3& start, const osg::Vec3& end)
{
    PickResult result;
    
    // 使用RayIntersector进行面拾取
    osg::ref_ptr<osgUtil::RayIntersector> picker = 
        new osgUtil::RayIntersector(start, end);
    
    osgUtil::IntersectionVisitor iv(picker.get());
    // 设置遍历掩码，排除指示器
    iv.setTraversalMask(0x7FFFFFFF);
    m_sceneRoot->accept(iv);
    
    if (picker->containsIntersections()) {
        const osgUtil::RayIntersector::Intersections& intersections = picker->getIntersections();
        
        for (const auto& intersection : intersections) {
            Geo3D* geometry = findGeometryFromNode(intersection.nodePath.back());
            if (geometry) {
                result.hasResult = true;
                result.geometry = geometry;
                result.featureType = PickFeatureType::FACE;
                result.worldPosition = glm::vec3(intersection.getWorldIntersectPoint().x(),
                                               intersection.getWorldIntersectPoint().y(),
                                               intersection.getWorldIntersectPoint().z());
                result.surfaceNormal = glm::vec3(intersection.getWorldIntersectNormal().x(),
                                               intersection.getWorldIntersectNormal().y(),
                                               intersection.getWorldIntersectNormal().z());
                // 计算距离：从相机位置到交点的距离
                osg::Vec3 cameraPos = m_camera->getInverseViewMatrix().getTrans();
                osg::Vec3 intersectPoint = intersection.getWorldIntersectPoint();
                result.distance = (intersectPoint - cameraPos).length();
                
                // 获取图元索引（如果可用）
                if (!intersection.indexList.empty()) {
                    result.primitiveIndex = intersection.indexList[0];
                }
                
                break;
            }
        }
    }
    
    return result;
}

PickResult RayPickingSystem::calculateSnapping(const PickResult& result)
{
    PickResult snappedResult = result;
    
    if (!result.hasResult || !m_config.enableSnapping) {
        return snappedResult;
    }
    
    float bestDistance = FLT_MAX;
    glm::vec3 bestSnapPoint;
    bool foundSnap = false;
    
    // 搜索所有几何体的捕捉点
    for (Geo3D* geometry : m_geometries) {
        std::vector<glm::vec3> snapPoints = getGeometrySnapPoints(geometry);
        
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
    }
    
    return snappedResult;
}

std::vector<glm::vec3> RayPickingSystem::getGeometrySnapPoints(Geo3D* geometry)
{
    std::vector<glm::vec3> snapPoints;
    
    if (!geometry || !geometry->mm_controlPoint()) {
        return snapPoints;
    }
    
    const auto& controlPoints = geometry->mm_controlPoint()->getControlPoints();
    for (const auto& point : controlPoints) {
        snapPoints.emplace_back(point.x(), point.y(), point.z());
    }
    
    return snapPoints;
}

Geo3D* RayPickingSystem::findGeometryFromNode(osg::Node* node)
{
    if (!node) return nullptr;
    
    // 直接查找
    auto it = m_nodeToGeometry.find(node);
    if (it != m_nodeToGeometry.end()) {
        return it->second;
    }
    
    // 向上遍历节点路径查找
    osg::Node* currentNode = node;
    while (currentNode) {
        auto it = m_nodeToGeometry.find(currentNode);
        if (it != m_nodeToGeometry.end()) {
            return it->second;
        }
        
        // 获取父节点
        if (currentNode->getNumParents() > 0) {
            currentNode = currentNode->getParent(0);
        } else {
            break;
        }
    }
    
    return nullptr;
}

glm::vec2 RayPickingSystem::worldToScreen(const glm::vec3& worldPos)
{
    if (!m_camera) return glm::vec2(0.0f, 0.0f);
    
    osg::Matrix viewMatrix = m_camera->getViewMatrix();
    osg::Matrix projectionMatrix = m_camera->getProjectionMatrix();
    osg::Matrix viewportMatrix = m_camera->getViewport()->computeWindowMatrix();
    
    osg::Matrix modelViewProjectionMatrix = viewMatrix * projectionMatrix * viewportMatrix;
    
    osg::Vec3 screenPos = osg::Vec3(worldPos.x, worldPos.y, worldPos.z) * modelViewProjectionMatrix;
    
    return glm::vec2(screenPos.x(), screenPos.y());
}

osg::Vec3 RayPickingSystem::screenToWorldRay(int screenX, int screenY, float rayLength)
{
    if (!m_camera) return osg::Vec3(0.0f, 0.0f, 1.0f);
    
    osg::Viewport* viewport = m_camera->getViewport();
    if (!viewport) return osg::Vec3(0.0f, 0.0f, 1.0f);
    
    osg::Matrix viewMatrix = m_camera->getViewMatrix();
    osg::Matrix projectionMatrix = m_camera->getProjectionMatrix();
    osg::Matrix windowMatrix = viewport->computeWindowMatrix();
    
    osg::Matrix inverseMatrix = osg::Matrix::inverse(viewMatrix * projectionMatrix * windowMatrix);
    
    osg::Vec3 nearPoint = osg::Vec3(screenX, screenY, 0.0f) * inverseMatrix;
    osg::Vec3 farPoint = osg::Vec3(screenX, screenY, 1.0f) * inverseMatrix;
    
    osg::Vec3 direction = farPoint - nearPoint;
    direction.normalize();
    
    return nearPoint + direction * rayLength;
}

float RayPickingSystem::calculateDistanceScale(const glm::vec3& position)
{
    if (!m_camera) return 1.0f;
    
    // 获取相机位置
    osg::Vec3 cameraPos = m_camera->getInverseViewMatrix().getTrans();
    osg::Vec3 targetPos(position.x, position.y, position.z);
    
    // 计算距离
    float distance = (targetPos - cameraPos).length();
    
    // 基础缩放：距离越远，指示器越大，保持屏幕大小恒定
    // 这里使用对数缩放避免过度放大
    float baseScale = std::max(0.1f, distance * 0.01f);
    
    // 限制缩放范围，避免过小或过大
    float minScale = 0.5f;  // 最小缩放
    float maxScale = 10.0f; // 最大缩放
    
    return std::clamp(baseScale, minScale, maxScale);
}

void RayPickingSystem::createIndicators()
{
    // 预先创建三个绿色细线几何图形指示器
    m_vertexIndicator = createVertexIndicator();
    m_edgeIndicator = createEdgeIndicator();
    m_faceIndicator = createFaceIndicator();
}

// 删除了createIndicatorGeometry函数，现在使用预先创建的指示器

osg::ref_ptr<osg::Geometry> RayPickingSystem::createVertexIndicator()
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 创建圆形（用多边形近似）
    const int segments = 16;
    float radius = 1.0f; // 基础大小，实际显示时会根据视距缩放
    
    for (int i = 0; i < segments; ++i) {
        float angle = (2.0f * M_PI * i) / segments;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        vertices->push_back(osg::Vec3(x, y, 0.0f));
        colors->push_back(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f)); // 绿色
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP, 0, vertices->size()));
    
    // 设置渲染状态
    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    stateSet->setRenderBinDetails(10000, "RenderBin");
    
    osg::LineWidth* lineWidth = new osg::LineWidth;
    lineWidth->setWidth(2.0f); // 细线条
    stateSet->setAttributeAndModes(lineWidth);
    
    osg::BlendFunc* blendFunc = new osg::BlendFunc;
    blendFunc->setSource(osg::BlendFunc::SRC_ALPHA);
    blendFunc->setDestination(osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    stateSet->setAttributeAndModes(blendFunc);
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> RayPickingSystem::createEdgeIndicator()
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 创建等边三角形
    float size = 1.0f; // 基础大小
    float height = size * 0.866f; // sqrt(3)/2
    
    // 等边三角形的三个顶点
    vertices->push_back(osg::Vec3(0.0f, height * 0.67f, 0.0f));        // 顶点
    vertices->push_back(osg::Vec3(-size * 0.5f, -height * 0.33f, 0.0f)); // 左下
    vertices->push_back(osg::Vec3(size * 0.5f, -height * 0.33f, 0.0f));  // 右下
    
    for (int i = 0; i < 3; ++i) {
        colors->push_back(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f)); // 绿色
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP, 0, vertices->size()));
    
    // 设置渲染状态
    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    stateSet->setRenderBinDetails(10000, "RenderBin");
    
    osg::LineWidth* lineWidth = new osg::LineWidth;
    lineWidth->setWidth(2.0f); // 细线条
    stateSet->setAttributeAndModes(lineWidth);
    
    osg::BlendFunc* blendFunc = new osg::BlendFunc;
    blendFunc->setSource(osg::BlendFunc::SRC_ALPHA);
    blendFunc->setDestination(osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    stateSet->setAttributeAndModes(blendFunc);
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> RayPickingSystem::createFaceIndicator()
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 创建正方形
    float size = 1.0f; // 基础大小
    
    vertices->push_back(osg::Vec3(-size, -size, 0.0f)); // 左下
    vertices->push_back(osg::Vec3(size, -size, 0.0f));  // 右下
    vertices->push_back(osg::Vec3(size, size, 0.0f));   // 右上
    vertices->push_back(osg::Vec3(-size, size, 0.0f));  // 左上
    
    for (int i = 0; i < 4; ++i) {
        colors->push_back(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f)); // 绿色
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP, 0, vertices->size()));
    
    // 设置渲染状态
    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    stateSet->setRenderBinDetails(10000, "RenderBin");
    
    osg::LineWidth* lineWidth = new osg::LineWidth;
    lineWidth->setWidth(2.0f); // 细线条
    stateSet->setAttributeAndModes(lineWidth);
    
    osg::BlendFunc* blendFunc = new osg::BlendFunc;
    blendFunc->setSource(osg::BlendFunc::SRC_ALPHA);
    blendFunc->setDestination(osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    stateSet->setAttributeAndModes(blendFunc);
    
    return geometry;
}

void RayPickingSystem::showIndicator(const glm::vec3& position, PickFeatureType featureType, const glm::vec3& normal)
{
    if (!m_currentIndicator || !m_config.enableIndicator) return;
    
    // 清除之前的指示器
    m_currentIndicator->removeChildren(0, m_currentIndicator->getNumChildren());
    
    // 选择预先创建的指示器几何体
    osg::ref_ptr<osg::Geometry> selectedGeometry = nullptr;
    switch (featureType) {
        case PickFeatureType::VERTEX:
            selectedGeometry = m_vertexIndicator;
            break;
        case PickFeatureType::EDGE:
            selectedGeometry = m_edgeIndicator;
            break;
        case PickFeatureType::FACE:
            selectedGeometry = m_faceIndicator;
            break;
        default:
            return;
    }
    
    if (!selectedGeometry) return;
    
    // 创建新的Geode节点来显示选中的几何体
    if (!m_currentIndicatorGeode) {
        m_currentIndicatorGeode = new osg::Geode;
        m_currentIndicatorGeode->setNodeMask(0x80000000);
    }
    
    // 清除之前的几何体
    m_currentIndicatorGeode->removeDrawables(0, m_currentIndicatorGeode->getNumDrawables());
    
    // 添加选中的几何体
    m_currentIndicatorGeode->addDrawable(selectedGeometry);
    
    // 计算变换矩阵：位置 + 法向量对齐 + 视距缩放
    osg::Matrix matrix;
    
    // 1. 视距自适应缩放
    float scale = calculateDistanceScale(position);
    matrix.makeScale(osg::Vec3(scale, scale, scale));
    
    // 2. 法向量对齐（如果法向量不是默认的(0,0,1)）
    if (normal != glm::vec3(0,0,1) && glm::length(normal) > 0.01f) {
        osg::Matrix orientationMatrix = calculateOrientationMatrix(normal);
        matrix.postMult(orientationMatrix);
    }
    
    // 3. 设置世界位置
    matrix.postMultTranslate(osg::Vec3(position.x, position.y, position.z));
    
    m_currentIndicator->setMatrix(matrix);
    m_currentIndicator->addChild(m_currentIndicatorGeode);
    m_currentIndicator->setNodeMask(0x80000000);
}

void RayPickingSystem::hideIndicator()
{
    if (m_currentIndicator) {
        // 清除所有子节点
        m_currentIndicator->removeChildren(0, m_currentIndicator->getNumChildren());
        // 设置为不可见
        m_currentIndicator->setNodeMask(0);
    }
}

// ============================================================================
// PickingEventHandler Implementation
// ============================================================================

PickingEventHandler::PickingEventHandler(RayPickingSystem* pickingSystem)
    : m_pickingSystem(pickingSystem)
{
    if (!m_pickingSystem) {
        LOG_ERROR("事件处理器初始化失败 - 拾取系统为空", "拾取");
    }
}

bool PickingEventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    if (!m_enabled || !m_pickingSystem) return false;
    
    switch (ea.getEventType()) {
        case osgGA::GUIEventAdapter::MOVE:
        case osgGA::GUIEventAdapter::DRAG:
        {
            int x = static_cast<int>(ea.getX());
            int y = static_cast<int>(ea.getY());
            
            // 检查鼠标位置是否发生变化
            if (x == m_lastX && y == m_lastY) {
                return false;
            }
            
            m_lastX = x;
            m_lastY = y;
            
            // 执行拾取
            PickResult result = m_pickingSystem->pick(x, y);
            
            // 处理吸附效果
            if (result.hasResult && result.isSnapped) {
                // 将吸附后的世界坐标转换回屏幕坐标
                glm::vec2 snapScreenPos = m_pickingSystem->worldToScreen(result.snapPosition);
                
                // 这里可以触发吸附回调，通知上层应用
                // 注意：我们不能在这里直接修改鼠标位置，但可以记录吸附信息
                m_snapScreenX = static_cast<int>(snapScreenPos.x);
                m_snapScreenY = static_cast<int>(snapScreenPos.y);
                m_hasSnapPosition = true;
                
                LOG_DEBUG(QString("拾取吸附: 鼠标(%1,%2) -> 吸附到(%3,%4)")
                    .arg(x).arg(y)
                    .arg(m_snapScreenX).arg(m_snapScreenY), "拾取");
            } else {
                m_hasSnapPosition = false;
            }
            
            break;
        }
        
        case osgGA::GUIEventAdapter::PUSH:
            if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON) {
                int x = static_cast<int>(ea.getX());
                int y = static_cast<int>(ea.getY());
                PickResult result = m_pickingSystem->pick(x, y);
                
                // 如果有吸附效果，使用吸附后的坐标
                if (result.hasResult && result.isSnapped) {
                    LOG_INFO(QString("点击拾取成功: %1 在位置(%2,%3,%4)")
                        .arg(result.featureType == PickFeatureType::VERTEX ? "顶点" : 
                             result.featureType == PickFeatureType::EDGE ? "边线" : "面")
                        .arg(result.snapPosition.x, 0, 'f', 3)
                        .arg(result.snapPosition.y, 0, 'f', 3)
                        .arg(result.snapPosition.z, 0, 'f', 3), "拾取");
                }
            }
            break;
            
        default:
            break;
    }
    
    return false; // 继续处理其他事件
}

// ============================================================================
// PickingSystemManager Implementation
// ============================================================================

PickingSystemManager& PickingSystemManager::getInstance()
{
    static PickingSystemManager instance;
    return instance;
}

bool PickingSystemManager::initialize(osg::Camera* camera, osg::Group* sceneRoot)
{
    m_pickingSystem = new RayPickingSystem();
    m_eventHandler = new PickingEventHandler(m_pickingSystem.get());
    
    if (!m_pickingSystem->initialize(camera, sceneRoot)) {
        LOG_ERROR("拾取系统管理器初始化失败", "拾取");
        return false;
    }
    
    LOG_SUCCESS("拾取系统管理器初始化成功", "拾取");
    return true;
}

void PickingSystemManager::shutdown()
{
    if (m_pickingSystem) {
        m_pickingSystem->shutdown();
    }
    
    m_eventHandler = nullptr;
    m_pickingSystem = nullptr;
    
    LOG_INFO("拾取系统管理器已关闭", "拾取");
}

void PickingSystemManager::setConfig(const PickConfig& config)
{
    if (m_pickingSystem) {
        m_pickingSystem->setConfig(config);
    }
}

const PickConfig& PickingSystemManager::getConfig() const
{
    if (m_pickingSystem) {
        return m_pickingSystem->getConfig();
    }
    
    static PickConfig defaultConfig;
    return defaultConfig;
}

void PickingSystemManager::addGeometry(Geo3D* geometry)
{
    if (m_pickingSystem) {
        m_pickingSystem->addGeometry(geometry);
    }
}

void PickingSystemManager::removeGeometry(Geo3D* geometry)
{
    if (m_pickingSystem) {
        m_pickingSystem->removeGeometry(geometry);
    }
}

void PickingSystemManager::clearAllGeometries()
{
    if (m_pickingSystem) {
        m_pickingSystem->clearAllGeometries();
    }
}

PickResult PickingSystemManager::pick(int mouseX, int mouseY)
{
    if (m_pickingSystem) {
        return m_pickingSystem->pick(mouseX, mouseY);
    }
    
    return PickResult();
}

void PickingSystemManager::setPickingCallback(std::function<void(const PickResult&)> callback)
{
    if (m_pickingSystem) {
        m_pickingSystem->setPickingCallback(callback);
    }
}

osg::Group* PickingSystemManager::getIndicatorRoot() const
{
    if (m_pickingSystem) {
        return m_pickingSystem->getIndicatorRoot();
    }
    
    return nullptr;
}

osgGA::GUIEventHandler* PickingSystemManager::getEventHandler() const
{
    return m_eventHandler.get();
}

bool PickingSystemManager::isInitialized() const
{
    return m_pickingSystem && m_pickingSystem->isInitialized();
}

osg::Matrix RayPickingSystem::calculateOrientationMatrix(const glm::vec3& normal)
{
    // 将法向量转换为OSG向量
    osg::Vec3 surfaceNormal(normal.x, normal.y, normal.z);
    surfaceNormal.normalize();
    
    // 默认的指示器朝向是Z轴正方向
    osg::Vec3 defaultDir(0.0f, 0.0f, 1.0f);
    
    // 计算旋转轴和角度
    osg::Vec3 rotationAxis = defaultDir ^ surfaceNormal;
    float rotationAngle = acos(defaultDir * surfaceNormal);
    
    osg::Matrix orientationMatrix;
    
    // 如果法向量与默认方向平行，不需要旋转
    if (rotationAxis.length() < 0.001f) {
        // 检查是否是反向
        if ((defaultDir * surfaceNormal) < 0.0f) {
            // 180度旋转
            orientationMatrix.makeRotate(osg::PI, osg::Vec3(1.0f, 0.0f, 0.0f));
        } else {
            orientationMatrix.makeIdentity();
        }
    } else {
        // 围绕旋转轴旋转
        rotationAxis.normalize();
        orientationMatrix.makeRotate(rotationAngle, rotationAxis);
    }
    
    return orientationMatrix;
}

bool RayPickingSystem::calculateRayToLineDistance(const osg::Vec3& rayStart, const osg::Vec3& rayDir, 
                                                 const glm::vec3& lineStart, const glm::vec3& lineEnd,
                                                 float& distance, glm::vec3& closestPoint)
{
    // 转换为glm向量进行计算
    glm::vec3 rayStartGLM(rayStart.x(), rayStart.y(), rayStart.z());
    glm::vec3 rayDirGLM(rayDir.x(), rayDir.y(), rayDir.z());
    
    // 线段方向向量
    glm::vec3 lineDir = lineEnd - lineStart;
    float lineLength = glm::length(lineDir);
    
    if (lineLength < 1e-6f) {
        // 线段退化为点，计算射线到点的距离
        glm::vec3 toPoint = lineStart - rayStartGLM;
        float projection = glm::dot(toPoint, rayDirGLM);
        if (projection < 0.0f) {
            // 点在射线起点之前
            distance = glm::length(toPoint);
            closestPoint = lineStart;
        } else {
            // 计算射线上最近点
            glm::vec3 rayPoint = rayStartGLM + projection * rayDirGLM;
            distance = glm::length(lineStart - rayPoint);
            closestPoint = lineStart;
        }
        return true;
    }
    
    lineDir = lineDir / lineLength; // 标准化线段方向
    
    // 计算射线和线段之间的最短距离
    glm::vec3 w0 = rayStartGLM - lineStart;
    
    float a = glm::dot(rayDirGLM, rayDirGLM);   // 射线方向长度的平方 (应该是1)
    float b = glm::dot(rayDirGLM, lineDir);     // 射线和线段方向的点积
    float c = glm::dot(lineDir, lineDir);       // 线段方向长度的平方 (应该是1)
    float d = glm::dot(rayDirGLM, w0);
    float e = glm::dot(lineDir, w0);
    
    float denominator = a * c - b * b;
    
    float sc, tc; // 射线和线段上的参数
    
    if (denominator < 1e-6f) {
        // 射线和线段平行
        sc = 0.0f;
        tc = (b > c ? d / b : e / c);
    } else {
        // 一般情况
        sc = (b * e - c * d) / denominator;
        tc = (a * e - b * d) / denominator;
    }
    
    // 限制tc在线段范围内 [0, lineLength]
    if (tc < 0.0f) {
        tc = 0.0f;
    } else if (tc > lineLength) {
        tc = lineLength;
    }
    
    // 确保sc >= 0 (射线方向)
    if (sc < 0.0f) {
        sc = 0.0f;
        // 重新计算tc
        glm::vec3 rayToLineStart = lineStart - rayStartGLM;
        tc = glm::clamp(glm::dot(rayToLineStart, lineDir), 0.0f, lineLength);
    }
    
    // 计算两条线上的最近点
    glm::vec3 rayPoint = rayStartGLM + sc * rayDirGLM;
    glm::vec3 linePoint = lineStart + (tc / lineLength) * (lineEnd - lineStart);
    
    // 计算距离
    distance = glm::length(rayPoint - linePoint);
    closestPoint = linePoint;
    
    return true;
}

bool RayPickingSystem::calculateScreenLineProjection(int mouseX, int mouseY, 
                                                    const glm::vec3& lineStart, const glm::vec3& lineEnd,
                                                    float& screenDistance, glm::vec3& projectedPoint)
{
    // 将线段端点转换到屏幕坐标
    glm::vec2 screenStart = worldToScreen(lineStart);
    glm::vec2 screenEnd = worldToScreen(lineEnd);
    glm::vec2 mousePos(mouseX, mouseY);
    
    // 计算线段在屏幕上的方向向量
    glm::vec2 lineVec = screenEnd - screenStart;
    float lineLength = glm::length(lineVec);
    
    if (lineLength < 1e-6f) {
        // 线段退化为点
        screenDistance = glm::distance(mousePos, screenStart);
        projectedPoint = lineStart;
        return true;
    }
    
    // 标准化线段向量
    glm::vec2 lineDir = lineVec / lineLength;
    
    // 计算鼠标位置到线段起点的向量
    glm::vec2 toMouse = mousePos - screenStart;
    
    // 计算在线段方向上的投影长度
    float projectionLength = glm::dot(toMouse, lineDir);
    
    // 限制投影在线段范围内
    projectionLength = glm::clamp(projectionLength, 0.0f, lineLength);
    
    // 计算屏幕上的投影点
    glm::vec2 projectedScreenPoint = screenStart + projectionLength * lineDir;
    
    // 计算屏幕距离
    screenDistance = glm::distance(mousePos, projectedScreenPoint);
    
    // 将投影参数转换回世界坐标
    float t = projectionLength / lineLength;
    projectedPoint = lineStart + t * (lineEnd - lineStart);
    
    return true;
}
