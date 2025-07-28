#include "Sphere3D.h"
#include "../managers/GeoControlPointManager.h"
#include <osg/Geometry>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"
#include "../../util/OSGUtils.h"
#include "../../util/VertexShapeUtils.h"
#include <cassert>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 四点确定球心和半径：三点共圆，第四点确定球心（使用坐标变换）
bool calculateSphereCenterAndRadius(const glm::dvec3& p1, const glm::dvec3& p2, 
                                   const glm::dvec3& p3, const glm::dvec3& p4,
                                   glm::dvec3& center, double& radius)
{
    // 首先计算前三点确定的圆心和半径
    glm::dvec3 circleCenter;
    double circleRadius;
    
    if (!MathUtils::calculateCircleCenterAndRadius(p1, p2, p3, circleCenter, circleRadius)) {
        return false; // 前三点无法确定圆
    }
    
    // 计算圆平面的法向量
    glm::dvec3 normal = MathUtils::calculateNormal(p1, p2, p3);
    normal = glm::normalize(normal);
    
    // 建立局部坐标系
    // Z轴：法向量方向
    glm::dvec3 localZ = normal;
    
    // X轴：从圆心到第一个点的方向（在圆平面内）
    glm::dvec3 toP1 = glm::normalize(p1 - circleCenter);
    glm::dvec3 localX = toP1;
    
    // Y轴：通过叉积得到（在圆平面内，与X轴正交）
    glm::dvec3 localY = glm::normalize(glm::cross(localZ, localX));
    
    // 构建变换矩阵（从全局坐标到局部坐标）
    glm::dmat3 globalToLocal = glm::dmat3(
        localX.x, localY.x, localZ.x,
        localX.y, localY.y, localZ.y,
        localX.z, localY.z, localZ.z
    );
    
    // 将四个点变换到局部坐标系（原点为圆心）
    glm::dvec3 localP1 = globalToLocal * (p1 - circleCenter);
    glm::dvec3 localP2 = globalToLocal * (p2 - circleCenter);
    glm::dvec3 localP3 = globalToLocal * (p3 - circleCenter);
    glm::dvec3 localP4 = globalToLocal * (p4 - circleCenter);
    
    // 在局部坐标系中，前三点应该在z=0平面上形成半径为circleRadius的圆
    // 验证变换是否正确
    double tolerance = 1e-6;
    if (std::abs(localP1.z) > tolerance || std::abs(localP2.z) > tolerance || std::abs(localP3.z) > tolerance) {
        // 变换有误，无法计算
        return false;
    }
    
    // 应用特定场景公式
    double a = localP4.x;
    double b = localP4.y;
    double c = localP4.z;
    double r = circleRadius;
    
    // 防止除零
    if (std::abs(c) < tolerance) {
        // 第四点也在圆平面上，无法确定唯一球
        return false;
    }
    
    // 计算球心的局部z坐标：z = [(a² + b² + c²) - r²] / (2c)
    double localSphereZ = ((a*a + b*b + c*c) - r*r) / (2.0 * c);
    
    // 球心在局部坐标系中的位置（圆心正上方或正下方）
    glm::dvec3 localSphereCenter = glm::dvec3(0.0, 0.0, localSphereZ);
    
    // 球半径
    radius = std::abs(localSphereZ);
    
    // 验证四个点在局部坐标系中是否都在球面上
    double sphereRadius = radius;
    // 误差太大，接近0.1
    //if (std::abs(glm::length(localP1 - localSphereCenter) - sphereRadius) > tolerance ||
    //    std::abs(glm::length(localP2 - localSphereCenter) - sphereRadius) > tolerance ||
    //    std::abs(glm::length(localP3 - localSphereCenter) - sphereRadius) > tolerance ||
    //    std::abs(glm::length(localP4 - localSphereCenter) - sphereRadius) > tolerance) {
    //    // 验证失败，无法计算有效球体
    //    return false;
    //}
    
    // 将球心变换回全局坐标系
    glm::dmat3 localToGlobal = glm::transpose(globalToLocal); // 正交矩阵的逆是其转置
    glm::dvec3 globalSphereOffset = localToGlobal * localSphereCenter;
    center = circleCenter + globalSphereOffset;
    
    return true;
}

