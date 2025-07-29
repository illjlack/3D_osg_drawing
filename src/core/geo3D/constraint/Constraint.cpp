#include "Constraint.h"
#include "../utils/MathUtils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Geo3D {

// 点约束
void PointConstraint::apply(const std::vector<Point3D>& points, std::vector<Point3D>& result) {
    if (points.empty() || result.empty()) return;
    
    // 将所有点移动到目标点
    for (auto& point : result) {
        point.position = target.position;
    }
}

// 线约束
void LineConstraint::apply(const std::vector<Point3D>& points, std::vector<Point3D>& result) {
    if (points.empty() || result.empty()) return;
    
    // 计算线的方向向量
    glm::dvec3 direction = end.position - start.position;
    double length = glm::length(direction);
    if (length < 1e-6) return;
    direction = direction / length;
    
    // 将所有点投影到线上
    for (size_t i = 0; i < result.size(); ++i) {
        glm::dvec3 v = points[i].position - start.position;
        double t = glm::dot(v, direction);
        result[i].position = start.position + direction * t;
    }
}

// 平面约束
void PlaneConstraint::apply(const std::vector<Point3D>& points, std::vector<Point3D>& result) {
    if (points.empty() || result.empty()) return;
    
    // 计算平面法向量
    glm::dvec3 v1 = p2.position - p1.position;
    glm::dvec3 v2 = p3.position - p1.position;
    glm::dvec3 normal = glm::normalize(glm::cross(v1, v2));
    
    // 将所有点投影到平面上
    for (size_t i = 0; i < result.size(); ++i) {
        glm::dvec3 v = points[i].position - p1.position;
        double d = glm::dot(v, normal);
        result[i].position = points[i].position - normal * d;
    }
}

// 圆约束
void CircleConstraint::apply(const std::vector<Point3D>& points, std::vector<Point3D>& result) {
    if (points.empty() || result.empty()) return;
    
    // 计算圆心和法向量
    glm::dvec3 v1 = p2.position - p1.position;
    glm::dvec3 v2 = p3.position - p1.position;
    glm::dvec3 normal = glm::normalize(glm::cross(v1, v2));
    
    // 计算圆心
    glm::dvec3 center = MathUtils::calculateCircleCenter(p1.position, p2.position, p3.position);
    double radius = glm::length(p1.position - center);
    
    // 将所有点投影到圆上
    for (size_t i = 0; i < result.size(); ++i) {
        // 首先投影到平面上
        glm::dvec3 v = points[i].position - center;
        double d = glm::dot(v, normal);
        glm::dvec3 projected = points[i].position - normal * d;
        
        // 然后投影到圆上
        glm::dvec3 toCenter = projected - center;
        double currentRadius = glm::length(toCenter);
        if (currentRadius > 1e-6) {
            result[i].position = center + (toCenter / currentRadius) * radius;
        } else {
            result[i].position = center + glm::dvec3(radius, 0.0, 0.0);
        }
    }
}

// 球面约束
void SphereConstraint::apply(const std::vector<Point3D>& points, std::vector<Point3D>& result) {
    if (points.empty() || result.empty()) return;
    
    // 计算球心和半径
    glm::dvec3 center = MathUtils::calculateSphereCenter(p1.position, p2.position, p3.position, p4.position);
    double radius = glm::length(p1.position - center);
    
    // 将所有点投影到球面上
    for (size_t i = 0; i < result.size(); ++i) {
        glm::dvec3 v = points[i].position - center;
        double length = glm::length(v);
        if (length > 1e-6) {
            result[i].position = center + (v / length) * radius;
        } else {
            result[i].position = center + glm::dvec3(radius, 0.0, 0.0);
        }
    }
}

// 对称约束
void SymmetryConstraint::apply(const std::vector<Point3D>& points, std::vector<Point3D>& result) {
    if (points.empty() || result.empty()) return;
    
    // 计算对称平面的法向量
    glm::dvec3 v1 = p2.position - p1.position;
    glm::dvec3 v2 = p3.position - p1.position;
    glm::dvec3 normal = glm::normalize(glm::cross(v1, v2));
    
    // 将所有点关于平面对称
    for (size_t i = 0; i < result.size(); ++i) {
        glm::dvec3 v = points[i].position - p1.position;
        double d = glm::dot(v, normal);
        result[i].position = points[i].position - 2.0 * normal * d;
    }
}

// 平行约束
void ParallelConstraint::apply(const std::vector<Point3D>& points, std::vector<Point3D>& result) {
    if (points.empty() || result.empty()) return;
    
    // 计算参考向量
    glm::dvec3 refDir = glm::normalize(p2.position - p1.position);
    
    // 将所有向量调整为平行
    for (size_t i = 0; i < result.size(); i += 2) {
        if (i + 1 >= result.size()) break;
        
        glm::dvec3 currentDir = result[i + 1].position - result[i].position;
        double length = glm::length(currentDir);
        if (length > 1e-6) {
            result[i + 1].position = result[i].position + refDir * length;
        }
    }
}

// 垂直约束
void PerpendicularConstraint::apply(const std::vector<Point3D>& points, std::vector<Point3D>& result) {
    if (points.empty() || result.empty()) return;
    
    // 计算参考向量
    glm::dvec3 refDir = glm::normalize(p2.position - p1.position);
    
    // 将所有向量调整为垂直
    for (size_t i = 0; i < result.size(); i += 2) {
        if (i + 1 >= result.size()) break;
        
        glm::dvec3 currentDir = result[i + 1].position - result[i].position;
        double length = glm::length(currentDir);
        if (length > 1e-6) {
            // 创建一个垂直向量
            glm::dvec3 perpDir = glm::normalize(glm::cross(refDir, glm::cross(currentDir, refDir)));
            result[i + 1].position = result[i].position + perpDir * length;
        }
    }
}

// 相等约束
void EqualConstraint::apply(const std::vector<Point3D>& points, std::vector<Point3D>& result) {
    if (points.empty() || result.empty()) return;
    
    // 计算参考长度
    double refLength = glm::length(p2.position - p1.position);
    
    // 将所有向量调整为相等长度
    for (size_t i = 0; i < result.size(); i += 2) {
        if (i + 1 >= result.size()) break;
        
        glm::dvec3 currentDir = result[i + 1].position - result[i].position;
        double length = glm::length(currentDir);
        if (length > 1e-6) {
            result[i + 1].position = result[i].position + (currentDir / length) * refLength;
        }
    }
}

} // namespace Geo3D

