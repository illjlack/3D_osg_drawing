#include "PickingSystemInterface.h"
#include "../../util/LogManager.h"
#include <osg/Timer>
#include <osgGA/GUIEventAdapter>

// ============================================================================
// UnifiedPickingResult 实现
// ============================================================================

UnifiedPickingResult UnifiedPickingResult::fromGPU(const PickingResult& gpuResult)
{
    UnifiedPickingResult result;
    result.hasResult = gpuResult.hasResult;
    result.geometry = gpuResult.geometry;
    result.worldPosition = gpuResult.worldPos;
    result.snapPosition = gpuResult.worldPos;  // GPU系统暂时不支持捕捉
    result.isSnapped = false;
    result.distance = gpuResult.depth;
    result.screenX = gpuResult.screenX;
    result.screenY = gpuResult.screenY;
    return result;
}

UnifiedPickingResult UnifiedPickingResult::fromCPU(const CPUPickingResult& cpuResult)
{
    UnifiedPickingResult result;
    result.hasResult = cpuResult.hasResult;
    result.geometry = cpuResult.geometry;
    result.worldPosition = cpuResult.worldPosition;
    result.snapPosition = cpuResult.snapPosition;
    result.isSnapped = cpuResult.isSnapped;
    result.distance = cpuResult.distance;
    result.screenX = cpuResult.screenX;
    result.screenY = cpuResult.screenY;
    return result;
}

// ============================================================================
// UnifiedPickingSystem 实现
// ============================================================================

UnifiedPickingSystem::UnifiedPickingSystem()
{
    m_gpuPickingSystem = new PickingSystem();
    m_cpuPickingSystem = new CPUPickingSystem();
}

UnifiedPickingSystem::~UnifiedPickingSystem()
{
}

bool UnifiedPickingSystem::initialize(osg::Camera* camera, osg::Group* sceneRoot, int width, int height)
{
    if (!camera || !sceneRoot) {
        LOG_ERROR("Invalid parameters for unified picking system initialization", "拾取");
        return false;
    }
    
    m_camera = camera;
    m_sceneRoot = sceneRoot;
    m_width = width;
    m_height = height;
    
    // 初始化GPU拾取系统
    if (!m_gpuPickingSystem->initialize(width, height)) {
        LOG_ERROR("Failed to initialize GPU picking system", "拾取");
        return false;
    }
    m_gpuPickingSystem->syncWithMainCamera(camera);
    
    // 初始化CPU拾取系统
    if (!m_cpuPickingSystem->initialize(camera, sceneRoot)) {
        LOG_ERROR("Failed to initialize CPU picking system", "拾取");
        return false;
    }
    
    // 初始化CPU拾取系统的指示器和高亮系统
    auto indicatorManager = new PickingIndicatorManager();
    if (indicatorManager->initialize()) {
        m_cpuPickingSystem->setIndicatorManager(indicatorManager);
        
        // 将指示器节点添加到场景图中
        if (indicatorManager->getIndicatorRoot()) {
            sceneRoot->addChild(indicatorManager->getIndicatorRoot());
            LOG_INFO("Added indicator root to scene graph", "拾取");
        }
        
        // 将高亮节点添加到场景图中  
        if (indicatorManager->getHighlightRoot()) {
            sceneRoot->addChild(indicatorManager->getHighlightRoot());
            LOG_INFO("Added highlight root to scene graph", "拾取");
        }
    }
    
    auto highlightSystem = new HighlightSystem();
    if (highlightSystem->initialize()) {
        m_cpuPickingSystem->setHighlightSystem(highlightSystem);
    }
    
    // 创建CPU事件处理器
    m_cpuEventHandler = new CPUPickingEventHandler(m_cpuPickingSystem.get());
    
    // 创建统一的事件处理器
    m_unifiedEventHandler = new UnifiedPickingEventHandler(this);
    
    m_initialized = true;
    
    LOG_SUCCESS("Unified picking system initialized successfully", "拾取");
    return true;
}

void UnifiedPickingSystem::setPickingSystemType(PickingSystemType type)
{
    if (m_currentSystemType == type) return;
    
    PickingSystemType oldType = m_currentSystemType;
    m_currentSystemType = type;
    
    switch (type) {
    case PickingSystemType::GPU_PICKING:
        switchToGPUPicking();
        break;
    case PickingSystemType::CPU_PICKING:
        switchToCPUPicking();
        break;
    }
    
    // 同步几何体数据
    syncGeometriesWithCurrentSystem();
    
    LOG_INFO(QString("Switched picking system from %1 to %2")
        .arg(oldType == PickingSystemType::GPU_PICKING ? "GPU" : "CPU")
        .arg(type == PickingSystemType::GPU_PICKING ? "GPU" : "CPU"), "拾取");
}

