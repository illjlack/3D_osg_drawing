#include "PickingIntegration.h"
#include "../GeometryBase.h"
#include <osg/ShapeDrawable>
#include <osg/PrimitiveSet>
#include <osg/PolygonMode>
#include <osg/PolygonOffset>
#include <osg/CullFace>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osgViewer/Viewer>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// ============================================================================
// SimplePickingIndicatorManager Implementation
// ============================================================================

SimplePickingIndicatorManager::SimplePickingIndicatorManager()
    : m_indicatorVisible(false)
    , m_animationTime(0.0f)
    , m_highlightedObject(nullptr)
{
    m_indicatorRoot = new osg::Group;
    m_highlightRoot = new osg::Group;
}

SimplePickingIndicatorManager::~SimplePickingIndicatorManager()
{
}

bool SimplePickingIndicatorManager::initialize()
{
    // 设置指示器根节点状态
    osg::StateSet* indicatorState = m_indicatorRoot->getOrCreateStateSet();
    indicatorState->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    indicatorState->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    indicatorState->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    
    // 设置高亮根节点状态
    osg::StateSet* highlightState = m_highlightRoot->getOrCreateStateSet();
    highlightState->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    highlightState->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    
    return true;
}

void SimplePickingIndicatorManager::showIndicator(const PickingResult& result)
{
    if (!result.hasResult)
    {
        hideIndicator();
        return;
    }
    
    // 检查是否需要创建新指示器
    if (!m_currentIndicator || 
        m_lastResult.id.pack() != result.id.pack() ||
        glm::distance(m_lastResult.worldPos, result.worldPos) > 0.001f)
    {
        createIndicator(result);
        m_lastResult = result;
    }
    
    // 高亮对象
    if (result.geometry)
    {
        highlightObject(result.geometry);
    }
    
    m_indicatorVisible = true;
}

void SimplePickingIndicatorManager::hideIndicator()
{
    if (m_currentIndicator)
    {
        m_indicatorRoot->removeChild(m_currentIndicator);
        m_currentIndicator = nullptr;
    }
    
    m_indicatorVisible = false;
}

void SimplePickingIndicatorManager::clearAll()
{
    hideIndicator();
    clearHighlight();
}

void SimplePickingIndicatorManager::highlightObject(Geo3D* geo)
{
    if (!geo || geo == m_highlightedObject)
        return;
    
    clearHighlight();
    createHighlight(geo);
    m_highlightedObject = geo;
}

void SimplePickingIndicatorManager::clearHighlight()
{
    if (m_currentHighlight)
    {
        m_highlightRoot->removeChild(m_currentHighlight);
        m_currentHighlight = nullptr;
    }
    
    m_highlightedObject = nullptr;
}

void SimplePickingIndicatorManager::update(float deltaTime)
{
    if (!m_currentIndicator || !m_indicatorVisible)
        return;
    
    m_animationTime += deltaTime * 2.0f;  // 动画速度
    
    // 简单的旋转动画
    osg::Matrix matrix;
    matrix.makeRotate(m_animationTime, osg::Vec3(0, 0, 1));
    matrix.preMultTranslate(osg::Vec3(m_lastResult.worldPos.x, 
                                     m_lastResult.worldPos.y, 
                                     m_lastResult.worldPos.z));
    
    m_currentIndicator->setMatrix(matrix);
}

void SimplePickingIndicatorManager::onPickingResult(const PickingResult& result)
{
    showIndicator(result);
}

void SimplePickingIndicatorManager::createIndicator(const PickingResult& result)
{
    // 移除旧指示器
    if (m_currentIndicator)
    {
        m_indicatorRoot->removeChild(m_currentIndicator);
    }
    
    // 创建新指示器
    m_currentIndicator = new osg::MatrixTransform;
    
    osg::ref_ptr<osg::Geometry> geometry;
    osg::Vec4 color;
    
    // 根据类型创建不同的指示器
    switch (result.id.typeCode)
    {
    case PickingID64::TYPE_VERTEX:
        geometry = createVertexIndicator(0.05f);
        color = osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f);  // 红色
        break;
    case PickingID64::TYPE_EDGE:
        geometry = createEdgeIndicator(0.08f);
        color = osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f);  // 绿色
        break;
    case PickingID64::TYPE_FACE:
        geometry = createFaceIndicator(0.1f);
        color = osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f);  // 蓝色
        break;
    default:
        geometry = createFaceIndicator(0.1f);
        color = osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f);  // 黄色
        break;
    }
    
    if (geometry)
    {
        m_currentIndicator->addChild(geometry);
        
        // 设置材质
        osg::StateSet* stateSet = geometry->getOrCreateStateSet();
        osg::Material* material = new osg::Material;
        material->setDiffuse(osg::Material::FRONT_AND_BACK, color);
        material->setAmbient(osg::Material::FRONT_AND_BACK, color * 0.3f);
        stateSet->setAttributeAndModes(material);
        stateSet->setAttributeAndModes(new osg::LineWidth(3.0f));
    }
    
    // 设置位置
    osg::Matrix matrix;
    matrix.makeTranslate(osg::Vec3(result.worldPos.x, result.worldPos.y, result.worldPos.z));
    m_currentIndicator->setMatrix(matrix);
    
    // 添加到场景图
    m_indicatorRoot->addChild(m_currentIndicator);
}

