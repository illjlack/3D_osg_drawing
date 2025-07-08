#include "PickingSystem.h"
#include "../GeometryBase.h"
#include "../../util/LogManager.h"
#include <osg/GL>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osg/RenderInfo>
#include <osg/GLExtensions>
#include <osg/Timer>
#include <osgGA/GUIEventAdapter>
#include <osgViewer/Viewer>
#include <algorithm>
#include <chrono>
#include <fstream>

// 拾取着色器代码
// OSG 3.6.5 拾取着色器
static const char* pickingVertexShaderSource = R"(
#version 330 core

attribute vec3 osg_Vertex;

uniform mat4 osg_ModelViewProjectionMatrix;

void main()
{
    gl_Position = osg_ModelViewProjectionMatrix * vec4(osg_Vertex, 1.0);
}
)";

static const char* pickingFragmentShaderSource = R"(
#version 330 core

uniform samplerBuffer u_IDBuffer;
uniform int u_IDOffset;

out vec4 FragColor;

void main()
{
    int primitiveID = gl_PrimitiveID + u_IDOffset;
    
    // 从ID缓冲区获取64位ID
    vec4 idData = texelFetch(u_IDBuffer, primitiveID);
    
    // 将64位ID的低32位打包到RGBA
    uint packedID = uint(idData.x);
    FragColor = vec4(
        float((packedID >> 0) & 0xFFu) / 255.0,
        float((packedID >> 8) & 0xFFu) / 255.0,
        float((packedID >> 16) & 0xFFu) / 255.0,
        float((packedID >> 24) & 0xFFu) / 255.0
    );
}
)";

// ============================================================================
// PickingSystem Implementation
// ============================================================================

PickingSystem::PickingSystem()
    : m_width(0), m_height(0)
    , m_initialized(false)
    , m_debugMode(false)
    , m_nextObjectID(1)
    , m_asyncPickingEnabled(false)
    , m_asyncPickingInProgress(false)
    , m_asyncPickingReady(false)
    , m_currentPBO(0)
    , m_lastPickTime(0.0)
    , m_pickFrequencyLimit(1.0 / 60.0)
    , m_avgPickTime(0.0)
    , m_pickCount(0)
{
}

PickingSystem::~PickingSystem()
{
}