void UnifiedPickingSystem::addGeometry(Geo3D* geometry)
{
    if (!geometry) return;
    
    // 添加到几何体列表
    auto it = std::find(m_geometries.begin(), m_geometries.end(), geometry);
    if (it == m_geometries.end()) {
        m_geometries.push_back(geometry);
    }
    
    // 添加到当前活动的拾取系统
    if (m_currentSystemType == PickingSystemType::GPU_PICKING) {
        m_gpuPickingSystem->addObject(geometry);
    } else {
        m_cpuPickingSystem->addGeometry(geometry);
    }
}

void UnifiedPickingSystem::removeGeometry(Geo3D* geometry)
{
    if (!geometry) return;
    
    // 从几何体列表中移除
    auto it = std::find(m_geometries.begin(), m_geometries.end(), geometry);
    if (it != m_geometries.end()) {
        m_geometries.erase(it);
    }
    
    // 从两个系统中都移除
    m_gpuPickingSystem->removeObject(geometry);
    m_cpuPickingSystem->removeGeometry(geometry);
}

void UnifiedPickingSystem::updateGeometry(Geo3D* geometry)
{
    if (!geometry) return;
    
    // 更新当前活动的拾取系统
    if (m_currentSystemType == PickingSystemType::GPU_PICKING) {
        m_gpuPickingSystem->updateObject(geometry);
    } else {
        m_cpuPickingSystem->updateGeometry(geometry);
    }
}

void UnifiedPickingSystem::clearAllGeometries()
{
    m_geometries.clear();
    m_gpuPickingSystem->clearAllObjects();
    m_cpuPickingSystem->clearAllGeometries();
}

UnifiedPickingResult UnifiedPickingSystem::pick(int mouseX, int mouseY)
{
    if (!m_initialized) {
        LOG_ERROR("Unified picking system not initialized", "拾取");
        return UnifiedPickingResult();
    }
    
    if (m_currentSystemType == PickingSystemType::GPU_PICKING) {
        PickingResult gpuResult = m_gpuPickingSystem->pick(mouseX, mouseY);
        return UnifiedPickingResult::fromGPU(gpuResult);
    } else {
        CPUPickingResult cpuResult = m_cpuPickingSystem->pick(mouseX, mouseY);
        return UnifiedPickingResult::fromCPU(cpuResult);
    }
}

void UnifiedPickingSystem::setCPUPickingConfig(const CPUPickingConfig& config)
{
    if (m_cpuPickingSystem) {
        m_cpuPickingSystem->setConfig(config);
    }
}

void UnifiedPickingSystem::setGPUPickingEnabled(bool enabled)
{
    // 这里可以添加GPU拾取的启用/禁用逻辑
    // 目前GPU拾取系统没有直接的启用/禁用接口
}

osgGA::GUIEventHandler* UnifiedPickingSystem::getEventHandler()
{
    return m_unifiedEventHandler.get();
}

void UnifiedPickingSystem::setPickingCallback(std::function<void(const UnifiedPickingResult&)> callback)
{
    m_pickingCallback = callback;
    
    // 设置统一事件处理器的回调
    if (m_unifiedEventHandler) {
        auto unifiedHandler = dynamic_cast<UnifiedPickingEventHandler*>(m_unifiedEventHandler.get());
        if (unifiedHandler) {
            unifiedHandler->setPickingCallback(callback);
        }
    }
}

QString UnifiedPickingSystem::getSystemInfo() const
{
    QString info = QString("Current System: %1\n")
        .arg(m_currentSystemType == PickingSystemType::GPU_PICKING ? "GPU" : "CPU");
    
    info += QString("Geometries: %1\n").arg(m_geometries.size());
    
    if (m_currentSystemType == PickingSystemType::CPU_PICKING) {
        const auto& config = m_cpuPickingSystem->getConfig();
        info += QString("CPU Config - Radius: %1, Rays: %2, Threshold: %3\n")
            .arg(config.pickingRadius)
            .arg(config.rayCount)
            .arg(config.snapThreshold);
    }
    
    return info;
}

void UnifiedPickingSystem::switchToGPUPicking()
{
    // GPU拾取系统切换逻辑
    if (m_gpuPickingSystem) {
        // 重新同步相机
        m_gpuPickingSystem->syncWithMainCamera(m_camera.get());
    }
}

void UnifiedPickingSystem::switchToCPUPicking()
{
    // CPU拾取系统切换逻辑
    // 已经在初始化时完成
}

void UnifiedPickingSystem::syncGeometriesWithCurrentSystem()
{
    // 清空当前系统的几何体
    if (m_currentSystemType == PickingSystemType::GPU_PICKING) {
        m_gpuPickingSystem->clearAllObjects();
        // 重新添加所有几何体到GPU系统
        for (Geo3D* geo : m_geometries) {
            m_gpuPickingSystem->addObject(geo);
        }
    } else {
        m_cpuPickingSystem->clearAllGeometries();
        // 重新添加所有几何体到CPU系统
        for (Geo3D* geo : m_geometries) {
            m_cpuPickingSystem->addGeometry(geo);
        }
    }
}

// ============================================================================
// UnifiedPickingEventHandler 实现
// ============================================================================

UnifiedPickingEventHandler::UnifiedPickingEventHandler(UnifiedPickingSystem* pickingSystem)
    : m_pickingSystem(pickingSystem)
{
}

