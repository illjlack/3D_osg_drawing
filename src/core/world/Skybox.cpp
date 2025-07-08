#include "Skybox.h"
#include "../../util/LogManager.h"
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/TextureCubeMap>
#include <osg/Texture2D>
#include <osg/Image>
#include <osg/Depth>
#include <osg/CullFace>
#include <osg/BlendFunc>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osg/Shader>
#include <osg/Program>
#include <osg/Uniform>
#include <osgDB/ReadFile>
#include <iostream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ========================================= Skybox 实现 =========================================

Skybox::Skybox()
    : m_skyboxNode(nullptr)
    , m_cubeMapTexture(nullptr)
    , m_gradientTexture(nullptr)
    , m_solidColorTexture(nullptr)
    , m_topColor(0.5f, 0.7f, 1.0f, 1.0f)      // 默认顶部颜色（天蓝色）
    , m_bottomColor(0.8f, 0.9f, 1.0f, 1.0f)   // 默认底部颜色（浅蓝色）
    , m_solidColor(0.2f, 0.2f, 0.2f, 1.0f)    // 默认纯色（深灰色）
    , m_enabled(true)
    , m_useCubeMap(false)
    , m_useGradient(true)
    , m_useSolidColor(false)
    , m_size(1000.0f)  // 默认大小改为1000，更适合坐标系统边界
    , m_center(0.0f, 0.0f, 0.0f)  // 默认以原点为中心
{
    // 默认使用渐变天空盒
    m_skyboxNode = createGradientSkybox(m_size);
}

Skybox::~Skybox()
{
}

osg::ref_ptr<osg::Node> Skybox::createSkybox(float size)
{
    if (m_useCubeMap)
    {
        return createCubeMapSkybox(size);
    }
    else if (m_useGradient)
    {
        return createGradientSkybox(size);
    }
    else if (m_useSolidColor)
    {
        return createSolidColorSkybox(size);
    }
    
    // 默认使用渐变天空盒
    return createGradientSkybox(size);
}

void Skybox::setCubeMapTexture(const std::string& positiveX, const std::string& negativeX,
                              const std::string& positiveY, const std::string& negativeY,
                              const std::string& positiveZ, const std::string& negativeZ)
{
    m_textureFiles[0] = positiveX;  // +X
    m_textureFiles[1] = negativeX;  // -X
    m_textureFiles[2] = positiveY;  // +Y
    m_textureFiles[3] = negativeY;  // -Y
    m_textureFiles[4] = positiveZ;  // +Z
    m_textureFiles[5] = negativeZ;  // -Z
    
    m_useCubeMap = true;
    m_useGradient = false;
    m_useSolidColor = false;
    
    if (m_enabled)
    {
        m_skyboxNode = createCubeMapSkybox(m_size);
    }
}

void Skybox::setGradientSkybox(const osg::Vec4& topColor, const osg::Vec4& bottomColor)
{
    m_topColor = topColor;
    m_bottomColor = bottomColor;
    
    m_useCubeMap = false;
    m_useGradient = true;
    m_useSolidColor = false;
    
    if (m_enabled)
    {
        m_skyboxNode = createGradientSkybox(m_size);
    }
}

void Skybox::setSolidColorSkybox(const osg::Vec4& color)
{
    m_solidColor = color;
    
    m_useCubeMap = false;
    m_useGradient = false;
    m_useSolidColor = true;
    
    if (m_enabled)
    {
        m_skyboxNode = createSolidColorSkybox(m_size);
    }
}

