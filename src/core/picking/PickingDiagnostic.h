#pragma once
#pragma execution_character_set("utf-8")

#include "PickingSystem.h"
#include "../GeometryBase.h"
#include <QString>
#include <vector>

// 拾取系统诊断结果
struct PickingDiagnosticResult
{
    bool isInitialized;
    bool hasValidCamera;
    bool hasValidFrameBuffer;
    bool hasValidShaders;
    bool hasObjects;
    bool hasFeatures;
    bool canRender;
    bool canReadPixels;
    
    QString errorMessage;
    QString warningMessage;
    QString suggestionMessage;
    
    PickingDiagnosticResult() : isInitialized(false), hasValidCamera(false), 
                               hasValidFrameBuffer(false), hasValidShaders(false),
                               hasObjects(false), hasFeatures(false), canRender(false), 
                               canReadPixels(false) {}
};

// 拾取系统诊断工具
class PickingDiagnostic
{
public:
    static PickingDiagnosticResult diagnosePickingSystem();
    static bool fixCommonIssues();
    static QString generateDiagnosticReport();
    
private:
    static bool checkInitialization();
    static bool checkCamera();
    static bool checkFrameBuffer();
    static bool checkShaders();
    static bool checkObjects();
    static bool checkFeatures();
    static bool checkRendering();
    static bool checkPixelReading();
    
    static void logDiagnosticInfo(const QString& message);
}; 