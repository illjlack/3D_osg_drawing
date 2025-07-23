#include "MathUtils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <cmath>
#include <algorithm>
#include "../core/Common3D.h"

float MathUtils::degToRad(float degrees)
{
    return degrees * DEG_TO_RAD;
}

float MathUtils::radToDeg(float radians)
{
    return radians * RAD_TO_DEG;
}

glm::vec3 MathUtils::normalize(const glm::vec3& vec)
{
    return glm::normalize(vec);
}

float MathUtils::distance(const glm::vec3& a, const glm::vec3& b)
{
    return glm::distance(a, b);
}

float MathUtils::distanceSquared(const glm::vec3& a, const glm::vec3& b)
{
    glm::vec3 diff = b - a;
    return glm::dot(diff, diff);
}

glm::vec3 MathUtils::lerp(const glm::vec3& a, const glm::vec3& b, float t)
{
    return glm::mix(a, b, t);
}

glm::vec3 MathUtils::slerp(const glm::vec3& a, const glm::vec3& b, float t)
{
    // 球面线性插值
    float dot = glm::dot(normalize(a), normalize(b));
    dot = std::clamp(dot, -1.0f, 1.0f);
    
    float theta = std::acos(dot);
    if (std::abs(theta) < EPSILON)
        return lerp(a, b, t);
    
    float sinTheta = std::sin(theta);
    float w1 = std::sin((1.0f - t) * theta) / sinTheta;
    float w2 = std::sin(t * theta) / sinTheta;
    
    return w1 * a + w2 * b;
}

glm::vec3 MathUtils::calculateNormal(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
{
    glm::vec3 v1 = b - a;
    glm::vec3 v2 = c - a;
    return normalize(glm::cross(v1, v2));
}

glm::vec3 MathUtils::calculateCentroid(const std::vector<glm::vec3>& points)
{
    if (points.empty())
        return glm::vec3(0.0f);
    
    glm::vec3 sum(0.0f);
    for (const auto& point : points)
    {
        sum += point;
    }
    return sum / static_cast<float>(points.size());
}

float MathUtils::calculateArea(const std::vector<glm::vec3>& points)
{
    if (points.size() < 3)
        return 0.0f;
    
    // 使用三角形面积公式
    float area = 0.0f;
    for (size_t i = 1; i < points.size() - 1; ++i)
    {
        glm::vec3 v1 = points[i] - points[0];
        glm::vec3 v2 = points[i + 1] - points[0];
        area += 0.5f * glm::length(glm::cross(v1, v2));
    }
    return area;
}

float MathUtils::calculateVolume(const std::vector<glm::vec3>& points)
{
    if (points.size() < 4)
        return 0.0f;
    
    // 使用四面体体积公式
    float volume = 0.0f;
    for (size_t i = 1; i < points.size() - 2; ++i)
    {
        glm::vec3 v1 = points[i] - points[0];
        glm::vec3 v2 = points[i + 1] - points[0];
        glm::vec3 v3 = points[i + 2] - points[0];
        volume += std::abs(glm::dot(v1, glm::cross(v2, v3))) / 6.0f;
    }
    return volume;
}

osg::BoundingBox MathUtils::calculateBoundingBox(const std::vector<glm::vec3>& points)
{
    if (points.empty())
        return osg::BoundingBox();
    
    osg::BoundingBox bbox;
    for (const auto& point : points)
    {
        bbox.expandBy(osg::Vec3(point.x, point.y, point.z));
    }
    return bbox;
}

glm::vec3 MathUtils::projectPointOnPlane(const glm::vec3& point, const glm::vec3& planeNormal, const glm::vec3& planePoint)
{
    glm::vec3 n = normalize(planeNormal);
    float d = glm::dot(point - planePoint, n);
    return point - d * n;
}

glm::vec3 MathUtils::projectPointOnLine(const glm::vec3& point, const glm::vec3& lineStart, const glm::vec3& lineEnd)
{
    glm::vec3 lineDir = lineEnd - lineStart;
    float lineLengthSq = glm::dot(lineDir, lineDir);
    
    if (lineLengthSq < EPSILON)
        return lineStart;
    
    float t = glm::dot(point - lineStart, lineDir) / lineLengthSq;
    t = std::clamp(t, 0.0f, 1.0f);
    
    return lineStart + t * lineDir;
}

bool MathUtils::rayIntersectsTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                     const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                                     float& t, glm::vec3& intersectionPoint)
{
    // Mller-Trumbore相交算法
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(rayDir, edge2);
    float a = glm::dot(edge1, h);
    
    if (a > -EPSILON && a < EPSILON)
        return false;
    
    float f = 1.0f / a;
    glm::vec3 s = rayOrigin - v0;
    float u = f * glm::dot(s, h);
    
    if (u < 0.0f || u > 1.0f)
        return false;
    
    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(rayDir, q);
    
    if (v < 0.0f || u + v > 1.0f)
        return false;
    
    t = f * glm::dot(edge2, q);
    
    if (t > EPSILON)
    {
        intersectionPoint = rayOrigin + t * rayDir;
        return true;
    }
    
    return false;
}

