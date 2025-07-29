#include "MathUtils.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <climits>

namespace Geo3D {

// ============= 基础数学运算 =============
double MathUtils::degToRad(double degrees) {
    return degrees * M_PI / 180.0;
}

double MathUtils::radToDeg(double radians) {
    return radians * 180.0 / M_PI;
}

bool MathUtils::isEqual(double a, double b, double epsilon) {
    return std::abs(a - b) <= epsilon;
}

bool MathUtils::isZero(double value, double epsilon) {
    return std::abs(value) <= epsilon;
}

bool MathUtils::isEqual(const glm::dvec3& a, const glm::dvec3& b, double epsilon) {
    return isEqual(a.x, b.x, epsilon) && 
           isEqual(a.y, b.y, epsilon) && 
           isEqual(a.z, b.z, epsilon);
}

// ============= 向量运算 =============
glm::dvec3 MathUtils::normalize(const glm::dvec3& vec) {
    return glm::normalize(vec);
}

double MathUtils::distance(const glm::dvec3& a, const glm::dvec3& b) {
    return glm::length(b - a);
}

double MathUtils::distanceSquared(const glm::dvec3& a, const glm::dvec3& b) {
    return glm::length2(b - a);
}

glm::dvec3 MathUtils::lerp(const glm::dvec3& a, const glm::dvec3& b, double t) {
    return a + t * (b - a);
}

glm::dvec3 MathUtils::slerp(const glm::dvec3& a, const glm::dvec3& b, double t) {
    // 球面线性插值
    glm::dvec3 na = glm::normalize(a);
    glm::dvec3 nb = glm::normalize(b);
    
    double dot = glm::dot(na, nb);
    
    // 如果点积接近1，使用线性插值
    if (std::abs(dot) > 0.9995) {
        return glm::normalize(lerp(a, b, t));
    }
    
    double theta = std::acos(std::abs(dot));
    double sinTheta = std::sin(theta);
    
    double wa = std::sin((1.0 - t) * theta) / sinTheta;
    double wb = std::sin(t * theta) / sinTheta;
    
    return wa * na + wb * nb;
}

glm::dvec3 MathUtils::calculateNormal(const glm::dvec3& a, const glm::dvec3& b, const glm::dvec3& c) {
    glm::dvec3 v1 = b - a;
    glm::dvec3 v2 = c - a;
    glm::dvec3 cross = glm::cross(v1, v2);
    
    if (glm::length(cross) < EPSILON) {
        // 三点共线，返回默认法向量
        return glm::dvec3(0.0, 0.0, 1.0);
    }
    
    return glm::normalize(cross);
}

glm::dvec3 MathUtils::calculateCentroid(const std::vector<glm::dvec3>& points) {
    if (points.empty()) {
        return glm::dvec3(0.0);
    }
    
    glm::dvec3 sum(0.0);
    for (const auto& point : points) {
        sum += point;
    }
    
    return sum / static_cast<double>(points.size());
}

// ============= 几何形状参数计算 =============
// Line
LineParameters MathUtils::calculateLineParameters(const glm::dvec3& start, const glm::dvec3& end) {
    LineParameters params;
    params.start = start;
    params.end = end;
    params.length = distance(start, end);
    return params;
}

double MathUtils::calculateLineLength(const LineParameters& params) {
    return params.length;
}

glm::dvec3 MathUtils::calculateLineCenter(const LineParameters& params) {
    return (params.start + params.end) * 0.5;
}

glm::dvec3 MathUtils::calculateLineDirection(const LineParameters& params) {
    return normalize(params.end - params.start);
}

// Triangle
TriangleParameters MathUtils::calculateTriangleParameters(const glm::dvec3& v1, const glm::dvec3& v2, const glm::dvec3& v3) {
    TriangleParameters params;
    params.v1 = v1;
    params.v2 = v2;
    params.v3 = v3;
    params.normal = calculateNormal(v1, v2, v3);
    return params;
}

double MathUtils::calculateTriangleArea(const TriangleParameters& params) {
    glm::dvec3 cross = glm::cross(params.v2 - params.v1, params.v3 - params.v1);
    return glm::length(cross) * 0.5;
}

glm::dvec3 MathUtils::calculateTriangleCenter(const TriangleParameters& params) {
    return (params.v1 + params.v2 + params.v3) / 3.0;
}

glm::dvec3 MathUtils::calculateTriangleNormal(const TriangleParameters& params) {
    return params.normal;
}

// Quad
QuadParameters MathUtils::calculateQuadParameters(const glm::dvec3& v1, const glm::dvec3& v2, const glm::dvec3& v3, const glm::dvec3& v4) {
    QuadParameters params;
    params.v1 = v1;
    params.v2 = v2;
    params.v3 = v3;
    params.v4 = v4;
    params.normal = calculateNormal(v1, v2, v3);
    return params;
}

double MathUtils::calculateQuadArea(const QuadParameters& params) {
    // 将四边形分解为两个三角形计算面积
    TriangleParameters tri1 = calculateTriangleParameters(params.v1, params.v2, params.v3);
    TriangleParameters tri2 = calculateTriangleParameters(params.v1, params.v3, params.v4);
    return calculateTriangleArea(tri1) + calculateTriangleArea(tri2);
}

glm::dvec3 MathUtils::calculateQuadCenter(const QuadParameters& params) {
    return (params.v1 + params.v2 + params.v3 + params.v4) / 4.0;
}

glm::dvec3 MathUtils::calculateQuadNormal(const QuadParameters& params) {
    return params.normal;
}

// Box
BoxParameters MathUtils::calculateBoxParameters(const glm::dvec3& min, const glm::dvec3& max) {
    BoxParameters params;
    params.min = min;
    params.max = max;
    params.size = max - min;
    return params;
}

double MathUtils::calculateBoxVolume(const BoxParameters& params) {
    return params.size.x * params.size.y * params.size.z;
}

double MathUtils::calculateBoxSurfaceArea(const BoxParameters& params) {
    return 2.0 * (params.size.x * params.size.y + params.size.y * params.size.z + params.size.z * params.size.x);
}

glm::dvec3 MathUtils::calculateBoxCenter(const BoxParameters& params) {
    return (params.min + params.max) * 0.5;
}

glm::dvec3 MathUtils::calculateBoxSize(const BoxParameters& params) {
    return params.size;
}

// Cube
CubeParameters MathUtils::calculateCubeParameters(const glm::dvec3& center, double size) {
    CubeParameters params;
    params.center = center;
    params.size = size;
    return params;
}

double MathUtils::calculateCubeVolume(const CubeParameters& params) {
    return params.size * params.size * params.size;
}

double MathUtils::calculateCubeSurfaceArea(const CubeParameters& params) {
    return 6.0 * params.size * params.size;
}

glm::dvec3 MathUtils::calculateCubeCenter(const CubeParameters& params) {
    return params.center;
}

double MathUtils::calculateCubeSize(const CubeParameters& params) {
    return params.size;
}

// Sphere
SphereParameters MathUtils::calculateSphereParameters(const glm::dvec3& center, double radius, int segments) {
    SphereParameters params;
    params.center = center;
    params.radius = radius;
    params.segments = segments;
    return params;
}

double MathUtils::calculateSphereVolume(const SphereParameters& params) {
    return (4.0 / 3.0) * M_PI * params.radius * params.radius * params.radius;
}

double MathUtils::calculateSphereSurfaceArea(const SphereParameters& params) {
    return 4.0 * M_PI * params.radius * params.radius;
}

glm::dvec3 MathUtils::calculateSphereCenter(const SphereParameters& params) {
    return params.center;
}

// Cylinder
CylinderParameters MathUtils::calculateCylinderParameters(const glm::dvec3& base, const glm::dvec3& top, double radius) {
    CylinderParameters params;
    params.base = base;
    params.top = top;
    params.radius = radius;
    params.height = distance(top, base);
    return params;
}

double MathUtils::calculateCylinderVolume(const CylinderParameters& params) {
    return M_PI * params.radius * params.radius * params.height;
}

double MathUtils::calculateCylinderSurfaceArea(const CylinderParameters& params) {
    return 2.0 * M_PI * params.radius * (params.radius + params.height);
}

glm::dvec3 MathUtils::calculateCylinderCenter(const CylinderParameters& params) {
    return (params.base + params.top) * 0.5;
}

// Cone
ConeParameters MathUtils::calculateConeParameters(const glm::dvec3& base, const glm::dvec3& apex, double radius) {
    ConeParameters params;
    params.base = base;
    params.apex = apex;
    params.radius = radius;
    params.height = distance(apex, base);
    return params;
}

double MathUtils::calculateConeVolume(const ConeParameters& params) {
    return (1.0 / 3.0) * M_PI * params.radius * params.radius * params.height;
}

double MathUtils::calculateConeSurfaceArea(const ConeParameters& params) {
    double slantHeight = std::sqrt(params.radius * params.radius + params.height * params.height);
    return M_PI * params.radius * (params.radius + slantHeight);
}

glm::dvec3 MathUtils::calculateConeCenter(const ConeParameters& params) {
    return (params.base + params.apex) * 0.5;
}

// Torus
TorusParameters MathUtils::calculateTorusParameters(const glm::dvec3& center, double majorRadius, double minorRadius, const glm::dvec3& axis) {
    TorusParameters params;
    params.center = center;
    params.majorRadius = majorRadius;
    params.minorRadius = minorRadius;
    params.axis = normalize(axis);
    return params;
}

double MathUtils::calculateTorusVolume(const TorusParameters& params) {
    return 2.0 * M_PI * M_PI * params.majorRadius * params.minorRadius * params.minorRadius;
}

double MathUtils::calculateTorusSurfaceArea(const TorusParameters& params) {
    return 4.0 * M_PI * M_PI * params.majorRadius * params.minorRadius;
}

glm::dvec3 MathUtils::calculateTorusCenter(const TorusParameters& params) {
    return params.center;
}

// ============= 相交测试 =============
bool MathUtils::rayIntersectsTriangle(const glm::dvec3& rayOrigin, const glm::dvec3& rayDir,
                                   const glm::dvec3& v0, const glm::dvec3& v1, const glm::dvec3& v2,
                                   double& t, double& u, double& v) {
    // Möller–Trumbore算法
    const double EPSILON = 1e-7;
    glm::dvec3 edge1 = v1 - v0;
    glm::dvec3 edge2 = v2 - v0;
    glm::dvec3 h = glm::cross(rayDir, edge2);
    double a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON) return false;

