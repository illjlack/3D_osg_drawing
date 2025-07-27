#include "Cone3D.h"
#include "../managers/GeoControlPointManager.h"
#include <osg/Geometry>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"
#include "../../util/VertexShapeUtils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <cassert>

Cone3D_Geo::Cone3D_Geo()
{
    m_geoType = Geo_Cone3D;
    // 确保基类正确初始化
    initialize();
    
    // 立体几何特定的可见性设置：显示线面
    GeoParameters3D params = getParameters();
    params.showPoints = false;
    params.showEdges = true;
    params.showFaces = true;

    setParameters(params);
}

void Cone3D_Geo::buildVertexGeometries()
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
    
    // 从参数获取圆周细分数量
    int circleSegments = static_cast<int>(m_parameters.subdivisionLevel);
    
    if (allStagePoints.size() == 1) 
    {
        // 第一阶段
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 1) 
        {
            // 添加圆心点
            Point3D center = stage1[0];
            vertices->push_back(osg::Vec3(center.x(), center.y(), center.z()));
        }
        
        if (stage1.size() >= 2) 
        {
            // 第一阶段只添加半径点，不生成完整圆周
            Point3D radiusPoint = stage1[1];
            vertices->push_back(osg::Vec3(radiusPoint.x(), radiusPoint.y(), radiusPoint.z()));
        }
    }
    else if (allStagePoints.size() == 2) 
    {
        // 第二阶段
        const auto& stage1 = allStagePoints[0];
        // 添加圆心点
        Point3D center = stage1[0];
        vertices->push_back(osg::Vec3(center.x(), center.y(), center.z()));
    }
    else if (allStagePoints.size() >= 3) 
    {
        return;
        // 第三阶段
        const auto& stage3 = allStagePoints[2];
        Point3D top = stage3[0];
        vertices->push_back(osg::Vec3(top.x(), top.y(), top.z()));
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

void Cone3D_Geo::buildEdgeGeometries()
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
    
    // 从参数获取圆周细分数量
    int circleSegments = static_cast<int>(m_parameters.subdivisionLevel);
    
    if (allStagePoints.size() == 1) 
    {
        // 第一阶段：圆心到半径点的线（如果有2个点）
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 2) 
        {
            // 第一阶段只绘制从圆心到半径点的一条线
            Point3D center = stage1[0];
            Point3D radiusPoint = stage1[1];
            
            // 添加半径点
            vertices->push_back(osg::Vec3(center.x(), center.y(), center.z())); // index 0

            vertices->push_back(osg::Vec3(radiusPoint.x(), radiusPoint.y(), radiusPoint.z())); // index 1
            
            // 半径线（从圆心到半径点）
            indices->push_back(0); // 圆心
            indices->push_back(1); // 半径点
        }
    }
    else if (allStagePoints.size() == 2) 
    {
        // 第二阶段：使用圆心、半径点、第三点确定圆后显示圆周线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        assert(stage1.size() >= 2 && stage2.size() >= 1);

        // stage1[0] 是圆心，stage1[1] 是半径点，stage2[0] 是第三点
        Point3D centerPoint = stage1[0];
        Point3D radiusPoint = stage1[1];
        Point3D thirdPoint = stage2[0];

        glm::dvec3 center = glm::dvec3(centerPoint.x(), centerPoint.y(), centerPoint.z());
        glm::dvec3 p1 = glm::dvec3(radiusPoint.x(), radiusPoint.y(), radiusPoint.z());
        glm::dvec3 p2 = glm::dvec3(thirdPoint.x(), thirdPoint.y(), thirdPoint.z());

        // 计算半径
        double radius = glm::distance(center, p1);

        // 计算圆的平面法向量
        glm::dvec3 v1 = glm::normalize(p1 - center);
        glm::dvec3 v2 = glm::normalize(p2 - center);
        glm::dvec3 normal = glm::normalize(glm::cross(v1, v2));

        // 检查是否三点共线
        if (glm::length(glm::cross(v1, v2)) < 1e-6f) 
        {
            // 三点共线，退化为直线处理
            vertices->push_back(osg::Vec3(centerPoint.x(), centerPoint.y(), centerPoint.z()));
            vertices->push_back(osg::Vec3(radiusPoint.x(), radiusPoint.y(), radiusPoint.z()));
            vertices->push_back(osg::Vec3(thirdPoint.x(), thirdPoint.y(), thirdPoint.z()));

            // 连接三点的线段
            indices->push_back(0);
            indices->push_back(1);
            indices->push_back(1);
            indices->push_back(2);
        }
        else 
        {
            // 计算圆平面上的两个正交向量
            glm::dvec3 radiusVec = v1;  // 从圆心到半径点的向量
            glm::dvec3 perpVec = glm::normalize(glm::cross(normal, radiusVec));

            // 生成圆周上的点
            for (int i = 0; i < circleSegments; i++) {
                double angle = 2.0 * M_PI * i / circleSegments;

                glm::dvec3 circlePoint = center + radius * (
                    cos(angle) * radiusVec + sin(angle) * perpVec
                    );

                vertices->push_back(osg::Vec3(circlePoint.x, circlePoint.y, circlePoint.z)); // index i
            }

            // 圆周连线
            for (int i = 0; i < circleSegments; i++) 
            {
                int next = (i + 1) % circleSegments;
                indices->push_back(i);
                indices->push_back(next);
            }
        }   
    }
    else if (allStagePoints.size() >= 3) 
    {
        // 第三阶段：完整圆锥的边线（圆周 + 母线）
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 2 && stage2.size() >= 1 && stage3.size() >= 1) {
            // stage1[0] 是圆心，stage1[1] 是半径点，stage2[0] 是第三点，stage3[0] 是锥顶点
            Point3D centerPoint = stage1[0];
            Point3D radiusPoint = stage1[1];
            Point3D thirdPoint = stage2[0];
            Point3D apexPoint = stage3[0];
            
            glm::dvec3 center = glm::dvec3(centerPoint.x(), centerPoint.y(), centerPoint.z());
            glm::dvec3 p1 = glm::dvec3(radiusPoint.x(), radiusPoint.y(), radiusPoint.z());
            glm::dvec3 p2 = glm::dvec3(thirdPoint.x(), thirdPoint.y(), thirdPoint.z());
            glm::dvec3 apex = glm::dvec3(apexPoint.x(), apexPoint.y(), apexPoint.z());
            
            // 计算半径
            double radius = glm::distance(center, p1);
            
            // 计算圆的平面法向量
            glm::dvec3 v1 = glm::normalize(p1 - center);
            glm::dvec3 v2 = glm::normalize(p2 - center);
            glm::dvec3 normal = glm::normalize(glm::cross(v1, v2));
            
            // 检查是否三点共线
            if (glm::length(glm::cross(v1, v2)) < 1e-6f) {
                // 三点共线，退化处理
                vertices->push_back(osg::Vec3(centerPoint.x(), centerPoint.y(), centerPoint.z()));
                vertices->push_back(osg::Vec3(radiusPoint.x(), radiusPoint.y(), radiusPoint.z()));
                vertices->push_back(osg::Vec3(thirdPoint.x(), thirdPoint.y(), thirdPoint.z()));
                vertices->push_back(osg::Vec3(apexPoint.x(), apexPoint.y(), apexPoint.z()));
                
                // 连接线段
                indices->push_back(0); indices->push_back(1);
                indices->push_back(1); indices->push_back(2);
                indices->push_back(0); indices->push_back(3);
                indices->push_back(1); indices->push_back(3);
                indices->push_back(2); indices->push_back(3);
            } else {
                // 检测锥顶点是否在底面平面上（退化情况）
                double distanceToPlane = std::abs(glm::dot(apex - center, normal));
                bool isDegenerate = distanceToPlane < 1e-4f; // 很小的阈值
                
                // 添加圆心
                vertices->push_back(osg::Vec3(center.x, center.y, center.z)); // index 0
                
                // 计算圆平面上的两个正交向量
                glm::dvec3 radiusVec = v1;  // 从圆心到半径点的向量
                glm::dvec3 perpVec = glm::normalize(glm::cross(normal, radiusVec));
                
                // 生成圆周上的点
                for (int i = 0; i < circleSegments; i++) {
                    double angle = 2.0 * M_PI * i / circleSegments;
                    
                    glm::dvec3 circlePoint = center + radius * (
                        cos(angle) * radiusVec + sin(angle) * perpVec
                    );
                    
                    vertices->push_back(osg::Vec3(circlePoint.x, circlePoint.y, circlePoint.z)); // index 1+i
                }
                
                // 圆周连线
                for (int i = 0; i < circleSegments; i++) {
                    int next = (i + 1) % circleSegments;
                    indices->push_back(1 + i);
                    indices->push_back(1 + next);
                }
                
                if (!isDegenerate) {
                    // 只有当锥顶点不在底面平面上时，才添加锥顶点和母线
                    
                    // 添加锥顶点
                    vertices->push_back(osg::Vec3(apexPoint.x(), apexPoint.y(), apexPoint.z())); // index circleSegments+1
                    
                    // 母线（从锥顶到圆周上的点）
                    for (int i = 0; i < circleSegments; i += 2) { // 只绘制一半的母线，避免太密
                        indices->push_back(circleSegments + 1); // 锥顶
                        indices->push_back(1 + i);               // 圆周点
                    }
                }
            }
        }
    }
    
    // 设置顶点数组和索引
    geometry->setVertexArray(vertices);
    geometry->addPrimitiveSet(indices);
}