bool MathUtils::rayIntersectsPlane(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                  const glm::vec3& planeNormal, const glm::vec3& planePoint,
                                  float& t, glm::vec3& intersectionPoint)
{
    float denom = glm::dot(planeNormal, rayDir);
    
    if (std::abs(denom) < EPSILON)
        return false;
    
    t = glm::dot(planePoint - rayOrigin, planeNormal) / denom;
    
    if (t >= 0.0f)
    {
        intersectionPoint = rayOrigin + t * rayDir;
        return true;
    }
    
    return false;
}

osg::Vec3 MathUtils::glmToOsg(const glm::vec3& vec)
{
    return osg::Vec3(vec.x, vec.y, vec.z);
}

glm::vec3 MathUtils::osgToGlm(const osg::Vec3& vec)
{
    return glm::vec3(vec.x(), vec.y(), vec.z());
}

MathUtils::ArcParameters MathUtils::calculateArcFromThreePoints(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3)
{
    ArcParameters params;
    
    // 计算法向量
    glm::vec3 a = p2 - p1;
    glm::vec3 b = p3 - p2;
    params.normal = normalize(glm::cross(a, b));
    
    // 计算圆心
    glm::vec3 midAB = (p1 + p2) * 0.5f;
    glm::vec3 midBC = (p2 + p3) * 0.5f;
    
    glm::vec3 perpA = glm::cross(a, params.normal);
    glm::vec3 perpB = glm::cross(b, params.normal);
    
    // 解线性方程组找圆心
    float t = 0.0f;
    if (glm::length(perpA) > EPSILON && glm::length(perpB) > EPSILON)
    {
        glm::vec3 diff = midBC - midAB;
        float denom = glm::dot(perpA, perpB);
        if (std::abs(denom) > EPSILON)
        {
            t = glm::dot(diff, perpB) / denom;
        }
    }
    
    params.center = midAB + t * perpA;
    params.radius = glm::length(p1 - params.center);
    
    // 计算起始和结束角度
    glm::vec3 ref = normalize(p1 - params.center);
    glm::vec3 perpRef = normalize(glm::cross(params.normal, ref));
    
    glm::vec3 v1 = normalize(p1 - params.center);
    glm::vec3 v3 = normalize(p3 - params.center);
    
    params.startAngle = atan2(glm::dot(v1, perpRef), glm::dot(v1, ref));
    params.endAngle = atan2(glm::dot(v3, perpRef), glm::dot(v3, ref));
    
    // 确保角度范围正确
    if (params.endAngle < params.startAngle)
        params.endAngle += 2.0f * PI;
    
    // 计算扫掠角度
    params.sweepAngle = params.endAngle - params.startAngle;
    if (params.sweepAngle < 0)
        params.sweepAngle += 2.0f * PI;
    
    // 计算坐标轴
    params.uAxis = ref;
    params.vAxis = normalize(glm::cross(params.normal, ref));
    
    return params;
}