    double f = 1.0 / a;
    glm::dvec3 s = rayOrigin - v0;
    u = f * glm::dot(s, h);

    if (u < 0.0 || u > 1.0) return false;

    glm::dvec3 q = glm::cross(s, edge1);
    v = f * glm::dot(rayDir, q);

    if (v < 0.0 || u + v > 1.0) return false;

    t = f * glm::dot(edge2, q);
    return t > EPSILON;
}

bool MathUtils::rayIntersectsPlane(const glm::dvec3& rayOrigin, const glm::dvec3& rayDir,
                                const glm::dvec3& planeNormal, const glm::dvec3& planePoint,
                                double& t) {
    double denom = glm::dot(planeNormal, rayDir);
    if (std::abs(denom) < EPSILON) return false;

    t = glm::dot(planePoint - rayOrigin, planeNormal) / denom;
    return t >= 0.0;
}

bool MathUtils::lineSegmentsIntersect(const glm::dvec3& p1, const glm::dvec3& q1,
                                   const glm::dvec3& p2, const glm::dvec3& q2) {
    // 检查两条线段是否相交
    glm::dvec3 v1 = q1 - p1;
    glm::dvec3 v2 = q2 - p2;
    glm::dvec3 cross = glm::cross(v1, v2);
    
    if (glm::length(cross) < EPSILON) {
        // 线段平行或共线
        return false;
    }
    
    glm::dvec3 v = p2 - p1;
    double t1 = glm::dot(glm::cross(v, v2), cross) / glm::dot(cross, cross);
    double t2 = glm::dot(glm::cross(v, v1), cross) / glm::dot(cross, cross);
    
    return t1 >= 0.0 && t1 <= 1.0 && t2 >= 0.0 && t2 <= 1.0;
}