bool PickingSystem::initialize(int width, int height)
{
    if (m_initialized)
        return true;
    
    m_width = width;
    m_height = height;
    
    if (!createPickingCamera())
    {
        LOG_ERROR("Failed to create picking camera", "拾取");
        return false;
    }
    
    if (!createFrameBuffer())
    {
        LOG_ERROR("Failed to create frame buffer", "拾取");
        return false;
    }
    
    if (!createShaders())
    {
        LOG_ERROR("Failed to create shaders", "拾取");
        return false;
    }
    
    setupRenderStates();
    
    m_pickingRoot = new osg::Group;
    m_faceGroup = new osg::Group;
    m_edgeGroup = new osg::Group;
    m_vertexGroup = new osg::Group;
    
    m_pickingRoot->addChild(m_faceGroup);
    m_pickingRoot->addChild(m_edgeGroup);
    m_pickingRoot->addChild(m_vertexGroup);
    
    m_pickingCamera->addChild(m_pickingRoot);
    
    // OSG 3.6.5: 使用Image对象进行像素读取
    for (int i = 0; i < 2; ++i)
    {
        m_readImage[i] = new osg::Image;
        m_readImage[i]->allocateImage(width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    }
    
    m_initialized = true;
    
    LOG_SUCCESS(QString("Picking system initialized successfully (%1x%2)").arg(width).arg(height), "拾取");
    return true;
}

void PickingSystem::resize(int width, int height)
{
    if (!m_initialized)
        return;
    
    m_width = width;
    m_height = height;
    
    m_pickingCamera->setViewport(0, 0, width, height);
    
    m_colorTexture->setTextureSize(width, height);
    m_depthTexture->setTextureSize(width, height);
    
    m_colorImage->allocateImage(width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    m_depthImage->allocateImage(width, height, 1, GL_DEPTH_COMPONENT, GL_FLOAT);
    
    // PBO大小调整 - OSG 3.6.5中BufferObject会自动管理大小
    
    LOG_INFO(QString("Picking system resized to %1x%2").arg(width).arg(height), "拾取");
}

void PickingSystem::syncWithMainCamera(osg::Camera* mainCamera)
{
    if (!m_initialized || !mainCamera)
        return;
    
    m_mainCamera = mainCamera;
    
    m_pickingCamera->setViewMatrix(mainCamera->getViewMatrix());
    m_pickingCamera->setProjectionMatrix(mainCamera->getProjectionMatrix());
    
    const osg::Viewport* viewport = mainCamera->getViewport();
    if (viewport)
    {
        m_pickingCamera->setViewport(viewport->x(), viewport->y(), 
                                   viewport->width(), viewport->height());
    }
}

bool PickingSystem::createPickingCamera()
{
    m_pickingCamera = new osg::Camera;
    
    m_pickingCamera->setRenderOrder(osg::Camera::PRE_RENDER);
    m_pickingCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    
    m_pickingCamera->setViewport(0, 0, m_width, m_height);
    
    m_pickingCamera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    m_pickingCamera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    return true;
}

bool PickingSystem::createFrameBuffer()
{
    m_colorTexture = new osg::Texture2D;
    m_colorTexture->setTextureSize(m_width, m_height);
    m_colorTexture->setInternalFormat(GL_RGBA8);
    m_colorTexture->setSourceFormat(GL_RGBA);
    m_colorTexture->setSourceType(GL_UNSIGNED_BYTE);
    m_colorTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
    m_colorTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
    m_colorTexture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
    m_colorTexture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
    
    m_depthTexture = new osg::Texture2D;
    m_depthTexture->setTextureSize(m_width, m_height);
    m_depthTexture->setInternalFormat(GL_DEPTH_COMPONENT32F);
    m_depthTexture->setSourceFormat(GL_DEPTH_COMPONENT);
    m_depthTexture->setSourceType(GL_FLOAT);
    m_depthTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
    m_depthTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
    m_depthTexture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
    m_depthTexture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);
    
    m_colorImage = new osg::Image;
    m_colorImage->allocateImage(m_width, m_height, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    
    m_depthImage = new osg::Image;
    m_depthImage->allocateImage(m_width, m_height, 1, GL_DEPTH_COMPONENT, GL_FLOAT);
    
    m_colorTexture->setImage(m_colorImage);
    m_depthTexture->setImage(m_depthImage);
    
    m_pickingCamera->attach(osg::Camera::COLOR_BUFFER0, m_colorTexture);
    m_pickingCamera->attach(osg::Camera::DEPTH_BUFFER, m_depthTexture);
    
    return true;
}

bool PickingSystem::createShaders()
{
    m_vertexShader = new osg::Shader(osg::Shader::VERTEX);
    m_vertexShader->setShaderSource(pickingVertexShaderSource);
    
    m_fragmentShader = new osg::Shader(osg::Shader::FRAGMENT);
    m_fragmentShader->setShaderSource(pickingFragmentShaderSource);
    
    m_pickingProgram = new osg::Program;
    m_pickingProgram->addShader(m_vertexShader);
    m_pickingProgram->addShader(m_fragmentShader);
    
    m_idBufferData = new osg::UIntArray;
    m_idBuffer = new osg::TextureBuffer;
    m_idBuffer->setBufferData(m_idBufferData.get());
    m_idBuffer->setInternalFormat(GL_R32UI);
    
    return true;
}

void PickingSystem::setupRenderStates()
{
    if (!m_pickingRoot)
        return;
    
    osg::StateSet* stateSet = m_pickingRoot->getOrCreateStateSet();
    
    stateSet->setAttributeAndModes(m_pickingProgram, osg::StateAttribute::ON);
    
    // 设置ID缓冲区纹理
    stateSet->setTextureAttributeAndModes(0, m_idBuffer, osg::StateAttribute::ON);
    stateSet->addUniform(new osg::Uniform("u_IDBuffer", 0));
    
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
    
    stateSet->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL));
}

uint64_t PickingSystem::addObject(Geo3D* geo)
{
    if (!geo || !m_initialized)
        return 0;
    
    uint64_t objectID = m_nextObjectID++;
    
    // 创建拾取数据
    PickingObjectData objData(geo);
    objData.supportedTypes = geo->getSupportedFeatureTypes();
    
    // 为每种支持的Feature类型创建组节点
    for (FeatureType type : objData.supportedTypes)
    {
        objData.featureGroups[type] = new osg::Group;
        
        // 添加到对应的渲染组
        switch (type)
        {
            case FeatureType::FACE:
                m_faceGroup->addChild(objData.featureGroups[type]);
                break;
            case FeatureType::EDGE:
                m_edgeGroup->addChild(objData.featureGroups[type]);
                break;
            case FeatureType::VERTEX:
                m_vertexGroup->addChild(objData.featureGroups[type]);
                break;
        }
    }
    
    m_objectMap.emplace(objectID, std::move(objData));
    m_geoToIDMap[geo] = objectID;
    
    // 抽取Feature
    extractFeatures(objectID);
    
    LOG_INFO(QString("Added object %1 to picking system").arg(objectID), "拾取");
    return objectID;
}

