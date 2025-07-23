#include "ConstraintSystem.h"
#include "../util/MathUtils.h"
#include <algorithm>
#include <cmath>

// ============= 基础约束函数实现 =============

Point3D ConstraintSystem::noConstraint(const Point3D& inputPoint, 
                                       const std::vector<Point3D>& currentStagePoints,
                                       const std::vector<std::vector<Point3D>>& allStagePoints,
                                       int currentStageIndex)
{
    return inputPoint; // 直接返回输入点，无约束
}

Point3D ConstraintSystem::planeConstraint(const Point3D& inputPoint, 
                                          const std::vector<Point3D>& currentStagePoints,
                                          const std::vector<std::vector<Point3D>>& allStagePoints,
                                          int currentStageIndex)
{
    // 如果第一阶段有至少3个点，投影到第一阶段构成的平面
    if (allStagePoints.size() > 0 && allStagePoints[0].size() >= 3) {
        const auto& firstStage = allStagePoints[0];
        glm::vec3 p1 = glm::vec3(firstStage[0].x(), firstStage[0].y(), firstStage[0].z());
        glm::vec3 p2 = glm::vec3(firstStage[1].x(), firstStage[1].y(), firstStage[1].z());
        glm::vec3 p3 = glm::vec3(firstStage[2].x(), firstStage[2].y(), firstStage[2].z());
        
        // 计算平面法向量
        glm::vec3 normal = MathUtils::calculateNormal(p1, p2, p3);
        
        // 投影输入点到平面
        glm::vec3 inputVec = glm::vec3(inputPoint.x(), inputPoint.y(), inputPoint.z());
        glm::vec3 projectedPoint = MathUtils::projectPointOnPlane(inputVec, normal, p1);
        
        return Point3D(projectedPoint.x, projectedPoint.y, projectedPoint.z);
    }
    
    return inputPoint; // 如果无法构成平面，返回原点
}

Point3D ConstraintSystem::previousTrianglePlaneConstraint(const Point3D& inputPoint, 
                                                          const std::vector<Point3D>& currentStagePoints,
                                                          const std::vector<std::vector<Point3D>>& allStagePoints,
                                                          int currentStageIndex)
{
    // 检查前一阶段是否存在且有至少3个点
    if (currentStageIndex > 0 && allStagePoints.size() > currentStageIndex - 1) {
        const auto& previousStage = allStagePoints[currentStageIndex - 1];
        if (previousStage.size() >= 3) {
            glm::vec3 p1 = glm::vec3(previousStage[0].x(), previousStage[0].y(), previousStage[0].z());
            glm::vec3 p2 = glm::vec3(previousStage[1].x(), previousStage[1].y(), previousStage[1].z());
            glm::vec3 p3 = glm::vec3(previousStage[2].x(), previousStage[2].y(), previousStage[2].z());
            
            // 计算平面法向量
            glm::vec3 normal = MathUtils::calculateNormal(p1, p2, p3);
            
            // 投影输入点到平面
            glm::vec3 inputVec = glm::vec3(inputPoint.x(), inputPoint.y(), inputPoint.z());
            glm::vec3 projectedPoint = MathUtils::projectPointOnPlane(inputVec, normal, p1);
            
            return Point3D(projectedPoint.x, projectedPoint.y, projectedPoint.z);
        }
    }
    
    return inputPoint; // 如果无法构成平面，返回原点
}

Point3D ConstraintSystem::lineConstraint(const Point3D& inputPoint, 
                                         const std::vector<Point3D>& currentStagePoints,
                                         const std::vector<std::vector<Point3D>>& allStagePoints,
                                         int currentStageIndex)
{
    // 如果当前阶段或前一阶段有至少2个点，投影到这两点构成的直线
    if (!currentStagePoints.empty() && allStagePoints.size() > 0) {
        glm::vec3 lineStart, lineEnd;
        
        if (currentStagePoints.size() >= 1 && allStagePoints.size() > 0 && !allStagePoints.back().empty()) {
            // 使用最后一个控制点和当前阶段第一个点构成直线
            lineStart = glm::vec3(allStagePoints.back().back().x(), allStagePoints.back().back().y(), allStagePoints.back().back().z());
            lineEnd = glm::vec3(currentStagePoints[0].x(), currentStagePoints[0].y(), currentStagePoints[0].z());
        } else if (!allStagePoints.empty() && allStagePoints.back().size() >= 2) {
            // 使用前一阶段最后两个点构成直线
            const auto& lastStage = allStagePoints.back();
            lineStart = glm::vec3(lastStage[lastStage.size()-2].x(), lastStage[lastStage.size()-2].y(), lastStage[lastStage.size()-2].z());
            lineEnd = glm::vec3(lastStage[lastStage.size()-1].x(), lastStage[lastStage.size()-1].y(), lastStage[lastStage.size()-1].z());
        } else {
            return inputPoint; // 无法构成直线
        }
        
        // 投影输入点到直线
        glm::vec3 inputVec = glm::vec3(inputPoint.x(), inputPoint.y(), inputPoint.z());
        glm::vec3 projectedPoint = MathUtils::projectPointOnLine(inputVec, lineStart, lineEnd);
        
        return Point3D(projectedPoint.x, projectedPoint.y, projectedPoint.z);
    }
    
    return inputPoint; // 如果无法构成直线，返回原点
}

