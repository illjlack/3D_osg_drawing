#include "SimplifiedPickingSystem.h"
#include "../GeometryBase.h"
#include "../../util/LogManager.h"
#include <osg/Timer>
#include <osg/ShapeDrawable>
#include <osg/Shape>
#include <osg/PolygonOffset>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/CullFace>
#include <osgViewer/Viewer>
#include <osgGA/GUIEventAdapter>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// SimplifiedPickingSystem Implementation
// ============================================================================

SimplifiedPickingSystem::SimplifiedPickingSystem()
{
    // 创建指示器根节点
    m_indicatorRoot = new osg::Group;
    m_indicatorRoot->setName("SimplifiedPickingIndicatorRoot");
    
    // 设置指示器根节点渲染状态
    osg::StateSet* stateSet = m_indicatorRoot->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    
    LOG_DEBUG("SimplifiedPickingSystem创建完成", "拾取");
}

SimplifiedPickingSystem::~SimplifiedPickingSystem()
{
    shutdown();
}

bool SimplifiedPickingSystem::initialize(osg::Camera* camera, osg::Group* sceneRoot)
{
    if (!camera || !sceneRoot) {
        LOG_ERROR("初始化参数无效", "拾取");
        return false;
    }
    
    m_camera = camera;
    m_sceneRoot = sceneRoot;
    
    // 创建指示器
    m_indicator = new osg::MatrixTransform;
    m_indicator->setName("PickingIndicator");
    
    // 创建指示器几何体
    osg::ref_ptr<osg::Geometry> indicatorGeometry = createIndicatorGeometry(m_config.indicatorSize);
    if (indicatorGeometry) {
        m_indicator->addChild(indicatorGeometry);
        m_indicatorRoot->addChild(m_indicator);
    }
    
    // 创建高亮节点
    m_highlightNode = new osg::Group;
    m_highlightNode->setName("PickingHighlight");
    m_indicatorRoot->addChild(m_highlightNode);
    
    // 初始时隐藏指示器
    m_indicator->setNodeMask(0);
    
    m_initialized = true;
    
    LOG_SUCCESS("SimplifiedPickingSystem初始化成功", "拾取");
    return true;
}

void SimplifiedPickingSystem::shutdown()
{
    if (!m_initialized) return;
    
    hideIndicator();
    hideHighlight();
    clearAllGeometries();
    
    m_camera = nullptr;
    m_sceneRoot = nullptr;
    m_indicator = nullptr;
    m_highlightNode = nullptr;
    m_indicatorRoot = nullptr;
    m_highlightedGeometry = nullptr;
    
    m_initialized = false;
    
    LOG_INFO("SimplifiedPickingSystem已关闭", "拾取");
}

void SimplifiedPickingSystem::addGeometry(Geo3D* geometry)
{
    if (!geometry || !m_initialized) return;
    
    // 检查是否已存在
    auto it = std::find_if(m_geometries.begin(), m_geometries.end(),
        [geometry](const osg::ref_ptr<Geo3D>& ref) { return ref.get() == geometry; });
    
    if (it != m_geometries.end()) {
        LOG_DEBUG("几何体已存在于拾取系统中", "拾取");
        return;
    }
    
    m_geometries.push_back(geometry);
    
    // 预计算捕捉点
    std::vector<glm::vec3> snapPoints = extractSnapPoints(geometry);
    m_snapPointsCache[geometry] = snapPoints;
    
    LOG_DEBUG(QString("添加几何体到拾取系统 - 捕捉点数量: %1").arg(snapPoints.size()), "拾取");
}

void SimplifiedPickingSystem::removeGeometry(Geo3D* geometry)
{
    if (!geometry) return;
    
    auto it = std::find_if(m_geometries.begin(), m_geometries.end(),
        [geometry](const osg::ref_ptr<Geo3D>& ref) { return ref.get() == geometry; });
    
    if (it != m_geometries.end()) {
        m_geometries.erase(it);
        m_snapPointsCache.erase(geometry);
        
        // 如果当前高亮的是这个几何体，清除高亮
        if (m_highlightedGeometry == geometry) {
            hideHighlight();
        }
        
        LOG_DEBUG("从拾取系统移除几何体", "拾取");
    }
}