void PickingSystem::removeObject(uint64_t objectID)
{
    if (!m_initialized)
        return;
    
    auto it = m_objectMap.find(objectID);
    if (it != m_objectMap.end())
    {
        const PickingObjectData& objData = it->second;
        
        // 从几何对象映射中删除
        if (objData.geometry)
        {
            m_geoToIDMap.erase(objData.geometry.get());
        }
        
        // 从渲染组中移除Feature组节点
        for (auto& pair : objData.featureGroups)
        {
            FeatureType type = pair.first;
            osg::Group* group = pair.second.get();
            
            switch (type)
            {
                case FeatureType::FACE:
                    m_faceGroup->removeChild(group);
                    break;
                case FeatureType::EDGE:
                    m_edgeGroup->removeChild(group);
                    break;
                case FeatureType::VERTEX:
                    m_vertexGroup->removeChild(group);
                    break;
            }
        }
        
        m_objectMap.erase(it);
    }
    
    LOG_INFO(QString("Removed object %1 from picking system").arg(objectID), "拾取");
}

void PickingSystem::removeObject(Geo3D* geo)
{
    if (!geo || !m_initialized)
        return;
    
    auto it = m_geoToIDMap.find(geo);
    if (it != m_geoToIDMap.end())
    {
        removeObject(it->second);
    }
}

void PickingSystem::updateObject(uint64_t objectID)
{
    if (!m_initialized)
        return;
    
    auto it = m_objectMap.find(objectID);
    if (it != m_objectMap.end())
    {
        Geo3D* geo = it->second.geometry.get();
        if (geo && geo->needsFeatureUpdate())
        {
            rebuildFeatureGeometry(objectID);
            geo->markFeatureUpdated();
        }
    }
}

void PickingSystem::updateObject(Geo3D* geo)
{
    if (!geo || !m_initialized)
        return;
    
    auto it = m_geoToIDMap.find(geo);
    if (it != m_geoToIDMap.end())
    {
        updateObject(it->second);
    }
}

void PickingSystem::clearAllObjects()
{
    if (!m_initialized)
        return;
    
    m_objectMap.clear();
    m_geoToIDMap.clear();
    
    m_faceGroup->removeChildren(0, m_faceGroup->getNumChildren());
    m_edgeGroup->removeChildren(0, m_edgeGroup->getNumChildren());
    m_vertexGroup->removeChildren(0, m_vertexGroup->getNumChildren());
    
    m_idArray.clear();
    
    LOG_INFO("Cleared all objects from picking system", "拾取");
}

void PickingSystem::extractFeatures(uint64_t objectID)
{
    auto it = m_objectMap.find(objectID);
    if (it == m_objectMap.end())
        return;
    
    PickingObjectData& objData = it->second;
    Geo3D* geo = objData.geometry.get();
    
    if (!geo)
        return;
    
    // 为每种支持的Feature类型构建几何体
    for (FeatureType type : objData.supportedTypes)
    {
        buildFeatureGeometry(objectID, type);
    }
    
    uploadIDBuffer();
}

void PickingSystem::buildFeatureGeometry(uint64_t objectID, FeatureType type)
{
    auto it = m_objectMap.find(objectID);
    if (it == m_objectMap.end())
        return;
    
    PickingObjectData& objData = it->second;
    Geo3D* geo = objData.geometry.get();
    
    if (!geo)
        return;
    
    // 从Geo3D获取指定类型的Feature
    std::vector<PickingFeature> features = geo->getFeatures(type);
    
    if (features.empty())
        return;
    
    // 清空原有的ID列表
    objData.featureIDs[type].clear();
    
    // 清空对应的Feature组
    osg::Group* featureGroup = objData.featureGroups[type].get();
    if (featureGroup)
    {
        featureGroup->removeChildren(0, featureGroup->getNumChildren());
    }
    
    // 为每个Feature创建ID和几何体
    for (size_t i = 0; i < features.size(); ++i)
    {
        const PickingFeature& feature = features[i];
        
        // 创建PickingID64
        PickingID64::TypeCode typeCode;
        switch (type)
        {
            case FeatureType::FACE:   typeCode = PickingID64::TYPE_FACE;   break;
            case FeatureType::EDGE:   typeCode = PickingID64::TYPE_EDGE;   break;
            case FeatureType::VERTEX: typeCode = PickingID64::TYPE_VERTEX; break;
            default: continue;
        }
        
        PickingID64 pickingID(objectID, typeCode, feature.index);
        objData.featureIDs[type].push_back(pickingID);
        
        // 添加Feature几何体到组中
        if (feature.geometry && featureGroup)
        {
            featureGroup->addChild(feature.geometry);
        }
    }
    
    QString featureTypeStr = (type == FeatureType::FACE ? "face" : 
                             type == FeatureType::EDGE ? "edge" : "vertex");
    LOG_DEBUG(QString("Built %1 %2 features for object %3").arg(features.size()).arg(featureTypeStr).arg(objectID), "拾取");
}