osg::ref_ptr<osg::Geometry> SimplePickingIndicatorManager::createVertexIndicator(float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    
    // 创建方框顶点
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    float half = size * 0.5f;
    
    // 立方体的8个顶点
    vertices->push_back(osg::Vec3(-half, -half, -half));
    vertices->push_back(osg::Vec3( half, -half, -half));
    vertices->push_back(osg::Vec3( half,  half, -half));
    vertices->push_back(osg::Vec3(-half,  half, -half));
    vertices->push_back(osg::Vec3(-half, -half,  half));
    vertices->push_back(osg::Vec3( half, -half,  half));
    vertices->push_back(osg::Vec3( half,  half,  half));
    vertices->push_back(osg::Vec3(-half,  half,  half));
    
    geometry->setVertexArray(vertices);
    
    // 创建线框
    osg::ref_ptr<osg::DrawElementsUInt> lines = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES);
    
    // 方框的12条边
    int edges[24] = {
        0, 1, 1, 2, 2, 3, 3, 0,  // 底面
        4, 5, 5, 6, 6, 7, 7, 4,  // 顶面
        0, 4, 1, 5, 2, 6, 3, 7   // 连接边
    };
    
    for (int i = 0; i < 24; ++i)
    {
        lines->push_back(edges[i]);
    }
    
    geometry->addPrimitiveSet(lines);
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> SimplePickingIndicatorManager::createEdgeIndicator(float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    
    // 创建三角形箭头
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->push_back(osg::Vec3(0.0f, size, 0.0f));           // 顶点
    vertices->push_back(osg::Vec3(-size * 0.5f, 0.0f, 0.0f));   // 左下
    vertices->push_back(osg::Vec3(size * 0.5f, 0.0f, 0.0f));    // 右下
    
    geometry->setVertexArray(vertices);
    
    // 创建三角形
    osg::ref_ptr<osg::DrawElementsUInt> triangle = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES);
    triangle->push_back(0);
    triangle->push_back(1);
    triangle->push_back(2);
    
    geometry->addPrimitiveSet(triangle);
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> SimplePickingIndicatorManager::createFaceIndicator(float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    
    // 创建圆环顶点
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    int segments = 32;
    
    for (int i = 0; i < segments; ++i)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = size * cos(angle);
        float y = size * sin(angle);
        vertices->push_back(osg::Vec3(x, y, 0.0f));
    }
    
    geometry->setVertexArray(vertices);
    
    // 创建线环
    osg::ref_ptr<osg::DrawElementsUInt> lineLoop = new osg::DrawElementsUInt(osg::PrimitiveSet::LINE_LOOP);
    
    for (int i = 0; i < segments; ++i)
    {
        lineLoop->push_back(i);
    }
    
    geometry->addPrimitiveSet(lineLoop);
    
    return geometry;
}

void SimplePickingIndicatorManager::createHighlight(Geo3D* geo)
{
    if (!geo || !geo->node()->getOSGNode())
        return;
    
    m_currentHighlight = new osg::Group;
    m_highlightRoot->addChild(m_currentHighlight);
    
    // 高亮控制点而不是整个对象
    const auto& controlPoints = geo->getControlPoints();
    if (!controlPoints.empty())
    {
        // 创建控制点高亮几何体
        osg::ref_ptr<osg::Geometry> highlightGeometry = new osg::Geometry;
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
        
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
        osg::ref_ptr<osg::StateSet> stateSet = highlightGeometry->getOrCreateStateSet();
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
        
        m_currentHighlight->addChild(highlightGeometry);
    }
    
    // 如果对象被选中，显示包围盒
    if (geo->isStateSelected())
    {
        createBoundingBoxHighlight(geo);
    }
    
    // 为所有选中的对象显示包围盒
    // 这里需要从OSGWidget获取选中列表，暂时只处理当前对象
}

