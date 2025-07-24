#include "Cube3D.h"
#include "../managers/GeoControlPointManager.h"
#include <osg/Geometry>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"
#include <glm/glm.hpp>
#include <cassert>

Cube3D_Geo::Cube3D_Geo()
{
    m_geoType = Geo_Cube3D;
    // 确保基类正确初始化
    initialize();
}

void Cube3D_Geo::buildVertexGeometries()
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
        // 第一阶段：确定一条边的轴
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 1) 
        {
            // 添加第一个点
            Point3D point1 = stage1[0];
            vertices->push_back(osg::Vec3(point1.x(), point1.y(), point1.z()));
        }
        
        if (stage1.size() >= 2) 
        {
            // 添加第二个点，这两个点定义了立方体的一条边轴
            Point3D point2 = stage1[1];
            vertices->push_back(osg::Vec3(point2.x(), point2.y(), point2.z()));
        }
    }
    else if (allStagePoints.size() == 2)
    {
        // 第二阶段：确定方向后显示完整立方体的顶点
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        assert(stage1.size() >= 2 && stage2.size() >= 1);
        
        Point3D point1 = stage1[0];  // 边轴起点
        Point3D point2 = stage1[1];  // 边轴终点
        Point3D point3 = stage2[0];  // 方向点
        
        // 计算立方体的8个顶点
        glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
        glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
        glm::dvec3 p3 = glm::dvec3(point3.x(), point3.y(), point3.z());
        
        // 计算边轴向量（立方体的一条边）
        glm::dvec3 edge = p2 - p1;
        double edgeLength = glm::length(edge);
        
        if (edgeLength > 1e-6) 
        {
            glm::dvec3 edgeDir = glm::normalize(edge);
            
            // 计算第二个方向（从p2指向p3的方向，但要垂直化）
            glm::dvec3 toP3 = p3 - p2;
            glm::dvec3 secondDir = toP3 - glm::dot(toP3, edgeDir) * edgeDir;  // 垂直化
            
            if (glm::length(secondDir) > 1e-6f) 
            {
                secondDir = glm::normalize(secondDir) * edgeLength;  // 等长
                
                // 计算第三个方向（垂直于前两个方向）
                glm::dvec3 thirdDir = glm::normalize(glm::cross(edgeDir, secondDir)) * edgeLength;
                
                // 计算立方体的8个顶点
                // 底面4个顶点
                glm::dvec3 v0 = p1;                           // 000
                glm::dvec3 v1 = p1 + edge;                    // 100
                glm::dvec3 v2 = p1 + secondDir;               // 010
                glm::dvec3 v3 = p1 + edge + secondDir;       // 110
                
                // 顶面4个顶点
                glm::dvec3 v4 = p1 + thirdDir;                       // 001
                glm::dvec3 v5 = p1 + edge + thirdDir;                // 101
                glm::dvec3 v6 = p1 + secondDir + thirdDir;           // 011
                glm::dvec3 v7 = p1 + edge + secondDir + thirdDir;    // 111
                
                // 添加所有8个顶点
                vertices->push_back(osg::Vec3(v0.x, v0.y, v0.z));
                vertices->push_back(osg::Vec3(v1.x, v1.y, v1.z));
                vertices->push_back(osg::Vec3(v2.x, v2.y, v2.z));
                vertices->push_back(osg::Vec3(v3.x, v3.y, v3.z));
                vertices->push_back(osg::Vec3(v4.x, v4.y, v4.z));
                vertices->push_back(osg::Vec3(v5.x, v5.y, v5.z));
                vertices->push_back(osg::Vec3(v6.x, v6.y, v6.z));
                vertices->push_back(osg::Vec3(v7.x, v7.y, v7.z));
            }
        }
    }
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
    
    // 添加点绘制原语
    if (vertices->size() > 0) 
    {
        geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size()));
    }
}