std::vector<glm::vec3> MathUtils::generateArcPoints(const ArcParameters& params, int segments)
{
    std::vector<glm::vec3> points;
    points.reserve(segments + 1);
    
    if (params.radius <= 0)
        return points;
    
    float angleRange = params.sweepAngle;
    
    for (int i = 0; i <= segments; ++i)
    {
        float t = static_cast<float>(i) / segments;
        float angle = params.startAngle + t * angleRange;
        
        glm::vec3 point = params.center + params.radius * (
            std::cos(angle) * params.uAxis + 
            std::sin(angle) * params.vAxis
        );
        points.push_back(point);
    }
    
    return points;
}

glm::vec3 MathUtils::evaluateBezierPoint(const std::vector<glm::vec3>& controlPoints, float t)
{
    if (controlPoints.empty())
        return glm::vec3(0);
    
    // De Casteljau算法
    std::vector<glm::vec3> tempPoints = controlPoints;
    
    while (tempPoints.size() > 1)
    {
        std::vector<glm::vec3> newPoints;
        for (size_t j = 0; j < tempPoints.size() - 1; ++j)
        {
            newPoints.push_back(glm::mix(tempPoints[j], tempPoints[j+1], t));
        }
        tempPoints = newPoints;
    }
    
    return tempPoints.empty() ? glm::vec3(0) : tempPoints[0];
}

std::vector<glm::vec3> MathUtils::generateBezierCurve(const std::vector<glm::vec3>& controlPoints, int steps)
{
    std::vector<glm::vec3> curvePoints;
    curvePoints.reserve(steps + 1);
    
    if (controlPoints.size() < 2)
        return curvePoints;
    
    for (int i = 0; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / steps;
        glm::vec3 point = evaluateBezierPoint(controlPoints, t);
        curvePoints.push_back(point);
    }
    
    return curvePoints;
}

MathUtils::ConeParameters MathUtils::calculateConeParameters(const glm::vec3& base, const glm::vec3& apex, float radius)
{
    ConeParameters params;
    params.base = base;
    params.apex = apex;
    params.radius = radius;
    params.axis = normalize(apex - base);
    params.height = distance(apex, base);
    
    // 计算垂直于轴的两个正交向量
    if (std::abs(params.axis.z) < 0.9f)
    {
        params.uAxis = normalize(glm::cross(params.axis, glm::vec3(0, 0, 1)));
    }
    else
    {
        params.uAxis = normalize(glm::cross(params.axis, glm::vec3(1, 0, 0)));
    }
    params.vAxis = normalize(glm::cross(params.axis, params.uAxis));
    
    return params;
}

float MathUtils::calculateConeVolume(const ConeParameters& params)
{
    return (1.0f / 3.0f) * PI * params.radius * params.radius * params.height;
}

float MathUtils::calculateConeSurfaceArea(const ConeParameters& params)
{
    float slantHeight = std::sqrt(params.radius * params.radius + params.height * params.height);
    return PI * params.radius * (params.radius + slantHeight);
}

glm::vec3 MathUtils::calculateConeCenter(const ConeParameters& params)
{
    return (params.base + params.apex) * 0.5f;
}

MathUtils::SphereParameters MathUtils::calculateSphereParameters(const glm::vec3& center, float radius, int segments)
{
    SphereParameters params;
    params.center = center;
    params.radius = radius;
    params.segments = segments;
    return params;
}

float MathUtils::calculateSphereVolume(const SphereParameters& params)
{
    return (4.0f / 3.0f) * PI * params.radius * params.radius * params.radius;
}

float MathUtils::calculateSphereSurfaceArea(const SphereParameters& params)
{
    return 4.0f * PI * params.radius * params.radius;
}

glm::vec3 MathUtils::calculateSphereCenter(const SphereParameters& params)
{
    return params.center;
}

MathUtils::BoxParameters MathUtils::calculateBoxParameters(const glm::vec3& min, const glm::vec3& max)
{
    BoxParameters params;
    params.min = min;
    params.max = max;
    params.size = max - min;
    params.center = (min + max) * 0.5f;
    return params;
}