void PickingSystem::rebuildFeatureGeometry(uint64_t objectID)
{
    auto it = m_objectMap.find(objectID);
    if (it == m_objectMap.end())
        return;
    
    PickingObjectData& objData = it->second;
    
    // 重建所有支持的Feature类型
    for (FeatureType type : objData.supportedTypes)
    {
        buildFeatureGeometry(objectID, type);
    }
    
    uploadIDBuffer();
}

void PickingSystem::uploadIDBuffer()
{
    m_idArray.clear();
    
    // 遍历所有对象，收集所有Feature ID
    for (const auto& objPair : m_objectMap)
    {
        const PickingObjectData& objData = objPair.second;
        
        for (const auto& featurePair : objData.featureIDs)
        {
            for (const PickingID64& id : featurePair.second)
            {
                m_idArray.push_back(id.pack());
            }
        }
    }
    
    if (!m_idArray.empty())
    {
        // OSG 3.6.5: 创建UIntArray来存储ID数据
        osg::ref_ptr<osg::UIntArray> idArrayData = new osg::UIntArray;
        
        for (uint64_t id : m_idArray)
        {
            // 使用低32位，高32位如需要可以用额外纹理存储
            idArrayData->push_back(static_cast<unsigned int>(id & 0xFFFFFFFF));
        }
        
        // OSG 3.6.5: 直接设置数据到TextureBuffer
        m_idBufferData = idArrayData;
        m_idBuffer->setBufferData(m_idBufferData.get());
    }
}

PickingResult PickingSystem::pick(int mouseX, int mouseY, int sampleRadius)
{
    if (!m_initialized)
        return PickingResult();
    
    double currentTime = osg::Timer::instance()->time_s();
    if (currentTime - m_lastPickTime < m_pickFrequencyLimit)
        return PickingResult();
    
    m_lastPickTime = currentTime;
    
    osg::Timer_t startTime = osg::Timer::instance()->tick();
    
    renderPickingPass();
    
    std::vector<PickingCandidate> candidates = sampleRegion(mouseX, mouseY, sampleRadius);
    
    PickingCandidate bestCandidate = selectBestCandidate(candidates);
    
    PickingResult result;
    result.hasResult = bestCandidate.id.isValid();
    result.id = bestCandidate.id;
    result.worldPos = bestCandidate.worldPos;
    result.depth = bestCandidate.depth;
    result.screenX = bestCandidate.screenX;
    result.screenY = bestCandidate.screenY;
    
    if (result.hasResult)
    {
        auto it = m_objectMap.find(bestCandidate.id.objectID);
        if (it != m_objectMap.end())
        {
            result.geometry = it->second.geometry.get();
        }
    }
    
    osg::Timer_t endTime = osg::Timer::instance()->tick();
    double pickTime = osg::Timer::instance()->delta_s(startTime, endTime);
    
    m_avgPickTime = (m_avgPickTime * m_pickCount + pickTime) / (m_pickCount + 1);
    ++m_pickCount;
    
    if (m_debugMode)
    {
        LOG_DEBUG(QString("Pick completed in %1ms, avg: %2ms").arg(pickTime * 1000.0, 0, 'f', 2).arg(m_avgPickTime * 1000.0, 0, 'f', 2), "拾取");
    }
    
    return result;
}

void PickingSystem::renderPickingPass()
{
    if (!m_pickingCamera)
        return;
    
    // OSG 3.6.5: 触发拾取相机的渲染
    // 这会渲染拾取场景到FBO中
    osgViewer::Viewer viewer;
    viewer.setCamera(m_pickingCamera.get());
    viewer.setSceneData(m_pickingRoot.get());
    viewer.frame();
}