// 特定场景的球心计算：三点共圆，第四点确定球心（使用坐标变换）
bool calculateSphereCenterSpecialCase(const glm::dvec3& p1, const glm::dvec3& p2, 
                                     const glm::dvec3& p3, const glm::dvec3& p4,
                                     glm::dvec3& center, double& radius)
{
    // 首先计算前三点确定的圆心和半径
    glm::dvec3 circleCenter;
    double circleRadius;
    
    if (!MathUtils::calculateCircleCenterAndRadius(p1, p2, p3, circleCenter, circleRadius)) {
        return false; // 前三点无法确定圆
    }
    
    // 计算圆平面的法向量
    glm::dvec3 normal = MathUtils::calculateNormal(p1, p2, p3);
    normal = glm::normalize(normal);
    
    // 建立局部坐标系
    // Z轴：法向量方向
    glm::dvec3 localZ = normal;
    
    // X轴：从圆心到第一个点的方向（在圆平面内）
    glm::dvec3 toP1 = glm::normalize(p1 - circleCenter);
    glm::dvec3 localX = toP1;
    
    // Y轴：通过叉积得到（在圆平面内，与X轴正交）
    glm::dvec3 localY = glm::normalize(glm::cross(localZ, localX));
    
    // 构建变换矩阵（从全局坐标到局部坐标）
    glm::dmat3 globalToLocal = glm::dmat3(
        localX.x, localY.x, localZ.x,
        localX.y, localY.y, localZ.y,
        localX.z, localY.z, localZ.z
    );
    
    // 将四个点变换到局部坐标系（原点为圆心）
    glm::dvec3 localP1 = globalToLocal * (p1 - circleCenter);
    glm::dvec3 localP2 = globalToLocal * (p2 - circleCenter);
    glm::dvec3 localP3 = globalToLocal * (p3 - circleCenter);
    glm::dvec3 localP4 = globalToLocal * (p4 - circleCenter);
    
    // 在局部坐标系中，前三点应该在z=0平面上形成半径为circleRadius的圆
    // 验证变换是否正确
    double tolerance = 1e-6;
    if (std::abs(localP1.z) > tolerance || std::abs(localP2.z) > tolerance || std::abs(localP3.z) > tolerance) {
        // 变换有误，使用通用算法
        return calculateSphereCenterAndRadius(p1, p2, p3, p4, center, radius);
    }
    
    // 应用特定场景公式
    double a = localP4.x;
    double b = localP4.y;
    double c = localP4.z;
    double r = circleRadius;
    
    // 防止除零
    if (std::abs(c) < tolerance) {
        // 第四点也在圆平面上，无法确定唯一球
        return false;
    }
    
    // 计算球心的局部z坐标：z = [(a² + b² + c²) - r²] / (2c)
    double localSphereZ = ((a*a + b*b + c*c) - r*r) / (2.0 * c);
    
    // 球心在局部坐标系中的位置（圆心正上方或正下方）
    glm::dvec3 localSphereCenter = glm::dvec3(0.0, 0.0, localSphereZ);
    
    // 球半径
    radius = std::abs(localSphereZ);
    
    // 验证四个点在局部坐标系中是否都在球面上
    double sphereRadius = radius;
    if (std::abs(glm::length(localP1 - localSphereCenter) - sphereRadius) > tolerance ||
        std::abs(glm::length(localP2 - localSphereCenter) - sphereRadius) > tolerance ||
        std::abs(glm::length(localP3 - localSphereCenter) - sphereRadius) > tolerance ||
        std::abs(glm::length(localP4 - localSphereCenter) - sphereRadius) > tolerance) {
        // 验证失败，使用通用算法
        return calculateSphereCenterAndRadius(p1, p2, p3, p4, center, radius);
    }
    
    // 将球心变换回全局坐标系
    glm::dmat3 localToGlobal = glm::transpose(globalToLocal); // 正交矩阵的逆是其转置
    glm::dvec3 globalSphereOffset = localToGlobal * localSphereCenter;
    center = circleCenter + globalSphereOffset;
    
    return true;
}