osg::ref_ptr<osg::Node> Skybox::createCubeMapSkybox(float size)
{
    // 创建立方体贴图纹理
    m_cubeMapTexture = createCubeMapTexture();
    if (!m_cubeMapTexture)
    {
        LOG_WARNING("Failed to create cube map texture, falling back to gradient skybox", "天空盒");
        return createGradientSkybox(size);
    }
    
    // 创建立方体几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec2Array> texCoords = new osg::Vec2Array();
    
    float halfSize = size * 0.5f;
    
    // 定义立方体的8个顶点（以指定中心为中心）
    std::vector<osg::Vec3> cubeVertices = {
        osg::Vec3(m_center.x() - halfSize, m_center.y() - halfSize, m_center.z() - halfSize), // 0: 左前下
        osg::Vec3(m_center.x() + halfSize, m_center.y() - halfSize, m_center.z() - halfSize), // 1: 右前下
        osg::Vec3(m_center.x() + halfSize, m_center.y() + halfSize, m_center.z() - halfSize), // 2: 右后下
        osg::Vec3(m_center.x() - halfSize, m_center.y() + halfSize, m_center.z() - halfSize), // 3: 左后下
        osg::Vec3(m_center.x() - halfSize, m_center.y() - halfSize, m_center.z() + halfSize), // 4: 左前上
        osg::Vec3(m_center.x() + halfSize, m_center.y() - halfSize, m_center.z() + halfSize), // 5: 右前上
        osg::Vec3(m_center.x() + halfSize, m_center.y() + halfSize, m_center.z() + halfSize), // 6: 右后上
        osg::Vec3(m_center.x() - halfSize, m_center.y() + halfSize, m_center.z() + halfSize)  // 7: 左后上
    };
    
    // 定义6个面的顶点索引（按照立方体贴图的标准顺序）
    // 立方体贴图面顺序：+X, -X, +Y, -Y, +Z, -Z
    std::vector<std::vector<int>> faces = {
        {1, 5, 6, 2}, // +X 面 (右面)
        {0, 3, 7, 4}, // -X 面 (左面)
        {3, 2, 6, 7}, // +Y 面 (后面)
        {0, 4, 5, 1}, // -Y 面 (前面)
        {4, 7, 6, 5}, // +Z 面 (上面)
        {0, 1, 2, 3}  // -Z 面 (下面)
    };
    
    // 每个面的纹理坐标（标准立方体贴图映射）
    std::vector<std::vector<osg::Vec2>> faceTexCoords = {
        {{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}}, // +X
        {{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}}, // -X
        {{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}}, // +Y
        {{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}}, // -Y
        {{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}}, // +Z
        {{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}}  // -Z
    };
    
    // 添加每个面的顶点和纹理坐标
    for (size_t f = 0; f < faces.size(); ++f)
    {
        const std::vector<int>& face = faces[f];
        const std::vector<osg::Vec2>& texCoord = faceTexCoords[f];
        
        // 三角化每个面 (四边形 -> 两个三角形)
        // 第一个三角形
        vertices->push_back(cubeVertices[face[0]]);
        vertices->push_back(cubeVertices[face[1]]);
        vertices->push_back(cubeVertices[face[2]]);
        
        texCoords->push_back(texCoord[0]);
        texCoords->push_back(texCoord[1]);
        texCoords->push_back(texCoord[2]);
        
        // 第二个三角形
        vertices->push_back(cubeVertices[face[0]]);
        vertices->push_back(cubeVertices[face[2]]);
        vertices->push_back(cubeVertices[face[3]]);
        
        texCoords->push_back(texCoord[0]);
        texCoords->push_back(texCoord[2]);
        texCoords->push_back(texCoord[3]);
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setTexCoordArray(0, texCoords.get());
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vertices->size()));
    
    // 创建Geode
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(geometry.get());
    
    // 设置渲染状态
    osg::ref_ptr<osg::StateSet> stateSet = geode->getOrCreateStateSet();
    
    // 设置立方体贴图纹理
    stateSet->setTextureAttributeAndModes(0, m_cubeMapTexture.get(), osg::StateAttribute::ON);
    
    // 设置材质
    osg::ref_ptr<osg::Material> material = new osg::Material();
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    stateSet->setAttribute(material.get());
    
    // 禁用光照
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    // 设置深度测试 - 天空盒应该始终在最远处渲染
    osg::ref_ptr<osg::Depth> depth = new osg::Depth();
    depth->setFunction(osg::Depth::LEQUAL);
    depth->setRange(1.0, 1.0);
    depth->setWriteMask(false); // 不写入深度缓冲区
    stateSet->setAttribute(depth.get());
    
    // 禁用背面剔除
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    
    // 设置渲染顺序 - 天空盒应该最先渲染
    stateSet->setRenderBinDetails(-1, "RenderBin");
    
    return geode;
}

