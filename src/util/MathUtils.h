#pragma once
#pragma execution_character_set("utf-8")

#include <glm/glm.hpp>
#include <vector>
#include <osg/Vec3>
#include <osg/BoundingBox>

// 数学工具类，提供常用的数学计算功能
class MathUtils
{
public:
    // 角度转弧度
    static float degToRad(float degrees);
    
    // 弧度转角度
    static float radToDeg(float radians);
    
    // 向量相关
    static glm::vec3 normalize(const glm::vec3& vec);
    static float distance(const glm::vec3& a, const glm::vec3& b);
    static float distanceSquared(const glm::vec3& a, const glm::vec3& b);
    static glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t);
    static glm::vec3 slerp(const glm::vec3& a, const glm::vec3& b, float t);
    
    // 几何计算
    static glm::vec3 calculateNormal(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c);
    static glm::vec3 calculateCentroid(const std::vector<glm::vec3>& points);
    static float calculateArea(const std::vector<glm::vec3>& points);
    static float calculateVolume(const std::vector<glm::vec3>& points);
    
    // 包围盒计算
    static osg::BoundingBox calculateBoundingBox(const std::vector<glm::vec3>& points);
    
    // 投影和变换
    static glm::vec3 projectPointOnPlane(const glm::vec3& point, const glm::vec3& planeNormal, const glm::vec3& planePoint);
    static glm::vec3 projectPointOnLine(const glm::vec3& point, const glm::vec3& lineStart, const glm::vec3& lineEnd);
    
    // 相交检测
    static bool rayIntersectsTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                     const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                                     float& t, glm::vec3& intersectionPoint);
    
    static bool rayIntersectsPlane(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                  const glm::vec3& planeNormal, const glm::vec3& planePoint,
                                  float& t, glm::vec3& intersectionPoint);
    
    // 坐标转换
    static osg::Vec3 glmToOsg(const glm::vec3& vec);
    static glm::vec3 osgToGlm(const osg::Vec3& vec);
    
    // 曲线计算
    static glm::vec3 evaluateBezier(const std::vector<glm::vec3>& controlPoints, float t);
    static glm::vec3 evaluateSpline(const std::vector<glm::vec3>& controlPoints, float t);
    
    // 旋转和变换
    static glm::mat4 createRotationMatrix(const glm::vec3& axis, float angle);
    static glm::mat4 createTranslationMatrix(const glm::vec3& translation);
    static glm::mat4 createScaleMatrix(const glm::vec3& scale);
    
    // 常用常量
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float EPSILON = 1e-6f;
    static constexpr float DEG_TO_RAD = PI / 180.0f;
    static constexpr float RAD_TO_DEG = 180.0f / PI;
    
    // 比较函数
    static bool isEqual(float a, float b, float epsilon = EPSILON);
    static bool isZero(float value, float epsilon = EPSILON);
    static bool isEqual(const glm::vec3& a, const glm::vec3& b, float epsilon = EPSILON);
}; 