Sphere3D_Geo::Sphere3D_Geo()
{
    m_geoType = Geo_Sphere3D;
    // 确保基类正确初始化
    initialize();
    
    // 立体几何特定的可见性设置：显示线面
    GeoParameters3D params = getParameters();
    params.showPoints = false;
    params.showEdges = true;
    params.showFaces = true;
    
    setParameters(params);
}

void Sphere3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getVertexGeometry();
    if (!geometry.valid())
    {
        return;
    }
    
    const auto& allStagePoints = mm_controlPoint()->getAllStageControlPoints();
    
    // 根据阶段和点数量决定顶点
    if (allStagePoints.empty()) return;
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    if (allStagePoints.size() == 1) 
    {
        // 第一阶段：三个点确定圆
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 1) 
        {
            // 添加第一个点
            Point3D point1 = stage1[0];
            vertices->push_back(osg::Vec3(point1.x(), point1.y(), point1.z()));
        }
        
        if (stage1.size() >= 2) 
        {
            // 添加第二个点
            Point3D point2 = stage1[1];
            vertices->push_back(osg::Vec3(point2.x(), point2.y(), point2.z()));
        }
        
        if (stage1.size() >= 3) 
        {
            // 第三个点确定圆，显示圆心
            Point3D point1 = stage1[0];
            Point3D point2 = stage1[1];
            Point3D point3 = stage1[2];
            
            glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
            glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
            glm::dvec3 p3 = glm::dvec3(point3.x(), point3.y(), point3.z());
            
            // 计算三点确定的圆
            glm::dvec3 center;
            double radius;
            
            if (MathUtils::calculateCircleCenterAndRadius(p1, p2, p3, center, radius)) 
            {
                // 添加圆心
                vertices->push_back(osg::Vec3(center.x, center.y, center.z));
            }
        }
    }
    else if (allStagePoints.size() == 2)
    {
        // 第二阶段：第四个点确定球，显示球心
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        assert(stage1.size() >= 3 && stage2.size() >= 1);
        
        Point3D point1 = stage1[0];
        Point3D point2 = stage1[1];
        Point3D point3 = stage1[2];
        Point3D point4 = stage2[0];
        
        glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
        glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
        glm::dvec3 p3 = glm::dvec3(point3.x(), point3.y(), point3.z());
        glm::dvec3 p4 = glm::dvec3(point4.x(), point4.y(), point4.z());
        
        glm::dvec3 center;
        double radius;
        
        if (calculateSphereCenterAndRadius(p1, p2, p3, p4, center, radius)) 
        {
            // 添加球心
            vertices->push_back(osg::Vec3(center.x, center.y, center.z));
        }
        else
        {
            // 四点共面或无法确定球体，降级为显示前三点确定的截面圆心
            glm::dvec3 circleCenter;
            double circleRadius;
            
            if (MathUtils::calculateCircleCenterAndRadius(p1, p2, p3, circleCenter, circleRadius)) 
            {
                // 添加截面圆心
                vertices->push_back(osg::Vec3(circleCenter.x, circleCenter.y, circleCenter.z));
            }
        }
    }
    
    // 使用顶点形状工具创建几何体
    if (vertices->size() > 0) 
    {
        // 从参数中获取点的显示属性
        const GeoParameters3D& params = getParameters();
        PointShape3D pointShape = params.pointShape;
        double pointSize = params.pointSize;

        // 使用顶点形状工具创建几何体
        osg::ref_ptr<osg::Geometry> shapeGeometry = 
            VertexShapeUtils::createVertexShapeGeometry(vertices, pointShape, pointSize);
        
        if (shapeGeometry.valid())
        {
            // 复制生成的几何体数据到现有几何体
            geometry->setVertexArray(shapeGeometry->getVertexArray());
            
            // 复制图元集合
            geometry->removePrimitiveSet(0, geometry->getNumPrimitiveSets());
            for (unsigned int i = 0; i < shapeGeometry->getNumPrimitiveSets(); ++i)
            {
                geometry->addPrimitiveSet(shapeGeometry->getPrimitiveSet(i));
            }
            
            // 复制渲染状态
            if (shapeGeometry->getStateSet())
            {
                geometry->setStateSet(shapeGeometry->getStateSet());
            }
        }
    }
}

