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
    static StageConstraintFunction createConstraintCall(ConstraintFunction constraintFunc, const std::vector<std::pair<int, int>>& indices);
      




    // ============= 基础约束函数 =============
    
    /**
     * @brief 无约束函数
     * @param inputPoint 输入点
     * @param points 相关控制点
     * @return 未经约束的原始输入点
     */
    static Point3D noConstraint(const Point3D& inputPoint, 
                                const std::vector<Point3D>& points);
    
    /**
     * @brief 平面约束函数
     * 将输入点投影到前三个点构成的平面上
     * @param inputPoint 输入点
     * @param points 相关控制点（至少需要3个点构成平面）
     * @return 投影到平面上的点
     */
    static Point3D planeConstraint(const Point3D& inputPoint, 
                                   const std::vector<Point3D>& points);
    
    /**
     * @brief 线约束函数
     * 将输入点投影到两点构成的直线上
     * @param inputPoint 输入点
     * @param points 相关控制点（至少需要2个点构成直线）
     * @return 投影到直线上的点
     */
    static Point3D lineConstraint(const Point3D& inputPoint, 
                                  const std::vector<Point3D>& points);
    
    /**
     * @brief Z平面约束函数
     * 保持输入点的Z坐标为指定值（基于已有控制点）
     * @param inputPoint 输入点
     * @param points 相关控制点（使用第一个点的Z坐标作为约束）
     * @return Z坐标被约束的点
     */
    static Point3D zPlaneConstraint(const Point3D& inputPoint, 
                                    const std::vector<Point3D>& points);
    
    /**
     * @brief 垂直于底面的约束函数
     * 用于棱柱等3D体的高度确定，将点约束在垂直于底面的直线上
     * @param inputPoint 输入点
     * @param points 相关控制点（构成底面的点）
     * @return 垂直约束后的点
     */
    static Point3D verticalToBaseConstraint(const Point3D& inputPoint, 
                                            const std::vector<Point3D>& points);
    
    /**
     * @brief 垂直于前两点连线的约束函数
     * 将输入点约束在垂直于前两点连线的平面上
     * 即确保BC垂直于AB，其中A、B是前两个点，C是当前输入点
     * @param inputPoint 输入点
     * @param points 相关控制点（至少需要2个点作为参考线）
     * @return 垂直约束后的点
     */
    static Point3D perpendicularToLastTwoPointsConstraint(const Point3D& inputPoint, 
                                                          const std::vector<Point3D>& points);

    // ============= 约束函数组合器 =============
    
    /**
     * @brief 串联约束组合器
     * 将多个约束函数按顺序依次执行
     * @param constraints 约束函数列表
     * @return 组合后的约束函数
     */
    static ConstraintFunction combineConstraints(const std::vector<ConstraintFunction>& constraints);

private:
    // 约束系统为静态工具类，禁止实例化
    ConstraintSystem() = delete;
    ~ConstraintSystem() = delete;
    ConstraintSystem(const ConstraintSystem&) = delete;
    ConstraintSystem& operator=(const ConstraintSystem&) = delete;
}; 