void SimplifiedPickingSystem::updateGeometry(Geo3D* geometry)
{
    if (!geometry) return;
    
    // 检查几何体是否已经在拾取系统中
    auto it = std::find_if(m_geometries.begin(), m_geometries.end(),
        [geometry](const osg::ref_ptr<Geo3D>& ref) { return ref.get() == geometry; });
    
    if (it == m_geometries.end()) {
        // 如果几何体不在拾取系统中，先添加它
        addGeometry(geometry);
        LOG_DEBUG(QString("几何体不在拾取系统中，已添加: %1").arg(geometry->getGeoType()), "拾取");
    } else {
        // 如果几何体已经在拾取系统中，更新捕捉点缓存
        std::vector<glm::vec3> snapPoints = extractSnapPoints(geometry);
        m_snapPointsCache[geometry] = snapPoints;
        
        LOG_DEBUG(QString("更新几何体 - 捕捉点数量: %1").arg(snapPoints.size()), "拾取");
    }
}

void SimplifiedPickingSystem::clearAllGeometries()
{
    m_geometries.clear();
    m_snapPointsCache.clear();
    hideHighlight();
    hideIndicator();
    m_lastResult = SimplePickingResult();
    
    LOG_DEBUG("清除所有几何体", "拾取");
}

SimplePickingResult SimplifiedPickingSystem::pick(int mouseX, int mouseY)
{
    if (!m_initialized) {
        LOG_ERROR("拾取系统未初始化", "拾取");
        return SimplePickingResult();
    }
    
    // 频率限制
    double currentTime = osg::Timer::instance()->time_s();
    if (currentTime - m_lastPickTime < 1.0 / m_config.pickingFrequency) {
        return m_lastResult;
    }
    m_lastPickTime = currentTime;
    
    osg::Timer_t startTime = osg::Timer::instance()->tick();
    
    // 执行射线相交检测
    SimplePickingResult result = performRayIntersection(mouseX, mouseY);
    
    // 计算捕捉
    if (result.hasResult && m_config.enableSnapping) {
        result = calculateSnapping(result);
    }
    
    // 更新指示器，但不更新高亮（高亮只在选择时显示）
    if (result.hasResult) {
        if (m_config.enableIndicator) {
            showIndicator(result);
        }
        // 注释掉高亮，因为高亮只在选择时显示
        // if (m_config.enableHighlight) {
        //     showHighlight(result.geometry);
        // }
    } else {
        hideIndicator();
        // 不清除高亮，因为高亮是选择状态的一部分
        // hideHighlight();
    }
    
    // 调用回调
    if (m_pickingCallback) {
        m_pickingCallback(result);
    }
    
    // 缓存结果
    m_lastResult = result;
    
    osg::Timer_t endTime = osg::Timer::instance()->tick();
    double pickTime = osg::Timer::instance()->delta_s(startTime, endTime);
    
    if (m_debugMode) {
        LOG_DEBUG(QString("拾取完成 - 时间: %1ms, 结果: %2")
            .arg(pickTime * 1000, 0, 'f', 2)
            .arg(result.hasResult ? "命中" : "未命中"), "拾取");
    }
    
    return result;
}

void SimplifiedPickingSystem::setPickingCallback(std::function<void(const SimplePickingResult&)> callback)
{
    m_pickingCallback = callback;
}

