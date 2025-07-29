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

class GeometryBase {
public:
    GeometryBase();
    virtual ~GeometryBase();

    // 基本属性
    virtual void setDrawMode(DrawMode3D mode);
    virtual void setPointShape(PointShape3D shape);
    virtual void setPointSize(double size);
    virtual void setPointColor(const glm::dvec3& color);
    virtual void setLineStyle(LineStyle3D style);
    virtual void setLineWidth(double width);
    virtual void setLineColor(const glm::dvec3& color);
    virtual void setLineDashPattern(double pattern);
    virtual void setFillType(FillType3D type);
    virtual void setFillColor(const glm::dvec3& color);
    virtual void setMaterialType(MaterialType3D type);
    virtual void setSubdivisionLevel(SubdivisionLevel3D level);

    // 显示控制
    virtual void setShowPoints(bool show);
    virtual void setShowEdges(bool show);
    virtual void setShowFaces(bool show);

    // 获取属性
    virtual DrawMode3D getDrawMode() const;
    virtual PointShape3D getPointShape() const;
    virtual double getPointSize() const;
    virtual glm::dvec3 getPointColor() const;
    virtual LineStyle3D getLineStyle() const;
    virtual double getLineWidth() const;
    virtual glm::dvec3 getLineColor() const;
    virtual double getLineDashPattern() const;
    virtual FillType3D getFillType() const;
    virtual glm::dvec3 getFillColor() const;
    virtual MaterialType3D getMaterialType() const;
    virtual SubdivisionLevel3D getSubdivisionLevel() const;
    virtual bool getShowPoints() const;
    virtual bool getShowEdges() const;
    virtual bool getShowFaces() const;

    // OSG 节点访问
    virtual osg::Node* getNode() const;
    virtual osg::Geode* getGeode() const;
    virtual osg::Geometry* getGeometry() const;

protected:
    // OSG 节点
    osg::ref_ptr<osg::Geode> geode_;
    osg::ref_ptr<osg::Geometry> geometry_;

    // 状态
    DrawMode3D drawMode_;
    PointShape3D pointShape_;
    double pointSize_;
    glm::dvec3 pointColor_;
    LineStyle3D lineStyle_;
    double lineWidth_;
    glm::dvec3 lineColor_;
    double lineDashPattern_;
    FillType3D fillType_;
    glm::dvec3 fillColor_;
    MaterialType3D materialType_;
    SubdivisionLevel3D subdivisionLevel_;
    bool showPoints_;
    bool showEdges_;
    bool showFaces_;

    // 辅助函数
    virtual void updateDrawMode();
    virtual void updatePointAttributes();
    virtual void updateLineAttributes();
    virtual void updateFillAttributes();
    virtual void updateMaterial();
    virtual void updateVisibility();
};

} // namespace Geo3D

