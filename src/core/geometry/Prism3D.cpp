#include "Prism3D.h"
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

Prism3D_Geo::Prism3D_Geo()
{
    m_geoType = Geo_Prism3D;
    // 确保基类正确初始化
    initialize();
    
    // 立体几何特定的可见性设置：显示线面
    GeoParameters3D params = getParameters();
    params.showPoints = false;
    params.showEdges = true;
    params.showFaces = true;
 
    setParameters(params);
}

void Prism3D_Geo::buildVertexGeometries()
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
        // 第一阶段：确定多边形顶点
        const auto& stage1 = allStagePoints[0];
        
        // 显示所有已输入的多边形顶点
        for (size_t i = 0; i < stage1.size(); i++) 
        {
            Point3D point = stage1[i];
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        }
    }
    else if (allStagePoints.size() == 2)
    {
        // 第二阶段：确定高度后显示完整棱柱体顶点
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        assert(stage1.size() >= 3 && stage2.size() >= 1);
        
        Point3D heightPoint = stage2[0];
        glm::dvec3 hp = glm::dvec3(heightPoint.x(), heightPoint.y(), heightPoint.z());
        
        // 计算高度向量（从第一个底面点到高度点）
        Point3D firstPoint = stage1[0];
        glm::dvec3 fp = glm::dvec3(firstPoint.x(), firstPoint.y(), firstPoint.z());
        glm::dvec3 heightVector = hp - fp;
        
        // 显示底面多边形的质心和顶面多边形的质心
        glm::dvec3 bottomCenter(0.0, 0.0, 0.0);
        for (size_t i = 0; i < stage1.size(); i++) 
        {
            Point3D point = stage1[i];
            bottomCenter += glm::dvec3(point.x(), point.y(), point.z());
        }
        bottomCenter /= stage1.size();
        
        glm::dvec3 topCenter = bottomCenter + heightVector;
        
        // 添加底面质心
        vertices->push_back(osg::Vec3(bottomCenter.x, bottomCenter.y, bottomCenter.z));
        
        // 添加顶面质心
        vertices->push_back(osg::Vec3(topCenter.x, topCenter.y, topCenter.z));
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

void Prism3D_Geo::buildEdgeGeometries()
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
        // 第一阶段：显示多边形的构建过程
        const auto& stage1 = allStagePoints[0];
        
        // 添加所有多边形顶点
        for (size_t i = 0; i < stage1.size(); i++) 
        {
            Point3D point = stage1[i];
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        }
        
        // 连接相邻的点形成多边形边
        if (stage1.size() >= 2) 
        {
            for (size_t i = 0; i < stage1.size() - 1; i++) 
            {
                indices->push_back(i);
                indices->push_back(i + 1);
            }
            
            // 如果有3个或更多点，闭合多边形
            if (stage1.size() >= 3) 
            {
                indices->push_back(stage1.size() - 1);
                indices->push_back(0);
            }
        }
    }
    else if (allStagePoints.size() == 2)
    {
        // 第二阶段：显示完整棱柱体的边线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        assert(stage1.size() >= 3 && stage2.size() >= 1);
        
        Point3D heightPoint = stage2[0];
        glm::dvec3 hp = glm::dvec3(heightPoint.x(), heightPoint.y(), heightPoint.z());
        
        // 计算高度向量
        Point3D firstPoint = stage1[0];
        glm::dvec3 fp = glm::dvec3(firstPoint.x(), firstPoint.y(), firstPoint.z());
        glm::dvec3 heightVector = hp - fp;
        
        // 添加底面和顶面的所有顶点
        for (size_t i = 0; i < stage1.size(); i++) 
        {
            Point3D bottomPoint = stage1[i];
            glm::dvec3 bp = glm::dvec3(bottomPoint.x(), bottomPoint.y(), bottomPoint.z());
            glm::dvec3 topPoint = bp + heightVector;
            
            vertices->push_back(osg::Vec3(bp.x, bp.y, bp.z)); // 底面点 index i*2
            vertices->push_back(osg::Vec3(topPoint.x, topPoint.y, topPoint.z)); // 顶面点 index i*2+1
        }
        
        size_t numPolygonVertices = stage1.size();
        
        // 底面多边形边线
        for (size_t i = 0; i < numPolygonVertices; i++) 
        {
            size_t next = (i + 1) % numPolygonVertices;
            indices->push_back(i * 2);
            indices->push_back(next * 2);
        }
        
        // 顶面多边形边线
        for (size_t i = 0; i < numPolygonVertices; i++) 
        {
            size_t next = (i + 1) % numPolygonVertices;
            indices->push_back(i * 2 + 1);
            indices->push_back(next * 2 + 1);
        }
        
        // 垂直边线
        for (size_t i = 0; i < numPolygonVertices; i++) 
        {
            indices->push_back(i * 2);     // 底面点
            indices->push_back(i * 2 + 1); // 对应顶面点
        }
    }
    
    // 设置顶点数组和索引
    geometry->setVertexArray(vertices);
    if (indices->size() > 0) {
        geometry->addPrimitiveSet(indices);
    }
}

