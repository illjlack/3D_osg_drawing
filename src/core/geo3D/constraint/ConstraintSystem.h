#pragma once
#pragma execution_character_set("utf-8")

#include <functional>
#include <vector>
#include <glm/glm.hpp>
#include "Types3D.h"

namespace Geo3D {

/**
 * @brief 控制点约束系统
 * 
 * 提供了一套完整的控制点约束功能，用于在几何图形绘制过程中
 * 对用户输入的控制点进行约束处理，确保几何图形的正确构建。
 */

// 约束系统类型定义和函数

// 实际位置的调用函数
typedef std::function<Point3D(const Point3D& inputPoint, 
    const std::vector<std::vector<Point3D>>& pointss)> StageConstraintFunction;

    // 约束函数类型定义
    typedef std::function<Point3D(const Point3D& inputPoint,
        const std::vector<Point3D>& points)> ConstraintFunction;

    // ============= 约束器生成器 =============
    
    /**
    * @brief 创建约束器调用包装器
    * @param constraintFunc 约束函数
    * @param indices 二维索引列表，每个索引包含[阶段索引, 点索引]，用于从pointss中获取对应的点
    * @return 包装后的调用函数，接受二维点数组作为参数
    */
    StageConstraintFunction createConstraintCall(ConstraintFunction constraintFunc, const std::vector<std::pair<int, int>>& indices);
      
    // ============= 约束函数组合器 =============
    /**
     * @brief 串联约束组合器
     * 将多个阶段约束函数按顺序依次执行
     * @param constraints 阶段约束函数列表
     * @return 组合后的阶段约束函数
     */
    StageConstraintFunction combineStageConstraints(const std::vector<StageConstraintFunction>& constraints);

    // ============= 基础约束函数 =============
    
    /**
     * @brief 无约束函数
     * @param inputPoint 输入点P
     * @param points 相关控制点
     * @return 未经约束的原始输入点P
     */
    Point3D noConstraint(const Point3D& inputPoint, 
                        const std::vector<Point3D>& points);
    
    /**
     * @brief 平面约束函数
     * 将输入点P投影到前三个点A、B、C构成的平面上
     * @param inputPoint 输入点P
     * @param points 相关控制点（points[0]=A, points[1]=B, points[2]=C，至少需要3个点）
     * @return 投影到平面ABC上的点P'
     */
    Point3D planeConstraint(const Point3D& inputPoint, 
                           const std::vector<Point3D>& points);
    
    /**
     * @brief 线约束函数
     * 将输入点P投影到两点A、B构成的直线上
     * @param inputPoint 输入点P
     * @param points 相关控制点（points[0]=A, points[1]=B，至少需要2个点）
     * @return 投影到直线AB上的点P'
     */
    Point3D lineConstraint(const Point3D& inputPoint, 
                          const std::vector<Point3D>& points);
    
    /**
     * @brief Z平面约束函数
     * 保持输入点P的Z坐标与参考点A相同
     * @param inputPoint 输入点P
     * @param points 相关控制点（points[0]=A，使用A点的Z坐标作为约束平面）
     * @return Z坐标被约束的点P'(Px, Py, Az)
     */
    Point3D zPlaneConstraint(const Point3D& inputPoint, 
                            const std::vector<Point3D>& points);
    
    /**
     * @brief 垂直于底面的约束函数
     * 将输入点P约束在垂直于底面的直线上
     * 底面由多个点A、B、C...构成，P被约束到通过底面中心且垂直于底面的直线上
     * @param inputPoint 输入点P
     * @param points 相关控制点（points[0]=A, points[1]=B, points[2]=C...，构成底面的点）
     * @return 垂直约束后的点P'，使得P'在过底面中心垂直于底面ABC...的直线上
     */
    Point3D verticalToBaseConstraint(const Point3D& inputPoint, 
                                    const std::vector<Point3D>& points);
    
    /**
     * @brief 垂直于底面的约束函数（智能版本）
     * 如果有3个或更多点，将输入点约束在垂直于底面平面的直线上
     * 如果只有2个点，则降级为垂直于连线的约束
     * @param inputPoint 输入点P
     * @param points 相关控制点（至少需要2个点，优先使用前3个点确定底面）
     * @return 垂直约束后的点P'
     */
    Point3D perpendicularToLastTwoPointsConstraint(const Point3D& inputPoint, 
                                                  const std::vector<Point3D>& points);

    /**
     * @brief 垂直于两点连线的约束函数
     * 将输入点P约束在过B点垂直于线段AB的平面上
     * 确保BP垂直于AB，其中A、B是前两个点，P是当前输入点
     * @param inputPoint 输入点P
     * @param points 相关控制点（points[0]=A, points[1]=B，至少需要2个点作为参考线）
     * @return 垂直约束后的点P'，满足BP'⊥AB (B点加垂直分量)
     */
    Point3D perpendicularToTwoPointsConstraint(const Point3D& inputPoint, 
                                              const std::vector<Point3D>& points);

    /**
     * @brief 圆形约束函数
     * 将输入点P约束到以点A为中心、与点B等距的球面上
     * 使得三点A、B、P满足|AB| = |AP|，从而能确定一个圆且都在圆的边上
     * @param inputPoint 输入点P
     * @param points 相关控制点（points[0]=A作为圆心，points[1]=B用于确定半径）
     * @return 约束后的点P'，满足|AP'| = |AB|
     */
    Point3D circleConstraint(const Point3D& inputPoint, 
                            const std::vector<Point3D>& points);

    /**
     * @brief 垂直于圆平面的约束函数
     * 将输入点P约束在垂直于三点A、B、C构成的圆平面且通过圆心的直线上
     * 适用于圆锥的锥顶点约束，确保锥顶点在圆形底面的法向量方向上
     * @param inputPoint 输入点P（锥顶点候选）
     * @param points 相关控制点（points[0]=A是圆心，points[1]=B、points[2]=C确定圆）
     * @return 约束到垂直直线上的点P'，使得P'在通过圆心A且垂直于圆平面ABC的直线上
     */
    Point3D perpendicularToCirclePlaneConstraint(const Point3D& inputPoint, 
                                                 const std::vector<Point3D>& points);

    /**
     * @brief 等长约束函数
     * 将输入点P约束到使得两条线段长度相等的位置
     * 确保|CP| = |AB|，其中A、B构成参考线段，C是新线段的起点，P是输入点
     * @param inputPoint 输入点P
     * @param points 相关控制点（points[0]=A, points[1]=B构成参考线段，points[2]=C是新线段起点）
     * @return 约束后的点P'，满足|CP'| = |AB|
     */
    Point3D equalLengthConstraint(const Point3D& inputPoint, 
                                 const std::vector<Point3D>& points);

} // namespace Geo3D