bool MathUtils::isPointInTriangle(const glm::dvec3& p, const glm::dvec3& a,
                               const glm::dvec3& b, const glm::dvec3& c) {
    // 使用重心坐标判断点是否在三角形内
    glm::dvec3 v0 = b - a;
    glm::dvec3 v1 = c - a;
    glm::dvec3 v2 = p - a;
    
    double d00 = glm::dot(v0, v0);
    double d01 = glm::dot(v0, v1);
    double d11 = glm::dot(v1, v1);
    double d20 = glm::dot(v2, v0);
    double d21 = glm::dot(v2, v1);
    
    double denom = d00 * d11 - d01 * d01;
    if (std::abs(denom) < EPSILON) return false;
    
    double v = (d11 * d20 - d01 * d21) / denom;
    double w = (d00 * d21 - d01 * d20) / denom;
    double u = 1.0 - v - w;
    
    return v >= 0.0 && w >= 0.0 && u >= 0.0;
}

// ============= 多边形操作 =============
PolygonParameters MathUtils::calculatePolygonParameters(const std::vector<glm::dvec3>& vertices) {
    PolygonParameters params;
    params.vertices = vertices;
    params.normal = calculatePolygonNormal(vertices);
    return params;
}

double MathUtils::calculatePolygonArea(const PolygonParameters& params) {
    if (params.vertices.size() < 3) return 0.0;
    
    double area = 0.0;
    const glm::dvec3& p0 = params.vertices[0];
    
    for (size_t i = 1; i < params.vertices.size() - 1; ++i) {
        const glm::dvec3& p1 = params.vertices[i];
        const glm::dvec3& p2 = params.vertices[i + 1];
        area += glm::length(glm::cross(p1 - p0, p2 - p0)) * 0.5;
    }
    
    return area;
}