SimplePickingResult SimplifiedPickingSystem::performRayIntersection(int mouseX, int mouseY)
{
    SimplePickingResult result;
    
    if (!m_camera || !m_sceneRoot) return result;
    
    // 创建射线
    osg::Viewport* viewport = m_camera->getViewport();
    if (!viewport) return result;
    
    float normalizedX = (float)mouseX / viewport->width();
    float normalizedY = (float)mouseY / viewport->height();
    
    osg::Matrix VPW = m_camera->getViewMatrix() * 
                      m_camera->getProjectionMatrix() * 
                      viewport->computeWindowMatrix();
    
    osg::Matrix invVPW;
    if (!invVPW.invert(VPW)) {
        LOG_WARNING("无法反转视口矩阵", "拾取");
        return result;
    }
    
    // 计算射线起点和终点
    osg::Vec3 nearPoint = osg::Vec3(mouseX, viewport->height() - mouseY, 0.0f) * invVPW;
    osg::Vec3 farPoint = osg::Vec3(mouseX, viewport->height() - mouseY, 1.0f) * invVPW;
    
    // 计算射线方向和起点
    glm::vec3 rayOrigin(nearPoint.x(), nearPoint.y(), nearPoint.z());
    glm::vec3 rayDirection = glm::normalize(glm::vec3(farPoint.x() - nearPoint.x(), 
                                                      farPoint.y() - nearPoint.y(), 
                                                      farPoint.z() - nearPoint.z()));
    
    Ray3D ray(rayOrigin, rayDirection);
    
    // 添加调试信息
    LOG_DEBUG(QString("射线拾取: 屏幕坐标(%1,%2), 射线起点(%3,%4,%5), 方向(%6,%7,%8)")
        .arg(mouseX).arg(mouseY)
        .arg(rayOrigin.x, 0, 'f', 3).arg(rayOrigin.y, 0, 'f', 3).arg(rayOrigin.z, 0, 'f', 3)
        .arg(rayDirection.x, 0, 'f', 3).arg(rayDirection.y, 0, 'f', 3).arg(rayDirection.z, 0, 'f', 3), "拾取");
    
    LOG_DEBUG(QString("几何体数量: %1").arg(m_geometries.size()), "拾取");
    
    // 测试所有几何对象，使用对象的hitTest方法
    float minDistance = FLT_MAX;
    for (const osg::ref_ptr<Geo3D>& geo : m_geometries)
    {
        if (!geo) continue;
        
        LOG_DEBUG(QString("测试几何体: 类型=%1, 状态=%2")
            .arg(geo->getGeoType())
            .arg(geo->isStateComplete() ? "完成" : "未完成"), "拾取");
        
        PickResult3D geoResult;
        // 使用对象的hitTest方法进行射线相交检测
        if (geo->hitTest(ray, geoResult))
        {
            LOG_DEBUG(QString("几何体命中: 类型=%1, 距离=%2")
                .arg(geo->getGeoType())
                .arg(geoResult.distance, 0, 'f', 3), "拾取");
            
            if (geoResult.distance < minDistance)
            {
                minDistance = geoResult.distance;
                
                // 转换为SimplePickingResult
                result.hasResult = true;
                result.geometry = geo.get();
                result.worldPosition = geoResult.point;
                result.distance = geoResult.distance;
                result.screenX = mouseX;
                result.screenY = mouseY;
                result.featureType = SimplePickingResult::FACE; // 默认为面
                result.isSnapped = false;
                result.snapPosition = result.worldPosition;
            }
        }
    }
    
    if (result.hasResult) {
        LOG_DEBUG(QString("射线拾取成功: 距离=%1").arg(result.distance, 0, 'f', 3), "拾取");
    } else {
        LOG_DEBUG("射线拾取失败: 没有命中任何几何体", "拾取");
    }
    
    return result;
}

SimplePickingResult SimplifiedPickingSystem::calculateSnapping(const SimplePickingResult& result)
{
    SimplePickingResult snappedResult = result;
    
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
        snappedResult.featureType = SimplePickingResult::VERTEX;
        
        // 更新屏幕坐标
        glm::vec2 screenPos = worldToScreen(bestSnapPoint);
        snappedResult.screenX = static_cast<int>(screenPos.x);
        snappedResult.screenY = static_cast<int>(screenPos.y);
        
        if (m_debugMode) {
            LOG_DEBUG(QString("捕捉到点 - 距离: %1").arg(bestDistance), "拾取");
        }
    }
    
    return snappedResult;
}