std::vector<PickingCandidate> PickingSystem::sampleRegion(int centerX, int centerY, int radius)
{
    std::vector<PickingCandidate> candidates;
    
    int minX = std::max(0, centerX - radius);
    int maxX = std::min(m_width - 1, centerX + radius);
    int minY = std::max(0, centerY - radius);
    int maxY = std::min(m_height - 1, centerY + radius);
    
    if (radius <= 3)
    {
        for (int y = minY; y <= maxY; ++y)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                PickingCandidate candidate = samplePixel(x, y);
                if (candidate.id.isValid())
                {
                    candidates.push_back(candidate);
                }
            }
        }
    }
    else
    {
        for (int y = centerY - 1; y <= centerY + 1; ++y)
        {
            for (int x = centerX - 1; x <= centerX + 1; ++x)
            {
                if (x >= 0 && x < m_width && y >= 0 && y < m_height)
                {
                    PickingCandidate candidate = samplePixel(x, y);
                    if (candidate.id.isValid())
                    {
                        candidates.push_back(candidate);
                    }
                }
            }
        }
        
        if (candidates.empty())
        {
            int step = std::max(1, radius / 4);
            for (int y = minY; y <= maxY; y += step)
            {
                for (int x = minX; x <= maxX; x += step)
                {
                    PickingCandidate candidate = samplePixel(x, y);
                    if (candidate.id.isValid())
                    {
                        candidates.push_back(candidate);
                    }
                }
            }
        }
    }
    
    return candidates;
}

PickingCandidate PickingSystem::samplePixel(int x, int y)
{
    PickingCandidate candidate;
    
    int flippedY = m_height - 1 - y;
    
    // OSG 3.5.6 兼容性：直接访问图像数据
    unsigned char colorData[4] = {0, 0, 0, 0};
    float depth = 1.0f;
    
    if (m_colorImage && m_colorImage->data() && 
        x >= 0 && x < m_colorImage->s() && flippedY >= 0 && flippedY < m_colorImage->t())
    {
        const unsigned char* data = m_colorImage->data();
        int index = (flippedY * m_colorImage->s() + x) * 4;
        colorData[0] = data[index + 0];
        colorData[1] = data[index + 1];
        colorData[2] = data[index + 2];
        colorData[3] = data[index + 3];
    }
    
    if (m_depthImage && m_depthImage->data() && 
        x >= 0 && x < m_depthImage->s() && flippedY >= 0 && flippedY < m_depthImage->t())
    {
        const float* depthData = reinterpret_cast<const float*>(m_depthImage->data());
        int index = flippedY * m_depthImage->s() + x;
        depth = depthData[index];
    }
    
    // OSG 3.5.6 兼容性：简化的颜色解码
    uint32_t objectID = colorData[0] | (colorData[1] << 8) | (colorData[2] << 16);
    float alphaValue = colorData[3] / 255.0f;
    
    if (objectID != 0)
    {
        // 根据alpha值确定类型
        PickingID64::TypeCode typeCode = PickingID64::TYPE_INVALID;
        if (alphaValue > 0.9f) // 接近1.0，面
            typeCode = PickingID64::TYPE_FACE;
        else if (alphaValue > 0.4f && alphaValue < 0.6f) // 接近0.5，边
            typeCode = PickingID64::TYPE_EDGE;
        else if (alphaValue > 0.2f && alphaValue < 0.3f) // 接近0.25，点
            typeCode = PickingID64::TYPE_VERTEX;
        
        candidate.id = PickingID64(objectID, typeCode, 0);
        candidate.depth = depth;
        candidate.screenX = x;
        candidate.screenY = y;
        candidate.worldPos = screenToWorld(x, y, depth);
    }
    
    return candidate;
}

PickingCandidate PickingSystem::selectBestCandidate(const std::vector<PickingCandidate>& candidates)
{
    if (candidates.empty())
        return PickingCandidate();
    
    std::vector<PickingCandidate> vertices, edges, faces;
    
    for (const auto& candidate : candidates)
    {
        switch (candidate.id.typeCode)
        {
        case PickingID64::TYPE_VERTEX:
            vertices.push_back(candidate);
            break;
        case PickingID64::TYPE_EDGE:
            edges.push_back(candidate);
            break;
        case PickingID64::TYPE_FACE:
            faces.push_back(candidate);
            break;
        }
    }
    
    if (!vertices.empty())
    {
        return *std::min_element(vertices.begin(), vertices.end());
    }
    else if (!edges.empty())
    {
        return *std::min_element(edges.begin(), edges.end());
    }
    else if (!faces.empty())
    {
        return *std::min_element(faces.begin(), faces.end());
    }
    
    return PickingCandidate();
}