glm::dvec3 MathUtils::calculatePolygonCenter(const PolygonParameters& params) {
    return calculateCentroid(params.vertices);
}

glm::dvec3 MathUtils::calculatePolygonNormal(const PolygonParameters& params) {
    return params.normal;
}

double MathUtils::calculatePolygonSignedArea(const std::vector<glm::dvec3>& vertices) {
    if (vertices.size() < 3) return 0.0;
    
    double area = 0.0;
    glm::dvec3 normal = calculatePolygonNormal(vertices);
    glm::dvec3 ref = vertices[0];
    
    for (size_t i = 1; i < vertices.size() - 1; ++i) {
        glm::dvec3 v1 = vertices[i] - ref;
        glm::dvec3 v2 = vertices[i + 1] - ref;
        area += glm::dot(normal, glm::cross(v1, v2));
    }
    
    return area * 0.5;
}

bool MathUtils::isPolygonClockwise(const std::vector<glm::dvec3>& vertices) {
    return calculatePolygonSignedArea(vertices) < 0.0;
}

bool MathUtils::isPolygonConvex(const std::vector<glm::dvec3>& vertices) {
    if (vertices.size() < 3) return true;
    
    glm::dvec3 normal = calculatePolygonNormal(vertices);
    double sign = 0.0;
    bool signSet = false;
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        const glm::dvec3& p1 = vertices[i];
        const glm::dvec3& p2 = vertices[(i + 1) % vertices.size()];
        const glm::dvec3& p3 = vertices[(i + 2) % vertices.size()];
        
        glm::dvec3 v1 = p2 - p1;
        glm::dvec3 v2 = p3 - p2;
        double cross = glm::dot(normal, glm::cross(v1, v2));
        
        if (!signSet) {
            sign = cross;
            signSet = true;
        } else if (sign * cross < 0.0) {
            return false;
        }
    }
    
    return true;
}