void Cube3D_Geo::buildEdgeGeometries()
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
    
    if (allStagePoints.size() == 1) 
    {
        // 第一阶段：显示边轴线
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 2) 
        {
            Point3D point1 = stage1[0];
            Point3D point2 = stage1[1];
            
            vertices->push_back(osg::Vec3(point1.x(), point1.y(), point1.z())); // index 0
            vertices->push_back(osg::Vec3(point2.x(), point2.y(), point2.z())); // index 1
            
            // 边轴线
            indices->push_back(0);
            indices->push_back(1);
        }
    }
    else if (allStagePoints.size() == 2)
    {
        // 第二阶段：显示完整立方体的边线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        assert(stage1.size() >= 2 && stage2.size() >= 1);
        
        Point3D point1 = stage1[0];
        Point3D point2 = stage1[1];
        Point3D point3 = stage2[0];
        
        glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
        glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
        glm::dvec3 p3 = glm::dvec3(point3.x(), point3.y(), point3.z());
        
        glm::dvec3 edge = p2 - p1;
        double edgeLength = glm::length(edge);
        
        if (edgeLength > 1e-6) 
        {
            glm::dvec3 edgeDir = glm::normalize(edge);
            glm::dvec3 toP3 = p3 - p2;
            glm::dvec3 secondDir = toP3 - glm::dot(toP3, edgeDir) * edgeDir;
            
            if (glm::length(secondDir) > 1e-6f) 
            {
                secondDir = glm::normalize(secondDir) * edgeLength;
                glm::dvec3 thirdDir = glm::normalize(glm::cross(edgeDir, secondDir)) * edgeLength;
                
                // 计算8个顶点
                glm::dvec3 v0 = p1;
                glm::dvec3 v1 = p1 + edge;
                glm::dvec3 v2 = p1 + secondDir;
                glm::dvec3 v3 = p1 + edge + secondDir;
                glm::dvec3 v4 = p1 + thirdDir;
                glm::dvec3 v5 = p1 + edge + thirdDir;
                glm::dvec3 v6 = p1 + secondDir + thirdDir;
                glm::dvec3 v7 = p1 + edge + secondDir + thirdDir;
                
                // 添加顶点
                vertices->push_back(osg::Vec3(v0.x, v0.y, v0.z)); // 0
                vertices->push_back(osg::Vec3(v1.x, v1.y, v1.z)); // 1
                vertices->push_back(osg::Vec3(v2.x, v2.y, v2.z)); // 2
                vertices->push_back(osg::Vec3(v3.x, v3.y, v3.z)); // 3
                vertices->push_back(osg::Vec3(v4.x, v4.y, v4.z)); // 4
                vertices->push_back(osg::Vec3(v5.x, v5.y, v5.z)); // 5
                vertices->push_back(osg::Vec3(v6.x, v6.y, v6.z)); // 6
                vertices->push_back(osg::Vec3(v7.x, v7.y, v7.z)); // 7
                
                // 立方体的12条边
                // 底面4条边
                indices->push_back(0); indices->push_back(1); // v0-v1
                indices->push_back(1); indices->push_back(3); // v1-v3
                indices->push_back(3); indices->push_back(2); // v3-v2
                indices->push_back(2); indices->push_back(0); // v2-v0
                
                // 顶面4条边
                indices->push_back(4); indices->push_back(5); // v4-v5
                indices->push_back(5); indices->push_back(7); // v5-v7
                indices->push_back(7); indices->push_back(6); // v7-v6
                indices->push_back(6); indices->push_back(4); // v6-v4
                
                // 4条垂直边
                indices->push_back(0); indices->push_back(4); // v0-v4
                indices->push_back(1); indices->push_back(5); // v1-v5
                indices->push_back(2); indices->push_back(6); // v2-v6
                indices->push_back(3); indices->push_back(7); // v3-v7
            }
        }
    }
    
    // 设置顶点数组和索引
    geometry->setVertexArray(vertices);
    if (indices->size() > 0) {
        geometry->addPrimitiveSet(indices);
    }
}

