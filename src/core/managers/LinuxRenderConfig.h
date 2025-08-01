#pragma once
#pragma execution_character_set("utf-8")

#include "../Enums3D.h"
#include <memory>

/**
 * @brief Linux平台渲染配置管理器
 * 专门处理Linux系统下的渲染优化和兼容性问题
 */
class LinuxRenderConfig
{
public:
    static LinuxRenderConfig& getInstance();
    
    // 设置和获取渲染质量
    void setRenderQuality(LinuxRenderQuality3D quality);
    LinuxRenderQuality3D getRenderQuality() const { return m_currentQuality; }
    
    // 根据当前质量设置获取具体的渲染参数
    bool shouldUseSimplifiedTransparency() const;
    bool shouldDisableLineSmooth() const;
    bool shouldForceCullFaceOff() const;
    bool shouldReduceKdTreeUpdates() const;
    bool shouldUseReducedMaterials() const;
    float getLineWidthMultiplier() const;
    float getPointSizeMultiplier() const;
    
    // 自动检测并设置最佳渲染质量
    void autoDetectOptimalQuality();
    
    // 获取GPU信息用于自动检测
    std::string getGPUVendor() const;
    std::string getGPURenderer() const;
    
private:
    LinuxRenderConfig() = default;
    LinuxRenderQuality3D m_currentQuality = Linux_Balanced3D;
    
    // GPU性能评估
    bool isHighPerformanceGPU() const;
    bool isIntegratedGPU() const;
}; 