std::vector<glm::dvec3> MathUtils::reversePolygonWinding(const std::vector<glm::dvec3>& vertices) {
    std::vector<glm::dvec3> reversed = vertices;
    std::reverse(reversed.begin(), reversed.end());
    return reversed;
}

// ============= 曲线计算 =============
ArcParameters MathUtils::calculateArcFromThreePoints(const glm::dvec3& p1, const glm::dvec3& p2, const glm::dvec3& p3)
{
    ArcParameters params;
    
    // 计算法向量
    glm::dvec3 a = p2 - p1;
    glm::dvec3 b = p3 - p2;
    params.normal = normalize(glm::cross(a, b));
    
    // 计算圆心
    glm::dvec3 midAB = (p1 + p2) * 0.5;
    glm::dvec3 midBC = (p2 + p3) * 0.5;
    
    glm::dvec3 perpA = glm::cross(a, params.normal);
    glm::dvec3 perpB = glm::cross(b, params.normal);
    
    // 解线性方程组找圆心
    double t = 0.0;
    if (glm::length(perpA) > EPSILON && glm::length(perpB) > EPSILON)
    {
        glm::dvec3 diff = midBC - midAB;
        double denom = glm::dot(perpA, perpB);
        if (std::abs(denom) > EPSILON)
        {
            t = glm::dot(diff, perpB) / denom;
        }
    }
    
    params.center = midAB + t * perpA;
    params.radius = glm::length(p1 - params.center);
    
    // 建立局部坐标系
    params.uAxis = normalize(p1 - params.center);
    params.vAxis = normalize(glm::cross(params.normal, params.uAxis));
    
    // 计算三个点在局部坐标系中的角度
    auto getAngle = [&](const glm::dvec3& point) -> double {
        glm::dvec3 vec = normalize(point - params.center);
        return atan2(glm::dot(vec, params.vAxis), glm::dot(vec, params.uAxis));
    };
    
    double angle1 = getAngle(p1);  // 起点角度
    double angle2 = getAngle(p2);  // 中间点角度  
    double angle3 = getAngle(p3);  // 终点角度
    
    // 规范化角度到 [0, 2π] 范围
    auto normalizeAngle = [](double angle) -> double {
        while (angle < 0) angle += 2.0 * M_PI;
        while (angle >= 2.0 * M_PI) angle -= 2.0 * M_PI;
        return angle;
    };
    
    angle1 = normalizeAngle(angle1);
    angle2 = normalizeAngle(angle2);
    angle3 = normalizeAngle(angle3);
    
    // 设置起点为angle1
    params.startAngle = angle1;
    
    // 确定从p1到p3经过p2的正确路径
    // 检查两种可能的路径：顺时针和逆时针
    
    // 路径1：顺时针从p1到p3
    double clockwiseEnd = angle3;
    if (clockwiseEnd <= angle1) {
        clockwiseEnd += 2.0 * M_PI;
    }
    double clockwiseSweep = clockwiseEnd - angle1;
    
    // 检查p2是否在顺时针路径上
    double angle2_cw = angle2;
    if (angle2_cw <= angle1) {
        angle2_cw += 2.0 * M_PI;
    }
    bool p2OnClockwise = (angle2_cw > angle1) && (angle2_cw < clockwiseEnd);
    
    // 路径2：逆时针从p1到p3
    double counterClockwiseEnd = angle3;
    if (counterClockwiseEnd >= angle1) {
        counterClockwiseEnd -= 2.0 * M_PI;
    }
    double counterClockwiseSweep = counterClockwiseEnd - angle1;
    
    // 检查p2是否在逆时针路径上
    double angle2_ccw = angle2;
    if (angle2_ccw >= angle1) {
        angle2_ccw -= 2.0 * M_PI;
    }
    bool p2OnCounterClockwise = (angle2_ccw < angle1) && (angle2_ccw > counterClockwiseEnd);
    
    // 选择经过p2的路径
    if (p2OnClockwise && !p2OnCounterClockwise) {
        // 使用顺时针路径
        params.endAngle = clockwiseEnd;
        params.sweepAngle = clockwiseSweep;
    } else if (p2OnCounterClockwise && !p2OnClockwise) {
        // 使用逆时针路径
        params.endAngle = counterClockwiseEnd;
        params.sweepAngle = counterClockwiseSweep;
    } else {
        // 如果两个路径都经过或都不经过p2，选择较短的路径
        if (std::abs(clockwiseSweep) <= std::abs(counterClockwiseSweep)) {
            params.endAngle = clockwiseEnd;
            params.sweepAngle = clockwiseSweep;
        } else {
            params.endAngle = counterClockwiseEnd;
            params.sweepAngle = counterClockwiseSweep;
        }
    }
    
    return params;
}

