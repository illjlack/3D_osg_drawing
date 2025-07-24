#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include "../GeometryBase.h"
#include "Skybox.h"
#include "CoordinateSystem3D.h"
#include "CoordinateSystemRenderer.h"
#include "../camera/CameraController.h"
#include "../picking/PickingIndicator.h"
#include "../picking/GeometryPickingSystem.h"
#include <osg/Group>
#include <osg/LightSource>
#include <memory>
#include <vector>
#include "../GeometryBase.h"

class osgViewer::Viewer;

class SceneManager3D
{
public:
    SceneManager3D();
    ~SceneManager3D();
    
    // 场景初始化
    bool initializeScene(osgViewer::Viewer* viewer);
    osg::Group* getRootNode() const { return m_rootNode.get(); }
    
    // 几何体管理
    void addGeometry(Geo3D::Ptr geo);
    void removeGeometry(Geo3D::Ptr geo);
    void removeAllGeometries();
    const std::vector<Geo3D::Ptr>& getAllGeometries() const { return m_geometries; }
    
    // 选择管理
    void setSelectedGeometry(Geo3D::Ptr geo);
    void addToSelection(Geo3D::Ptr geo);
    void removeFromSelection(Geo3D::Ptr geo);
    void clearSelection();
    Geo3D::Ptr getSelectedGeometry() const { return m_selectedGeometry; }
    const std::vector<Geo3D::Ptr>& getSelectedGeometries() const { return m_selectedGeometries; }
    bool isSelected(Geo3D::Ptr geo) const;
    int getSelectionCount() const { return static_cast<int>(m_selectedGeometries.size()); }
    
    // 拾取系统
    PickResult performPicking(int mouseX, int mouseY);
    PickingIndicator* getPickingIndicator() const { return m_pickingIndicator.get(); }
    
    // 显示模式
    void setWireframeMode(bool wireframe);
    void setShadedMode(bool shaded);
    void setPointMode(bool point);
    
    // 天空盒
    void enableSkybox(bool enabled);
    bool isSkyboxEnabled() const { return m_skyboxEnabled; }
    void setSkyboxGradient(const osg::Vec4& topColor, const osg::Vec4& bottomColor);
    void setSkyboxSolidColor(const osg::Vec4& color);
    void setSkyboxCubeMap(const std::string& positiveX, const std::string& negativeX,
                         const std::string& positiveY, const std::string& negativeY,
                         const std::string& positiveZ, const std::string& negativeZ);
    void refreshSkybox();
    
    // 坐标系
    void enableCoordinateSystem(bool enabled);
    bool isCoordinateSystemEnabled() const { return m_coordinateSystemEnabled; }
    void refreshCoordinateSystem();
    
    // 相机操作（需要相机控制器支持）
    void resetCamera();
    void fitAll();
    void setViewDirection(const glm::dvec3& direction, const glm::dvec3& up = glm::dvec3(0, 0, 1));
    
    // 绘制管理
    Geo3D::Ptr startDrawing(DrawMode3D mode);
    Geo3D::Ptr completeDrawing();
    void cancelDrawing();
    void updateDrawingPreview(const glm::dvec3& worldPos);
    bool isDrawing() const { return m_isDrawing; }
    Geo3D::Ptr getCurrentDrawingGeometry() const { return m_currentDrawingGeometry.get(); }
    
    // 控制点拖动
    void startDraggingControlPoint(Geo3D::Ptr geo, int controlPointIndex);
    void stopDraggingControlPoint();
    bool isDraggingControlPoint() const { return m_isDraggingControlPoint; }
    Geo3D::Ptr getDraggingGeometry() const { return m_draggingGeometry; }
    int getDraggingControlPointIndex() const { return m_draggingControlPointIndex; }
    void updateDraggingControlPoint(const glm::dvec3& worldPos);

private:
    // 初始化方法
    void setupSceneGraph();
    void setupLighting();
    void setupPickingSystem(osgViewer::Viewer* viewer);
    void setupSkybox();
    void setupCoordinateSystem();
    void setupRenderingStates();
    
private:
    // 场景图节点
    osg::ref_ptr<osg::Group> m_rootNode;
    osg::ref_ptr<osg::Group> m_sceneNode;
    osg::ref_ptr<osg::Group> m_geometryNode;
    osg::ref_ptr<osg::Group> m_lightNode;
    osg::ref_ptr<osg::Group> m_pickingIndicatorNode;
    osg::ref_ptr<osg::Group> m_skyboxNode;
    
    // 几何体管理
    std::vector<Geo3D::Ptr> m_geometries;
    Geo3D::Ptr m_selectedGeometry;
    std::vector<Geo3D::Ptr> m_selectedGeometries;
    
    // 绘制状态
    bool m_isDrawing;
    Geo3D::Ptr m_currentDrawingGeometry;
    
    // 控制点拖动
    bool m_isDraggingControlPoint;
    Geo3D::Ptr m_draggingGeometry;
    int m_draggingControlPointIndex;
    
    // 拾取系统
    osg::ref_ptr<PickingIndicator> m_pickingIndicator;
    osg::ref_ptr<GeometryPickingSystem> m_geometryPickingSystem;
    
    // 天空盒
    std::unique_ptr<Skybox> m_skybox;
    bool m_skyboxEnabled;
    
    // 坐标系
    std::unique_ptr<CoordinateSystemRenderer> m_coordinateSystemRenderer;
    bool m_coordinateSystemEnabled;
}; 