void SimplePickingIndicatorManager::createBoundingBoxHighlight(Geo3D* geo)
{
    if (!geo) return;
    
    auto* boundingBoxManager = geo->getBoundingBoxManager();
    if (!boundingBoxManager || !boundingBoxManager->isValid())
        return;
    
    // 创建包围盒线框
    osg::ref_ptr<osg::Geometry> bboxGeometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 获取包围盒的8个角点
    std::vector<glm::vec3> corners = boundingBoxManager->getCorners();
    
    // 添加所有角点
    for (const auto& corner : corners)
    {
        vertices->push_back(osg::Vec3(corner.x, corner.y, corner.z));
        colors->push_back(osg::Vec4(0.0f, 1.0f, 1.0f, 1.0f)); // 青色包围盒
    }
    
    bboxGeometry->setVertexArray(vertices);
    bboxGeometry->setColorArray(colors);
    bboxGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 绘制包围盒线框
    // 前面
    osg::ref_ptr<osg::DrawElementsUInt> frontIndices = new osg::DrawElementsUInt(osg::PrimitiveSet::LINE_LOOP);
    frontIndices->push_back(0); frontIndices->push_back(1); frontIndices->push_back(2); frontIndices->push_back(3);
    bboxGeometry->addPrimitiveSet(frontIndices);
    
    // 后面
    osg::ref_ptr<osg::DrawElementsUInt> backIndices = new osg::DrawElementsUInt(osg::PrimitiveSet::LINE_LOOP);
    backIndices->push_back(4); backIndices->push_back(5); backIndices->push_back(6); backIndices->push_back(7);
    bboxGeometry->addPrimitiveSet(backIndices);
    
    // 连接线
    osg::ref_ptr<osg::DrawElementsUInt> connectIndices = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES);
    connectIndices->push_back(0); connectIndices->push_back(4);
    connectIndices->push_back(1); connectIndices->push_back(5);
    connectIndices->push_back(2); connectIndices->push_back(6);
    connectIndices->push_back(3); connectIndices->push_back(7);
    bboxGeometry->addPrimitiveSet(connectIndices);
    
    // 设置线框状态
    osg::StateSet* stateSet = bboxGeometry->getOrCreateStateSet();
    stateSet->setAttributeAndModes(new osg::LineWidth(2.0f));
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    // 设置多边形偏移
    osg::PolygonOffset* polygonOffset = new osg::PolygonOffset;
    polygonOffset->setFactor(-1.0f);
    polygonOffset->setUnits(-1.0f);
    stateSet->setAttributeAndModes(polygonOffset);
    
    m_currentHighlight->addChild(bboxGeometry);
}

// ============================================================================
// PickingSystemIntegration Implementation
// ============================================================================

osg::ref_ptr<SimplePickingIndicatorManager> PickingSystemIntegration::s_indicatorManager;

bool PickingSystemIntegration::initializePickingSystem(int width, int height)
{
    // 初始化拾取系统管理器
    if (!PickingSystemManager::getInstance().initialize(width, height))
    {
        return false;
    }
    
    // 初始化指示器管理器
    s_indicatorManager = new SimplePickingIndicatorManager;
    if (!s_indicatorManager->initialize())
    {
        return false;
    }
    
    return true;
}

void PickingSystemIntegration::setMainCamera(osg::Camera* camera)
{
    PickingSystemManager::getInstance().setMainCamera(camera);
}

void PickingSystemIntegration::addPickingEventHandler(osgViewer::Viewer* viewer, 
    std::function<void(const PickingResult&)> callback)
{
    if (!viewer)
        return;
    
    osg::ref_ptr<PickingEventHandler> handler = new PickingEventHandler;
    handler->setPickingCallback(callback);
    viewer->addEventHandler(handler);
}

SimplePickingIndicatorManager* PickingSystemIntegration::getIndicatorManager()
{
    return s_indicatorManager.get();
}

void PickingSystemIntegration::addGeometry(Geo3D* geo)
{
    PickingSystemManager::getInstance().addObject(geo);
}

void PickingSystemIntegration::removeGeometry(Geo3D* geo)
{
    PickingSystemManager::getInstance().removeObject(geo);
}

void PickingSystemIntegration::updateGeometry(Geo3D* geo)
{
    PickingSystemManager::getInstance().updateObject(geo);
}

void PickingSystemIntegration::clearAllObjects()
{
    PickingSystemManager::getInstance().getPickingSystem()->clearAllObjects();
}

PickingResult PickingSystemIntegration::pick(int mouseX, int mouseY, int radius)
{
    return PickingSystemManager::getInstance().pick(mouseX, mouseY, radius);
} 