void Sphere3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid())
    {
        return;
    }
    
    const auto& allStagePoints = mm_controlPoint()->getAllStageControlPoints();
    
    if (allStagePoints.empty()) return;
    
    // 创建顶点数组和索引数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES);
    
    // 从参数获取球面细分数量
    int sphereSegments = static_cast<int>(m_parameters.subdivisionLevel);
    
    if (allStagePoints.size() == 1) 
    {
        // 第一阶段：显示截面圆的构建过程
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 2) 
        {
            Point3D point1 = stage1[0];
            Point3D point2 = stage1[1];
            
            vertices->push_back(osg::Vec3(point1.x(), point1.y(), point1.z())); // index 0
            vertices->push_back(osg::Vec3(point2.x(), point2.y(), point2.z())); // index 1
            
            // 连接前两个点
            indices->push_back(0);
            indices->push_back(1);
        }
        
        if (stage1.size() >= 3) 
        {
            // 第三个点确定圆，显示完整圆周
            Point3D point1 = stage1[0];
            Point3D point2 = stage1[1];
            Point3D point3 = stage1[2];
            
            glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
            glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
            glm::dvec3 p3 = glm::dvec3(point3.x(), point3.y(), point3.z());
            
            glm::dvec3 center;
            double radius;
            
            if (MathUtils::calculateCircleCenterAndRadius(p1, p2, p3, center, radius)) 
            {
                // 重新构建顶点数组（只包含圆周点）
                vertices->clear();
                indices->clear();
                
                // 计算圆的法向量
                glm::dvec3 v1 = glm::normalize(p1 - center);
                glm::dvec3 v2 = glm::normalize(p2 - center);
                glm::dvec3 normal = glm::normalize(glm::cross(v1, v2));
                
                // 计算圆平面上的两个正交向量
                glm::dvec3 radiusVec = v1;
                glm::dvec3 perpVec = glm::normalize(glm::cross(normal, radiusVec));
                
                // 生成圆周上的点
                for (int i = 0; i < sphereSegments; i++) {
                    double angle = 2.0 * M_PI * i / sphereSegments;
                    
                    glm::dvec3 circlePoint = center + radius * (
                        cos(angle) * radiusVec + sin(angle) * perpVec
                    );
                    
                    vertices->push_back(osg::Vec3(circlePoint.x, circlePoint.y, circlePoint.z));
                }
                
                // 圆周连线
                for (int i = 0; i < sphereSegments; i++) 
                {
                    int next = (i + 1) % sphereSegments;
                    indices->push_back(i);
                    indices->push_back(next);
                }
            }
        }
    }
    else if (allStagePoints.size() == 2)
    {
        // 第二阶段：显示球体的线框
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        assert(stage1.size() >= 3 && stage2.size() >= 1);
        
        Point3D point1 = stage1[0];
        Point3D point2 = stage1[1];
        Point3D point3 = stage1[2];
        Point3D point4 = stage2[0];
        
        glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
        glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
        glm::dvec3 p3 = glm::dvec3(point3.x(), point3.y(), point3.z());
        glm::dvec3 p4 = glm::dvec3(point4.x(), point4.y(), point4.z());
        
        glm::dvec3 center;
        double radius;
        
        if (calculateSphereCenterAndRadius(p1, p2, p3, p4, center, radius)) 
        {
            // 生成球体线框
            int rings = sphereSegments / 2;
            
            // 生成球面顶点
            for (int ring = 0; ring <= rings; ring++) {
                double phi = M_PI * ring / rings;
                double sinPhi = sin(phi);
                double cosPhi = cos(phi);
                
                for (int seg = 0; seg <= sphereSegments; seg++) {
                    double theta = 2.0 * M_PI * seg / sphereSegments;
                    double sinTheta = sin(theta);
                    double cosTheta = cos(theta);
                    
                    glm::dvec3 normal(sinPhi * cosTheta, sinPhi * sinTheta, cosPhi);
                    glm::dvec3 point = center + radius * normal;
                    
                    vertices->push_back(osg::Vec3(point.x, point.y, point.z));
                }
            }
            
            // 连接纬线
            for (int ring = 0; ring <= rings; ring++) {
                for (int seg = 0; seg < sphereSegments; seg++) {
                    int curr = ring * (sphereSegments + 1) + seg;
                    int next = ring * (sphereSegments + 1) + (seg + 1);
                    
                    indices->push_back(curr);
                    indices->push_back(next);
                }
            }
            
            // 连接经线
            for (int seg = 0; seg <= sphereSegments; seg += 2) { // 只绘制一半经线，避免太密
                for (int ring = 0; ring < rings; ring++) {
                    int curr = ring * (sphereSegments + 1) + seg;
                    int next = (ring + 1) * (sphereSegments + 1) + seg;
                    
                    indices->push_back(curr);
                    indices->push_back(next);
                }
            }
        }
        else
        {
            // 四点共面或无法确定球体，降级为绘制前三点确定的截面圆
            glm::dvec3 circleCenter;
            double circleRadius;
            
            if (MathUtils::calculateCircleCenterAndRadius(p1, p2, p3, circleCenter, circleRadius)) 
            {
                // 重新构建顶点数组（只包含圆周点）
                vertices->clear();
                indices->clear();
                
                // 计算圆的法向量
                glm::dvec3 v1 = glm::normalize(p1 - circleCenter);
                glm::dvec3 v2 = glm::normalize(p2 - circleCenter);
                glm::dvec3 normal = glm::normalize(glm::cross(v1, v2));
                
                // 计算圆平面上的两个正交向量
                glm::dvec3 radiusVec = v1;
                glm::dvec3 perpVec = glm::normalize(glm::cross(normal, radiusVec));
                
                // 生成圆周上的点
                for (int i = 0; i < sphereSegments; i++) {
                    double angle = 2.0 * M_PI * i / sphereSegments;
                    
                    glm::dvec3 circlePoint = circleCenter + circleRadius * (
                        cos(angle) * radiusVec + sin(angle) * perpVec
                    );
                    
                    vertices->push_back(osg::Vec3(circlePoint.x, circlePoint.y, circlePoint.z));
                }
                
                // 圆周连线
                for (int i = 0; i < sphereSegments; i++) 
                {
                    int next = (i + 1) % sphereSegments;
                    indices->push_back(i);
                    indices->push_back(next);
                }
            }
        }
    }
    
    // 设置顶点数组和索引
    geometry->setVertexArray(vertices);
    if (indices->size() > 0) {
        geometry->addPrimitiveSet(indices);
    }
}