std::vector<glm::vec3> SimplifiedPickingSystem::extractSnapPoints(Geo3D* geometry)
{
    std::vector<glm::vec3> snapPoints;
    
    if (!geometry) return snapPoints;
    
    // 获取几何体的控制点
    const auto& controlPoints = geometry->getControlPoints();
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

void SimplifiedPickingSystem::showIndicator(const SimplePickingResult& result)
{
    if (!m_indicator) return;
    
    // 设置指示器位置
    osg::Matrix matrix;
    matrix.makeTranslate(osg::Vec3(
        result.worldPosition.x, 
        result.worldPosition.y, 
        result.worldPosition.z
    ));
    m_indicator->setMatrix(matrix);
    
    // 显示指示器
    m_indicator->setNodeMask(0xFFFFFFFF);
    
    // 根据特征类型设置颜色
    osg::Vec4 color;
    switch (result.featureType) {
        case SimplePickingResult::VERTEX:
            color = osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f); // 红色
            break;
        case SimplePickingResult::EDGE:
            color = osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f); // 绿色
            break;
        case SimplePickingResult::FACE:
            color = osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f); // 蓝色
            break;
        default:
            color = osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f); // 黄色
            break;
    }
    
    // 更新指示器颜色
    if (m_indicator->getNumChildren() > 0) {
        osg::Node* child = m_indicator->getChild(0);
        osg::StateSet* stateSet = child->getOrCreateStateSet();
        osg::Material* material = new osg::Material;
        material->setDiffuse(osg::Material::FRONT_AND_BACK, color);
        stateSet->setAttributeAndModes(material);
    }
}

void SimplifiedPickingSystem::hideIndicator()
{
    if (m_indicator) {
        m_indicator->setNodeMask(0);
    }
}

void SimplifiedPickingSystem::showHighlight(Geo3D* geometry)
{
    if (!geometry || !m_highlightNode) return;
    
    // 如果已经高亮了同一个几何体，不需要重新创建
    if (m_highlightedGeometry == geometry) return;
    
    // 清除之前的高亮
    hideHighlight();
    
    // 创建控制点高亮几何体
    osg::ref_ptr<osg::Geometry> highlightGeometry = createControlPointHighlightGeometry(geometry);
    if (highlightGeometry) {
        m_highlightNode->addChild(highlightGeometry);
        m_highlightedGeometry = geometry;
    }
}

void SimplifiedPickingSystem::hideHighlight()
{
    if (m_highlightNode) {
        m_highlightNode->removeChildren(0, m_highlightNode->getNumChildren());
        m_highlightedGeometry = nullptr;
    }
}

void SimplifiedPickingSystem::showSelectionHighlight(Geo3D* geometry)
{
    if (!geometry || !m_highlightNode) return;
    
    // 清除之前的选择高亮
    hideSelectionHighlight();
    
    // 创建控制点高亮几何体
    osg::ref_ptr<osg::Geometry> highlightGeometry = createControlPointHighlightGeometry(geometry);
    if (highlightGeometry) {
        m_highlightNode->addChild(highlightGeometry);
        m_highlightedGeometry = geometry;
    }
}

void SimplifiedPickingSystem::hideSelectionHighlight()
{
    if (m_highlightNode) {
        m_highlightNode->removeChildren(0, m_highlightNode->getNumChildren());
        m_highlightedGeometry = nullptr;
    }
}

glm::vec3 SimplifiedPickingSystem::screenToWorld(int x, int y, float depth)
{
    if (!m_camera) return glm::vec3(0.0f);
    
    osg::Viewport* viewport = m_camera->getViewport();
    if (!viewport) return glm::vec3(0.0f);
    
    osg::Matrix VPW = m_camera->getViewMatrix() * 
                      m_camera->getProjectionMatrix() * 
                      viewport->computeWindowMatrix();
    
    osg::Matrix invVPW;
    if (!invVPW.invert(VPW)) {
        return glm::vec3(0.0f);
    }
    
    osg::Vec3 worldPos = osg::Vec3(x, viewport->height() - y, depth) * invVPW;
    return glm::vec3(worldPos.x(), worldPos.y(), worldPos.z());
}

