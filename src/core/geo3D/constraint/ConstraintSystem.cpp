#include "ConstraintSystem.h"
#include "../utils/MathUtils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Geo3D {

void ConstraintSystem::addPointConstraint(const Point3D& target) {
    auto constraint = std::make_shared<PointConstraint>();
    constraint->target = target;
    constraints.push_back(constraint);
}

void ConstraintSystem::addLineConstraint(const Point3D& start, const Point3D& end) {
    auto constraint = std::make_shared<LineConstraint>();
    constraint->start = start;
    constraint->end = end;
    constraints.push_back(constraint);
}

void ConstraintSystem::addPlaneConstraint(const Point3D& p1, const Point3D& p2, const Point3D& p3) {
    auto constraint = std::make_shared<PlaneConstraint>();
    constraint->p1 = p1;
    constraint->p2 = p2;
    constraint->p3 = p3;
    constraints.push_back(constraint);
}

void ConstraintSystem::addCircleConstraint(const Point3D& p1, const Point3D& p2, const Point3D& p3) {
    auto constraint = std::make_shared<CircleConstraint>();
    constraint->p1 = p1;
    constraint->p2 = p2;
    constraint->p3 = p3;
    constraints.push_back(constraint);
}

void ConstraintSystem::addSphereConstraint(const Point3D& p1, const Point3D& p2, const Point3D& p3, const Point3D& p4) {
    auto constraint = std::make_shared<SphereConstraint>();
    constraint->p1 = p1;
    constraint->p2 = p2;
    constraint->p3 = p3;
    constraint->p4 = p4;
    constraints.push_back(constraint);
}

void ConstraintSystem::addSymmetryConstraint(const Point3D& p1, const Point3D& p2, const Point3D& p3) {
    auto constraint = std::make_shared<SymmetryConstraint>();
    constraint->p1 = p1;
    constraint->p2 = p2;
    constraint->p3 = p3;
    constraints.push_back(constraint);
}

void ConstraintSystem::addParallelConstraint(const Point3D& p1, const Point3D& p2) {
    auto constraint = std::make_shared<ParallelConstraint>();
    constraint->p1 = p1;
    constraint->p2 = p2;
    constraints.push_back(constraint);
}

void ConstraintSystem::addPerpendicularConstraint(const Point3D& p1, const Point3D& p2) {
    auto constraint = std::make_shared<PerpendicularConstraint>();
    constraint->p1 = p1;
    constraint->p2 = p2;
    constraints.push_back(constraint);
}

void ConstraintSystem::addEqualConstraint(const Point3D& p1, const Point3D& p2) {
    auto constraint = std::make_shared<EqualConstraint>();
    constraint->p1 = p1;
    constraint->p2 = p2;
    constraints.push_back(constraint);
}

void ConstraintSystem::solve(std::vector<Point3D>& points) {
    if (points.empty() || constraints.empty()) return;
    
    // 创建临时结果向量
    std::vector<Point3D> result = points;
    
    // 应用所有约束
        for (const auto& constraint : constraints) {
        constraint->apply(points, result);
        points = result;
    }
}

} // namespace Geo3D

