#pragma once
#pragma execution_character_set("utf-8")

#include "CPUPickingSystem.h"
#include "PickingSystem.h"
#include "../Common3D.h"
#include <osg/Camera>
#include <osg/Group>
#include <osgGA/GUIEventHandler>
#include <functional>

// 拾取系统类型
enum class PickingSystemType
{
    GPU_PICKING,
    CPU_PICKING
};

// 统一的拾取结果
struct UnifiedPickingResult
{
    bool hasResult = false;
    Geo3D* geometry = nullptr;
    glm::vec3 worldPosition{0.0f};
    glm::vec3 snapPosition{0.0f};
    bool isSnapped = false;
    float distance = FLT_MAX;
    int screenX = 0;
    int screenY = 0;
    
    // 从GPU拾取结果转换
    static UnifiedPickingResult fromGPU(const PickingResult& gpuResult);
    
    // 从CPU拾取结果转换
    static UnifiedPickingResult fromCPU(const CPUPickingResult& cpuResult);
};

// 统一的拾取系统接口
class UnifiedPickingSystem : public osg::Referenced
{
public:
    UnifiedPickingSystem();
    virtual ~UnifiedPickingSystem();
    
    // 初始化
    bool initialize(osg::Camera* camera, osg::Group* sceneRoot, int width, int height);
    
    // 系统切换
    void setPickingSystemType(PickingSystemType type);
    PickingSystemType getPickingSystemType() const { return m_currentSystemType; }
    
    // 几何体管理
    void addGeometry(Geo3D* geometry);
    void removeGeometry(Geo3D* geometry);
    void updateGeometry(Geo3D* geometry);
    void clearAllGeometries();
    
    // 拾取操作
    UnifiedPickingResult pick(int mouseX, int mouseY);
    
    // 配置
    void setCPUPickingConfig(const CPUPickingConfig& config);
    void setGPUPickingEnabled(bool enabled);
    
    // 事件处理器
    osgGA::GUIEventHandler* getEventHandler();
    void setPickingCallback(std::function<void(const UnifiedPickingResult&)> callback);
    
    // 调试信息
    QString getSystemInfo() const;
    
private:
    void switchToGPUPicking();
    void switchToCPUPicking();
    void syncGeometriesWithCurrentSystem();
    
private:
    PickingSystemType m_currentSystemType = PickingSystemType::GPU_PICKING;
    
    // GPU拾取系统
    osg::ref_ptr<PickingSystem> m_gpuPickingSystem;
    
    // CPU拾取系统
    osg::ref_ptr<CPUPickingSystem> m_cpuPickingSystem;
    osg::ref_ptr<CPUPickingEventHandler> m_cpuEventHandler;
    
    // 统一的事件处理器
    osg::ref_ptr<osgGA::GUIEventHandler> m_unifiedEventHandler;
    
    // 系统状态
    osg::ref_ptr<osg::Camera> m_camera;
    osg::ref_ptr<osg::Group> m_sceneRoot;
    int m_width = 0;
    int m_height = 0;
    bool m_initialized = false;
    
    // 几何体列表
    std::vector<Geo3D*> m_geometries;
    
    // 回调函数
    std::function<void(const UnifiedPickingResult&)> m_pickingCallback;
};

// 统一的拾取事件处理器
class UnifiedPickingEventHandler : public osgGA::GUIEventHandler
{
public:
    UnifiedPickingEventHandler(UnifiedPickingSystem* pickingSystem);
    virtual ~UnifiedPickingEventHandler();
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override;
    
    void setPickingCallback(std::function<void(const UnifiedPickingResult&)> callback);
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
    // 键盘快捷键处理
    void setSystemSwitchKey(int key) { m_systemSwitchKey = key; }
    
private:
    void processPicking(int x, int y);
    void handleSystemSwitch();
    
private:
    osg::ref_ptr<UnifiedPickingSystem> m_pickingSystem;
    std::function<void(const UnifiedPickingResult&)> m_pickingCallback;
    
    bool m_enabled = true;
    int m_systemSwitchKey = 'p';  // 默认使用P键切换系统
    
    // 频率控制
    float m_pickingFrequency = 60.0f;
    double m_lastPickTime = 0.0;
    int m_lastX = -1;
    int m_lastY = -1;
};

// 单例管理器
class UnifiedPickingSystemManager
{
public:
    static UnifiedPickingSystemManager& getInstance();
    
    bool initialize(osg::Camera* camera, osg::Group* sceneRoot, int width, int height);
    
    // 系统切换
    void setPickingSystemType(PickingSystemType type);
    PickingSystemType getPickingSystemType() const;
    
    // 几何体管理
    void addGeometry(Geo3D* geometry);
    void removeGeometry(Geo3D* geometry);
    void updateGeometry(Geo3D* geometry);
    
    // 拾取操作
    UnifiedPickingResult pick(int mouseX, int mouseY);
    
    // 事件处理
    osgGA::GUIEventHandler* getEventHandler();
    void setPickingCallback(std::function<void(const UnifiedPickingResult&)> callback);
    
    // 配置
    void setCPUPickingConfig(const CPUPickingConfig& config);
    QString getSystemInfo() const;
    
private:
    UnifiedPickingSystemManager();
    ~UnifiedPickingSystemManager();
    
    UnifiedPickingSystemManager(const UnifiedPickingSystemManager&) = delete;
    UnifiedPickingSystemManager& operator=(const UnifiedPickingSystemManager&) = delete;
    
    osg::ref_ptr<UnifiedPickingSystem> m_pickingSystem;
}; 