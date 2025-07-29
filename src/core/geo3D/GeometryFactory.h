#pragma once
#pragma execution_character_set("utf-8")

#include "GlobalState3D.h"
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

class GeometryFactory {
public:
    static osg::ref_ptr<osg::Geometry> createPoint(const glm::dvec3& position);
    static osg::ref_ptr<osg::Geometry> createLine(const glm::dvec3& start, const glm::dvec3& end);
    static osg::ref_ptr<osg::Geometry> createTriangle(const glm::dvec3& v1, const glm::dvec3& v2, const glm::dvec3& v3);
    static osg::ref_ptr<osg::Geometry> createQuad(const glm::dvec3& v1, const glm::dvec3& v2, const glm::dvec3& v3, const glm::dvec3& v4);
    static osg::ref_ptr<osg::Geometry> createPolygon(const std::vector<glm::dvec3>& vertices);
    static osg::ref_ptr<osg::Geometry> createBox(const glm::dvec3& min, const glm::dvec3& max);
    static osg::ref_ptr<osg::Geometry> createCube(const glm::dvec3& center, double size);
    static osg::ref_ptr<osg::Geometry> createSphere(const glm::dvec3& center, double radius, int segments = 32);
    static osg::ref_ptr<osg::Geometry> createCylinder(const glm::dvec3& base, const glm::dvec3& top, double radius, int segments = 32);
    static osg::ref_ptr<osg::Geometry> createCone(const glm::dvec3& base, const glm::dvec3& apex, double radius, int segments = 32);
    static osg::ref_ptr<osg::Geometry> createTorus(const glm::dvec3& center, double majorRadius, double minorRadius, int segments = 32);
    static osg::ref_ptr<osg::Geometry> createArc(const glm::dvec3& center, double radius, double startAngle, double endAngle, int segments = 32);
    static osg::ref_ptr<osg::Geometry> createBezierCurve(const std::vector<glm::dvec3>& controlPoints, int segments = 32);
    static osg::ref_ptr<osg::Geometry> createEllipsoid(const glm::dvec3& center, const glm::dvec3& radii, int segments = 32);
    static osg::ref_ptr<osg::Geometry> createHemisphere(const glm::dvec3& center, double radius, int segments = 32);
    static osg::ref_ptr<osg::Geometry> createPrism(const std::vector<glm::dvec3>& baseVertices, double height, int segments = 32);

protected:
    static osg::ref_ptr<osg::StateSet> createDefaultStateSet();
    static void setupDefaultAttributes(osg::ref_ptr<osg::Geometry> geometry);
};

} // namespace Geo3D

