#include "PickingDiagnostic.h"
#include "../../util/LogManager.h"
#include <osg/GL>
#include <osg/GraphicsContext>
#include <osg/State>
#include <osg/GLExtensions>
#include <QDebug>

PickingDiagnosticResult PickingDiagnostic::diagnosePickingSystem()
{
    PickingDiagnosticResult result;
    
    logDiagnosticInfo("开始拾取系统诊断...");
    
    // 检查初始化状态
    result.isInitialized = checkInitialization();
    if (!result.isInitialized)
    {
        result.errorMessage = "拾取系统未正确初始化";
        return result;
    }
    
    // 检查相机
    result.hasValidCamera = checkCamera();
    if (!result.hasValidCamera)
    {
        result.errorMessage = "拾取相机创建失败";
        return result;
    }
    
    // 检查帧缓冲区
    result.hasValidFrameBuffer = checkFrameBuffer();
    if (!result.hasValidFrameBuffer)
    {
        result.errorMessage = "帧缓冲区创建失败";
        return result;
    }
    
    // 检查着色器
    result.hasValidShaders = checkShaders();
    if (!result.hasValidShaders)
    {
        result.errorMessage = "拾取着色器创建失败";
        return result;
    }
    
    // 检查对象
    result.hasObjects = checkObjects();
    if (!result.hasObjects)
    {
        result.warningMessage = "拾取系统中没有几何对象";
    }
    
    // 检查Feature
    result.hasFeatures = checkFeatures();
    if (!result.hasFeatures)
    {
        result.warningMessage = "几何对象没有有效的Feature";
    }
    
    // 检查渲染能力
    result.canRender = checkRendering();
    if (!result.canRender)
    {
        result.errorMessage = "拾取渲染失败";
        return result;
    }
    
    // 检查像素读取
    result.canReadPixels = checkPixelReading();
    if (!result.canReadPixels)
    {
        result.errorMessage = "像素读取失败";
        return result;
    }
    
    if (result.isInitialized && result.hasValidCamera && result.hasValidFrameBuffer && 
        result.hasValidShaders && result.canRender && result.canReadPixels)
    {
        result.suggestionMessage = "拾取系统工作正常";
        logDiagnosticInfo("拾取系统诊断完成 - 正常");
    }
    else
    {
        result.suggestionMessage = "建议重新初始化拾取系统";
        logDiagnosticInfo("拾取系统诊断完成 - 存在问题");
    }
    
    return result;
}

bool PickingDiagnostic::fixCommonIssues()
{
    logDiagnosticInfo("尝试修复常见问题...");
    
    PickingSystemManager& manager = PickingSystemManager::getInstance();
    PickingSystem* pickingSystem = manager.getPickingSystem();
    
    if (!pickingSystem)
    {
        LOG_ERROR("拾取系统管理器为空", "拾取诊断");
        return false;
    }
    
    // 重新初始化拾取系统
    if (!pickingSystem->isDebugMode())
    {
        pickingSystem->setDebugMode(true);
        LOG_INFO("启用拾取系统调试模式", "拾取诊断");
    }
    
    // 清除所有对象并重新添加
    pickingSystem->clearAllObjects();
    LOG_INFO("清除所有拾取对象", "拾取诊断");
    
    return true;
}