osg::ref_ptr<osg::Node> Skybox::createGradientSkybox(float size)
{
    // 创建渐变纹理
    m_gradientTexture = createGradientTexture();
    
    // 创建立方体几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec2Array> texCoords = new osg::Vec2Array();
    
    float halfSize = size * 0.5f;
    
    // 定义立方体的8个顶点（以指定中心为中心）
    std::vector<osg::Vec3> cubeVertices = {
        osg::Vec3(m_center.x() - halfSize, m_center.y() - halfSize, m_center.z() - halfSize), // 0: 左前下
        osg::Vec3(m_center.x() + halfSize, m_center.y() - halfSize, m_center.z() - halfSize), // 1: 右前下
        osg::Vec3(m_center.x() + halfSize, m_center.y() + halfSize, m_center.z() - halfSize), // 2: 右后下
        osg::Vec3(m_center.x() - halfSize, m_center.y() + halfSize, m_center.z() - halfSize), // 3: 左后下
        osg::Vec3(m_center.x() - halfSize, m_center.y() - halfSize, m_center.z() + halfSize), // 4: 左前上
        osg::Vec3(m_center.x() + halfSize, m_center.y() - halfSize, m_center.z() + halfSize), // 5: 右前上
        osg::Vec3(m_center.x() + halfSize, m_center.y() + halfSize, m_center.z() + halfSize), // 6: 右后上
        osg::Vec3(m_center.x() - halfSize, m_center.y() + halfSize, m_center.z() + halfSize)  // 7: 左后上
    };
    
    // 定义6个面的顶点索引
    std::vector<std::vector<int>> faces = {
        {1, 5, 6, 2}, // 右面
        {0, 3, 7, 4}, // 左面
        {3, 2, 6, 7}, // 后面
        {0, 4, 5, 1}, // 前面
        {4, 7, 6, 5}, // 上面
        {0, 1, 2, 3}  // 下面
    };
    
    // 每个面的纹理坐标（渐变纹理映射）
    std::vector<std::vector<osg::Vec2>> faceTexCoords = {
        {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}, // 右面
        {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}, // 左面
        {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}, // 后面
        {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}, // 前面
        {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}, // 上面
        {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}  // 下面
    };
    
    // 添加每个面的顶点和纹理坐标
    for (size_t f = 0; f < faces.size(); ++f)
    {
        const std::vector<int>& face = faces[f];
        const std::vector<osg::Vec2>& texCoord = faceTexCoords[f];
        
        // 三角化每个面 (四边形 -> 两个三角形)
        // 第一个三角形
        vertices->push_back(cubeVertices[face[0]]);
        vertices->push_back(cubeVertices[face[1]]);
        vertices->push_back(cubeVertices[face[2]]);
        
        texCoords->push_back(texCoord[0]);
        texCoords->push_back(texCoord[1]);
        texCoords->push_back(texCoord[2]);
        
        // 第二个三角形
        vertices->push_back(cubeVertices[face[0]]);
        vertices->push_back(cubeVertices[face[2]]);
        vertices->push_back(cubeVertices[face[3]]);
        
        texCoords->push_back(texCoord[0]);
        texCoords->push_back(texCoord[2]);
        texCoords->push_back(texCoord[3]);
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setTexCoordArray(0, texCoords.get());
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vertices->size()));
    
    // 创建Geode
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(geometry.get());
    
    // 设置渲染状态
    osg::ref_ptr<osg::StateSet> stateSet = geode->getOrCreateStateSet();
    
    // 设置渐变纹理
    stateSet->setTextureAttributeAndModes(0, m_gradientTexture.get(), osg::StateAttribute::ON);
    
    // 设置材质
    osg::ref_ptr<osg::Material> material = new osg::Material();
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    stateSet->setAttribute(material.get());
    
    // 禁用光照
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    // 设置深度测试 - 天空盒应该始终在最远处渲染
    osg::ref_ptr<osg::Depth> depth = new osg::Depth();
    depth->setFunction(osg::Depth::LEQUAL);
    depth->setRange(1.0, 1.0);
    depth->setWriteMask(false); // 不写入深度缓冲区
    stateSet->setAttribute(depth.get());
    
    // 禁用背面剔除
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    
    // 设置渲染顺序 - 天空盒应该最先渲染
    stateSet->setRenderBinDetails(-1, "RenderBin");
    
    return geode;
}

