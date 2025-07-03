#include "MathUtils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <cmath>
#include <algorithm>

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
    // Möller-Trumbore相交算法
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