#include "LinuxRenderConfig.h"
#include "../../util/LogManager.h"
#include <GL/gl.h>
#include <cstring>
#include <string>
#include <algorithm>
#include <QString>

LinuxRenderConfig& LinuxRenderConfig::getInstance()
{
    static LinuxRenderConfig instance;
    return instance;
}

void LinuxRenderConfig::setRenderQuality(LinuxRenderQuality3D quality)
{
    if (m_currentQuality != quality) {
        m_currentQuality = quality;
        
        const char* qualityNames[] = {
            "性能优先", "平衡模式", "质量优先", "自动检测"
        };
        
        int index = static_cast<int>(quality) - static_cast<int>(Linux_Performance3D);
        if (index >= 0 && index < 4) {
            LOG_INFO(QString::fromStdString(std::string("Linux渲染质量设置为: ") + qualityNames[index]), "渲染配置");
        }
    }
}

bool LinuxRenderConfig::shouldUseSimplifiedTransparency() const
{
    return m_currentQuality == Linux_Performance3D || m_currentQuality == Linux_Balanced3D;
}

bool LinuxRenderConfig::shouldDisableLineSmooth() const
{
    return m_currentQuality == Linux_Performance3D || 
           (m_currentQuality == Linux_Balanced3D && isIntegratedGPU());
}

bool LinuxRenderConfig::shouldForceCullFaceOff() const
{
    return m_currentQuality == Linux_Performance3D || 
           (m_currentQuality == Linux_Balanced3D && isIntegratedGPU());
}

bool LinuxRenderConfig::shouldReduceKdTreeUpdates() const
{
    return m_currentQuality != Linux_Quality3D;
}

bool LinuxRenderConfig::shouldUseReducedMaterials() const
{
    return m_currentQuality == Linux_Performance3D;
}

float LinuxRenderConfig::getLineWidthMultiplier() const
{
    switch (m_currentQuality) {
        case Linux_Performance3D: return 0.7f;
        case Linux_Balanced3D: return 0.85f;
        case Linux_Quality3D: return 1.0f;
        default: return 0.85f;
    }
}

float LinuxRenderConfig::getPointSizeMultiplier() const
{
    switch (m_currentQuality) {
        case Linux_Performance3D: return 0.75f;
        case Linux_Balanced3D: return 0.9f;
        case Linux_Quality3D: return 1.0f;
        default: return 0.9f;
    }
}

void LinuxRenderConfig::autoDetectOptimalQuality()
{
    LOG_INFO("开始自动检测Linux平台最佳渲染质量", "渲染配置");
    
    std::string vendor = getGPUVendor();
    std::string renderer = getGPURenderer();
    
    LOG_INFO(QString::fromStdString("检测到GPU厂商: " + vendor), "渲染配置");
    LOG_INFO(QString::fromStdString("检测到GPU渲染器: " + renderer), "渲染配置");
    
    LinuxRenderQuality3D optimalQuality = Linux_Balanced3D;
    
    if (isHighPerformanceGPU()) {
        optimalQuality = Linux_Quality3D;
        LOG_INFO("检测到高性能GPU，使用质量优先模式", "渲染配置");
    } else if (isIntegratedGPU()) {
        optimalQuality = Linux_Performance3D;
        LOG_INFO("检测到集成GPU，使用性能优先模式", "渲染配置");
    } else {
        optimalQuality = Linux_Balanced3D;
        LOG_INFO("使用平衡模式", "渲染配置");
    }
    
    setRenderQuality(optimalQuality);
}

std::string LinuxRenderConfig::getGPUVendor() const
{
    const GLubyte* vendor = glGetString(GL_VENDOR);
    return vendor ? std::string(reinterpret_cast<const char*>(vendor)) : "Unknown";
}

std::string LinuxRenderConfig::getGPURenderer() const
{
    const GLubyte* renderer = glGetString(GL_RENDERER);
    return renderer ? std::string(reinterpret_cast<const char*>(renderer)) : "Unknown";
}

bool LinuxRenderConfig::isHighPerformanceGPU() const
{
    std::string vendor = getGPUVendor();
    std::string renderer = getGPURenderer();
    
    // 转换为小写以便比较
    std::transform(vendor.begin(), vendor.end(), vendor.begin(), ::tolower);
    std::transform(renderer.begin(), renderer.end(), renderer.begin(), ::tolower);
    
    // 检测NVIDIA高端显卡
    if (vendor.find("nvidia") != std::string::npos) {
        return renderer.find("rtx") != std::string::npos ||
               renderer.find("gtx") != std::string::npos ||
               renderer.find("titan") != std::string::npos ||
               renderer.find("quadro") != std::string::npos;
    }
    
    // 检测AMD高端显卡
    if (vendor.find("amd") != std::string::npos || vendor.find("ati") != std::string::npos) {
        return renderer.find("radeon") != std::string::npos ||
               renderer.find("rx") != std::string::npos ||
               renderer.find("vega") != std::string::npos ||
               renderer.find("navi") != std::string::npos;
    }
    
    return false;
}

bool LinuxRenderConfig::isIntegratedGPU() const
{
    std::string renderer = getGPURenderer();
    std::transform(renderer.begin(), renderer.end(), renderer.begin(), ::tolower);
    
    // 检测常见的集成GPU标识
    return renderer.find("intel") != std::string::npos ||
           renderer.find("integrated") != std::string::npos ||
           renderer.find("iris") != std::string::npos ||
           renderer.find("uhd") != std::string::npos ||
           renderer.find("hd graphics") != std::string::npos ||
           renderer.find("mesa") != std::string::npos ||
           renderer.find("llvmpipe") != std::string::npos;
} 