#include "Cylinder3D.h"
#include "../managers/GeoControlPointManager.h"
#include <osg/Geometry>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"
#include "../../util/VertexShapeUtils.h"
#include <cassert>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Cylinder3D_Geo::Cylinder3D_Geo()
{
    m_geoType = Geo_Cylinder3D;
    // 确保基类正确初始化
    initialize();
    
    // 立体几何特定的可见性设置：显示线面
    GeoParameters3D params = getParameters();
    params.showPoints = false;
    params.showEdges = true;
    params.showFaces = true;
    
    // 更新渲染参数
    if (mm_render()) {
        mm_render()->updateRenderingParameters(params);
    }
    // 同步参数到基类
    setParameters(params);
}

void Cylinder3D_Geo::buildVertexGeometries()
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
        // 第一阶段：确定底面圆
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
        // 第二阶段：确定高度后显示完整圆柱体顶点
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        assert(stage1.size() >= 3 && stage2.size() >= 1);
        
        Point3D point1 = stage1[0];
        Point3D point2 = stage1[1];
        Point3D point3 = stage1[2];
        Point3D heightPoint = stage2[0];
        
        glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
        glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
        glm::dvec3 p3 = glm::dvec3(point3.x(), point3.y(), point3.z());
        glm::dvec3 hp = glm::dvec3(heightPoint.x(), heightPoint.y(), heightPoint.z());
        
        glm::dvec3 center;
        double radius;
        
        if (MathUtils::calculateCircleCenterAndRadius(p1, p2, p3, center, radius)) 
        {
            // 添加底面圆心
            vertices->push_back(osg::Vec3(center.x, center.y, center.z));
            
            // 计算高度向量
            glm::dvec3 heightVector = hp - p1;  // 从p1到高度点的向量
            
            // 添加顶面圆心
            glm::dvec3 topCenter = center + heightVector;
            vertices->push_back(osg::Vec3(topCenter.x, topCenter.y, topCenter.z));
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

void Cylinder3D_Geo::buildEdgeGeometries()
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
        // 第一阶段：显示底面圆的构建过程
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
    }
    else if (allStagePoints.size() == 2)
    {
        // 第二阶段：显示完整圆柱体的边线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        assert(stage1.size() >= 3 && stage2.size() >= 1);
        
        Point3D point1 = stage1[0];
        Point3D point2 = stage1[1];
        Point3D point3 = stage1[2];
        Point3D heightPoint = stage2[0];
        
        glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
        glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
        glm::dvec3 p3 = glm::dvec3(point3.x(), point3.y(), point3.z());
        glm::dvec3 hp = glm::dvec3(heightPoint.x(), heightPoint.y(), heightPoint.z());
        
        glm::dvec3 center;
        double radius;
        
        if (MathUtils::calculateCircleCenterAndRadius(p1, p2, p3, center, radius)) 
        {
            // 计算高度向量
            glm::dvec3 heightVector = hp - p1;
            
            // 计算圆的法向量
            glm::dvec3 v1 = glm::normalize(p1 - center);
            glm::dvec3 v2 = glm::normalize(p2 - center);
            glm::dvec3 normal = glm::normalize(glm::cross(v1, v2));
            
            // 误差累积得好大
            assert(std::abs(glm::length(glm::cross(glm::normalize(heightVector), normal)) - 1) > 0.1 && "再怎么样为啥高会与平面法向量垂直啊");
            assert(glm::length(glm::cross(glm::normalize(heightVector), normal)) < 0.1 && "约束计算错误");

            // 计算圆平面上的两个正交向量
            glm::dvec3 radiusVec = v1;
            glm::dvec3 perpVec = glm::normalize(glm::cross(normal, radiusVec));
            
            // 生成底面圆周上的点
            for (int i = 0; i < circleSegments; i++) {
                double angle = 2.0 * M_PI * i / circleSegments;
                
                glm::dvec3 bottomPoint = center + radius * (
                    cos(angle) * radiusVec + sin(angle) * perpVec
                );
                
                glm::dvec3 topPoint = bottomPoint + heightVector;
                
                vertices->push_back(osg::Vec3(bottomPoint.x, bottomPoint.y, bottomPoint.z)); // index i*2
                vertices->push_back(osg::Vec3(topPoint.x, topPoint.y, topPoint.z)); // index i*2+1
            }
            
            // 底面圆周连线
            for (int i = 0; i < circleSegments; i++) 
            {
                int next = (i + 1) % circleSegments;
                indices->push_back(i * 2);
                indices->push_back(next * 2);
            }
            
            // 顶面圆周连线
            for (int i = 0; i < circleSegments; i++) 
            {
                int next = (i + 1) % circleSegments;
                indices->push_back(i * 2 + 1);
                indices->push_back(next * 2 + 1);
            }
            
            // // 垂直母线（只绘制一半，避免太密）
            // for (int i = 0; i < circleSegments; i += 2) 
            // {
            //     indices->push_back(i * 2);     // 底面点
            //     indices->push_back(i * 2 + 1); // 对应顶面点
            // }
        }
    }
    
    // 设置顶点数组和索引
    geometry->setVertexArray(vertices);
    if (indices->size() > 0) {
        geometry->addPrimitiveSet(indices);
    }
}