float MathUtils::calculateBoxVolume(const BoxParameters& params)
{
    return params.size.x * params.size.y * params.size.z;
}

float MathUtils::calculateBoxSurfaceArea(const BoxParameters& params)
{
    return 2.0f * (params.size.x * params.size.y + params.size.y * params.size.z + params.size.z * params.size.x);
}

glm::vec3 MathUtils::calculateBoxCenter(const BoxParameters& params)
{
    return params.center;
}

glm::vec3 MathUtils::calculateBoxSize(const BoxParameters& params)
{
    return params.size;
}

MathUtils::CylinderParameters MathUtils::calculateCylinderParameters(const glm::vec3& base, const glm::vec3& top, float radius)
{
    CylinderParameters params;
    params.base = base;
    params.top = top;
    params.radius = radius;
    params.axis = normalize(top - base);
    params.height = distance(top, base);
    
    // 计算垂直于轴的两个正交向量
    if (std::abs(params.axis.z) < 0.9f)
    {
        params.uAxis = normalize(glm::cross(params.axis, glm::vec3(0, 0, 1)));
    }
    else
    {
        params.uAxis = normalize(glm::cross(params.axis, glm::vec3(1, 0, 0)));
    }
    params.vAxis = normalize(glm::cross(params.axis, params.uAxis));
    
    return params;
}

float MathUtils::calculateCylinderVolume(const CylinderParameters& params)
{
    return PI * params.radius * params.radius * params.height;
}

float MathUtils::calculateCylinderSurfaceArea(const CylinderParameters& params)
{
    return 2.0f * PI * params.radius * (params.radius + params.height);
}

glm::vec3 MathUtils::calculateCylinderCenter(const CylinderParameters& params)
{
    return (params.base + params.top) * 0.5f;
}

MathUtils::TorusParameters MathUtils::calculateTorusParameters(const glm::vec3& center, float majorRadius, float minorRadius, const glm::vec3& axis)
{
    TorusParameters params;
    params.center = center;
    params.majorRadius = majorRadius;
    params.minorRadius = minorRadius;
    params.axis = normalize(axis);
    
    // 计算垂直于轴的两个正交向量
    if (std::abs(params.axis.z) < 0.9f)
    {
        params.uAxis = normalize(glm::cross(params.axis, glm::vec3(0, 0, 1)));
    }
    else
    {
        params.uAxis = normalize(glm::cross(params.axis, glm::vec3(1, 0, 0)));
    }
    params.vAxis = normalize(glm::cross(params.axis, params.uAxis));
    
    return params;
}

float MathUtils::calculateTorusVolume(const TorusParameters& params)
{
    return 2.0f * PI * PI * params.majorRadius * params.minorRadius * params.minorRadius;
}

float MathUtils::calculateTorusSurfaceArea(const TorusParameters& params)
{
    return 4.0f * PI * PI * params.majorRadius * params.minorRadius;
}

glm::vec3 MathUtils::calculateTorusCenter(const TorusParameters& params)
{
    return params.center;
}

MathUtils::TriangleParameters MathUtils::calculateTriangleParameters(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3)
{
    TriangleParameters params;
    params.v1 = v1;
    params.v2 = v2;
    params.v3 = v3;
    params.center = (v1 + v2 + v3) / 3.0f;
    params.normal = calculateNormal(v1, v2, v3);
    params.area = calculateTriangleArea(params);
    return params;
}

float MathUtils::calculateTriangleArea(const TriangleParameters& params)
{
    glm::vec3 edge1 = params.v2 - params.v1;
    glm::vec3 edge2 = params.v3 - params.v1;
    return 0.5f * glm::length(glm::cross(edge1, edge2));
}

glm::vec3 MathUtils::calculateTriangleCenter(const TriangleParameters& params)
{
    return params.center;
}

glm::vec3 MathUtils::calculateTriangleNormal(const TriangleParameters& params)
{
    return params.normal;
}