osg::ref_ptr<osg::Node> Skybox::createSolidColorSkybox(float size)
{
    // 创建纯色纹理
    m_solidColorTexture = createSolidColorTexture();
    
    // 创建立方体几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec2Array> texCoords = new osg::Vec2Array();
    
    float halfSize = size * 0.5f;
    
    // 定义立方体的8个顶点（以指定中心为中心）
    std::vector<osg::Vec3> cubeVertices = {
        osg::Vec3(m_center.x() - halfSize, m_center.y() - halfSize, m_center.z() - halfSize), // 0: 左前下
        osg::Vec3(m_center.x() + halfSize, m_center.y() - halfSize, m_center.z() - halfSize), // 1: 右前下
        osg::Vec3(m_center.x() + halfSize, m_center.y() + halfSize, m_center.z() - halfSize), // 2: 右后下
        osg::Vec3(m_center.x() - halfSize, m_center.y() + halfSize, m_center.z() - halfSize), // 3: 左后下
        osg::Vec3(m_center.x() - halfSize, m_center.y() - halfSize, m_center.z() + halfSize), // 4: 左前上
        osg::Vec3(m_center.x() + halfSize, m_center.y() - halfSize, m_center.z() + halfSize), // 5: 右前上
        osg::Vec3(m_center.x() + halfSize, m_center.y() + halfSize, m_center.z() + halfSize), // 6: 右后上
        osg::Vec3(m_center.x() - halfSize, m_center.y() + halfSize, m_center.z() + halfSize)  // 7: 左后上
    };
    
    // 定义6个面的顶点索引
    std::vector<std::vector<int>> faces = {
        {1, 5, 6, 2}, // 右面
        {0, 3, 7, 4}, // 左面
        {3, 2, 6, 7}, // 后面
        {0, 4, 5, 1}, // 前面
        {4, 7, 6, 5}, // 上面
        {0, 1, 2, 3}  // 下面
    };
    
    // 每个面的纹理坐标
    std::vector<std::vector<osg::Vec2>> faceTexCoords = {
        {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}, // 右面
        {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}, // 左面
        {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}, // 后面
        {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}, // 前面
        {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}, // 上面
        {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}  // 下面
    };
    
    // 添加每个面的顶点和纹理坐标
    for (size_t f = 0; f < faces.size(); ++f)
    {
        const std::vector<int>& face = faces[f];
        const std::vector<osg::Vec2>& texCoord = faceTexCoords[f];
        
        // 三角化每个面 (四边形 -> 两个三角形)
        // 第一个三角形
        vertices->push_back(cubeVertices[face[0]]);
        vertices->push_back(cubeVertices[face[1]]);
        vertices->push_back(cubeVertices[face[2]]);
        
        texCoords->push_back(texCoord[0]);
        texCoords->push_back(texCoord[1]);
        texCoords->push_back(texCoord[2]);
        
        // 第二个三角形
        vertices->push_back(cubeVertices[face[0]]);
        vertices->push_back(cubeVertices[face[2]]);
        vertices->push_back(cubeVertices[face[3]]);
        
        texCoords->push_back(texCoord[0]);
        texCoords->push_back(texCoord[2]);
        texCoords->push_back(texCoord[3]);
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setTexCoordArray(0, texCoords.get());
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vertices->size()));
    
    // 创建Geode
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(geometry.get());
    
    // 设置渲染状态
    osg::ref_ptr<osg::StateSet> stateSet = geode->getOrCreateStateSet();
    
    // 设置纯色纹理
    stateSet->setTextureAttributeAndModes(0, m_solidColorTexture.get(), osg::StateAttribute::ON);
    
    // 设置材质
    osg::ref_ptr<osg::Material> material = new osg::Material();
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    stateSet->setAttribute(material.get());
    
    // 禁用光照
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    // 设置深度测试 - 天空盒应该始终在最远处渲染
    osg::ref_ptr<osg::Depth> depth = new osg::Depth();
    depth->setFunction(osg::Depth::LEQUAL);
    depth->setRange(1.0, 1.0);
    depth->setWriteMask(false); // 不写入深度缓冲区
    stateSet->setAttribute(depth.get());
    
    // 禁用背面剔除
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    
    // 设置渲染顺序 - 天空盒应该最先渲染
    stateSet->setRenderBinDetails(-1, "RenderBin");
    
    return geode;
}

osg::ref_ptr<osg::TextureCubeMap> Skybox::createCubeMapTexture()
{
    osg::ref_ptr<osg::TextureCubeMap> texture = new osg::TextureCubeMap();
    
    // 设置纹理参数
    texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    
    // 加载六个面的纹理
    for (int i = 0; i < 6; ++i)
    {
        if (!m_textureFiles[i].empty())
        {
            osg::ref_ptr<osg::Image> image = osgDB::readImageFile(m_textureFiles[i]);
            if (image)
            {
                texture->setImage(i, image.get());
            }
            else
            {
                LOG_ERROR(QString("Failed to load texture: %1").arg(QString::fromStdString(m_textureFiles[i])), "天空盒");
                return nullptr;
            }
        }
        else
        {
            // 如果没有提供纹理文件，创建默认的纯色纹理
            osg::ref_ptr<osg::Image> image = new osg::Image();
            image->allocateImage(256, 256, 1, GL_RGBA, GL_UNSIGNED_BYTE);
            
            // 设置默认颜色
            osg::Vec4 color(0.5f, 0.7f, 1.0f, 1.0f);
            unsigned char* data = image->data();
            for (int j = 0; j < 256 * 256; ++j)
            {
                data[j * 4 + 0] = static_cast<unsigned char>(color.r() * 255);
                data[j * 4 + 1] = static_cast<unsigned char>(color.g() * 255);
                data[j * 4 + 2] = static_cast<unsigned char>(color.b() * 255);
                data[j * 4 + 3] = static_cast<unsigned char>(color.a() * 255);
            }
            
            texture->setImage(i, image.get());
        }
    }
    
    return texture;
}