std::vector<glm::dvec3> MathUtils::generateArcPoints(const ArcParameters& params, int segments)
{
    std::vector<glm::dvec3> points;
    points.reserve(segments + 1);
    
    if (params.radius <= 0)
        return points;
    
    double angleRange = params.sweepAngle;
    
    for (int i = 0; i <= segments; ++i)
    {
        double t = static_cast<double>(i) / segments;
        double angle = params.startAngle + t * angleRange;
        
        glm::dvec3 point = params.center + params.radius * (
            std::cos(angle) * params.uAxis + 
            std::sin(angle) * params.vAxis
        );
        points.push_back(point);
    }
    
    return points;
}

glm::dvec3 MathUtils::evaluateBezierPoint(const std::vector<glm::dvec3>& controlPoints, double t)
{
    if (controlPoints.empty())
        return glm::dvec3(0);
    
    // De Casteljau算法
    std::vector<glm::dvec3> tempPoints = controlPoints;
    
    while (tempPoints.size() > 1)
    {
        std::vector<glm::dvec3> newPoints;
        for (size_t j = 0; j < tempPoints.size() - 1; ++j)
        {
            newPoints.push_back(glm::mix(tempPoints[j], tempPoints[j+1], t));
        }
        tempPoints = newPoints;
    }
    
    return tempPoints.empty() ? glm::dvec3(0) : tempPoints[0];
}

std::vector<glm::dvec3> MathUtils::generateBezierCurve(const std::vector<glm::dvec3>& controlPoints, int steps)
{
    std::vector<glm::dvec3> curvePoints;
    curvePoints.reserve(steps + 1);
    
    if (controlPoints.size() < 2)
        return curvePoints;
    
    for (int i = 0; i <= steps; ++i)
    {
        double t = static_cast<double>(i) / steps;
        glm::dvec3 point = evaluateBezierPoint(controlPoints, t);
        curvePoints.push_back(point);
    }
    
    return curvePoints;
}

// ============= 边界框计算 =============
osg::BoundingBox MathUtils::calculateBoundingBox(const std::vector<glm::dvec3>& points) {
    if (points.empty()) return osg::BoundingBox();
    
    osg::BoundingBox bbox;
    for (const auto& p : points) {
        bbox.expandBy(glmToOsg(p));
    }
    return bbox;
}

double MathUtils::calculateArea(const std::vector<glm::dvec3>& points) {
    if (points.size() < 3) return 0.0;
    
    double area = 0.0;
    const glm::dvec3& p0 = points[0];
    
    for (size_t i = 1; i < points.size() - 1; ++i) {
        const glm::dvec3& p1 = points[i];
        const glm::dvec3& p2 = points[i + 1];
        area += glm::length(glm::cross(p1 - p0, p2 - p0)) * 0.5;
    }
    
    return area;
}

double MathUtils::calculateVolume(const std::vector<glm::dvec3>& points) {
    if (points.size() < 4) return 0.0;
    
    double volume = 0.0;
    const glm::dvec3& p0 = points[0];
    
    for (size_t i = 1; i < points.size() - 2; ++i) {
        const glm::dvec3& p1 = points[i];
        const glm::dvec3& p2 = points[i + 1];
        const glm::dvec3& p3 = points[i + 2];
        volume += glm::dot(glm::cross(p1 - p0, p2 - p0), p3 - p0) / 6.0;
    }
    
    return std::abs(volume);
}

