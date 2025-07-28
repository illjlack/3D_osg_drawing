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
    static double degToRad(double degrees);
    
    // 弧度转角度
    static double radToDeg(double radians);
    
    // 向量相关
    static glm::dvec3 normalize(const glm::dvec3& vec);
    static double distance(const glm::dvec3& a, const glm::dvec3& b);
    static double distanceSquared(const glm::dvec3& a, const glm::dvec3& b);
    static glm::dvec3 lerp(const glm::dvec3& a, const glm::dvec3& b, double t);
    static glm::dvec3 slerp(const glm::dvec3& a, const glm::dvec3& b, double t);
    
    // 几何计算
    static glm::dvec3 calculateNormal(const glm::dvec3& a, const glm::dvec3& b, const glm::dvec3& c);
    static glm::dvec3 calculateCentroid(const std::vector<glm::dvec3>& points);
    static double calculateArea(const std::vector<glm::dvec3>& points);
    static double calculateVolume(const std::vector<glm::dvec3>& points);
    
    // 包围盒计算
    static osg::BoundingBox calculateBoundingBox(const std::vector<glm::dvec3>& points);
    
    // 投影和变换
    static glm::dvec3 projectPointOnPlane(const glm::dvec3& point, const glm::dvec3& planeNormal, const glm::dvec3& planePoint);
    static glm::dvec3 projectPointOnLine(const glm::dvec3& point, const glm::dvec3& lineStart, const glm::dvec3& lineEnd);
    
    // 相交检测
    static bool rayIntersectsTriangle(const glm::dvec3& rayOrigin, const glm::dvec3& rayDir,
                                     const glm::dvec3& v0, const glm::dvec3& v1, const glm::dvec3& v2,
                                     double& t, glm::dvec3& intersectionPoint);
    
    static bool rayIntersectsPlane(const glm::dvec3& rayOrigin, const glm::dvec3& rayDir,
                                  const glm::dvec3& planeNormal, const glm::dvec3& planePoint,
                                  double& t, glm::dvec3& intersectionPoint);
    
    // 坐标转换
    static osg::Vec3 glmToOsg(const glm::dvec3& vec);
    static glm::dvec3 osgToGlm(const osg::Vec3& vec);
    
    // 曲线和样条线
    static glm::dvec3 evaluateBezier(const std::vector<glm::dvec3>& controlPoints, double t);
    static glm::dvec3 evaluateSpline(const std::vector<glm::dvec3>& controlPoints, double t);
    
    // 弧线参数结构
    struct ArcParameters {
        glm::dvec3 center;
        double radius;
        double startAngle;
        double endAngle;
        double sweepAngle;
        glm::dvec3 normal;
        glm::dvec3 uAxis;
        glm::dvec3 vAxis;
    };
    
    static ArcParameters calculateArcFromThreePoints(const glm::dvec3& p1, const glm::dvec3& p2, const glm::dvec3& p3);
    static std::vector<glm::dvec3> generateArcPoints(const ArcParameters& params, int segments = 50);
    
    // Bezier曲线
    static glm::dvec3 evaluateBezierPoint(const std::vector<glm::dvec3>& controlPoints, double t);
    static std::vector<glm::dvec3> generateBezierCurve(const std::vector<glm::dvec3>& controlPoints, int steps = 50);
    
    // 圆锥参数结构
    struct ConeParameters {
        glm::dvec3 base;
        glm::dvec3 apex;
        double radius;
        double height;
        glm::dvec3 axis;
        glm::dvec3 uAxis;
        glm::dvec3 vAxis;
    };
    
    static ConeParameters calculateConeParameters(const glm::dvec3& base, const glm::dvec3& apex, double radius);
    static double calculateConeVolume(const ConeParameters& params);
    static double calculateConeSurfaceArea(const ConeParameters& params);
    static glm::dvec3 calculateConeCenter(const ConeParameters& params);
    
    // 球体参数结构
    struct SphereParameters {
        glm::dvec3 center;
        double radius;
        int segments;
    };
    
    static SphereParameters calculateSphereParameters(const glm::dvec3& center, double radius, int segments = 16);
    static double calculateSphereVolume(const SphereParameters& params);
    static double calculateSphereSurfaceArea(const SphereParameters& params);
    static glm::dvec3 calculateSphereCenter(const SphereParameters& params);
    
    // 盒子参数结构
    struct BoxParameters {
        glm::dvec3 min;
        glm::dvec3 max;
        glm::dvec3 size;
        glm::dvec3 center;
    };
    
    static BoxParameters calculateBoxParameters(const glm::dvec3& min, const glm::dvec3& max);
    static double calculateBoxVolume(const BoxParameters& params);
    static double calculateBoxSurfaceArea(const BoxParameters& params);
    static glm::dvec3 calculateBoxCenter(const BoxParameters& params);
    static glm::dvec3 calculateBoxSize(const BoxParameters& params);
    
    // 圆柱参数结构
    struct CylinderParameters {
        glm::dvec3 base;
        glm::dvec3 top;
        double radius;
        double height;
        glm::dvec3 axis;
        glm::dvec3 uAxis;
        glm::dvec3 vAxis;
    };
    
    static CylinderParameters calculateCylinderParameters(const glm::dvec3& base, const glm::dvec3& top, double radius);
    static double calculateCylinderVolume(const CylinderParameters& params);
    static double calculateCylinderSurfaceArea(const CylinderParameters& params);
    static glm::dvec3 calculateCylinderCenter(const CylinderParameters& params);
    
    // 圆环参数结构
    struct TorusParameters {
        glm::dvec3 center;
        double majorRadius;
        double minorRadius;
        glm::dvec3 axis;
        glm::dvec3 uAxis;
        glm::dvec3 vAxis;
    };
    
    static TorusParameters calculateTorusParameters(const glm::dvec3& center, double majorRadius, double minorRadius, const glm::dvec3& axis = glm::dvec3(0, 0, 1));
    static double calculateTorusVolume(const TorusParameters& params);
    static double calculateTorusSurfaceArea(const TorusParameters& params);
    static glm::dvec3 calculateTorusCenter(const TorusParameters& params);
    
    // 三角形参数结构
    struct TriangleParameters {
        glm::dvec3 v1;
        glm::dvec3 v2;
        glm::dvec3 v3;
        glm::dvec3 center;
        glm::dvec3 normal;
        double area;
    };
    
    static TriangleParameters calculateTriangleParameters(const glm::dvec3& v1, const glm::dvec3& v2, const glm::dvec3& v3);
    static double calculateTriangleArea(const TriangleParameters& params);
    
    // 四边形参数结构
    struct QuadParameters {
        glm::dvec3 v1;
        glm::dvec3 v2;
        glm::dvec3 v3;
        glm::dvec3 v4;
        glm::dvec3 center;
        glm::dvec3 normal;
        double area;
    };
    
    static QuadParameters calculateQuadParameters(const glm::dvec3& v1, const glm::dvec3& v2, const glm::dvec3& v3, const glm::dvec3& v4);
    static double calculateQuadArea(const QuadParameters& params);
    
    // 多边形参数结构
    struct PolygonParameters {
        std::vector<glm::dvec3> vertices;
        glm::dvec3 center;
        glm::dvec3 normal;
        double area;
        std::vector<unsigned int> triangleIndices;  // 添加缺少的成员
    };
    
    static PolygonParameters calculatePolygonParameters(const std::vector<glm::dvec3>& vertices);
    static double calculatePolygonArea(const PolygonParameters& params);
    static glm::dvec3 calculatePolygonNormal(const std::vector<glm::dvec3>& vertices);
    static glm::dvec3 calculatePolygonNormal(const PolygonParameters& params);  // 添加重载版本
    static glm::dvec3 calculatePolygonCenter(const PolygonParameters& params);  // 添加缺少的声明
    
    // 线段参数结构
    struct LineParameters {
        glm::dvec3 start;
        glm::dvec3 end;
        glm::dvec3 direction;
        double length;
        glm::dvec3 center;  // 添加缺少的成员
    };
    
    static LineParameters calculateLineParameters(const glm::dvec3& start, const glm::dvec3& end);
    static double calculateLineLength(const LineParameters& params);
    static glm::dvec3 calculateLineCenter(const LineParameters& params);  // 添加缺少的声明
    static glm::dvec3 calculateLineDirection(const LineParameters& params);  // 添加缺少的声明
    
    // 立方体参数结构
    struct CubeParameters {
        glm::dvec3 center;
        double size;
        glm::dvec3 min;  // 添加缺少的成员
        glm::dvec3 max;  // 添加缺少的成员
    };
    
    static CubeParameters calculateCubeParameters(const glm::dvec3& center, double size);
    static double calculateCubeVolume(const CubeParameters& params);
    static double calculateCubeSurfaceArea(const CubeParameters& params);
    static glm::dvec3 calculateCubeCenter(const CubeParameters& params);
    static double calculateCubeSize(const CubeParameters& params);
    
    // 矩阵操作
    static glm::dmat4 createRotationMatrix(const glm::dvec3& axis, double angle);
    static glm::dmat4 createTranslationMatrix(const glm::dvec3& translation);
    static glm::dmat4 createScaleMatrix(const glm::dvec3& scale);
    
    // 常量
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double EPSILON = 1e-6;
    static constexpr double DEG_TO_RAD = PI / 180.0;
    static constexpr double RAD_TO_DEG = 180.0 / PI;
    
    // 比较函数
    static bool isEqual(double a, double b, double epsilon = EPSILON);
    static bool isZero(double value, double epsilon = EPSILON);
    static bool isEqual(const glm::dvec3& a, const glm::dvec3& b, double epsilon = EPSILON);
    
    // 添加缺少的方法声明
    static glm::dvec3 calculateTriangleCenter(const TriangleParameters& params);
    static glm::dvec3 calculateTriangleNormal(const TriangleParameters& params);
    static glm::dvec3 calculateQuadCenter(const QuadParameters& params);
    static glm::dvec3 calculateQuadNormal(const QuadParameters& params);
    
    // 圆形计算
    static bool calculateCircleCenterAndRadius(const glm::dvec3& p1, const glm::dvec3& p2, const glm::dvec3& p3,
                                               glm::dvec3& center, double& radius);
    
    // 插值函数
    static double lerp(double a, double b, double t);
    static double smoothstep(double edge0, double edge1, double x);
    static double clamp(double value, double min, double max);
    static glm::dvec3 clamp(const glm::dvec3& value, const glm::dvec3& min, const glm::dvec3& max);
    
    // 几何体顶点生成函数
    static std::vector<glm::dvec3> generateLineVertices(const glm::dvec3& start, const glm::dvec3& end);
    static std::vector<glm::dvec3> generateArcPointsFromThreePoints(const glm::dvec3& p1, const glm::dvec3& p2, const glm::dvec3& p3, int segments = 32);
    static std::vector<glm::dvec3> generateTriangleVertices(const glm::dvec3& v1, const glm::dvec3& v2, const glm::dvec3& v3, glm::dvec3& normal);
    static std::vector<glm::dvec3> generateQuadVertices(const glm::dvec3& v1, const glm::dvec3& v2, const glm::dvec3& v3, const glm::dvec3& v4, std::vector<glm::dvec3>& normals);
    static std::vector<glm::dvec3> generateRectangleVertices(const glm::dvec3& p1, const glm::dvec3& p2, const glm::dvec3& p3, const glm::dvec3& p4);
    
    // 三角剖分函数声明
    static std::vector<unsigned int> triangulatePolygon(const std::vector<glm::dvec3>& vertices);
    
    // 高级多边形处理
    static std::vector<unsigned int> triangulateSimplePolygon(const std::vector<glm::dvec3>& vertices);
    static std::vector<unsigned int> triangulateSelfIntersectingPolygon(const std::vector<glm::dvec3>& vertices);
    static bool isPolygonSelfIntersecting(const std::vector<glm::dvec3>& vertices);
    static std::vector<glm::dvec3> fixSelfIntersection(const std::vector<glm::dvec3>& vertices);
    
    // 多边形辅助函数
    static bool isPolygonConvex(const std::vector<glm::dvec3>& vertices);
    static double calculatePolygonSignedArea(const std::vector<glm::dvec3>& vertices);
    static bool isPolygonClockwise(const std::vector<glm::dvec3>& vertices);
    static std::vector<glm::dvec3> reversePolygonWinding(const std::vector<glm::dvec3>& vertices);
    
    // 耳切算法 (Ear Clipping Algorithm)
    static std::vector<unsigned int> earClippingTriangulation(const std::vector<glm::dvec3>& vertices);
    static bool isEar(const std::vector<glm::dvec3>& vertices, int i);
    static bool isPointInTriangle(const glm::dvec3& p, const glm::dvec3& a, const glm::dvec3& b, const glm::dvec3& c);
    
    // 线段相交检测
    static bool lineSegmentsIntersect(const glm::dvec3& p1, const glm::dvec3& q1, const glm::dvec3& p2, const glm::dvec3& q2);
}; 
