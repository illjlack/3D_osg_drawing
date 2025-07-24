#pragma once
#pragma execution_character_set("utf-8")

#include <osg/Node>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Material>
#include <osg/StateSet>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/CullFace>
#include <osg/PolygonMode>
#include <glm/glm.hpp>

class OSGUtils
{
public:
    // 类型转换工具
    static osg::Vec3 glmToOsg(const glm::dvec3& vec);
    static osg::Vec4 glmToOsg(const glm::dvec4& vec);
    static osg::Matrix glmToOsg(const glm::dmat4& mat);
    
    // 反向转换
    static glm::dvec3 osgToGlm(const osg::Vec3& vec);
    static glm::dvec4 osgToGlm(const osg::Vec4& vec);
    static glm::dmat4 osgToGlm(const osg::Matrix& mat);
    
    // 创建基本几何体（实体模式）
    static osg::ref_ptr<osg::Geometry> createPoint(const glm::dvec3& position);
    static osg::ref_ptr<osg::Geometry> createLine(const glm::dvec3& start, const glm::dvec3& end);
    static osg::ref_ptr<osg::Geometry> createTriangle(const glm::dvec3& v1, const glm::dvec3& v2, const glm::dvec3& v3);
    static osg::ref_ptr<osg::Geometry> createQuad(const glm::dvec3& v1, const glm::dvec3& v2, const glm::dvec3& v3, const glm::dvec3& v4);
    
    // 创建复杂几何体
    static osg::ref_ptr<osg::Geometry> createBox(const glm::dvec3& center, const glm::dvec3& size);
    static osg::ref_ptr<osg::Geometry> createSphere(const glm::dvec3& center, double radius, int segments = 16);
    static osg::ref_ptr<osg::Geometry> createCylinder(const glm::dvec3& center, double radius, double height, int segments = 16);
    static osg::ref_ptr<osg::Geometry> createCone(const glm::dvec3& base, double radius, double height, int segments = 16);
    static osg::ref_ptr<osg::Geometry> createPlane(const glm::dvec3& center, const glm::dvec3& normal, double size);
    
    // 创建线框几何体
    static osg::ref_ptr<osg::Geometry> createWireframeBox(const glm::dvec3& center, const glm::dvec3& size);
    static osg::ref_ptr<osg::Geometry> createWireframeSphere(const glm::dvec3& center, double radius, int segments = 16);
    static osg::ref_ptr<osg::Geometry> createAxisArrows(const glm::dvec3& center, double length = 1.0);
    
    // 材质和渲染状态
    static osg::ref_ptr<osg::StateSet> createBasicMaterial(const osg::Vec4& color);
    static osg::ref_ptr<osg::StateSet> createTransparentMaterial(const osg::Vec4& color);
    
    // 创建线框材质，包含线宽设置
    static osg::ref_ptr<osg::StateSet> createWireframeMaterial(const osg::Vec4& color, double lineWidth = 1.0);
    static osg::ref_ptr<osg::StateSet> createPointMaterial(const osg::Vec4& color, double pointSize = 1.0);
    
    // 透明度控制
    static void setTransparency(osg::StateSet* stateSet, double alpha);
    
    // 设置渲染优先级
    static void setRenderOrder(osg::StateSet* stateSet, int order);
    
    // 设置双面渲染
    static void setDoubleSided(osg::StateSet* stateSet, bool doubleSided = true);
    
    // 深度测试控制
    static void setDepthTest(osg::StateSet* stateSet, bool enable = true);
    
    // 创建带颜色的顶点数组
    static osg::ref_ptr<osg::Vec4Array> createColorArray(const osg::Vec4& color, size_t count);
    static osg::ref_ptr<osg::Vec3Array> createNormalArray(const osg::Vec3& normal, size_t count);
    
    // 几何计算辅助函数
    static osg::Vec3 calculateTriangleNormal(const osg::Vec3& v1, const osg::Vec3& v2, const osg::Vec3& v3);
    static osg::BoundingBox calculateBoundingBox(const std::vector<osg::Vec3>& vertices);
    
    // 距离计算
    static double distance(const osg::Vec3& p1, const osg::Vec3& p2);
    
    // 点到平面距离
    static double distanceToPlane(const osg::Vec3& point, const osg::Vec3& planePoint, const osg::Vec3& planeNormal);
    
    // 射线与球体相交
    static bool rayIntersectsSphere(const osg::Vec3& rayOrigin, const osg::Vec3& rayDirection,
                                  const osg::Vec3& sphereCenter, double sphereRadius,
                                  double& t1, double& t2);
    
    // 射线与三角形相交
    static bool rayIntersectsTriangle(const osg::Vec3& rayOrigin, const osg::Vec3& rayDirection,
                                    const osg::Vec3& v0, const osg::Vec3& v1, const osg::Vec3& v2,
                                    double& t, double& u, double& v);
    
    // 创建调试和辅助对象
    static osg::ref_ptr<osg::Node> createCoordinateSystem(double scale = 1.0);
    static osg::ref_ptr<osg::Group> createAxisIndicator(double length = 1.0);
    
    // 创建网格
    static osg::ref_ptr<osg::Geometry> createGrid(double size = 10.0, int divisions = 10);
    
    // 纹理坐标生成
    static osg::ref_ptr<osg::Vec2Array> createTextureCoords(const std::vector<osg::Vec2>& coords);
    
    // 索引数组创建
    static osg::ref_ptr<osg::DrawElementsUInt> createTriangleIndices(const std::vector<unsigned int>& indices);
    static osg::ref_ptr<osg::DrawElementsUInt> createLineIndices(const std::vector<unsigned int>& indices);
    
    // 法向量计算和设置
    static void calculateAndSetNormals(osg::Geometry* geometry);
    static void setFlatNormals(osg::Geometry* geometry);
    static void setSmoothNormals(osg::Geometry* geometry);
    
    // 几何体优化
    static void optimizeGeometry(osg::Geometry* geometry);
    static void mergeGeometries(osg::Group* group);
    
    // 辅助函数
    static osg::ref_ptr<osg::Vec3Array> convertGlmToOsg(const std::vector<glm::dvec3>& glmVertices);
    static std::vector<glm::dvec3> convertOsgToGlm(const osg::Vec3Array* osgVertices);
}; 



