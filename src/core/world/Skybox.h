#pragma once

#include "../Common3D.h"
#include <osg/Node>
#include <osg/TextureCubeMap>
#include <osg/Texture2D>
#include <osg/Vec4>
#include <string>

// 天空盒类
class Skybox
{
public:
    Skybox();
    ~Skybox();
    
    // 创建天空盒
    osg::ref_ptr<osg::Node> createSkybox(float size = 1000.0f);
    
    // 设置天空盒大小
    void setSize(float size);
    float getSize() const { return m_size; }
    
    // 根据坐标范围自动设置大小
    void setSizeFromRange(double minX, double maxX, double minY, double maxY, double minZ, double maxZ);
    
    // 设置天空盒中心位置（默认为原点）
    void setCenter(const osg::Vec3& center);
    osg::Vec3 getCenter() const { return m_center; }
    
    // 设置立方体贴图纹理
    void setCubeMapTexture(const std::string& positiveX, const std::string& negativeX,
                          const std::string& positiveY, const std::string& negativeY,
                          const std::string& positiveZ, const std::string& negativeZ);
    
    // 设置渐变天空盒
    void setGradientSkybox(const osg::Vec4& topColor, const osg::Vec4& bottomColor);
    
    // 设置纯色天空盒
    void setSolidColorSkybox(const osg::Vec4& color);
    
    // 启用/禁用天空盒
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    // 获取天空盒节点
    osg::ref_ptr<osg::Node> getSkyboxNode() const { return m_skyboxNode; }

private:
    osg::ref_ptr<osg::Node> createCubeMapSkybox(float size);
    osg::ref_ptr<osg::Node> createGradientSkybox(float size);
    osg::ref_ptr<osg::Node> createSolidColorSkybox(float size);
    
    osg::ref_ptr<osg::TextureCubeMap> createCubeMapTexture();
    osg::ref_ptr<osg::Texture2D> createGradientTexture();
    osg::ref_ptr<osg::Texture2D> createSolidColorTexture();

private:
    osg::ref_ptr<osg::Node> m_skyboxNode;
    osg::ref_ptr<osg::TextureCubeMap> m_cubeMapTexture;
    osg::ref_ptr<osg::Texture2D> m_gradientTexture;
    osg::ref_ptr<osg::Texture2D> m_solidColorTexture;
    
    std::string m_textureFiles[6];  // 六个面的纹理文件路径
    osg::Vec4 m_topColor;
    osg::Vec4 m_bottomColor;
    osg::Vec4 m_solidColor;
    
    bool m_enabled;
    bool m_useCubeMap;
    bool m_useGradient;
    bool m_useSolidColor;
    float m_size;
    osg::Vec3 m_center;  // 天空盒中心位置
}; 