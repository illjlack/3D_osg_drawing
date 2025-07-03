#pragma once

#include "../core/Common3D.h"
#include <osg/Node>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/StateSet>
#include <memory>

// 拾取指示器工厂类
class IndicatorFactory
{
public:
    // 创建顶点指示器
    static osg::ref_ptr<osg::Node> createVertexIndicator(const osg::Vec3& position, float size = 0.05f, 
                                                         const Color3D& color = Color3D(1.0f, 1.0f, 0.0f, 1.0f));
    
    // 创建边指示器
    static osg::ref_ptr<osg::Node> createEdgeIndicator(const osg::Vec3& center, const osg::Vec3& direction, 
                                                       float size = 0.08f, const Color3D& color = Color3D(0.0f, 1.0f, 0.0f, 1.0f));
    
    // 创建面指示器
    static osg::ref_ptr<osg::Node> createFaceIndicator(const osg::Vec3& center, const osg::Vec3& normal, 
                                                       float size = 0.1f, const Color3D& color = Color3D(0.0f, 0.0f, 1.0f, 1.0f));
    
    // 创建体指示器
    static osg::ref_ptr<osg::Node> createVolumeIndicator(const osg::Vec3& center, float size = 0.15f, 
                                                         const Color3D& color = Color3D(1.0f, 0.0f, 1.0f, 1.0f));
    
    // 创建一般指示器
    static osg::ref_ptr<osg::Node> createGeneralIndicator(IndicatorType type, const osg::Vec3& position, 
                                                          float size, const Color3D& color);
    
    // 创建高亮指示器
    static osg::ref_ptr<osg::Node> createHighlightIndicator(FeatureType featureType, const osg::Vec3& position, 
                                                            float size, const Color3D& color);
    
    // 创建选择指示器
    static osg::ref_ptr<osg::Node> createSelectionIndicator(FeatureType featureType, const osg::Vec3& position, 
                                                            float size, const Color3D& color);
    
    // 创建动画指示器
    static osg::ref_ptr<osg::Node> createAnimatedIndicator(IndicatorType type, const osg::Vec3& position, 
                                                           float size, const Color3D& color);
    
    // 更新指示器属性
    static void updateIndicatorColor(osg::Node* indicator, const Color3D& color);
    static void updateIndicatorSize(osg::Node* indicator, float size);
    static void updateIndicatorPosition(osg::Node* indicator, const osg::Vec3& position);
    
    // 创建指示器几何体
    static osg::ref_ptr<osg::Geometry> createSphereGeometry(float radius, int segments = 16);
    static osg::ref_ptr<osg::Geometry> createBoxGeometry(float size);
    static osg::ref_ptr<osg::Geometry> createArrowGeometry(float length, float width);
    static osg::ref_ptr<osg::Geometry> createPlaneGeometry(float size);
    
    // 工具方法
    static osg::ref_ptr<osg::StateSet> createIndicatorStateSet(const Color3D& color, bool transparent = false);
    static osg::ref_ptr<osg::Material> createIndicatorMaterial(const Color3D& color);
    
    // 默认指示器设置
    struct DefaultSettings
    {
        static constexpr float VERTEX_SIZE = 0.05f;
        static constexpr float EDGE_SIZE = 0.08f;
        static constexpr float FACE_SIZE = 0.1f;
        static constexpr float VOLUME_SIZE = 0.15f;
        
        static const Color3D VERTEX_COLOR;
        static const Color3D EDGE_COLOR;
        static const Color3D FACE_COLOR;
        static const Color3D VOLUME_COLOR;
        static const Color3D HIGHLIGHT_COLOR;
        static const Color3D SELECTION_COLOR;
    };
    
private:
    IndicatorFactory() = default;
    
    // 辅助方法
    static osg::ref_ptr<osg::Geode> createIndicatorGeode(osg::ref_ptr<osg::Geometry> geometry, 
                                                         const Color3D& color, bool transparent = false);
    static void setupIndicatorTransform(osg::Node* node, const osg::Vec3& position);
}; 