#include "GeometryFactory.h"
#include "GeometryBase.h"
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/PolygonMode>
#include <osg/Material>
#include <osg/StateSet>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/LightModel>
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>
#include <osg/AutoTransform>
#include <osg/Billboard>
#include <osg/Texture2D>
#include <osg/TexGen>
#include <osg/TexEnv>
#include <osg/TexMat>
#include <osg/CullFace>
#include <osg/FrontFace>
#include <osg/LineStipple>
#include <osg/PolygonOffset>
#include <osg/PolygonStipple>
#include <osg/ShadeModel>
#include <osg/Stencil>
#include <osg/TexEnvCombine>
#include <osg/TexEnvFilter>
#include <osg/TexGenNode>
#include <osg/TextureCubeMap>
#include <osg/TextureRectangle>
#include <osg/AlphaFunc>
#include <osg/BlendColor>
#include <osg/BlendEquation>
#include <osg/ClipNode>
#include <osg/ClipPlane>
#include <osg/ColorMask>
#include <osg/ColorMatrix>
#include <osg/CullFace>
#include <osg/Depth>
#include <osg/Fog>
#include <osg/FragmentProgram>
#include <osg/FrontFace>
#include <osg/Light>
#include <osg/LightModel>
#include <osg/LightSource>
#include <osg/LineStipple>
#include <osg/LineWidth>
#include <osg/LogicOp>
#include <osg/Material>
#include <osg/Point>
#include <osg/PointSprite>
#include <osg/PolygonMode>
#include <osg/PolygonOffset>
#include <osg/PolygonStipple>
#include <osg/Program>
#include <osg/Shader>
#include <osg/ShadeModel>
#include <osg/Stencil>
#include <osg/TexEnv>
#include <osg/TexEnvCombine>
#include <osg/TexEnvFilter>
#include <osg/TexGen>
#include <osg/TexGenNode>
#include <osg/Texture>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/TextureCubeMap>
#include <osg/TextureRectangle>
#include <osg/VertexProgram>
#include <osg/Viewport>

namespace Geo3D {

osg::ref_ptr<osg::StateSet> GeometryFactory::createDefaultStateSet() {
    osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet();
    
    // 设置默认材质
    osg::ref_ptr<osg::Material> material = new osg::Material;
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0.2f, 0.2f, 0.2f, 1.0f));
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f));
    material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    material->setShininess(osg::Material::FRONT_AND_BACK, 0.0f);
    stateSet->setAttributeAndModes(material, osg::StateAttribute::ON);
    
    // 设置默认光照模型
    osg::ref_ptr<osg::LightModel> lightModel = new osg::LightModel;
    lightModel->setTwoSided(true);
    stateSet->setAttributeAndModes(lightModel, osg::StateAttribute::ON);
    
    // 设置默认多边形模式
    osg::ref_ptr<osg::PolygonMode> polygonMode = new osg::PolygonMode;
    polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
    stateSet->setAttributeAndModes(polygonMode, osg::StateAttribute::ON);
    
    // 设置默认点大小
    osg::ref_ptr<osg::Point> point = new osg::Point;
    point->setSize(5.0f);
    stateSet->setAttributeAndModes(point, osg::StateAttribute::ON);
    
    // 设置默认线宽
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth;
    lineWidth->setWidth(1.0f);
    stateSet->setAttributeAndModes(lineWidth, osg::StateAttribute::ON);
    
    return stateSet;
}

void GeometryFactory::setupDefaultAttributes(osg::ref_ptr<osg::Geometry> geometry) {
    // 设置默认状态集
    geometry->setStateSet(createDefaultStateSet());
    
    // 设置顶点属性绑定
    geometry->setUseDisplayList(false);
    geometry->setUseVertexBufferObjects(true);
    
    // 设置顶点数组
    geometry->setVertexArray(new osg::Vec3Array);
    geometry->setNormalArray(new osg::Vec3Array);
    geometry->setColorArray(new osg::Vec4Array);
    geometry->setTexCoordArray(0, new osg::Vec2Array);
    
    // 设置顶点属性绑定模式
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
}

} // namespace Geo3D 

