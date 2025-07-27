#include "VertexShapeUtils.h"
#include <osg/StateSet>
#include <osg/PolygonMode>
#include <osg/LineWidth>
#include <osg/Point>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

osg::ref_ptr<osg::Geometry> VertexShapeUtils::createVertexShapeGeometry(
    const osg::Vec3Array* vertices,
    PointShape3D shape,
    double size,
    int segments)
{
    if (!vertices || vertices->empty()) {
        return nullptr;
    }

    // 创建合成几何体
    osg::ref_ptr<osg::Geometry> combinedGeometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> allVertices = new osg::Vec3Array;
    
    // 特殊处理Point_Dot3D - 使用原生点绘制
    if (shape == Point_Dot3D) {
        // 直接添加所有顶点
        for (size_t i = 0; i < vertices->size(); ++i) {
            allVertices->push_back((*vertices)[i]);
        }
        
        // 设置几何体属性
        combinedGeometry->setVertexArray(allVertices);
        
        // 使用原生点绘制
        osg::ref_ptr<osg::DrawArrays> drawArrays = 
            new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, allVertices->size());
        combinedGeometry->addPrimitiveSet(drawArrays);
        
        // 设置点大小
        osg::ref_ptr<osg::StateSet> stateSet = combinedGeometry->getOrCreateStateSet();
        stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        
        // 设置点大小属性
        osg::ref_ptr<osg::Point> pointSize = new osg::Point;
        pointSize->setSize(static_cast<float>(size));
        stateSet->setAttributeAndModes(pointSize, osg::StateAttribute::ON);
        
        return combinedGeometry;
    }
    
    // 其他形状使用原有逻辑
    std::vector<unsigned int> allIndices;
    unsigned int currentIndex = 0;

    // 为每个顶点生成形状
    for (size_t i = 0; i < vertices->size(); ++i) {
        const osg::Vec3& position = (*vertices)[i];
        
        std::vector<osg::Vec3> shapeVertices;
        std::vector<unsigned int> shapeIndices;
        
        // 根据形状类型生成顶点
        switch (shape) {
            case Point_Circle3D:
                generateCircleVertices(position, size * 0.5, segments, shapeVertices, shapeIndices);
                break;
            case Point_Square3D:
                generateSquareVertices(position, size, shapeVertices, shapeIndices);
                break;
            case Point_Triangle3D:
                generateTriangleVertices(position, size, shapeVertices, shapeIndices);
                break;
            case Point_Diamond3D:
                generateDiamondVertices(position, size, shapeVertices, shapeIndices);
                break;
            case Point_Cross3D:
                generateCrossVertices(position, size, shapeVertices, shapeIndices);
                break;
            case Point_Star3D:
                generateStarVertices(position, size, shapeVertices, shapeIndices);
                break;
            default:
                // 默认使用圆形
                generateCircleVertices(position, size * 0.5, segments, shapeVertices, shapeIndices);
                break;
        }
        
        // 添加顶点到总数组
        for (const auto& vertex : shapeVertices) {
            allVertices->push_back(vertex);
        }
        
        // 调整索引并添加到总索引数组
        for (unsigned int index : shapeIndices) {
            allIndices.push_back(currentIndex + index);
        }
        
        currentIndex += shapeVertices.size();
    }

    // 设置几何体属性
    combinedGeometry->setVertexArray(allVertices);

    // 添加绘制元素
    if (!allIndices.empty()) {
        osg::ref_ptr<osg::DrawElementsUInt> drawElements = 
            new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, allIndices.size(), &allIndices[0]);
        combinedGeometry->addPrimitiveSet(drawElements);
    }

    // 设置渲染状态
    osg::ref_ptr<osg::StateSet> stateSet = combinedGeometry->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);

    return combinedGeometry;
}

osg::ref_ptr<osg::Geometry> VertexShapeUtils::createSingleVertexShapeGeometry(
    const osg::Vec3& position,
    PointShape3D shape,
    double size,
    int segments)
{
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->push_back(position);
    return createVertexShapeGeometry(vertices, shape, size, segments);
}

void VertexShapeUtils::generateCircleVertices(
    const osg::Vec3& center,
    double radius,
    int segments,
    std::vector<osg::Vec3>& vertices,
    std::vector<unsigned int>& indices)
{
    vertices.clear();
    indices.clear();

    // 获取朝向相机的方向向量
    auto [up, right] = calculateCameraFacingVectors(center);
    
    // 添加中心点
    vertices.push_back(center);
    
    // 生成圆周上的点
    for (int i = 0; i <= segments; ++i) {
        double angle = 2.0 * M_PI * i / segments;
        double x = radius * cos(angle);
        double y = radius * sin(angle);
        
        osg::Vec3 point = center + right * x + up * y;
        vertices.push_back(point);
    }
    
    // 生成三角形索引
    for (int i = 1; i <= segments; ++i) {
        indices.push_back(0);           // 中心点
        indices.push_back(i);           // 当前点
        indices.push_back(i % segments + 1); // 下一个点
    }
}

