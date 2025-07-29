#pragma once
#pragma execution_character_set("utf-8")

#include "../Types3D.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace Geo3D {

class MathUtils {
public:
    // ============= 参数结构定义 =============
    struct LineParameters {
        glm::dvec3 direction;
        double length;
    };

    struct ArcParameters {
        glm::dvec3 center;
        double radius;
        double startAngle;
        double endAngle;
    };

    struct PolygonParameters {
        std::vector<glm::dvec3> vertices;
        glm::dvec3 normal;
        double area;
    };

    // ============= 基础数学运算 =============
    static double degToRad(double degrees);
    static double radToDeg(double radians);
    static bool isEqual(double a, double b, double epsilon = 1e-6);
    static bool isZero(double value, double epsilon = 1e-6);
    static bool isEqual(const glm::dvec3& a, const glm::dvec3& b, double epsilon = 1e-6);

    // ============= 向量运算 =============
    static glm::dvec3 normalize(const glm::dvec3& vec);
    static double dot(const glm::dvec3& a, const glm::dvec3& b);
    static glm::dvec3 cross(const glm::dvec3& a, const glm::dvec3& b);
    static double length(const glm::dvec3& vec);
    static double distance(const glm::dvec3& a, const glm::dvec3& b);
    static double angle(const glm::dvec3& a, const glm::dvec3& b);

    // ============= 几何形状参数计算 =============
    // Line
    static LineParameters calculateLineParameters(const glm::dvec3& start, const glm::dvec3& end);
    
    // Arc
    static ArcParameters calculateArcParameters(const glm::dvec3& p1, const glm::dvec3& p2, const glm::dvec3& p3);
    static ArcParameters calculateArcFromThreePoints(const glm::dvec3& p1, const glm::dvec3& p2, const glm::dvec3& p3);
    
    // Polygon
    static PolygonParameters calculatePolygonParameters(const std::vector<glm::dvec3>& vertices);
    static glm::dvec3 calculatePolygonNormal(const PolygonParameters& params);
    static double calculatePolygonArea(const PolygonParameters& params);

    // ============= 相交测试 =============
    static bool rayIntersectsTriangle(const glm::dvec3& rayOrigin, const glm::dvec3& rayDir,
                                    const glm::dvec3& v0, const glm::dvec3& v1, const glm::dvec3& v2,
                                    double& t, double& u, double& v);
    static bool lineIntersectsPlane(const glm::dvec3& lineStart, const glm::dvec3& lineEnd,
                                   const glm::dvec3& planePoint, const glm::dvec3& planeNormal,
                                   glm::dvec3& intersection);
    static bool sphereIntersectsSphere(const glm::dvec3& center1, double radius1,
                                     const glm::dvec3& center2, double radius2);

    // ============= 多边形操作 =============
    static bool isPointInPolygon(const glm::dvec3& point, const std::vector<glm::dvec3>& vertices);
    static bool isPolygonConvex(const std::vector<glm::dvec3>& vertices);
    static std::vector<glm::dvec3> triangulatePolygon(const std::vector<glm::dvec3>& vertices);

    // ============= 曲线计算 =============
    static glm::dvec3 evaluateBezier(const std::vector<glm::dvec3>& controlPoints, double t);
    static std::vector<glm::dvec3> generateBezierPoints(const std::vector<glm::dvec3>& controlPoints, int numPoints);

    // ============= 坐标转换 =============
    static glm::dvec3 worldToLocal(const glm::dvec3& point, const glm::dmat4& transform);
    static glm::dvec3 localToWorld(const glm::dvec3& point, const glm::dmat4& transform);

    // ============= 边界框计算 =============
    static BoundingBox3D calculateBoundingBox(const std::vector<glm::dvec3>& points);
    static double calculateVolume(const std::vector<glm::dvec3>& points);

    // ============= 约束系统辅助函数 =============
    static glm::dvec3 calculateCircleCenter(const glm::dvec3& p1, const glm::dvec3& p2, const glm::dvec3& p3);
    static glm::dvec3 calculateSphereCenter(const glm::dvec3& p1, const glm::dvec3& p2, const glm::dvec3& p3, const glm::dvec3& p4);
};

} // namespace Geo3D
