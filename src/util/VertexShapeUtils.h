#pragma once
#pragma execution_character_set("utf-8")

#include "../core/Enums3D.h"
#include <osg/Geometry>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <vector>

// 前向声明
class CameraController;

/**
 * @brief 顶点形状工具类
 * 用于生成不同形状的顶点几何体（圆形、方形、三角形、菱形、十字、星形）
 */
class VertexShapeUtils
{
public:
    /**
     * @brief 设置全局相机控制器引用
     * 
     * 设置一个全局的相机控制器引用，供calculateCameraFacingVectors使用。
     * 这样就不需要在每次调用顶点形状生成函数时都传递相机控制器参数。
     * 
     * @param cameraController 相机控制器指针（可以为nullptr清除引用）
     * @note 这是一个静态函数，设置的是全局状态
     * @note 通常在OSGWidget初始化时调用，在析构时传入nullptr清理
     */
    static void setCameraController(CameraController* cameraController);
    
    /**
     * @brief 获取全局相机控制器引用
     * @return 当前设置的全局相机控制器指针，可能为nullptr
     */
    static CameraController* getCameraController();
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
     * @brief 计算面向相机的方向向量
     * 
     * 该函数计算用于生成面向相机的2D形状（如点的可视化）的正交向量对。
     * 这些向量确保生成的形状始终面向观察者，无论相机的角度如何。
     * 
     * @param center 形状的中心位置（世界坐标）
     * @param cameraController 相机控制器指针（可选，如果为空则使用全局相机控制器）
     * @return 一对正交向量（上向量，右向量），用于在屏幕空间中定位形状顶点
     * 
     * @note 如果没有可用的相机控制器，函数会降级为使用默认的固定方向
     * @note 返回的向量已经标准化并且相互正交
     */
    static std::pair<osg::Vec3, osg::Vec3> calculateCameraFacingVectors(const osg::Vec3& center, class CameraController* cameraController = nullptr);
}; 