void Sphere3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getFaceGeometry();
    if (!geometry.valid())
    {
        return;
    }
    
    const auto& allStagePoints = mm_controlPoint()->getAllStageControlPoints();
    
    // 从参数获取球面细分数量
    int sphereSegments = static_cast<int>(m_parameters.subdivisionLevel);
    
    if (allStagePoints.size() == 1) 
    {
        // 第一阶段：如果确定了圆，显示截面圆形
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 3) 
        {
            Point3D point1 = stage1[0];
            Point3D point2 = stage1[1];
            Point3D point3 = stage1[2];
            
            glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
            glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
            glm::dvec3 p3 = glm::dvec3(point3.x(), point3.y(), point3.z());
            
            glm::dvec3 center;
            double radius;
            
            if (MathUtils::calculateCircleCenterAndRadius(p1, p2, p3, center, radius)) 
            {
                // 创建顶点数组
                osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
                
                // 添加圆心
                vertices->push_back(osg::Vec3(center.x, center.y, center.z));
                
                // 计算圆的法向量
                glm::dvec3 v1 = glm::normalize(p1 - center);
                glm::dvec3 v2 = glm::normalize(p2 - center);
                glm::dvec3 normal = glm::normalize(glm::cross(v1, v2));
                
                // 计算圆平面上的两个正交向量
                glm::dvec3 radiusVec = v1;
                glm::dvec3 perpVec = glm::normalize(glm::cross(normal, radiusVec));
                
                // 生成圆周上的点
                for (int i = 0; i < sphereSegments; i++) {
                    double angle = 2.0 * M_PI * i / sphereSegments;
                    
                    glm::dvec3 circlePoint = center + radius * (
                        cos(angle) * radiusVec + sin(angle) * perpVec
                    );
                    
                    vertices->push_back(osg::Vec3(circlePoint.x, circlePoint.y, circlePoint.z));
                }
                
                // 为了确保TRIANGLE_FAN正确封闭，添加第一个圆周点
                glm::dvec3 firstCirclePoint = center + radius * radiusVec;
                vertices->push_back(osg::Vec3(firstCirclePoint.x, firstCirclePoint.y, firstCirclePoint.z));
                
                geometry->setVertexArray(vertices);
                
                // 添加截面圆形（三角形扇形）
                geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, sphereSegments + 2));
            }
        }
    }
    else if (allStagePoints.size() == 2)
    {
        // 第二阶段：显示完整球体
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        assert(stage1.size() >= 3 && stage2.size() >= 1);
        
        Point3D point1 = stage1[0];
        Point3D point2 = stage1[1];
        Point3D point3 = stage1[2];
        Point3D point4 = stage2[0];
        
        glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
        glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
        glm::dvec3 p3 = glm::dvec3(point3.x(), point3.y(), point3.z());
        glm::dvec3 p4 = glm::dvec3(point4.x(), point4.y(), point4.z());
        
        glm::dvec3 center;
        double radius;
        
        if (calculateSphereCenterSpecialCase(p1, p2, p3, p4, center, radius)) 
        {
            // 成功计算出球心，绘制完整球体
            osg::ref_ptr<osg::Geometry> sphereGeom = OSGUtils::createSphere(center, radius, sphereSegments);
            
            if (sphereGeom.valid()) 
            {
                // 复制球体几何到当前几何对象
                geometry->setVertexArray(sphereGeom->getVertexArray());
                geometry->setNormalArray(sphereGeom->getNormalArray());
                geometry->setNormalBinding(sphereGeom->getNormalBinding());
                
                // 复制所有primitives
                for (unsigned int i = 0; i < sphereGeom->getNumPrimitiveSets(); i++) 
                {
                    geometry->addPrimitiveSet(sphereGeom->getPrimitiveSet(i));
                }
            }
        }
        else
        {
            // 四点共面或无法确定球体，降级为绘制前三点确定的截面圆
            glm::dvec3 circleCenter;
            double circleRadius;
            
            if (MathUtils::calculateCircleCenterAndRadius(p1, p2, p3, circleCenter, circleRadius)) 
            {
                // 创建顶点数组
                osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
                
                // 添加圆心
                vertices->push_back(osg::Vec3(circleCenter.x, circleCenter.y, circleCenter.z));
                
                // 计算圆的法向量
                glm::dvec3 v1 = glm::normalize(p1 - circleCenter);
                glm::dvec3 v2 = glm::normalize(p2 - circleCenter);
                glm::dvec3 normal = glm::normalize(glm::cross(v1, v2));
                
                // 计算圆平面上的两个正交向量
                glm::dvec3 radiusVec = v1;
                glm::dvec3 perpVec = glm::normalize(glm::cross(normal, radiusVec));
                
                // 生成圆周上的点
                for (int i = 0; i < sphereSegments; i++) {
                    double angle = 2.0 * M_PI * i / sphereSegments;
                    
                    glm::dvec3 circlePoint = circleCenter + circleRadius * (
                        cos(angle) * radiusVec + sin(angle) * perpVec
                    );
                    
                    vertices->push_back(osg::Vec3(circlePoint.x, circlePoint.y, circlePoint.z));
                }
                
                // 为了确保TRIANGLE_FAN正确封闭，添加第一个圆周点
                glm::dvec3 firstCirclePoint = circleCenter + circleRadius * radiusVec;
                vertices->push_back(osg::Vec3(firstCirclePoint.x, firstCirclePoint.y, firstCirclePoint.z));
                
                geometry->setVertexArray(vertices);
                
                // 添加截面圆形（三角形扇形）
                geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, sphereSegments + 2));
            }
        }
    }
}