glm::vec2 SimplifiedPickingSystem::worldToScreen(const glm::vec3& worldPos)
{
    if (!m_camera) return glm::vec2(0.0f);
    
    osg::Viewport* viewport = m_camera->getViewport();
    if (!viewport) return glm::vec2(0.0f);
    
    osg::Matrix VPW = m_camera->getViewMatrix() * 
                      m_camera->getProjectionMatrix() * 
                      viewport->computeWindowMatrix();
    
    osg::Vec3 screenPos = osg::Vec3(worldPos.x, worldPos.y, worldPos.z) * VPW;
    return glm::vec2(screenPos.x(), viewport->height() - screenPos.y());
}

osg::ref_ptr<osg::Geometry> SimplifiedPickingSystem::createIndicatorGeometry(float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    
    // 创建十字形指示器
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->push_back(osg::Vec3(-size, 0.0f, 0.0f));
    vertices->push_back(osg::Vec3(size, 0.0f, 0.0f));
    vertices->push_back(osg::Vec3(0.0f, -size, 0.0f));
    vertices->push_back(osg::Vec3(0.0f, size, 0.0f));
    vertices->push_back(osg::Vec3(0.0f, 0.0f, -size));
    vertices->push_back(osg::Vec3(0.0f, 0.0f, size));
    
    geometry->setVertexArray(vertices);
    
    // 创建线段
    osg::ref_ptr<osg::DrawElementsUInt> lines = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES);
    lines->push_back(0); lines->push_back(1); // X轴
    lines->push_back(2); lines->push_back(3); // Y轴
    lines->push_back(4); lines->push_back(5); // Z轴
    
    geometry->addPrimitiveSet(lines);
    
    // 设置状态
    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setAttributeAndModes(new osg::LineWidth(2.0f));
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> SimplifiedPickingSystem::createHighlightGeometry(Geo3D* geometry)
{
    if (!geometry) return nullptr;
    
    osg::ref_ptr<osg::Group> geoNode = geometry->mm_node()->getOSGNode();
    if (!geoNode) return nullptr;
    
    // 创建高亮几何体组
    osg::ref_ptr<osg::Group> highlightGroup = new osg::Group;
    
    // 递归遍历几何体节点，创建高亮版本
    class HighlightVisitor : public osg::NodeVisitor {
    public:
        HighlightVisitor(osg::Group* highlightGroup) 
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN), m_highlightGroup(highlightGroup) {}
        
        void apply(osg::Geometry& geom) override {
            osg::ref_ptr<osg::Geometry> highlightGeom = new osg::Geometry;
            highlightGeom->setVertexArray(geom.getVertexArray());
            
            for (unsigned int i = 0; i < geom.getNumPrimitiveSets(); ++i) {
                highlightGeom->addPrimitiveSet(geom.getPrimitiveSet(i));
            }
            
            // 设置高亮状态
            osg::StateSet* stateSet = highlightGeom->getOrCreateStateSet();
            
            // 线框模式
            osg::PolygonMode* polygonMode = new osg::PolygonMode;
            polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
            stateSet->setAttributeAndModes(polygonMode);
            
            // 设置线宽和颜色
            stateSet->setAttributeAndModes(new osg::LineWidth(3.0f));
            stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
            
            osg::Material* material = new osg::Material;
            material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));
            stateSet->setAttributeAndModes(material);
            
            // 设置多边形偏移
            osg::PolygonOffset* polygonOffset = new osg::PolygonOffset;
            polygonOffset->setFactor(-1.0f);
            polygonOffset->setUnits(-1.0f);
            stateSet->setAttributeAndModes(polygonOffset);
            
            m_highlightGroup->addChild(highlightGeom);
        }
        
    private:
        osg::Group* m_highlightGroup;
    };
    
    HighlightVisitor visitor(highlightGroup.get());
    geoNode->accept(visitor);
    
    return highlightGroup->getNumChildren() > 0 ? highlightGroup->getChild(0)->asGeometry() : nullptr;
}