void Cube3D_Geo::buildFaceGeometries()
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
    
    if (allStagePoints.size() == 2)
    {
        // 第二阶段：显示完整立方体的面
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        assert(stage1.size() >= 2 && stage2.size() >= 1);
        
        Point3D point1 = stage1[0];
        Point3D point2 = stage1[1];
        Point3D point3 = stage2[0];
        
        glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
        glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
        glm::dvec3 p3 = glm::dvec3(point3.x(), point3.y(), point3.z());
        
        glm::dvec3 edge = p2 - p1;
        double edgeLength = glm::length(edge);
        
        if (edgeLength > 1e-6) 
        {
            glm::dvec3 edgeDir = glm::normalize(edge);
            glm::dvec3 toP3 = p3 - p2;
            glm::dvec3 secondDir = toP3 - glm::dot(toP3, edgeDir) * edgeDir;
            
            if (glm::length(secondDir) > 1e-6f) 
            {
                secondDir = glm::normalize(secondDir) * edgeLength;
                glm::dvec3 thirdDir = glm::normalize(glm::cross(edgeDir, secondDir)) * edgeLength;
                
                // 计算8个顶点
                glm::dvec3 v0 = p1;
                glm::dvec3 v1 = p1 + edge;
                glm::dvec3 v2 = p1 + secondDir;
                glm::dvec3 v3 = p1 + edge + secondDir;
                glm::dvec3 v4 = p1 + thirdDir;
                glm::dvec3 v5 = p1 + edge + thirdDir;
                glm::dvec3 v6 = p1 + secondDir + thirdDir;
                glm::dvec3 v7 = p1 + edge + secondDir + thirdDir;
                
                // 添加顶点
                vertices->push_back(osg::Vec3(v0.x, v0.y, v0.z)); // 0
                vertices->push_back(osg::Vec3(v1.x, v1.y, v1.z)); // 1
                vertices->push_back(osg::Vec3(v2.x, v2.y, v2.z)); // 2
                vertices->push_back(osg::Vec3(v3.x, v3.y, v3.z)); // 3
                vertices->push_back(osg::Vec3(v4.x, v4.y, v4.z)); // 4
                vertices->push_back(osg::Vec3(v5.x, v5.y, v5.z)); // 5
                vertices->push_back(osg::Vec3(v6.x, v6.y, v6.z)); // 6
                vertices->push_back(osg::Vec3(v7.x, v7.y, v7.z)); // 7
                
                // 立方体的6个面，每个面用4个顶点的四边形
                
                // 底面：v0,v1,v3,v2
                osg::ref_ptr<osg::DrawElementsUInt> bottomFace = 
                    new osg::DrawElementsUInt(osg::PrimitiveSet::QUADS);
                bottomFace->push_back(0); bottomFace->push_back(1); 
                bottomFace->push_back(3); bottomFace->push_back(2);
                geometry->addPrimitiveSet(bottomFace);
                
                // 顶面：v4,v6,v7,v5
                osg::ref_ptr<osg::DrawElementsUInt> topFace = 
                    new osg::DrawElementsUInt(osg::PrimitiveSet::QUADS);
                topFace->push_back(4); topFace->push_back(6); 
                topFace->push_back(7); topFace->push_back(5);
                geometry->addPrimitiveSet(topFace);
                
                // 前面：v0,v4,v5,v1
                osg::ref_ptr<osg::DrawElementsUInt> frontFace = 
                    new osg::DrawElementsUInt(osg::PrimitiveSet::QUADS);
                frontFace->push_back(0); frontFace->push_back(4); 
                frontFace->push_back(5); frontFace->push_back(1);
                geometry->addPrimitiveSet(frontFace);
                
                // 后面：v2,v3,v7,v6
                osg::ref_ptr<osg::DrawElementsUInt> backFace = 
                    new osg::DrawElementsUInt(osg::PrimitiveSet::QUADS);
                backFace->push_back(2); backFace->push_back(3); 
                backFace->push_back(7); backFace->push_back(6);
                geometry->addPrimitiveSet(backFace);
                
                // 左面：v0,v2,v6,v4
                osg::ref_ptr<osg::DrawElementsUInt> leftFace = 
                    new osg::DrawElementsUInt(osg::PrimitiveSet::QUADS);
                leftFace->push_back(0); leftFace->push_back(2); 
                leftFace->push_back(6); leftFace->push_back(4);
                geometry->addPrimitiveSet(leftFace);
                
                // 右面：v1,v5,v7,v3
                osg::ref_ptr<osg::DrawElementsUInt> rightFace = 
                    new osg::DrawElementsUInt(osg::PrimitiveSet::QUADS);
                rightFace->push_back(1); rightFace->push_back(5); 
                rightFace->push_back(7); rightFace->push_back(3);
                geometry->addPrimitiveSet(rightFace);
            }
        }
    }
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
}