void Prism3D_Geo::buildFaceGeometries()
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
    
    if (allStagePoints.size() == 1) 
    {
        // 第一阶段：如果确定了多边形（至少3个点），显示底面多边形
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 3) 
        {
            // 计算多边形质心
            glm::dvec3 center(0.0, 0.0, 0.0);
            for (size_t i = 0; i < stage1.size(); i++) 
            {
                Point3D point = stage1[i];
                center += glm::dvec3(point.x(), point.y(), point.z());
            }
            center /= stage1.size();
            
            // 添加质心
            vertices->push_back(osg::Vec3(center.x, center.y, center.z));
            
            // 添加多边形顶点
            for (size_t i = 0; i < stage1.size(); i++) 
            {
                Point3D point = stage1[i];
                vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
            }
            
            // 为了确保TRIANGLE_FAN正确封闭，添加第一个顶点
            Point3D firstPoint = stage1[0];
            vertices->push_back(osg::Vec3(firstPoint.x(), firstPoint.y(), firstPoint.z()));
            
            // 添加底面多边形（三角形扇形）
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, stage1.size() + 2));
        }
    }
    else if (allStagePoints.size() == 2)
    {
        // 第二阶段：显示完整棱柱体（底面 + 顶面 + 侧面）
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        assert(stage1.size() >= 3 && stage2.size() >= 1);
        
        Point3D heightPoint = stage2[0];
        glm::dvec3 hp = glm::dvec3(heightPoint.x(), heightPoint.y(), heightPoint.z());
        
        // 计算高度向量
        Point3D firstPoint = stage1[0];
        glm::dvec3 fp = glm::dvec3(firstPoint.x(), firstPoint.y(), firstPoint.z());
        glm::dvec3 heightVector = hp - fp;
        
        size_t numPolygonVertices = stage1.size();
        
        // 计算底面多边形质心
        glm::dvec3 bottomCenter(0.0, 0.0, 0.0);
        for (size_t i = 0; i < numPolygonVertices; i++) 
        {
            Point3D point = stage1[i];
            bottomCenter += glm::dvec3(point.x(), point.y(), point.z());
        }
        bottomCenter /= numPolygonVertices;
        
        glm::dvec3 topCenter = bottomCenter + heightVector;
        
        // 添加底面质心
        vertices->push_back(osg::Vec3(bottomCenter.x, bottomCenter.y, bottomCenter.z)); // index 0
        
        // 添加底面多边形顶点
        for (size_t i = 0; i < numPolygonVertices; i++) 
        {
            Point3D point = stage1[i];
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z())); // index 1+i
        }
        
        // 为了确保TRIANGLE_FAN正确封闭，添加第一个底面顶点
        Point3D firstBottomPoint = stage1[0];
        vertices->push_back(osg::Vec3(firstBottomPoint.x(), firstBottomPoint.y(), firstBottomPoint.z())); // index numPolygonVertices+1
        
        // 添加顶面质心
        vertices->push_back(osg::Vec3(topCenter.x, topCenter.y, topCenter.z)); // index numPolygonVertices+2
        
        // 添加顶面多边形顶点
        for (size_t i = 0; i < numPolygonVertices; i++) 
        {
            Point3D bottomPoint = stage1[i];
            glm::dvec3 bp = glm::dvec3(bottomPoint.x(), bottomPoint.y(), bottomPoint.z());
            glm::dvec3 topPoint = bp + heightVector;
            
            vertices->push_back(osg::Vec3(topPoint.x, topPoint.y, topPoint.z)); // index numPolygonVertices+3+i
        }
        
        // 为了确保TRIANGLE_FAN正确封闭，添加第一个顶面顶点
        glm::dvec3 firstTopPoint = fp + heightVector;
        vertices->push_back(osg::Vec3(firstTopPoint.x, firstTopPoint.y, firstTopPoint.z)); // index 2*numPolygonVertices+3
        
        // 底面多边形（三角形扇形）
        geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, numPolygonVertices + 2));
        
        // 顶面多边形（三角形扇形）
        geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, numPolygonVertices + 2, numPolygonVertices + 2));
        
        // 侧面：使用四边形条带
        for (size_t i = 0; i < numPolygonVertices; i++) 
        {
            size_t next = (i + 1) % numPolygonVertices;
            
            // 为每个侧面四边形创建单独的primitive
            osg::ref_ptr<osg::DrawElementsUInt> quadIndices = 
                new osg::DrawElementsUInt(osg::PrimitiveSet::QUADS);
            
            // 四边形：底面当前点 -> 底面下一点 -> 顶面下一点 -> 顶面当前点
            quadIndices->push_back(1 + i);                                          // 底面当前点
            quadIndices->push_back(1 + next);                                       // 底面下一点
            quadIndices->push_back(numPolygonVertices + 3 + next);                  // 顶面下一点
            quadIndices->push_back(numPolygonVertices + 3 + i);                     // 顶面当前点
            
            geometry->addPrimitiveSet(quadIndices);
        }
    }
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
} 

