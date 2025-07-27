#pragma once
#pragma execution_character_set("utf-8")

#include "../core/Enums3D.h"
#include <osg/Geometry>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <vector>

/**
 * @brief 顶点形状工具类
 * 用于生成不同形状的顶点几何体（圆形、方形、三角形、菱形、十字、星形）
 */
class VertexShapeUtils
{
public:
    /**
     * @brief 为给定的顶点位置数组生成形状几何体
     * @param vertices 顶点位置数组
     * @param shape 顶点形状类型
     * @param size 顶点大小
     * @param segments 圆形的分段数（默认16）
     * @return 生成的几何体对象
     */
    static osg::ref_ptr<osg::Geometry> createVertexShapeGeometry(
        const osg::Vec3Array* vertices,
        PointShape3D shape,
        double size,
        int segments = 16
    );

    /**
     * @brief 为单个顶点生成形状几何体
     * @param position 顶点位置
     * @param shape 顶点形状类型
     * @param size 顶点大小
     * @param segments 圆形的分段数
     * @return 生成的几何体对象
     */
    static osg::ref_ptr<osg::Geometry> createSingleVertexShapeGeometry(
        const osg::Vec3& position,
        PointShape3D shape,
        double size,
        int segments = 16
    );

private:
    /**
     * @brief 生成圆形顶点
     * @param center 中心位置
     * @param radius 半径
     * @param segments 分段数
     * @param vertices 输出顶点数组
     * @param indices 输出索引数组
     */
    static void generateCircleVertices(
        const osg::Vec3& center,
        double radius,
        int segments,
        std::vector<osg::Vec3>& vertices,
        std::vector<unsigned int>& indices
    );

    /**
     * @brief 生成方形顶点
     * @param center 中心位置
     * @param size 大小
     * @param vertices 输出顶点数组
     * @param indices 输出索引数组
     */
    static void generateSquareVertices(
        const osg::Vec3& center,
        double size,
        std::vector<osg::Vec3>& vertices,
        std::vector<unsigned int>& indices
    );

    /**
     * @brief 生成三角形顶点
     * @param center 中心位置
     * @param size 大小
     * @param vertices 输出顶点数组
     * @param indices 输出索引数组
     */
    static void generateTriangleVertices(
        const osg::Vec3& center,
        double size,
        std::vector<osg::Vec3>& vertices,
        std::vector<unsigned int>& indices
    );

    /**
     * @brief 生成菱形顶点
     * @param center 中心位置
     * @param size 大小
     * @param vertices 输出顶点数组
     * @param indices 输出索引数组
     */
    static void generateDiamondVertices(
        const osg::Vec3& center,
        double size,
        std::vector<osg::Vec3>& vertices,
        std::vector<unsigned int>& indices
    );

    /**
     * @brief 生成十字形顶点
     * @param center 中心位置
     * @param size 大小
     * @param vertices 输出顶点数组
     * @param indices 输出索引数组
     */
    static void generateCrossVertices(
        const osg::Vec3& center,
        double size,
        std::vector<osg::Vec3>& vertices,
        std::vector<unsigned int>& indices
    );

    /**
     * @brief 生成星形顶点
     * @param center 中心位置
     * @param size 大小
     * @param vertices 输出顶点数组
     * @param indices 输出索引数组
     */
    static void generateStarVertices(
        const osg::Vec3& center,
        double size,
        std::vector<osg::Vec3>& vertices,
        std::vector<unsigned int>& indices
    );

    /**
     * @brief 计算相机朝向的方向向量
     * @param center 中心位置
     * @return 朝向摄像机的上向量和右向量
     */
    static std::pair<osg::Vec3, osg::Vec3> calculateCameraFacingVectors(const osg::Vec3& center);
}; 