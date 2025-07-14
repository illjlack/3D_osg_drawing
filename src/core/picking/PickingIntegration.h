#pragma once
#pragma execution_character_set("utf-8")

#include "PickingSystem.h"
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/LineWidth>
#include <osg/Timer>
#include <osgViewer/Viewer>
#include <functional>
#include <map>

class Geo3D;

// 简化的指示器管理器
class SimplePickingIndicatorManager : public osg::Referenced
{
public:
    SimplePickingIndicatorManager();
    virtual ~SimplePickingIndicatorManager();
    
    bool initialize();
    void showIndicator(const PickingResult& result);
    void hideIndicator();
    void clearAll();
    void highlightObject(Geo3D* geo);
    void clearHighlight();
    void update(float deltaTime);
    
    osg::Group* getIndicatorRoot() const { return m_indicatorRoot.get(); }
    osg::Group* getHighlightRoot() const { return m_highlightRoot.get(); }
    
    void onPickingResult(const PickingResult& result);

private:
    void createIndicator(const PickingResult& result);
    osg::ref_ptr<osg::Geometry> createVertexIndicator(float size);
    osg::ref_ptr<osg::Geometry> createEdgeIndicator(float size);
    osg::ref_ptr<osg::Geometry> createFaceIndicator(float size);
    void createHighlight(Geo3D* geo);

private:
    osg::ref_ptr<osg::Group> m_indicatorRoot;
    osg::ref_ptr<osg::Group> m_highlightRoot;
    osg::ref_ptr<osg::MatrixTransform> m_currentIndicator;
    osg::ref_ptr<osg::Group> m_currentHighlight;
    
    PickingResult m_lastResult;
    bool m_indicatorVisible;
    float m_animationTime;
    Geo3D* m_highlightedObject;
};

// 拾取系统集成类
class PickingSystemIntegration
{
public:
    static bool initializePickingSystem(int width, int height);
    static void setMainCamera(osg::Camera* camera);
    static void addPickingEventHandler(osgViewer::Viewer* viewer, 
        std::function<void(const PickingResult&)> callback);
    static SimplePickingIndicatorManager* getIndicatorManager();
    static void addGeometry(Geo3D* geo);
    static void removeGeometry(Geo3D* geo);
    static void updateGeometry(Geo3D* geo);
    static void clearAllObjects();
    static PickingResult pick(int mouseX, int mouseY, int radius = 8);

private:
    static osg::ref_ptr<SimplePickingIndicatorManager> s_indicatorManager;
}; 