void Cone3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();

    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getFaceGeometry();
    if (!geometry.valid())
    {
        return;
    }
    const auto& allStagePoints = mm_controlPoint()->getAllStageControlPoints();
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 从参数获取圆周细分数量
    int circleSegments = static_cast<int>(m_parameters.subdivisionLevel);
    
    if (allStagePoints.size() == 2) 
    {
        // 第二阶段：使用圆心、半径点、第三点确定圆后显示底面圆形
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 2 && stage2.size() >= 1) 
        {
            // stage1[0] 是圆心，stage1[1] 是半径点，stage2[0] 是第三点
            Point3D centerPoint = stage1[0];
            Point3D radiusPoint = stage1[1];
            Point3D thirdPoint = stage2[0];
            
            glm::dvec3 center = glm::dvec3(centerPoint.x(), centerPoint.y(), centerPoint.z());
            glm::dvec3 p1 = glm::dvec3(radiusPoint.x(), radiusPoint.y(), radiusPoint.z());
            glm::dvec3 p2 = glm::dvec3(thirdPoint.x(), thirdPoint.y(), thirdPoint.z());
            
            // 计算半径
            double radius = glm::distance(center, p1);
            
            // 计算圆的平面法向量
            glm::dvec3 v1 = glm::normalize(p1 - center);
            glm::dvec3 v2 = glm::normalize(p2 - center);
            glm::dvec3 normal = glm::normalize(glm::cross(v1, v2));
            
            // 检查是否三点共线
            if (glm::length(glm::cross(v1, v2)) >= 1e-6f) 
            {
                // 添加圆心
                vertices->push_back(osg::Vec3(center.x, center.y, center.z)); // 圆心
                
                // 计算圆平面上的两个正交向量
                glm::dvec3 radiusVec = v1;  // 从圆心到半径点的向量
                glm::dvec3 perpVec = glm::normalize(glm::cross(normal, radiusVec));
                
                // 生成圆周上的点
                for (int i = 0; i < circleSegments; i++) {
                    double angle = 2.0 * M_PI * i / circleSegments;
                    
                    glm::dvec3 circlePoint = center + radius * (
                        cos(angle) * radiusVec + sin(angle) * perpVec
                    );
                    
                    vertices->push_back(osg::Vec3(circlePoint.x, circlePoint.y, circlePoint.z));
                }
                
                // 为了确保TRIANGLE_FAN正确封闭，添加第一个圆周点
                glm::dvec3 firstCirclePoint = center + radius * radiusVec; // angle = 0
                vertices->push_back(osg::Vec3(firstCirclePoint.x, firstCirclePoint.y, firstCirclePoint.z));
                
                // 添加底面圆形（三角形扇形）
                geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, circleSegments + 2));
            }
            // 如果三点共线，不绘制面
        }
    }
    else if (allStagePoints.size() >= 3) 
    {
        // 第三阶段：完整圆锥（底面 + 侧面）
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 2 && stage2.size() >= 1 && stage3.size() >= 1) {
            // stage1[0] 是圆心，stage1[1] 是半径点，stage2[0] 是第三点，stage3[0] 是锥顶点
            Point3D centerPoint = stage1[0];
            Point3D radiusPoint = stage1[1];
            Point3D thirdPoint = stage2[0];
            Point3D apexPoint = stage3[0];
            
            glm::dvec3 center = glm::dvec3(centerPoint.x(), centerPoint.y(), centerPoint.z());
            glm::dvec3 p1 = glm::dvec3(radiusPoint.x(), radiusPoint.y(), radiusPoint.z());
            glm::dvec3 p2 = glm::dvec3(thirdPoint.x(), thirdPoint.y(), thirdPoint.z());
            glm::dvec3 apex = glm::dvec3(apexPoint.x(), apexPoint.y(), apexPoint.z());
            
            // 计算半径
            double radius = glm::distance(center, p1);
            
            // 计算圆的平面法向量
            glm::dvec3 v1 = glm::normalize(p1 - center);
            glm::dvec3 v2 = glm::normalize(p2 - center);
            glm::dvec3 normal = glm::normalize(glm::cross(v1, v2));
            
            // 检查是否三点共线
            if (glm::length(glm::cross(v1, v2)) >= 1e-6f) {
                // 检测锥顶点是否在底面平面上（退化情况）
                double distanceToPlane = std::abs(glm::dot(apex - center, normal));
                bool isDegenerate = distanceToPlane < 1e-4f; // 很小的阈值
                
                if (isDegenerate) {
                    // 锥顶点在底面平面上，只绘制底面圆
                    
                    // 底面圆心
                    vertices->push_back(osg::Vec3(center.x, center.y, center.z)); // index 0
                    
                    // 计算圆平面上的两个正交向量
                    glm::dvec3 radiusVec = v1;  // 从圆心到半径点的向量
                    glm::dvec3 perpVec = glm::normalize(glm::cross(normal, radiusVec));
                    
                    // 圆周上的点
                    for (int i = 0; i < circleSegments; i++) {
                        double angle = 2.0 * M_PI * i / circleSegments;
                        
                        glm::dvec3 circlePoint = center + radius * (
                            cos(angle) * radiusVec + sin(angle) * perpVec
                        );
                        
                        vertices->push_back(osg::Vec3(circlePoint.x, circlePoint.y, circlePoint.z)); // index 1+i
                    }
                    
                    // 为了确保TRIANGLE_FAN正确封闭，添加第一个圆周点
                    glm::dvec3 firstCirclePoint = center + radius * radiusVec; // angle = 0
                    vertices->push_back(osg::Vec3(firstCirclePoint.x, firstCirclePoint.y, firstCirclePoint.z));
                    
                    // 只绘制底面圆形（三角形扇形）
                    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, circleSegments + 2));
                } else {
                    // 正常圆锥，锥顶点不在底面平面上
                    
                    // 底面圆心
                    vertices->push_back(osg::Vec3(center.x, center.y, center.z)); // index 0
                    
                    // 计算圆平面上的两个正交向量
                    glm::dvec3 radiusVec = v1;  // 从圆心到半径点的向量
                    glm::dvec3 perpVec = glm::normalize(glm::cross(normal, radiusVec));
                    
                    // 圆周上的点
                    for (int i = 0; i < circleSegments; i++) {
                        double angle = 2.0 * M_PI * i / circleSegments;
                        
                        glm::dvec3 circlePoint = center + radius * (
                            cos(angle) * radiusVec + sin(angle) * perpVec
                        );
                        
                        vertices->push_back(osg::Vec3(circlePoint.x, circlePoint.y, circlePoint.z)); // index 1+i
                    }
                    
                    // 为了确保TRIANGLE_FAN正确封闭，添加第一个圆周点
                    glm::dvec3 firstCirclePoint = center + radius * radiusVec; // angle = 0
                    vertices->push_back(osg::Vec3(firstCirclePoint.x, firstCirclePoint.y, firstCirclePoint.z));
                    
                    // 锥顶点
                    vertices->push_back(osg::Vec3(apexPoint.x(), apexPoint.y(), apexPoint.z())); // index circleSegments+2
                    
                    // 底面圆形（三角形扇形）
                    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, circleSegments + 2));
                    
                    // 侧面：为了正确绘制，需要重新排列顶点
                    // 我们使用多个三角形来绘制侧面，而不是一个扇形
                    for (int i = 0; i < circleSegments; i++) {
                        int next = (i + 1) % circleSegments;
                        
                        // 为每个侧面三角形创建单独的primitive
                        osg::ref_ptr<osg::DrawElementsUInt> triangleIndices = 
                            new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES);
                        
                        // 三角形：锥顶点 -> 当前圆周点 -> 下一个圆周点
                        triangleIndices->push_back(circleSegments + 2);  // 锥顶点（索引调整）
                        triangleIndices->push_back(1 + i);               // 当前圆周点
                        triangleIndices->push_back(1 + next);            // 下一个圆周点
                        
                        geometry->addPrimitiveSet(triangleIndices);
                    }
                }
            }
            // 如果三点共线，不绘制面
        }
    }
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
}