osg::ref_ptr<osg::Geometry> SimplifiedPickingSystem::createControlPointHighlightGeometry(Geo3D* geometry)
{
    if (!geometry) return nullptr;
    
    // 创建控制点高亮几何体
    osg::ref_ptr<osg::Geometry> highlightGeometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 获取控制点
    const auto& controlPoints = geometry->getControlPoints();
    if (!controlPoints.empty())
    {
        // 添加所有控制点
        for (const auto& cp : controlPoints)
        {
            vertices->push_back(osg::Vec3(cp.x(), cp.y(), cp.z()));
            colors->push_back(osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f)); // 黄色高亮
        }
        
        highlightGeometry->setVertexArray(vertices);
        highlightGeometry->setColorArray(colors);
        highlightGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        
        // 点绘制
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
        highlightGeometry->addPrimitiveSet(drawArrays);
        
        // 设置点的大小
        osg::StateSet* stateSet = highlightGeometry->getOrCreateStateSet();
        osg::ref_ptr<osg::Point> point = new osg::Point;
        point->setSize(12.0f);  // 高亮控制点大小
        stateSet->setAttribute(point);
        
        // 禁用光照
        stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        
        // 设置深度测试
        stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
        
        // 设置多边形偏移，避免Z-fighting
        osg::PolygonOffset* polygonOffset = new osg::PolygonOffset;
        polygonOffset->setFactor(-1.0f);
        polygonOffset->setUnits(-1.0f);
        stateSet->setAttributeAndModes(polygonOffset);
        
        return highlightGeometry;
    }
    
    return nullptr;
}

// ============================================================================
// SimplifiedPickingEventHandler Implementation
// ============================================================================

SimplifiedPickingEventHandler::SimplifiedPickingEventHandler(SimplifiedPickingSystem* pickingSystem)
    : m_pickingSystem(pickingSystem)
{
}

SimplifiedPickingEventHandler::~SimplifiedPickingEventHandler()
{
}

bool SimplifiedPickingEventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    if (!m_enabled || !m_pickingSystem) return false;
    
    switch (ea.getEventType()) {
    case osgGA::GUIEventAdapter::MOVE:
        {
            int x = static_cast<int>(ea.getX());
            int y = static_cast<int>(ea.getY());
            
            // 获取视口高度以进行Y轴翻转
            osg::Camera* camera = m_pickingSystem->getCamera();
            if (camera && camera->getViewport()) {
                int viewportHeight = static_cast<int>(camera->getViewport()->height());
                y = viewportHeight - y; // 翻转Y轴
            }
            
            // 避免重复处理相同位置
            if (x != m_lastX || y != m_lastY) {
                processPicking(x, y);
                m_lastX = x;
                m_lastY = y;
            }
            break;
        }
    default:
        break;
    }
    
    return false;
}

void SimplifiedPickingEventHandler::setPickingCallback(std::function<void(const SimplePickingResult&)> callback)
{
    m_pickingCallback = callback;
}

void SimplifiedPickingEventHandler::processPicking(int x, int y)
{
    if (!m_pickingSystem) return;
    
    // 频率控制
    double currentTime = osg::Timer::instance()->time_s();
    if (currentTime - m_lastPickTime < 1.0 / m_pickingSystem->getConfig().pickingFrequency) {
        return;
    }
    m_lastPickTime = currentTime;
    
    // 执行拾取
    SimplePickingResult result = m_pickingSystem->pick(x, y);
    
    // 调用回调
    if (m_pickingCallback) {
        m_pickingCallback(result);
    }
}

// ============================================================================
// SimplifiedPickingSystemManager Implementation
// ============================================================================