MathUtils::QuadParameters MathUtils::calculateQuadParameters(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4)
{
    QuadParameters params;
    params.v1 = v1;
    params.v2 = v2;
    params.v3 = v3;
    params.v4 = v4;
    params.center = (v1 + v2 + v3 + v4) / 4.0f;
    params.normal = calculateNormal(v1, v2, v3);
    params.area = calculateQuadArea(params);
    return params;
}

float MathUtils::calculateQuadArea(const QuadParameters& params)
{
    // 将四边形分解为两个三角形计算面积
    TriangleParameters tri1 = calculateTriangleParameters(params.v1, params.v2, params.v3);
    TriangleParameters tri2 = calculateTriangleParameters(params.v1, params.v3, params.v4);
    return calculateTriangleArea(tri1) + calculateTriangleArea(tri2);
}

glm::vec3 MathUtils::calculateQuadCenter(const QuadParameters& params)
{
    return params.center;
}

glm::vec3 MathUtils::calculateQuadNormal(const QuadParameters& params)
{
    return params.normal;
}

MathUtils::PolygonParameters MathUtils::calculatePolygonParameters(const std::vector<glm::vec3>& vertices)
{
    PolygonParameters params;
    params.vertices = vertices;
    params.center = calculateCentroid(vertices);
    params.triangleIndices = triangulatePolygon(vertices);
    params.area = calculatePolygonArea(params);
    params.normal = calculatePolygonNormal(params);
    return params;
}

float MathUtils::calculatePolygonArea(const PolygonParameters& params)
{
    if (params.vertices.size() < 3)
        return 0.0f;
    
    // 使用三角形分解计算面积
    float area = 0.0f;
    for (size_t i = 0; i < params.triangleIndices.size(); i += 3)
    {
        if (i + 2 < params.triangleIndices.size())
        {
            glm::vec3 v1 = params.vertices[params.triangleIndices[i]];
            glm::vec3 v2 = params.vertices[params.triangleIndices[i + 1]];
            glm::vec3 v3 = params.vertices[params.triangleIndices[i + 2]];
            
            TriangleParameters triParams = calculateTriangleParameters(v1, v2, v3);
            area += calculateTriangleArea(triParams);
        }
    }
    return area;
}

glm::vec3 MathUtils::calculatePolygonCenter(const PolygonParameters& params)
{
    return params.center;
}

glm::vec3 MathUtils::calculatePolygonNormal(const PolygonParameters& params)
{
    if (params.vertices.size() < 3)
        return glm::vec3(0, 0, 1);
    
    // 使用前三个点计算法向量
    return calculateNormal(params.vertices[0], params.vertices[1], params.vertices[2]);
}

std::vector<unsigned int> MathUtils::triangulatePolygon(const std::vector<glm::vec3>& vertices)
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

MathUtils::LineParameters MathUtils::calculateLineParameters(const glm::vec3& start, const glm::vec3& end)
{
    LineParameters params;
    params.start = start;
    params.end = end;
    params.direction = normalize(end - start);
    params.length = distance(start, end);
    params.center = (start + end) * 0.5f;
    return params;
}

float MathUtils::calculateLineLength(const LineParameters& params)
{
    return params.length;
}

glm::vec3 MathUtils::calculateLineCenter(const LineParameters& params)
{
    return params.center;
}

glm::vec3 MathUtils::calculateLineDirection(const LineParameters& params)
{
    return params.direction;
}

MathUtils::CubeParameters MathUtils::calculateCubeParameters(const glm::vec3& center, float size)
{
    CubeParameters params;
    params.center = center;
    params.size = size;
    float halfSize = size * 0.5f;
    params.min = center - glm::vec3(halfSize);
    params.max = center + glm::vec3(halfSize);
    return params;
}

float MathUtils::calculateCubeVolume(const CubeParameters& params)
{
    return params.size * params.size * params.size;
}

float MathUtils::calculateCubeSurfaceArea(const CubeParameters& params)
{
    return 6.0f * params.size * params.size;
}

glm::vec3 MathUtils::calculateCubeCenter(const CubeParameters& params)
{
    return params.center;
}

float MathUtils::calculateCubeSize(const CubeParameters& params)
{
    return params.size;
}

