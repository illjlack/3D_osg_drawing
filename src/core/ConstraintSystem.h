#pragma once
#pragma execution_character_set("utf-8")

#include <functional>
#include <vector>
#include <glm/glm.hpp>
#include "Common3D.h"

/**
 * @brief 控制点约束系统
 * 
 * 该类提供了一套完整的控制点约束功能，用于在几何图形绘制过程中
 * 对用户输入的控制点进行约束处理，确保几何图形的正确构建。
 */
class ConstraintSystem
{
public:
    // 约束函数类型定义
    // 参数：待添加的点，当前阶段的控制点，所有阶段的控制点，当前阶段索引
    // 返回：约束后的点
    typedef std::function<Point3D(const Point3D& inputPoint, 
                                  const std::vector<Point3D>& currentStagePoints,
                                  const std::vector<std::vector<Point3D>>& allStagePoints,
                                  int currentStageIndex)> ConstraintFunction;

    // ============= 基础约束函数 =============
    
    /**
     * @brief 无约束函数
     * @param inputPoint 输入点
     * @param currentStagePoints 当前阶段控制点
     * @param allStagePoints 所有阶段控制点
     * @param currentStageIndex 当前阶段索引
     * @return 未经约束的原始输入点
     */
    static Point3D noConstraint(const Point3D& inputPoint, 
                                const std::vector<Point3D>& currentStagePoints,
                                const std::vector<std::vector<Point3D>>& allStagePoints,
                                int currentStageIndex);
    
    /**
     * @brief 平面约束函数
     * 将输入点投影到第一阶段构成的平面上
     * @param inputPoint 输入点
     * @param currentStagePoints 当前阶段控制点
     * @param allStagePoints 所有阶段控制点
     * @param currentStageIndex 当前阶段索引
     * @return 投影到平面上的点
     */
    static Point3D planeConstraint(const Point3D& inputPoint, 
                                   const std::vector<Point3D>& currentStagePoints,
                                   const std::vector<std::vector<Point3D>>& allStagePoints,
                                   int currentStageIndex);
    
    /**
     * @brief 基于前一阶段平面的约束函数
     * 将输入点投影到前一阶段三点构成的平面上
     * @param inputPoint 输入点
     * @param currentStagePoints 当前阶段控制点
     * @param allStagePoints 所有阶段控制点
     * @param currentStageIndex 当前阶段索引
     * @return 投影到前一阶段平面上的点
     */
    static Point3D previousTrianglePlaneConstraint(const Point3D& inputPoint, 
                                                    const std::vector<Point3D>& currentStagePoints,
                                                    const std::vector<std::vector<Point3D>>& allStagePoints,
                                                    int currentStageIndex);
    
    /**
     * @brief 线约束函数
     * 将输入点投影到两点构成的直线上
     * @param inputPoint 输入点
     * @param currentStagePoints 当前阶段控制点
     * @param allStagePoints 所有阶段控制点
     * @param currentStageIndex 当前阶段索引
     * @return 投影到直线上的点
     */
    static Point3D lineConstraint(const Point3D& inputPoint, 
                                  const std::vector<Point3D>& currentStagePoints,
                                  const std::vector<std::vector<Point3D>>& allStagePoints,
                                  int currentStageIndex);
    
    /**
     * @brief Z平面约束函数
     * 保持输入点的Z坐标为指定值（基于已有控制点）
     * @param inputPoint 输入点
     * @param currentStagePoints 当前阶段控制点
     * @param allStagePoints 所有阶段控制点
     * @param currentStageIndex 当前阶段索引
     * @return Z坐标被约束的点
     */
    static Point3D zPlaneConstraint(const Point3D& inputPoint, 
                                    const std::vector<Point3D>& currentStagePoints,
                                    const std::vector<std::vector<Point3D>>& allStagePoints,
                                    int currentStageIndex);
    
    /**
     * @brief 垂直于底面的约束函数
     * 用于棱柱等3D体的高度确定，将点约束在垂直于底面的直线上
     * @param inputPoint 输入点
     * @param currentStagePoints 当前阶段控制点
     * @param allStagePoints 所有阶段控制点
     * @param currentStageIndex 当前阶段索引
     * @return 垂直约束后的点
     */
    static Point3D verticalToBaseConstraint(const Point3D& inputPoint, 
                                            const std::vector<Point3D>& currentStagePoints,
                                            const std::vector<std::vector<Point3D>>& allStagePoints,
                                            int currentStageIndex);

    // ============= 约束函数组合器 =============
    
    /**
     * @brief 串联约束组合器
     * 将多个约束函数按顺序依次执行
     * @param constraints 约束函数列表
     * @return 组合后的约束函数
     */
    static ConstraintFunction combineConstraints(const std::vector<ConstraintFunction>& constraints);
      
    /**
     * @brief 条件约束选择器
     * 根据条件选择不同的约束函数
     * @param condition 条件判断函数
     * @param trueConstraint 条件为真时执行的约束
     * @param falseConstraint 条件为假时执行的约束
     * @return 条件约束函数
     */
    static ConstraintFunction conditionalConstraint(
        std::function<bool(const Point3D&, const std::vector<Point3D>&, const std::vector<std::vector<Point3D>>&, int)> condition,
        ConstraintFunction trueConstraint,
        ConstraintFunction falseConstraint);

    // ============= 常用约束组合 =============
    
    /**
     * @brief 平面约束 + Z轴锁定组合
     * 先进行平面约束，再锁定Z轴
     * @return 组合约束函数
     */
    static ConstraintFunction planeAndZLockConstraint();
    
    /**
     * @brief 基于前一阶段的平面约束
     * @return 前一阶段平面约束函数
     */
    static ConstraintFunction basedOnPreviousStageConstraint();
    
    /**
     * @brief 2D平面绘制约束（锁定Z轴）
     * 用于平面图形绘制，保持所有点在同一Z平面
     * @return Z轴锁定约束函数
     */
    static ConstraintFunction flatDrawingConstraint();
    
    /**
     * @brief 3D立体约束（垂直于底面）
     * 用于3D体绘制，确保高度点垂直于底面
     * @return 3D立体约束函数
     */
    static ConstraintFunction volumeConstraint();

private:
    // 约束系统为静态工具类，禁止实例化
    ConstraintSystem() = delete;
    ~ConstraintSystem() = delete;
    ConstraintSystem(const ConstraintSystem&) = delete;
    ConstraintSystem& operator=(const ConstraintSystem&) = delete;
}; 