// ============= 坐标转换 =============
osg::Vec3 MathUtils::glmToOsg(const glm::dvec3& vec) {
    return osg::Vec3(vec.x, vec.y, vec.z);
}

glm::dvec3 MathUtils::osgToGlm(const osg::Vec3& vec) {
    return glm::dvec3(vec.x(), vec.y(), vec.z());
}

glm::dvec3 MathUtils::projectPointOnPlane(const glm::dvec3& point, const glm::dvec3& planeNormal, const glm::dvec3& planePoint) {
    glm::dvec3 n = normalize(planeNormal);
    double d = glm::dot(point - planePoint, n);
    return point - d * n;
}

glm::dvec3 MathUtils::projectPointOnLine(const glm::dvec3& point, const glm::dvec3& lineStart, const glm::dvec3& lineEnd) {
    glm::dvec3 lineDir = lineEnd - lineStart;
    double lineLengthSq = glm::dot(lineDir, lineDir);
    
    if (lineLengthSq < EPSILON) return lineStart;
    
    double t = glm::dot(point - lineStart, lineDir) / lineLengthSq;
    t = std::clamp(t, 0.0, 1.0);
    
    return lineStart + t * lineDir;
}

glm::dmat4 MathUtils::createRotationMatrix(const glm::dvec3& axis, double angle) {
    return glm::rotate(glm::dmat4(1.0), angle, axis);
}

glm::dmat4 MathUtils::createTranslationMatrix(const glm::dvec3& translation) {
    return glm::translate(glm::dmat4(1.0), translation);
}

glm::dmat4 MathUtils::createScaleMatrix(const glm::dvec3& scale) {
    return glm::scale(glm::dmat4(1.0), scale);
}

// ============= 曲线计算 =============
glm::dvec3 MathUtils::evaluateBezier(const std::vector<glm::dvec3>& controlPoints, double t)
{
    if (controlPoints.empty())
        return glm::dvec3(0.0);
    
    if (controlPoints.size() == 1)
        return controlPoints[0];
    
    // De Casteljau算法
    std::vector<glm::dvec3> tempPoints = controlPoints;
    
    while (tempPoints.size() > 1)
    {
        std::vector<glm::dvec3> newPoints;
        for (size_t i = 0; i < tempPoints.size() - 1; ++i)
        {
            newPoints.push_back(lerp(tempPoints[i], tempPoints[i + 1], t));
        }
        tempPoints = newPoints;
    }
    
    return tempPoints[0];
}