Point3D ConstraintSystem::zPlaneConstraint(const Point3D& inputPoint, 
                                           const std::vector<Point3D>& currentStagePoints,
                                           const std::vector<std::vector<Point3D>>& allStagePoints,
                                           int currentStageIndex)
{
    // 如果有之前的控制点，使用第一个点的Z坐标作为约束平面
    float constraintZ = 0.0f;
    
    if (!allStagePoints.empty() && !allStagePoints[0].empty()) {
        constraintZ = allStagePoints[0][0].z();
    } else if (!currentStagePoints.empty()) {
        constraintZ = currentStagePoints[0].z();
    }
    
    return Point3D(inputPoint.x(), inputPoint.y(), constraintZ);
}

Point3D ConstraintSystem::verticalToBaseConstraint(const Point3D& inputPoint, 
                                                   const std::vector<Point3D>& currentStagePoints,
                                                   const std::vector<std::vector<Point3D>>& allStagePoints,
                                                   int currentStageIndex)
{
    // 检查前一阶段是否存在且有至少3个点构成底面
    if (currentStageIndex > 0 && allStagePoints.size() > currentStageIndex - 1) {
        const auto& previousStage = allStagePoints[currentStageIndex - 1];
        if (previousStage.size() >= 3) {
            // 计算底面的中心点和法向量
            std::vector<glm::vec3> basePoints;
            for (const auto& point : previousStage) {
                basePoints.push_back(glm::vec3(point.x(), point.y(), point.z()));
            }
            
            glm::vec3 baseCenter = MathUtils::calculateCentroid(basePoints);
            glm::vec3 baseNormal = MathUtils::calculatePolygonNormal(basePoints);
            
            // 如果当前阶段已有点，使用该点作为垂直线的起点
            glm::vec3 verticalStart = baseCenter;
            if (!currentStagePoints.empty()) {
                verticalStart = glm::vec3(currentStagePoints[0].x(), currentStagePoints[0].y(), currentStagePoints[0].z());
            }
            
            // 计算输入点在垂直于底面的直线上的投影
            glm::vec3 inputVec = glm::vec3(inputPoint.x(), inputPoint.y(), inputPoint.z());
            
            // 垂直线的方向就是底面法向量
            glm::vec3 verticalDirection = baseNormal;
            
            // 计算投影：input点到垂直起点的向量在法向量上的投影
            glm::vec3 toInput = inputVec - verticalStart;
            float projectionLength = glm::dot(toInput, verticalDirection);
            
            // 约束后的点 = 垂直起点 + 投影长度 * 法向量
            glm::vec3 constrainedPoint = verticalStart + projectionLength * verticalDirection;
            
            return Point3D(constrainedPoint.x, constrainedPoint.y, constrainedPoint.z);
        }
    }
    
    return inputPoint; // 如果无法构成底面，返回原点
}

Point3D ConstraintSystem::perpendicularToLastTwoPointsConstraint(const Point3D& inputPoint, 
                                                                 const std::vector<Point3D>& currentStagePoints,
                                                                 const std::vector<std::vector<Point3D>>& allStagePoints,
                                                                 int currentStageIndex)
{
    // 检查前一阶段是否存在且有至少2个点
    if (currentStageIndex > 0 && allStagePoints.size() > currentStageIndex - 1) {
        const auto& previousStage = allStagePoints[currentStageIndex - 1];
        if (previousStage.size() >= 2) {
            // 获取上一阶段的最后两个点A和B
            size_t lastIndex = previousStage.size() - 1;
            Point3D pointA = previousStage[lastIndex - 1]; // 倒数第二个点
            Point3D pointB = previousStage[lastIndex];     // 最后一个点
            
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
    }
    
    return inputPoint; // 如果无法构成约束条件，返回原点
}

// ============= 约束函数组合器实现 =============

ConstraintSystem::ConstraintFunction ConstraintSystem::combineConstraints(const std::vector<ConstraintFunction>& constraints)
{
    return [constraints](const Point3D& inputPoint, 
                        const std::vector<Point3D>& currentStagePoints,
                        const std::vector<std::vector<Point3D>>& allStagePoints,
                        int currentStageIndex) -> Point3D {
        Point3D result = inputPoint;
        
        // 依次执行每个约束函数
        for (const auto& constraint : constraints) {
            if (constraint) {
                result = constraint(result, currentStagePoints, allStagePoints, currentStageIndex);
            }
        }
        
        return result;
    };
}

ConstraintSystem::ConstraintFunction ConstraintSystem::conditionalConstraint(
    std::function<bool(const Point3D&, const std::vector<Point3D>&, const std::vector<std::vector<Point3D>>&, int)> condition,
    ConstraintFunction trueConstraint,
    ConstraintFunction falseConstraint)
{
    return [condition, trueConstraint, falseConstraint](const Point3D& inputPoint, 
                                                       const std::vector<Point3D>& currentStagePoints,
                                                       const std::vector<std::vector<Point3D>>& allStagePoints,
                                                       int currentStageIndex) -> Point3D {
        if (condition && condition(inputPoint, currentStagePoints, allStagePoints, currentStageIndex)) {
            return trueConstraint ? trueConstraint(inputPoint, currentStagePoints, allStagePoints, currentStageIndex) : inputPoint;
        } else {
            return falseConstraint ? falseConstraint(inputPoint, currentStagePoints, allStagePoints, currentStageIndex) : inputPoint;
        }
    };
}