void Cylinder3D_Geo::buildFaceGeometries()
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
    
    if (allStagePoints.size() == 1) 
    {
        // 第一阶段：如果确定了圆，显示底面圆形
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
                // 添加圆心
                vertices->push_back(osg::Vec3(center.x, center.y, center.z)); // 圆心
                
                // 计算圆的法向量
                glm::dvec3 v1 = glm::normalize(p1 - center);
                glm::dvec3 v2 = glm::normalize(p2 - center);
                glm::dvec3 normal = glm::normalize(glm::cross(v1, v2));
                
                // 计算圆平面上的两个正交向量
                glm::dvec3 radiusVec = v1;
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
        }
    }
    else if (allStagePoints.size() == 2)
    {
        // 第二阶段：显示完整圆柱体（底面 + 顶面 + 侧面）
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        assert(stage1.size() >= 3 && stage2.size() >= 1);
        
        Point3D point1 = stage1[0];
        Point3D point2 = stage1[1];
        Point3D point3 = stage1[2];
        Point3D heightPoint = stage2[0];
        
        glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
        glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
        glm::dvec3 p3 = glm::dvec3(point3.x(), point3.y(), point3.z());
        glm::dvec3 hp = glm::dvec3(heightPoint.x(), heightPoint.y(), heightPoint.z());
        
        glm::dvec3 center;
        double radius;
        
        if (MathUtils::calculateCircleCenterAndRadius(p1, p2, p3, center, radius)) 
        {
            // 计算高度向量
            glm::dvec3 heightVector = hp - p1;
            glm::dvec3 topCenter = center + heightVector;
            
            // 计算圆的法向量
            glm::dvec3 v1 = glm::normalize(p1 - center);
            glm::dvec3 v2 = glm::normalize(p2 - center);
            glm::dvec3 normal = glm::normalize(glm::cross(v1, v2));
            
            assert(glm::length(glm::cross(glm::normalize(heightVector), normal)) < 0.1 && "不垂直，约束计算错误(误差好像很大)");

            // 计算圆平面上的两个正交向量
            glm::dvec3 radiusVec = v1;
            glm::dvec3 perpVec = glm::normalize(glm::cross(normal, radiusVec));
            
            // 添加底面圆心
            vertices->push_back(osg::Vec3(center.x, center.y, center.z)); // index 0
            
            // 添加底面圆周上的点
            for (int i = 0; i < circleSegments; i++) {
                double angle = 2.0 * M_PI * i / circleSegments;
                
                glm::dvec3 circlePoint = center + radius * (
                    cos(angle) * radiusVec + sin(angle) * perpVec
                );
                
                vertices->push_back(osg::Vec3(circlePoint.x, circlePoint.y, circlePoint.z)); // index 1+i
            }
            
            // 为了确保TRIANGLE_FAN正确封闭，添加第一个圆周点
            glm::dvec3 firstCirclePoint = center + radius * radiusVec;
            vertices->push_back(osg::Vec3(firstCirclePoint.x, firstCirclePoint.y, firstCirclePoint.z)); // index circleSegments+1
            
            // 添加顶面圆心
            vertices->push_back(osg::Vec3(topCenter.x, topCenter.y, topCenter.z)); // index circleSegments+2
            
            // 添加顶面圆周上的点
            for (int i = 0; i < circleSegments; i++) {
                double angle = 2.0 * M_PI * i / circleSegments;
                
                glm::dvec3 circlePoint = topCenter + radius * (
                    cos(angle) * radiusVec + sin(angle) * perpVec
                );
                
                vertices->push_back(osg::Vec3(circlePoint.x, circlePoint.y, circlePoint.z)); // index circleSegments+3+i
            }
            
            // 为了确保TRIANGLE_FAN正确封闭，添加第一个顶面圆周点
            glm::dvec3 firstTopCirclePoint = topCenter + radius * radiusVec;
            vertices->push_back(osg::Vec3(firstTopCirclePoint.x, firstTopCirclePoint.y, firstTopCirclePoint.z)); // index 2*circleSegments+3
            
            // 底面圆形（三角形扇形）
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, circleSegments + 2));
            
            // 顶面圆形（三角形扇形）
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, circleSegments + 2, circleSegments + 2));
            
            // 侧面：使用四边形条带
            for (int i = 0; i < circleSegments; i++) {
                int next = (i + 1) % circleSegments;
                
                // 为每个侧面四边形创建单独的primitive
                osg::ref_ptr<osg::DrawElementsUInt> quadIndices = 
                    new osg::DrawElementsUInt(osg::PrimitiveSet::QUADS);
                
                // 四边形：底面当前点 -> 底面下一点 -> 顶面下一点 -> 顶面当前点
                quadIndices->push_back(1 + i);                           // 底面当前点
                quadIndices->push_back(1 + next);                        // 底面下一点
                quadIndices->push_back(circleSegments + 3 + next);       // 顶面下一点
                quadIndices->push_back(circleSegments + 3 + i);          // 顶面当前点
                
                geometry->addPrimitiveSet(quadIndices);
            }
        }
    }
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
} 