SimplifiedPickingSystemManager& SimplifiedPickingSystemManager::getInstance()
{
    static SimplifiedPickingSystemManager instance;
    return instance;
}

SimplifiedPickingSystemManager::SimplifiedPickingSystemManager()
{
    m_pickingSystem = new SimplifiedPickingSystem();
}

SimplifiedPickingSystemManager::~SimplifiedPickingSystemManager()
{
    shutdown();
}

bool SimplifiedPickingSystemManager::initialize(osg::Camera* camera, osg::Group* sceneRoot)
{
    bool result = m_pickingSystem->initialize(camera, sceneRoot);
    if (result) {
        m_eventHandler = new SimplifiedPickingEventHandler(m_pickingSystem.get());
        LOG_SUCCESS("SimplifiedPickingSystemManager初始化成功", "拾取");
    }
    return result;
}

void SimplifiedPickingSystemManager::shutdown()
{
    if (m_pickingSystem) {
        m_pickingSystem->shutdown();
    }
    m_eventHandler = nullptr;
}

void SimplifiedPickingSystemManager::setConfig(const SimplePickingConfig& config)
{
    if (m_pickingSystem) {
        m_pickingSystem->setConfig(config);
    }
}

const SimplePickingConfig& SimplifiedPickingSystemManager::getConfig() const
{
    static SimplePickingConfig defaultConfig;
    return m_pickingSystem ? m_pickingSystem->getConfig() : defaultConfig;
}

void SimplifiedPickingSystemManager::addGeometry(Geo3D* geometry)
{
    if (m_pickingSystem) {
        m_pickingSystem->addGeometry(geometry);
    }
}

void SimplifiedPickingSystemManager::removeGeometry(Geo3D* geometry)
{
    if (m_pickingSystem) {
        m_pickingSystem->removeGeometry(geometry);
    }
}

void SimplifiedPickingSystemManager::updateGeometry(Geo3D* geometry)
{
    if (m_pickingSystem) {
        m_pickingSystem->updateGeometry(geometry);
    }
}

void SimplifiedPickingSystemManager::clearAllGeometries()
{
    if (m_pickingSystem) {
        m_pickingSystem->clearAllGeometries();
    }
}

SimplePickingResult SimplifiedPickingSystemManager::pick(int mouseX, int mouseY)
{
    if (m_pickingSystem) {
        return m_pickingSystem->pick(mouseX, mouseY);
    }
    return SimplePickingResult();
}

void SimplifiedPickingSystemManager::showSelectionHighlight(Geo3D* geometry)
{
    if (m_pickingSystem) {
        m_pickingSystem->showSelectionHighlight(geometry);
    }
}

void SimplifiedPickingSystemManager::hideSelectionHighlight()
{
    if (m_pickingSystem) {
        m_pickingSystem->hideSelectionHighlight();
    }
}

osgGA::GUIEventHandler* SimplifiedPickingSystemManager::getEventHandler()
{
    return m_eventHandler.get();
}

void SimplifiedPickingSystemManager::setPickingCallback(std::function<void(const SimplePickingResult&)> callback)
{
    if (m_pickingSystem) {
        m_pickingSystem->setPickingCallback(callback);
    }
    if (m_eventHandler) {
        m_eventHandler->setPickingCallback(callback);
    }
}

osg::Group* SimplifiedPickingSystemManager::getIndicatorRoot()
{
    return m_pickingSystem ? m_pickingSystem->getIndicatorRoot() : nullptr;
}

bool SimplifiedPickingSystemManager::isInitialized() const
{
    return m_pickingSystem && m_pickingSystem->isInitialized();
}

QString SimplifiedPickingSystemManager::getSystemInfo() const
{
    if (!m_pickingSystem) {
        return "拾取系统未创建";
    }
    
    if (!m_pickingSystem->isInitialized()) {
        return "拾取系统未初始化";
    }
    
    return QString("简化拾取系统 - 几何体数量: %1")
        .arg(m_pickingSystem->getGeometryCount());
} 