glm::vec3 MathUtils::evaluateBezier(const std::vector<glm::vec3>& controlPoints, float t)
{
    if (controlPoints.empty())
        return glm::vec3(0.0f);
    
    if (controlPoints.size() == 1)
        return controlPoints[0];
    
    // De Casteljau算法
    std::vector<glm::vec3> tempPoints = controlPoints;
    
    while (tempPoints.size() > 1)
    {
        std::vector<glm::vec3> newPoints;
        for (size_t i = 0; i < tempPoints.size() - 1; ++i)
        {
            newPoints.push_back(lerp(tempPoints[i], tempPoints[i + 1], t));
        }
        tempPoints = newPoints;
    }
    
    return tempPoints[0];
}

glm::vec3 MathUtils::evaluateSpline(const std::vector<glm::vec3>& controlPoints, float t)
{
    if (controlPoints.size() < 2)
        return controlPoints.empty() ? glm::vec3(0.0f) : controlPoints[0];
    
    if (controlPoints.size() == 2)
        return lerp(controlPoints[0], controlPoints[1], t);
    
    // Catmull-Rom样条
    int n = static_cast<int>(controlPoints.size()) - 1;
    float scaledT = t * n;
    int i = static_cast<int>(scaledT);
    float localT = scaledT - i;
    
    i = std::clamp(i, 0, n - 1);
    
    glm::vec3 p0 = (i > 0) ? controlPoints[i - 1] : controlPoints[i];
    glm::vec3 p1 = controlPoints[i];
    glm::vec3 p2 = controlPoints[i + 1];
    glm::vec3 p3 = (i + 2 < static_cast<int>(controlPoints.size())) ? controlPoints[i + 2] : controlPoints[i + 1];
    
    float t2 = localT * localT;
    float t3 = t2 * localT;
    
    return 0.5f * (
        (2.0f * p1) +
        (-p0 + p2) * localT +
        (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
        (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
    );
}

glm::mat4 MathUtils::createRotationMatrix(const glm::vec3& axis, float angle)
{
    return glm::rotate(glm::mat4(1.0f), angle, axis);
}

glm::mat4 MathUtils::createTranslationMatrix(const glm::vec3& translation)
{
    return glm::translate(glm::mat4(1.0f), translation);
}

glm::mat4 MathUtils::createScaleMatrix(const glm::vec3& scale)
{
    return glm::scale(glm::mat4(1.0f), scale);
}

bool MathUtils::isEqual(float a, float b, float epsilon)
{
    return std::abs(a - b) < epsilon;
}

bool MathUtils::isZero(float value, float epsilon)
{
    return std::abs(value) < epsilon;
}

bool MathUtils::isEqual(const glm::vec3& a, const glm::vec3& b, float epsilon)
{
    return isEqual(a.x, b.x, epsilon) && isEqual(a.y, b.y, epsilon) && isEqual(a.z, b.z, epsilon);
} 

// ============= 基础几何计算函数 =============

// 通过三点计算圆心和半径
bool MathUtils::calculateCircleCenterAndRadius(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, 
                                               glm::vec3& center, float& radius)
{
    // 计算两个向量
    glm::vec3 v1 = p2 - p1;
    glm::vec3 v2 = p3 - p1;
    
    // 检查三点是否共线
    glm::vec3 cross = glm::cross(v1, v2);
    if (glm::length(cross) < EPSILON) {
        return false; // 三点共线，无法构成圆
    }
    
    // 计算平面法向量
    glm::vec3 normal = glm::normalize(cross);
    
    // 计算圆心
    // 使用三点圆的几何方法
    float d1 = glm::dot(p1, p1);
    float d2 = glm::dot(p2, p2);
    float d3 = glm::dot(p3, p3);
    
    float denominator = 2.0f * (p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y));
    
    if (std::abs(denominator) < EPSILON) {
        return false;
    }
    
    center.x = (d1 * (p2.y - p3.y) + d2 * (p3.y - p1.y) + d3 * (p1.y - p2.y)) / denominator;
    center.y = (d1 * (p3.x - p2.x) + d2 * (p1.x - p3.x) + d3 * (p2.x - p1.x)) / denominator;
    
    // 对于3D情况，我们投影到最适合的平面
    // 这里简化处理，假设z坐标为三点的平均值
    center.z = (p1.z + p2.z + p3.z) / 3.0f;
    
    // 计算半径
    radius = glm::distance(center, p1);
    
    return true;
}

// 生成圆弧上的点
std::vector<glm::vec3> MathUtils::generateArcPointsFromThreePoints(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, int segments)
{
    std::vector<glm::vec3> points;
    
    glm::vec3 center;
    float radius;
    
    if (!calculateCircleCenterAndRadius(p1, p2, p3, center, radius)) {
        // 三点共线，返回直线段
        for (int i = 0; i <= segments; ++i) {
            float t = static_cast<float>(i) / segments;
            if (t <= 0.5f) {
                points.push_back(lerp(p1, p2, t * 2.0f));
            } else {
                points.push_back(lerp(p2, p3, (t - 0.5f) * 2.0f));
            }
        }
        return points;
    }
    
    // 计算起始角度和结束角度
    glm::vec3 v1 = glm::normalize(p1 - center);
    glm::vec3 v2 = glm::normalize(p2 - center);
    glm::vec3 v3 = glm::normalize(p3 - center);
    
    // 计算角度
    float angle1 = std::atan2(v1.y, v1.x);
    float angle2 = std::atan2(v2.y, v2.x);
    float angle3 = std::atan2(v3.y, v3.x);
    
    // 确保角度顺序正确
    while (angle2 < angle1) angle2 += 2.0f * PI;
    while (angle3 < angle2) angle3 += 2.0f * PI;
    
    // 生成圆弧点
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        float angle = angle1 + t * (angle3 - angle1);
        
        glm::vec3 point;
        point.x = center.x + radius * std::cos(angle);
        point.y = center.y + radius * std::sin(angle);
        point.z = center.z; // 简化处理，保持z坐标不变
        
        points.push_back(point);
    }
    
    return points;
}

