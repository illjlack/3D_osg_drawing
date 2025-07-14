#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <osg/Material>
#include <osg/StateSet>
#include <osg/BlendFunc>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/ref_ptr>
#include <QObject>

// 前向声明
class Geo3D;

/**
 * @brief 材质管理器
 * 负责管理几何对象的材质、渲染状态和外观属性
 */
class GeoMaterialManager : public QObject
{
    Q_OBJECT

public:
    explicit GeoMaterialManager(Geo3D* parent);
    ~GeoMaterialManager() = default;

    // 材质管理
    void setMaterial(const Material3D& material);
    const Material3D& getMaterial() const { return m_material; }
    void updateMaterial();
    void resetMaterial();

    // 颜色管理
    void setPointColor(const Color3D& color);
    void setLineColor(const Color3D& color);
    void setFaceColor(const Color3D& color);
    const Color3D& getPointColor() const;
    const Color3D& getLineColor() const;
    const Color3D& getFaceColor() const;

    // 线条属性
    void setLineWidth(float width);
    float getLineWidth() const;
    void setLineStyle(LineStyle3D style);
    LineStyle3D getLineStyle() const;
    void setLineDashPattern(float pattern);
    float getLineDashPattern() const;

    // 点属性
    void setPointSize(float size);
    float getPointSize() const;
    void setPointShape(PointShape3D shape);
    PointShape3D getPointShape() const;

    // 面属性
    void setFillType(FillType3D type);
    FillType3D getFillType() const;
    void setTransparency(float transparency);
    float getTransparency() const;

    // 材质类型
    void setMaterialType(MaterialType3D type);
    MaterialType3D getMaterialType() const;

    // 光照属性
    void setAmbient(const Color3D& ambient);
    void setDiffuse(const Color3D& diffuse);
    void setSpecular(const Color3D& specular);
    void setEmission(const Color3D& emission);
    void setShininess(float shininess);

    // 混合模式
    void setBlendMode(osg::BlendFunc::BlendFuncMode src, osg::BlendFunc::BlendFuncMode dst);
    void enableBlending(bool enable);
    bool isBlendingEnabled() const;

    // 状态集管理
    osg::ref_ptr<osg::StateSet> createStateSet();
    void applyStateSet(osg::ref_ptr<osg::StateSet> stateSet);
    void updateStateSet();

    // 渲染模式
    void setWireframeMode(bool enable);
    bool isWireframeMode() const;
    void setPointMode(bool enable);
    bool isPointMode() const;

    // 深度测试
    void setDepthTest(bool enable);
    bool isDepthTestEnabled() const;
    void setDepthWrite(bool enable);
    bool isDepthWriteEnabled() const;

    // 双面渲染
    void setTwoSided(bool enable);
    bool isTwoSided() const;

    // 材质预设
    void applyBasicMaterial();
    void applyPhongMaterial();
    void applyBlinnMaterial();
    void applyLambertMaterial();
    void applyPBRMaterial();

    // 材质验证
    bool validateMaterial() const;
    bool isMaterialValid() const;

signals:
    void materialChanged();
    void colorChanged();
    void linePropertiesChanged();
    void pointPropertiesChanged();
    void facePropertiesChanged();
    void blendingChanged();
    void renderModeChanged();

private:
    void initializeMaterial();
    void updateOSGMaterial();
    void updateRenderingAttributes();
    void createDefaultStateSet();
    void applyMaterialPreset(MaterialType3D type);

private:
    Geo3D* m_parent;
    
    // 材质属性
    Material3D m_material;
    
    // 渲染状态
    osg::ref_ptr<osg::StateSet> m_stateSet;
    osg::ref_ptr<osg::Material> m_osgMaterial;
    osg::ref_ptr<osg::BlendFunc> m_blendFunc;
    osg::ref_ptr<osg::LineWidth> m_lineWidth;
    osg::ref_ptr<osg::Point> m_pointSize;
    
    // 渲染属性
    bool m_blendingEnabled;
    bool m_wireframeMode;
    bool m_pointMode;
    bool m_depthTest;
    bool m_depthWrite;
    bool m_twoSided;
    
    // 材质状态
    bool m_materialDirty;
    bool m_stateSetDirty;
}; 