void VertexShapeUtils::generateSquareVertices(
    const osg::Vec3& center,
    double size,
    std::vector<osg::Vec3>& vertices,
    std::vector<unsigned int>& indices)
{
    vertices.clear();
    indices.clear();

    auto [up, right] = calculateCameraFacingVectors(center);
    
    double halfSize = size * 0.5;
    
    // 四个角点
    vertices.push_back(center + right * (-halfSize) + up * (-halfSize)); // 左下
    vertices.push_back(center + right * halfSize + up * (-halfSize));    // 右下
    vertices.push_back(center + right * halfSize + up * halfSize);       // 右上
    vertices.push_back(center + right * (-halfSize) + up * halfSize);    // 左上
    
    // 两个三角形
    indices = {0, 1, 2, 0, 2, 3};
}

void VertexShapeUtils::generateTriangleVertices(
    const osg::Vec3& center,
    double size,
    std::vector<osg::Vec3>& vertices,
    std::vector<unsigned int>& indices)
{
    vertices.clear();
    indices.clear();

    auto [up, right] = calculateCameraFacingVectors(center);
    
    double radius = size * 0.5;
    
    // 三个顶点，等边三角形
    for (int i = 0; i < 3; ++i) {
        double angle = 2.0 * M_PI * i / 3.0 - M_PI / 2.0; // 从上方开始
        double x = radius * cos(angle);
        double y = radius * sin(angle);
        
        osg::Vec3 point = center + right * x + up * y;
        vertices.push_back(point);
    }
    
    indices = {0, 1, 2};
}

void VertexShapeUtils::generateDiamondVertices(
    const osg::Vec3& center,
    double size,
    std::vector<osg::Vec3>& vertices,
    std::vector<unsigned int>& indices)
{
    vertices.clear();
    indices.clear();

    auto [up, right] = calculateCameraFacingVectors(center);
    
    double halfSize = size * 0.5;
    
    // 四个顶点，形成菱形
    vertices.push_back(center + up * halfSize);        // 上
    vertices.push_back(center + right * halfSize);     // 右
    vertices.push_back(center + up * (-halfSize));     // 下
    vertices.push_back(center + right * (-halfSize));  // 左
    
    // 两个三角形
    indices = {0, 1, 2, 0, 2, 3};
}

void VertexShapeUtils::generateCrossVertices(
    const osg::Vec3& center,
    double size,
    std::vector<osg::Vec3>& vertices,
    std::vector<unsigned int>& indices)
{
    vertices.clear();
    indices.clear();

    auto [up, right] = calculateCameraFacingVectors(center);
    
    double halfSize = size * 0.5;
    double thickness = size * 0.1; // 十字的厚度
    
    // 垂直条的顶点
    vertices.push_back(center + right * (-thickness) + up * halfSize);     // 0
    vertices.push_back(center + right * thickness + up * halfSize);        // 1
    vertices.push_back(center + right * thickness + up * (-halfSize));     // 2
    vertices.push_back(center + right * (-thickness) + up * (-halfSize));  // 3
    
    // 水平条的顶点
    vertices.push_back(center + right * (-halfSize) + up * thickness);     // 4
    vertices.push_back(center + right * halfSize + up * thickness);        // 5
    vertices.push_back(center + right * halfSize + up * (-thickness));     // 6
    vertices.push_back(center + right * (-halfSize) + up * (-thickness));  // 7
    
    // 垂直条的三角形
    indices = {0, 1, 2, 0, 2, 3};
    // 水平条的三角形
    indices.insert(indices.end(), {4, 5, 6, 4, 6, 7});
}

void VertexShapeUtils::generateStarVertices(
    const osg::Vec3& center,
    double size,
    std::vector<osg::Vec3>& vertices,
    std::vector<unsigned int>& indices)
{
    vertices.clear();
    indices.clear();

    auto [up, right] = calculateCameraFacingVectors(center);
    
    double outerRadius = size * 0.5;
    double innerRadius = outerRadius * 0.4; // 内半径是外半径的40%
    int points = 5; // 五角星
    
    // 添加中心点
    vertices.push_back(center);
    
    // 生成星形的外顶点和内顶点
    for (int i = 0; i < points * 2; ++i) {
        double angle = 2.0 * M_PI * i / (points * 2) - M_PI / 2.0;
        double radius = (i % 2 == 0) ? outerRadius : innerRadius;
        
        double x = radius * cos(angle);
        double y = radius * sin(angle);
        
        osg::Vec3 point = center + right * x + up * y;
        vertices.push_back(point);
    }
    
    // 生成三角形索引
    for (int i = 1; i <= points * 2; ++i) {
        indices.push_back(0);           // 中心点
        indices.push_back(i);           // 当前点
        indices.push_back((i % (points * 2)) + 1); // 下一个点
    }
}

std::pair<osg::Vec3, osg::Vec3> VertexShapeUtils::calculateCameraFacingVectors(const osg::Vec3& center)
{
    // 简化版本：假设相机朝向Z轴负方向，上向量为Y轴正方向
    // 在实际应用中，应该从相机控制器获取真实的视图方向
    osg::Vec3 up(0.0, 1.0, 0.0);
    osg::Vec3 right(1.0, 0.0, 0.0);
    
    return std::make_pair(up, right);
} 