// 计算多边形的法向量
glm::vec3 MathUtils::calculatePolygonNormal(const std::vector<glm::vec3>& vertices)
{
    if (vertices.size() < 3) {
        return glm::vec3(0.0f, 0.0f, 1.0f); // 默认向上
    }
    
    glm::vec3 normal(0.0f);
    
    // 使用Newell方法计算法向量
    for (size_t i = 0; i < vertices.size(); ++i) {
        const glm::vec3& v1 = vertices[i];
        const glm::vec3& v2 = vertices[(i + 1) % vertices.size()];
        
        normal.x += (v1.y - v2.y) * (v1.z + v2.z);
        normal.y += (v1.z - v2.z) * (v1.x + v2.x);
        normal.z += (v1.x - v2.x) * (v1.y + v2.y);
    }
    
    return glm::normalize(normal);
}

// 生成线段的顶点
std::vector<glm::vec3> MathUtils::generateLineVertices(const glm::vec3& start, const glm::vec3& end)
{
    return {start, end};
}

// 生成矩形的顶点
std::vector<glm::vec3> MathUtils::generateRectangleVertices(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& p4)
{
    return {p1, p2, p3, p4};
}

// 生成三角形的顶点和法向量
std::vector<glm::vec3> MathUtils::generateTriangleVertices(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, glm::vec3& normal)
{
    normal = calculateNormal(v1, v2, v3);
    return {v1, v2, v3};
}

// 生成四边形的顶点和法向量（三角化）
std::vector<glm::vec3> MathUtils::generateQuadVertices(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4, std::vector<glm::vec3>& normals)
{
    // 将四边形分成两个三角形
    glm::vec3 normal1 = calculateNormal(v1, v2, v3);
    glm::vec3 normal2 = calculateNormal(v1, v3, v4);
    
    normals = {normal1, normal1, normal1, normal2, normal2, normal2};
    
    // 返回两个三角形的顶点
    return {v1, v2, v3, v1, v3, v4};
}

 