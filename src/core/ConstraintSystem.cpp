#include "ConstraintSystem.h"
#include "../util/MathUtils.h"
#include <algorithm>
#include <cmath>

namespace constraint 
{

// ============= 基础约束函数实现 =============

Point3D noConstraint(const Point3D& inputPoint, 
                    const std::vector<Point3D>& points)
{
    return inputPoint; // 直接返回输入点，无约束
}

Point3D planeConstraint(const Point3D& inputPoint, 
                       const std::vector<Point3D>& points)
{
    // 如果有至少3个点，投影到这3个点构成的平面
    if (points.size() >= 3) {
        glm::vec3 p1 = glm::vec3(points[0].x(), points[0].y(), points[0].z());
        glm::vec3 p2 = glm::vec3(points[1].x(), points[1].y(), points[1].z());
        glm::vec3 p3 = glm::vec3(points[2].x(), points[2].y(), points[2].z());
        
        // 检查三点是否共线
        glm::vec3 v1 = p2 - p1;
        glm::vec3 v2 = p3 - p1;
        glm::vec3 cross = glm::cross(v1, v2);
        
        if (glm::length(cross) < 1e-6f) {
            // 三点共线，降级为线约束
            return lineConstraint(inputPoint, points);
        }
        
        // 计算平面法向量
        glm::vec3 normal = MathUtils::calculateNormal(p1, p2, p3);
        
        // 投影输入点到平面
        glm::vec3 inputVec = glm::vec3(inputPoint.x(), inputPoint.y(), inputPoint.z());
        glm::vec3 projectedPoint = MathUtils::projectPointOnPlane(inputVec, normal, p1);
        
        return Point3D(projectedPoint.x, projectedPoint.y, projectedPoint.z);
    }
    
    return inputPoint; // 如果无法构成平面，返回原点
}

Point3D lineConstraint(const Point3D& inputPoint, 
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

Point3D zPlaneConstraint(const Point3D& inputPoint, 
                        const std::vector<Point3D>& points)
{
    // 如果有控制点，使用第一个点的Z坐标作为约束平面
    float constraintZ = 0.0f;
    
    if (!points.empty()) {
        constraintZ = points[0].z();
    }
    
    return Point3D(inputPoint.x(), inputPoint.y(), constraintZ);
}

Point3D verticalToBaseConstraint(const Point3D& inputPoint, 
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

Point3D perpendicularToLastTwoPointsConstraint(const Point3D& inputPoint, 
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

Point3D circleConstraint(const Point3D& inputPoint, 
                        const std::vector<Point3D>& points)
{
    // 检查是否有至少2个点
    if (points.size() >= 2) {
        Point3D pointA = points[0];     // 第一个点A
        Point3D pointB = points[1];     // 第二个点B
        
        // 计算AB的距离，这将作为约束半径
        float targetRadius = sqrt(
            (pointB.x() - pointA.x()) * (pointB.x() - pointA.x()) +
            (pointB.y() - pointA.y()) * (pointB.y() - pointA.y()) +
            (pointB.z() - pointA.z()) * (pointB.z() - pointA.z())
        );
        
        // 如果AB距离太小，直接返回B点
        if (targetRadius < 1e-6f) {
            return pointB;
        }
        
        // 计算从A到输入点的向量
        glm::vec3 pointAVec = glm::vec3(pointA.x(), pointA.y(), pointA.z());
        glm::vec3 inputVec = glm::vec3(inputPoint.x(), inputPoint.y(), inputPoint.z());
        glm::vec3 toInput = inputVec - pointAVec;
        
        // 如果输入点就在A点，返回B点
        if (glm::length(toInput) < 1e-6f) {
            return pointB;
        }
        
        // 将输入点约束到以A为圆心、|AB|为半径的球面上
        // 保持输入点相对于A的方向，但距离调整为|AB|
        glm::vec3 normalizedDirection = glm::normalize(toInput);
        glm::vec3 constrainedPoint = pointAVec + normalizedDirection * targetRadius;
        
        return Point3D(constrainedPoint.x, constrainedPoint.y, constrainedPoint.z);
    }
    
    return inputPoint; // 如果无法构成约束条件，返回原点
}

// ============= 约束函数组合器实现 =============

StageConstraintFunction combineStageConstraints(const std::vector<StageConstraintFunction>& constraints)
{
    return [constraints](const Point3D& inputPoint, 
                        const std::vector<std::vector<Point3D>>& pointss) -> Point3D {
        Point3D result = inputPoint;
        
        // 依次执行每个阶段约束函数
        for (const auto& constraint : constraints) {
            if (constraint) {
                result = constraint(result, pointss);
            }
        }
        
        return result;
    };
}

// ============= 约束器生成器实现 =============

StageConstraintFunction createConstraintCall(ConstraintFunction constraintFunc, const std::vector<std::pair<int, int>>& indices)
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

Point3D perpendicularToCirclePlaneConstraint(const Point3D& inputPoint, 
                                             const std::vector<Point3D>& points)
{
    // 需要至少3个点来确定圆平面
    if (points.size() >= 3) {
        Point3D circleCenter = points[0];   // A: 圆心
        Point3D pointB = points[1];         // B: 圆上第一个点
        Point3D pointC = points[2];         // C: 圆上第二个点
        
        // 转换为glm向量进行计算
        glm::vec3 centerVec = glm::vec3(circleCenter.x(), circleCenter.y(), circleCenter.z());
        glm::vec3 bVec = glm::vec3(pointB.x(), pointB.y(), pointB.z());
        glm::vec3 cVec = glm::vec3(pointC.x(), pointC.y(), pointC.z());
        glm::vec3 inputVec = glm::vec3(inputPoint.x(), inputPoint.y(), inputPoint.z());
        
        // 计算圆平面的两个向量
        glm::vec3 vectorAB = bVec - centerVec;
        glm::vec3 vectorAC = cVec - centerVec;
        
        // 计算圆平面的法向量
        glm::vec3 normal = glm::cross(vectorAB, vectorAC);
        
        // 检查法向量是否为零（三点共线的情况）
        if (glm::length(normal) < 1e-6f) {
            // 三点共线，无法确定平面，返回原点
            return inputPoint;
        }
        
        // 标准化法向量
        normal = glm::normalize(normal);
        
        // 计算从圆心到输入点的向量
        glm::vec3 toInput = inputVec - centerVec;
        
        // 计算输入点在法向量方向上的投影长度
        float projectionLength = glm::dot(toInput, normal);
        
        // 约束后的点 = 圆心 + 投影长度 * 法向量
        glm::vec3 constrainedPoint = centerVec + projectionLength * normal;
        
        return Point3D(constrainedPoint.x, constrainedPoint.y, constrainedPoint.z);
    }
    
    return inputPoint; // 如果无法构成约束条件，返回原点
}

}