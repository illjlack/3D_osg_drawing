#include "ConstraintSystem.h"
#include "../util/MathUtils.h"
#include <algorithm>
#include <cmath>

// ============= 基础约束函数实现 =============

Point3D ConstraintSystem::noConstraint(const Point3D& inputPoint, 
                                       const std::vector<Point3D>& points)
{
    return inputPoint; // 直接返回输入点，无约束
}

Point3D ConstraintSystem::planeConstraint(const Point3D& inputPoint, 
                                          const std::vector<Point3D>& points)
{
    // 如果有至少3个点，投影到这3个点构成的平面
    if (points.size() >= 3) {
        glm::vec3 p1 = glm::vec3(points[0].x(), points[0].y(), points[0].z());
        glm::vec3 p2 = glm::vec3(points[1].x(), points[1].y(), points[1].z());
        glm::vec3 p3 = glm::vec3(points[2].x(), points[2].y(), points[2].z());
        
        // 计算平面法向量
        glm::vec3 normal = MathUtils::calculateNormal(p1, p2, p3);
        
        // 投影输入点到平面
        glm::vec3 inputVec = glm::vec3(inputPoint.x(), inputPoint.y(), inputPoint.z());
        glm::vec3 projectedPoint = MathUtils::projectPointOnPlane(inputVec, normal, p1);
        
        return Point3D(projectedPoint.x, projectedPoint.y, projectedPoint.z);
    }
    
    return inputPoint; // 如果无法构成平面，返回原点
}

Point3D ConstraintSystem::lineConstraint(const Point3D& inputPoint, 
                                         const std::vector<Point3D>& points)
{
    // 如果有至少2个点，投影到这两点构成的直线
    if (points.size() >= 2) {
        glm::vec3 lineStart = glm::vec3(points[0].x(), points[0].y(), points[0].z());
        glm::vec3 lineEnd = glm::vec3(points[1].x(), points[1].y(), points[1].z());
        
        // 投影输入点到直线
        glm::vec3 inputVec = glm::vec3(inputPoint.x(), inputPoint.y(), inputPoint.z());
        glm::vec3 projectedPoint = MathUtils::projectPointOnLine(inputVec, lineStart, lineEnd);
        
        return Point3D(projectedPoint.x, projectedPoint.y, projectedPoint.z);
    }
    
    return inputPoint; // 如果无法构成直线，返回原点
}

Point3D ConstraintSystem::zPlaneConstraint(const Point3D& inputPoint, 
                                           const std::vector<Point3D>& points)
{
    // 如果有控制点，使用第一个点的Z坐标作为约束平面
    float constraintZ = 0.0f;
    
    if (!points.empty()) {
        constraintZ = points[0].z();
    }
    
    return Point3D(inputPoint.x(), inputPoint.y(), constraintZ);
}

Point3D ConstraintSystem::verticalToBaseConstraint(const Point3D& inputPoint, 
                                                   const std::vector<Point3D>& points)
{
    // 检查是否有至少3个点构成底面
    if (points.size() >= 3) {
        // 计算底面的中心点和法向量
        std::vector<glm::vec3> basePoints;
        for (const auto& point : points) {
            basePoints.push_back(glm::vec3(point.x(), point.y(), point.z()));
        }
        
        glm::vec3 baseCenter = MathUtils::calculateCentroid(basePoints);
        glm::vec3 baseNormal = MathUtils::calculatePolygonNormal(basePoints);
        
        // 计算输入点在垂直于底面的直线上的投影
        glm::vec3 inputVec = glm::vec3(inputPoint.x(), inputPoint.y(), inputPoint.z());
        
        // 垂直线的方向就是底面法向量
        glm::vec3 verticalDirection = baseNormal;
        
        // 计算投影：input点到垂直起点的向量在法向量上的投影
        glm::vec3 toInput = inputVec - baseCenter;
        float projectionLength = glm::dot(toInput, verticalDirection);
        
        // 约束后的点 = 垂直起点 + 投影长度 * 法向量
        glm::vec3 constrainedPoint = baseCenter + projectionLength * verticalDirection;
        
        return Point3D(constrainedPoint.x, constrainedPoint.y, constrainedPoint.z);
    }
    
    return inputPoint; // 如果无法构成底面，返回原点
}

Point3D ConstraintSystem::perpendicularToLastTwoPointsConstraint(const Point3D& inputPoint, 
                                                                 const std::vector<Point3D>& points)
{
    // 检查是否有至少2个点
    if (points.size() >= 2) {
        // 获取前两个点A和B
        Point3D pointA = points[0];
        Point3D pointB = points[1];
        
        // 计算向量AB和BC
        glm::vec3 vecA = glm::vec3(pointA.x(), pointA.y(), pointA.z());
        glm::vec3 vecB = glm::vec3(pointB.x(), pointB.y(), pointB.z());
        glm::vec3 vecC = glm::vec3(inputPoint.x(), inputPoint.y(), inputPoint.z());
        
        glm::vec3 AB = vecB - vecA;
        glm::vec3 BC = vecC - vecB;
        
        // 检查AB是否为零向量
        if (glm::length(AB) < 1e-6f) {
            return inputPoint; // AB向量太小，无法约束
        }
        
        // 计算BC在AB方向上的投影
        glm::vec3 ABNormalized = glm::normalize(AB);
        float projectionLength = glm::dot(BC, ABNormalized);
        glm::vec3 projectionOnAB = projectionLength * ABNormalized;
        
        // 计算垂直于AB的BC分量
        glm::vec3 perpendicularBC = BC - projectionOnAB;
        
        // 约束后的点C = B + 垂直分量
        glm::vec3 constrainedPoint = vecB + perpendicularBC;
        
        return Point3D(constrainedPoint.x, constrainedPoint.y, constrainedPoint.z);
    }
    
    return inputPoint; // 如果无法构成约束条件，返回原点
}

// ============= 约束函数组合器实现 =============

ConstraintSystem::ConstraintFunction ConstraintSystem::combineConstraints(const std::vector<ConstraintFunction>& constraints)
{
    return [constraints](const Point3D& inputPoint, 
                        const std::vector<Point3D>& points) -> Point3D {
        Point3D result = inputPoint;
        
        // 依次执行每个约束函数
        for (const auto& constraint : constraints) {
            if (constraint) {
                result = constraint(result, points);
            }
        }
        
        return result;
    };
}

// ============= 约束器生成器实现 =============

ConstraintSystem::StageConstraintFunction ConstraintSystem::createConstraintCall(ConstraintFunction constraintFunc, const std::vector<std::pair<int, int>>& indices)
{
    return [constraintFunc, indices](const Point3D& inputPoint, 
                                    const std::vector<std::vector<Point3D>>& pointss) -> Point3D {
        std::vector<Point3D> points;
        
        // 根据二维索引从pointss中提取相应的点
        for (const auto& index : indices) {
            int stageIndex = index.first;
            int pointIndex = index.second;
            
            assert (stageIndex >= 0 && stageIndex < static_cast<int>(pointss.size()) &&
                pointIndex >= 0 && pointIndex < static_cast<int>(pointss[stageIndex].size()) && "静态索引应该要正确");

            points.push_back(pointss[stageIndex][pointIndex]);
        }
        
        // 调用约束函数
        if (constraintFunc) {
            return constraintFunc(inputPoint, points);
        }
        
        return inputPoint; // 如果约束函数为空，返回原始输入点
    };
}