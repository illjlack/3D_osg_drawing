#pragma once

#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Matrix>
#include <osg/Group>
#include <osg/Node>
#include <osg/Geometry>
#include <osg/PrimitiveSet>
#include <glm/glm.hpp>
#include <vector>

// OSG工具函数库
class OSGUtils
{
public:
    // ============= 坐标转换 =============
    
    // GLM vec3 转 OSG Vec3
    static osg::Vec3 glmToOsg(const glm::vec3& vec);
    
    // OSG Vec3 转 GLM vec3
    static glm::vec3 osgToGlm(const osg::Vec3& vec);
    
    // GLM vec4 转 OSG Vec4
    static osg::Vec4 glmToOsg(const glm::vec4& vec);
    
    // OSG Vec4 转 GLM vec4
    static glm::vec4 osgToGlm(const osg::Vec4& vec);
    
    // GLM mat4 转 OSG Matrix
    static osg::Matrix glmToOsg(const glm::mat4& mat);
    
    // OSG Matrix 转 GLM mat4
    static glm::mat4 osgToGlm(const osg::Matrix& mat);
    
    // ============= 几何体创建 =============
    
    // 创建基本几何体
    static osg::ref_ptr<osg::Geometry> createBox(const glm::vec3& center, const glm::vec3& size);
    static osg::ref_ptr<osg::Geometry> createSphere(const glm::vec3& center, float radius, int segments = 16);
    static osg::ref_ptr<osg::Geometry> createCylinder(const glm::vec3& center, float radius, float height, int segments = 16);
    static osg::ref_ptr<osg::Geometry> createCone(const glm::vec3& base, float radius, float height, int segments = 16);
    static osg::ref_ptr<osg::Geometry> createPlane(const glm::vec3& center, const glm::vec3& normal, float size);
    
    // 创建指示器几何体
    static osg::ref_ptr<osg::Geometry> createWireframeBox(const glm::vec3& center, const glm::vec3& size);
    static osg::ref_ptr<osg::Geometry> createWireframeSphere(const glm::vec3& center, float radius, int segments = 16);
    static osg::ref_ptr<osg::Geometry> createAxisArrows(const glm::vec3& center, float length = 1.0f);
    
    // ============= 几何体操作 =============
    
    // 计算几何体包围盒
    static osg::BoundingBox computeBoundingBox(osg::Geometry* geometry);
    
    // 合并多个几何体
    static osg::ref_ptr<osg::Geometry> mergeGeometries(const std::vector<osg::ref_ptr<osg::Geometry>>& geometries);
    
    // 变换几何体
    static void transformGeometry(osg::Geometry* geometry, const osg::Matrix& matrix);
    
    // ============= 材质和渲染状态 =============
    
    // 创建基本材质
    static osg::ref_ptr<osg::StateSet> createMaterial(const osg::Vec4& color, bool transparent = false);
    static osg::ref_ptr<osg::StateSet> createWireframeMaterial(const osg::Vec4& color, float lineWidth = 1.0f);
    static osg::ref_ptr<osg::StateSet> createPointMaterial(const osg::Vec4& color, float pointSize = 1.0f);
    
    // 设置透明度
    static void setTransparency(osg::StateSet* stateSet, float alpha);
    
    // 设置深度测试
    static void setDepthTest(osg::StateSet* stateSet, bool enable);
    
    // ============= 场景图操作 =============
    
    // 查找节点
    static osg::Node* findNode(osg::Group* root, const std::string& name);
    
    // 移除所有子节点
    static void removeAllChildren(osg::Group* group);
    
    // 计算场景包围盒
    static osg::BoundingSphere computeSceneBound(osg::Node* node);
    
    // ============= 数学工具 =============
    
    // 计算三角形法向量
    static osg::Vec3 computeTriangleNormal(const osg::Vec3& v1, const osg::Vec3& v2, const osg::Vec3& v3);
    
    // 计算两点距离
    static float distance(const osg::Vec3& p1, const osg::Vec3& p2);
    
    // 点到平面距离
    static float distanceToPlane(const osg::Vec3& point, const osg::Vec3& planePoint, const osg::Vec3& planeNormal);
    
    // 射线与球体相交检测
    static bool rayIntersectSphere(const osg::Vec3& rayOrigin, const osg::Vec3& rayDirection,
                                  const osg::Vec3& sphereCenter, float sphereRadius,
                                  float& t1, float& t2);
    
    // 射线与三角形相交检测
    static bool rayIntersectTriangle(const osg::Vec3& rayOrigin, const osg::Vec3& rayDirection,
                                   const osg::Vec3& v0, const osg::Vec3& v1, const osg::Vec3& v2,
                                   float& t, float& u, float& v);
    
    // ============= 调试工具 =============
    
    // 创建坐标轴
    static osg::ref_ptr<osg::Group> createAxisIndicator(float length = 1.0f);
    
    // 创建网格
    static osg::ref_ptr<osg::Geometry> createGrid(float size = 10.0f, int divisions = 10);
    
    // 打印节点信息
    static void printNodeInfo(osg::Node* node, int indent = 0);
    
    // 打印几何体信息
    static void printGeometryInfo(osg::Geometry* geometry);
    
private:
    OSGUtils() = default; // 禁止实例化
}; 