glm::dvec3 MathUtils::evaluateSpline(const std::vector<glm::dvec3>& controlPoints, double t)
{
    if (controlPoints.size() < 2)
        return controlPoints.empty() ? glm::dvec3(0.0) : controlPoints[0];
    
    if (controlPoints.size() == 2)
        return lerp(controlPoints[0], controlPoints[1], t);
    
    // Catmull-Rom样条
    int n = static_cast<int>(controlPoints.size()) - 1;
    double scaledT = t * n;
    int i = static_cast<int>(scaledT);
    double localT = scaledT - i;
    
    i = std::clamp(i, 0, n - 1);
    
    glm::dvec3 p0 = (i > 0) ? controlPoints[i - 1] : controlPoints[i];
    glm::dvec3 p1 = controlPoints[i];
    glm::dvec3 p2 = controlPoints[i + 1];
    glm::dvec3 p3 = (i + 2 < static_cast<int>(controlPoints.size())) ? controlPoints[i + 2] : controlPoints[i + 1];
    
    double t2 = localT * localT;
    double t3 = t2 * localT;
    
    return 0.5 * (
        (2.0 * p1) +
        (-p0 + p2) * localT +
        (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
        (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3
    );
}

// ============= 多边形操作 =============
std::vector<unsigned int> MathUtils::triangulatePolygon(const std::vector<glm::dvec3>& vertices)
{
    std::vector<unsigned int> indices;
    if (vertices.size() < 3)
        return indices;
    
    // 简单的扇形三角剖分
    for (size_t i = 1; i < vertices.size() - 1; ++i)
    {
        indices.push_back(0);
        indices.push_back(static_cast<unsigned int>(i));
        indices.push_back(static_cast<unsigned int>(i + 1));
    }
    
    return indices;
}

// 生成圆弧上的点
std::vector<glm::dvec3> MathUtils::generateArcPointsFromThreePoints(const glm::dvec3& p1, const glm::dvec3& p2, const glm::dvec3& p3, int segments)
{
    std::vector<glm::dvec3> points;
    
    glm::dvec3 center;
    double radius;
    
    if (!calculateCircleCenterAndRadius(p1, p2, p3, center, radius)) {
        // 三点共线，返回直线段
        for (int i = 0; i <= segments; ++i) {
            double t = static_cast<double>(i) / segments;
            if (t <= 0.5) {
                points.push_back(lerp(p1, p2, t * 2.0));
            } else {
                points.push_back(lerp(p2, p3, (t - 0.5) * 2.0));
            }
        }
        return points;
    }
    
    // 使用完整的3D圆弧参数计算
    ArcParameters arcParams = calculateArcFromThreePoints(p1, p2, p3);
    
    // 使用3D圆弧生成函数
    return generateArcPoints(arcParams, segments);
}

// 计算多边形的法向量
glm::dvec3 MathUtils::calculatePolygonNormal(const std::vector<glm::dvec3>& vertices)
{
    if (vertices.size() < 3) {
        return glm::dvec3(0.0, 0.0, 1.0); // 默认向上
    }
    
    glm::dvec3 normal(0.0);
    
    // 使用Newell方法计算法向量
    for (size_t i = 0; i < vertices.size(); ++i) {
        const glm::dvec3& v1 = vertices[i];
        const glm::dvec3& v2 = vertices[(i + 1) % vertices.size()];
        
        normal.x += (v1.y - v2.y) * (v1.z + v2.z);
        normal.y += (v1.z - v2.z) * (v1.x + v2.x);
        normal.z += (v1.x - v2.x) * (v1.y + v2.y);
    }
    
    // 检查法向量是否为零（所有顶点共线或重合）
    double normalLength = glm::length(normal);
    if (normalLength < EPSILON) {
        // 尝试使用前三个点的叉积计算法向量
        if (vertices.size() >= 3) {
            return calculateNormal(vertices[0], vertices[1], vertices[2]);
        }
        return glm::dvec3(0.0, 0.0, 1.0); // 默认向上
    }
    
    return normal / normalLength; // 手动标准化
}

// 生成线段的顶点
std::vector<glm::dvec3> MathUtils::generateLineVertices(const glm::dvec3& start, const glm::dvec3& end)
{
    return {start, end};
}

// 生成矩形的顶点
std::vector<glm::dvec3> MathUtils::generateRectangleVertices(const glm::dvec3& p1, const glm::dvec3& p2, const glm::dvec3& p3, const glm::dvec3& p4)
{
    return {p1, p2, p3, p4};
}

// 生成三角形的顶点和法向量
std::vector<glm::dvec3> MathUtils::generateTriangleVertices(const glm::dvec3& v1, const glm::dvec3& v2, const glm::dvec3& v3, glm::dvec3& normal)
{
    normal = calculateNormal(v1, v2, v3);
    return {v1, v2, v3};
}

// 生成四边形的顶点和法向量（三角化）
std::vector<glm::dvec3> MathUtils::generateQuadVertices(const glm::dvec3& v1, const glm::dvec3& v2, const glm::dvec3& v3, const glm::dvec3& v4, std::vector<glm::dvec3>& normals)
{
    // 将四边形分成两个三角形
    glm::dvec3 normal1 = calculateNormal(v1, v2, v3);
    glm::dvec3 normal2 = calculateNormal(v1, v3, v4);
    
    normals = {normal1, normal1, normal1, normal2, normal2, normal2};
    
    // 返回两个三角形的顶点
    return {v1, v2, v3, v1, v3, v4};
}

} // namespace Geo3D

 