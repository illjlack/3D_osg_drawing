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
    if (!geo || !geo->getOSGNode())
        return;
    
    m_currentHighlight = new osg::Group;
    m_highlightRoot->addChild(m_currentHighlight);
    
    // 遍历几何对象的子节点
    osg::Group* geoGroup = geo->getOSGNode()->asGroup();
    if (geoGroup)
    {
        for (unsigned int i = 0; i < geoGroup->getNumChildren(); ++i)
        {
            osg::Node* child = geoGroup->getChild(i);
            osg::Geometry* geometry = child->asGeometry();
            
            // OSG 3.5.6 兼容性：也检查Geode节点
            if (!geometry)
            {
                osg::Geode* geode = child->asGeode();
                if (geode && geode->getNumDrawables() > 0)
                {
                    geometry = geode->getDrawable(0)->asGeometry();
                }
            }
            
            if (geometry)
            {
                // 创建高亮几何体
                osg::ref_ptr<osg::Geometry> highlightGeometry = new osg::Geometry;
                highlightGeometry->setVertexArray(geometry->getVertexArray());
                
                // 复制图元集
                for (unsigned int j = 0; j < geometry->getNumPrimitiveSets(); ++j)
                {
                    highlightGeometry->addPrimitiveSet(geometry->getPrimitiveSet(j));
                }
                
                // 设置高亮状态
                osg::StateSet* stateSet = highlightGeometry->getOrCreateStateSet();
                
                // 设置线框模式
                osg::PolygonMode* polygonMode = new osg::PolygonMode;
                polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
                stateSet->setAttributeAndModes(polygonMode);
                
                // 设置线宽
                stateSet->setAttributeAndModes(new osg::LineWidth(3.0f));
                
                // 设置颜色
                osg::Material* material = new osg::Material;
                material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));
                stateSet->setAttributeAndModes(material);
                
                // 设置多边形偏移
                osg::PolygonOffset* polygonOffset = new osg::PolygonOffset;
                polygonOffset->setFactor(-1.0f);
                polygonOffset->setUnits(-1.0f);
                stateSet->setAttributeAndModes(polygonOffset);
                
                m_currentHighlight->addChild(highlightGeometry);
            }
        }
    }
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

PickingResult PickingSystemIntegration::pick(int mouseX, int mouseY, int radius)
{
    return PickingSystemManager::getInstance().pick(mouseX, mouseY, radius);
} 