osg::ref_ptr<osg::Texture2D> Skybox::createGradientTexture()
{
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D();
    
    // 设置纹理参数
    texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    
    // 创建渐变纹理
    osg::ref_ptr<osg::Image> image = new osg::Image();
    int width = 256;
    int height = 256;
    image->allocateImage(width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    
    unsigned char* data = image->data();
    for (int y = 0; y < height; ++y)
    {
        float t = static_cast<float>(y) / (height - 1);
        
        // 在顶部和底部颜色之间插值
        osg::Vec4 color = m_topColor * (1.0f - t) + m_bottomColor * t;
        
        for (int x = 0; x < width; ++x)
        {
            int index = (y * width + x) * 4;
            data[index + 0] = static_cast<unsigned char>(color.r() * 255);
            data[index + 1] = static_cast<unsigned char>(color.g() * 255);
            data[index + 2] = static_cast<unsigned char>(color.b() * 255);
            data[index + 3] = static_cast<unsigned char>(color.a() * 255);
        }
    }
    
    texture->setImage(image.get());
    return texture;
}

osg::ref_ptr<osg::Texture2D> Skybox::createSolidColorTexture()
{
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D();
    
    // 设置纹理参数
    texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    
    // 创建纯色纹理
    osg::ref_ptr<osg::Image> image = new osg::Image();
    image->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    
    unsigned char* data = image->data();
    data[0] = static_cast<unsigned char>(m_solidColor.r() * 255);
    data[1] = static_cast<unsigned char>(m_solidColor.g() * 255);
    data[2] = static_cast<unsigned char>(m_solidColor.b() * 255);
    data[3] = static_cast<unsigned char>(m_solidColor.a() * 255);
    
    texture->setImage(image.get());
    return texture;
} 

void Skybox::setSize(float size)
{
    if (m_size != size && size > 0)
    {
        m_size = size;
        // 重新创建天空盒
        if (m_useCubeMap)
        {
            m_skyboxNode = createCubeMapSkybox(m_size);
        }
        else if (m_useGradient)
        {
            m_skyboxNode = createGradientSkybox(m_size);
        }
        else if (m_useSolidColor)
        {
            m_skyboxNode = createSolidColorSkybox(m_size);
        }
    }
}

void Skybox::setSizeFromRange(double minX, double maxX, double minY, double maxY, double minZ, double maxZ)
{
    // 计算范围的最大值
    double rangeX = std::abs(maxX - minX);
    double rangeY = std::abs(maxY - minY);
    double rangeZ = std::abs(maxZ - minZ);
    
    // 取最大值作为天空盒大小，并添加适当的边距
    double maxRange = std::max({rangeX, rangeY, rangeZ});
    
    // 对于坐标系统边界，使用更合理的边距比例
    // 如果范围很小（<1000），使用较大的边距比例
    // 如果范围很大（>10000），使用较小的边距比例
    double marginRatio;
    if (maxRange < 1000.0)
    {
        marginRatio = 0.5; // 50%边距，适合小范围
    }
    else if (maxRange < 10000.0)
    {
        marginRatio = 0.3; // 30%边距，适合中等范围
    }
    else if (maxRange < 100000.0)
    {
        marginRatio = 0.2; // 20%边距，适合大范围
    }
    else
    {
        marginRatio = 0.1; // 10%边距，适合超大范围
    }
    
    float skyboxSize = static_cast<float>(maxRange * (1.0 + marginRatio));
    
    // 确保天空盒大小在合理范围内
    skyboxSize = std::max(skyboxSize, 100.0f);  // 最小100单位
    skyboxSize = std::min(skyboxSize, 1e6f);    // 最大1e6单位，避免过大
    
    // 对于坐标系统边界，确保天空盒不会过大
    if (skyboxSize > maxRange * 3.0)
    {
        skyboxSize = static_cast<float>(maxRange * 3.0);
    }
    
    setSize(skyboxSize);
}

void Skybox::setCenter(const osg::Vec3& center)
{
    if (m_center != center)
    {
        m_center = center;
        
        // 重新创建天空盒以应用新的中心位置
        if (m_useCubeMap)
        {
            m_skyboxNode = createCubeMapSkybox(m_size);
        }
        else if (m_useGradient)
        {
            m_skyboxNode = createGradientSkybox(m_size);
        }
        else if (m_useSolidColor)
        {
            m_skyboxNode = createSolidColorSkybox(m_size);
        }
    }
}