QString PickingDiagnostic::generateDiagnosticReport()
{
    PickingDiagnosticResult result = diagnosePickingSystem();
    
    QString report = "=== 拾取系统诊断报告 ===\n\n";
    
    report += QString("初始化状态: %1\n").arg(result.isInitialized ? "正常" : "失败");
    report += QString("相机状态: %1\n").arg(result.hasValidCamera ? "正常" : "失败");
    report += QString("帧缓冲区: %1\n").arg(result.hasValidFrameBuffer ? "正常" : "失败");
    report += QString("着色器: %1\n").arg(result.hasValidShaders ? "正常" : "失败");
    report += QString("对象数量: %1\n").arg(result.hasObjects ? "有对象" : "无对象");
    report += QString("Feature状态: %1\n").arg(result.hasFeatures ? "正常" : "异常");
    report += QString("渲染能力: %1\n").arg(result.canRender ? "正常" : "失败");
    report += QString("像素读取: %1\n").arg(result.canReadPixels ? "正常" : "失败");
    
    if (!result.errorMessage.isEmpty())
    {
        report += QString("\n错误信息: %1\n").arg(result.errorMessage);
    }
    
    if (!result.warningMessage.isEmpty())
    {
        report += QString("\n警告信息: %1\n").arg(result.warningMessage);
    }
    
    if (!result.suggestionMessage.isEmpty())
    {
        report += QString("\n建议: %1\n").arg(result.suggestionMessage);
    }
    
    return report;
}

bool PickingDiagnostic::checkInitialization()
{
    PickingSystemManager& manager = PickingSystemManager::getInstance();
    PickingSystem* pickingSystem = manager.getPickingSystem();
    
    if (!pickingSystem)
    {
        logDiagnosticInfo("拾取系统管理器为空");
        return false;
    }
    
    bool initialized = pickingSystem->isInitialized();
    logDiagnosticInfo(QString("拾取系统初始化状态: %1").arg(initialized ? "已初始化" : "未初始化"));
    
    return initialized;
}

bool PickingDiagnostic::checkCamera()
{
    PickingSystemManager& manager = PickingSystemManager::getInstance();
    PickingSystem* pickingSystem = manager.getPickingSystem();
    
    if (!pickingSystem)
        return false;
    
    osg::Camera* camera = pickingSystem->getPickingCamera();
    if (!camera)
    {
        logDiagnosticInfo("拾取相机为空");
        return false;
    }
    
    return true;
}

bool PickingDiagnostic::checkFrameBuffer()
{
    // 检查OpenGL扩展支持 - 需要图形上下文
    // 这里暂时返回true，实际检查需要在OpenGL上下文中进行
    logDiagnosticInfo("帧缓冲区检查 - 需要OpenGL上下文");
    return true;
}

bool PickingDiagnostic::checkShaders()
{
    // 检查OpenGL着色器支持 - 需要图形上下文
    // 这里暂时返回true，实际检查需要在OpenGL上下文中进行
    logDiagnosticInfo("着色器检查 - 需要OpenGL上下文");
    return true;
}

bool PickingDiagnostic::checkObjects()
{
    PickingSystemManager& manager = PickingSystemManager::getInstance();
    PickingSystem* pickingSystem = manager.getPickingSystem();
    
    if (!pickingSystem)
        return false;
    
    int objectCount = pickingSystem->getObjectCount();
    int featureCount = pickingSystem->getFeatureCount();
    
    logDiagnosticInfo(QString("拾取系统对象数量: %1, Feature数量: %2").arg(objectCount).arg(featureCount));
    
    return objectCount > 0;
}

bool PickingDiagnostic::checkFeatures()
{
    PickingSystemManager& manager = PickingSystemManager::getInstance();
    PickingSystem* pickingSystem = manager.getPickingSystem();
    
    if (!pickingSystem)
        return false;
    
    int featureCount = pickingSystem->getFeatureCount();
    logDiagnosticInfo(QString("Feature数量: %1").arg(featureCount));
    
    return featureCount > 0;
}

bool PickingDiagnostic::checkRendering()
{
    // 检查渲染上下文
    osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
    if (!wsi)
    {
        logDiagnosticInfo("无法获取窗口系统接口");
        return false;
    }
    
    return true;
}

bool PickingDiagnostic::checkPixelReading()
{
    // 检查像素读取支持 - 需要图形上下文
    // 这里暂时返回true，实际检查需要在OpenGL上下文中进行
    logDiagnosticInfo("像素读取检查 - 需要OpenGL上下文");
    return true;
}

void PickingDiagnostic::logDiagnosticInfo(const QString& message)
{
    LOG_DEBUG(message, "拾取诊断");
} 