UnifiedPickingEventHandler::~UnifiedPickingEventHandler()
{
}

bool UnifiedPickingEventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    if (!m_enabled || !m_pickingSystem) return false;
    
    switch (ea.getEventType()) {
    case osgGA::GUIEventAdapter::MOVE:
        {
            int x = static_cast<int>(ea.getX());
            int y = static_cast<int>(ea.getY());
            
            double currentTime = osg::Timer::instance()->time_s();
            double timeDelta = currentTime - m_lastPickTime;
            
            if (timeDelta >= (1.0 / m_pickingFrequency)) {
                int dx = x - m_lastX;
                int dy = y - m_lastY;
                int distanceSquared = dx * dx + dy * dy;
                
                if (distanceSquared > 1 || timeDelta > 0.1) {
                    processPicking(x, y);
                    m_lastPickTime = currentTime;
                    m_lastX = x;
                    m_lastY = y;
                }
            }
            break;
        }
    case osgGA::GUIEventAdapter::KEYDOWN:
        {
            int key = ea.getKey();
            if (key == m_systemSwitchKey) {
                handleSystemSwitch();
                return true;
            }
            break;
        }
    default:
        break;
    }
    
    return false;
}

void UnifiedPickingEventHandler::setPickingCallback(std::function<void(const UnifiedPickingResult&)> callback)
{
    m_pickingCallback = callback;
}

void UnifiedPickingEventHandler::processPicking(int x, int y)
{
    if (!m_pickingCallback) return;
    
    UnifiedPickingResult result = m_pickingSystem->pick(x, y);
    m_pickingCallback(result);
}

void UnifiedPickingEventHandler::handleSystemSwitch()
{
    PickingSystemType currentType = m_pickingSystem->getPickingSystemType();
    PickingSystemType newType = (currentType == PickingSystemType::GPU_PICKING) ? 
        PickingSystemType::CPU_PICKING : PickingSystemType::GPU_PICKING;
    
    m_pickingSystem->setPickingSystemType(newType);
    
    LOG_INFO(QString("Switched to %1 picking system")
        .arg(newType == PickingSystemType::GPU_PICKING ? "GPU" : "CPU"), "拾取");
}

// ============================================================================
// UnifiedPickingSystemManager 实现
// ============================================================================

UnifiedPickingSystemManager& UnifiedPickingSystemManager::getInstance()
{
    static UnifiedPickingSystemManager instance;
    return instance;
}

UnifiedPickingSystemManager::UnifiedPickingSystemManager()
{
    m_pickingSystem = new UnifiedPickingSystem();
}

UnifiedPickingSystemManager::~UnifiedPickingSystemManager()
{
}

bool UnifiedPickingSystemManager::initialize(osg::Camera* camera, osg::Group* sceneRoot, int width, int height)
{
    return m_pickingSystem->initialize(camera, sceneRoot, width, height);
}

void UnifiedPickingSystemManager::setPickingSystemType(PickingSystemType type)
{
    if (m_pickingSystem) {
        m_pickingSystem->setPickingSystemType(type);
    }
}

PickingSystemType UnifiedPickingSystemManager::getPickingSystemType() const
{
    if (m_pickingSystem) {
        return m_pickingSystem->getPickingSystemType();
    }
    return PickingSystemType::GPU_PICKING;
}

void UnifiedPickingSystemManager::addGeometry(Geo3D* geometry)
{
    if (m_pickingSystem) {
        m_pickingSystem->addGeometry(geometry);
    }
}

void UnifiedPickingSystemManager::removeGeometry(Geo3D* geometry)
{
    if (m_pickingSystem) {
        m_pickingSystem->removeGeometry(geometry);
    }
}

void UnifiedPickingSystemManager::updateGeometry(Geo3D* geometry)
{
    if (m_pickingSystem) {
        m_pickingSystem->updateGeometry(geometry);
    }
}

UnifiedPickingResult UnifiedPickingSystemManager::pick(int mouseX, int mouseY)
{
    if (m_pickingSystem) {
        return m_pickingSystem->pick(mouseX, mouseY);
    }
    return UnifiedPickingResult();
}

osgGA::GUIEventHandler* UnifiedPickingSystemManager::getEventHandler()
{
    if (m_pickingSystem) {
        return m_pickingSystem->getEventHandler();
    }
    return nullptr;
}

void UnifiedPickingSystemManager::setPickingCallback(std::function<void(const UnifiedPickingResult&)> callback)
{
    if (m_pickingSystem) {
        m_pickingSystem->setPickingCallback(callback);
    }
}

void UnifiedPickingSystemManager::setCPUPickingConfig(const CPUPickingConfig& config)
{
    if (m_pickingSystem) {
        m_pickingSystem->setCPUPickingConfig(config);
    }
}

QString UnifiedPickingSystemManager::getSystemInfo() const
{
    if (m_pickingSystem) {
        return m_pickingSystem->getSystemInfo();
    }
    return QString("Picking system not initialized");
} 