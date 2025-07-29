#pragma once
#pragma execution_character_set("utf-8")

#include "Common3D.h"
#include <osg/Material>
#include <osg/StateSet>
#include <osg/BlendFunc>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/LineStipple>
#include <osg/AlphaFunc>
#include <osg/ref_ptr>

// 前向声明
class Geo3D;

/**
 * @brief 几何体渲染管理器
 * 专注于几何体的渲染属性设置（颜色、大小、透明度、线型等）
 * 选中状态显示由GeoNodeManager管理
 */
class GeoRenderManager
{
public:
    explicit GeoRenderManager(osg::ref_ptr<Geo3D> parent);
    ~GeoRenderManager() = default;


    void updateRenderingParameters(const GeoParameters3D& params);
    
    // 重新初始化渲染状态（用于从外部加载节点后）
    void reinitializeRenderStates();

private:
    void initializeRenderStates();
    void updatePointRendering(const GeoParameters3D& params);
    void updateLineRendering(const GeoParameters3D& params);
    void updateSurfaceRendering(const GeoParameters3D& params);
    void updateVisibilityStates(const GeoParameters3D& params);
    
    void applyLineStyle(LineStyle3D style, double dashPattern);
    osg::Vec4 colorToOsgVec4(const Color3D& color) const;
    
    void setupPointParameters(const GeoParameters3D& params, osg::StateSet* stateSet);
    void setupTriangleMeshParameters(const GeoParameters3D& params, osg::StateSet* stateSet);
    
    // 透明度渲染处理
    void setupTransparencyRendering(osg::StateSet* stateSet, float alpha);
    
private:
    osg::ref_ptr<Geo3D> m_parent;
    

    GeoParameters3D m_currentParams;
    
    osg::ref_ptr<osg::Material> m_pointMaterial;
    osg::ref_ptr<osg::Material> m_lineMaterial;
    osg::ref_ptr<osg::Material> m_surfaceMaterial;
    
    osg::ref_ptr<osg::Point> m_pointSize;
    osg::ref_ptr<osg::LineWidth> m_lineWidth;
    osg::ref_ptr<osg::LineStipple> m_lineStipple;
    osg::ref_ptr<osg::BlendFunc> m_blendFunc;
}; 