glm::vec3 PickingSystem::screenToWorld(int x, int y, float depth) const
{
    if (!m_mainCamera)
        return glm::vec3(0.0f);
    
    float ndcX = 2.0f * (x + 0.5f) / m_width - 1.0f;
    float ndcY = 2.0f * (y + 0.5f) / m_height - 1.0f;
    float ndcZ = 2.0f * depth - 1.0f;
    
    osg::Matrix viewMatrix = m_mainCamera->getViewMatrix();
    osg::Matrix projMatrix = m_mainCamera->getProjectionMatrix();
    osg::Matrix vpMatrix = viewMatrix * projMatrix;
    osg::Matrix invVPMatrix = osg::Matrix::inverse(vpMatrix);
    
    osg::Vec4 ndcPos(ndcX, ndcY, ndcZ, 1.0f);
    osg::Vec4 worldPos = ndcPos * invVPMatrix;
    
    if (worldPos.w() != 0.0f)
    {
        worldPos /= worldPos.w();
    }
    
    return glm::vec3(worldPos.x(), worldPos.y(), worldPos.z());
}

void PickingSystem::dumpPickingBuffer(const std::string& filename) const
{
    if (!m_colorImage)
        return;
    
    osgDB::writeImageFile(*m_colorImage, filename);
}

// ============================================================================
// PickingSystemManager Implementation
// ============================================================================

PickingSystemManager& PickingSystemManager::getInstance()
{
    static PickingSystemManager instance;
    return instance;
}

PickingSystemManager::PickingSystemManager()
{
    m_pickingSystem = new PickingSystem;
}

PickingSystemManager::~PickingSystemManager()
{
}

bool PickingSystemManager::initialize(int width, int height)
{
    return m_pickingSystem->initialize(width, height);
}

void PickingSystemManager::setMainCamera(osg::Camera* camera)
{
    m_pickingSystem->syncWithMainCamera(camera);
}

uint64_t PickingSystemManager::addObject(Geo3D* geo)
{
    if (!geo)
        return 0;
    
    return m_pickingSystem->addObject(geo);
}

void PickingSystemManager::removeObject(Geo3D* geo)
{
    if (!geo)
        return;
    
    m_pickingSystem->removeObject(geo);
}

void PickingSystemManager::updateObject(Geo3D* geo)
{
    if (!geo)
        return;
    
    m_pickingSystem->updateObject(geo);
}

PickingResult PickingSystemManager::pick(int mouseX, int mouseY, int sampleRadius)
{
    return m_pickingSystem->pick(mouseX, mouseY, sampleRadius);
}

// ============================================================================
// PickingEventHandler Implementation
// ============================================================================

PickingEventHandler::PickingEventHandler()
    : m_pickingRadius(8)
    , m_pickingFrequency(60.0)
    , m_lastPickTime(0.0)
    , m_enabled(true)
    , m_lastX(-1)
    , m_lastY(-1)
{
}

PickingEventHandler::~PickingEventHandler()
{
}

bool PickingEventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    if (!m_enabled)
        return false;
    
    switch (ea.getEventType())
    {
    case osgGA::GUIEventAdapter::MOVE:
        {
            int x = static_cast<int>(ea.getX());
            int y = static_cast<int>(ea.getY());
            
            double currentTime = osg::Timer::instance()->time_s();
            double timeDelta = currentTime - m_lastPickTime;
            
            if (timeDelta >= (1.0 / m_pickingFrequency))
            {
                int dx = x - m_lastX;
                int dy = y - m_lastY;
                int distanceSquared = dx * dx + dy * dy;
                
                if (distanceSquared > 1 || timeDelta > 0.1)
                {
                    processPicking(x, y);
                    m_lastPickTime = currentTime;
                    m_lastX = x;
                    m_lastY = y;
                }
            }
            break;
        }
    default:
        break;
    }
    
    return false;
}

void PickingEventHandler::setPickingCallback(std::function<void(const PickingResult&)> callback)
{
    m_pickingCallback = callback;
}

void PickingEventHandler::processPicking(int x, int y)
{
    if (!m_pickingCallback)
        return;
    
    PickingResult result = PickingSystemManager::getInstance().pick(x, y, m_pickingRadius);
    m_pickingCallback(result);
} 