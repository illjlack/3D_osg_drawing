#include "Torus3D.h"
#include "managers/GeoControlPointManager.h"
#include <osg/Geometry>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "util/MathUtils.h"
#include "util/VertexShapeUtils.h"
#include <cassert>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Torus3D_Geo::Torus3D_Geo()
{
    m_geoType = Geo_Torus3D;
    // 确保基类正确初始化
    initialize();
    
    // 立体几何特定的可见性设置：显示线面
    GeoParameters3D params = getParameters();
    params.showPoints = false;
    params.showEdges = true;
    params.showFaces = true;

    setParameters(params);
}

void Torus3D_Geo::buildVertexGeometries()
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
        // 第一阶段：确定环面轴线
        const auto& stage1 = allStagePoints[0];
        
        // 显示轴线的两个端点
        for (size_t i = 0; i < stage1.size(); i++) 
        {
            Point3D point = stage1[i];
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        }
    }
    else if (allStagePoints.size() == 2)
    {
        // 第二阶段：确定主圆，显示主圆圆心
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        assert(stage1.size() >= 2 && stage2.size() >= 1);
        
        Point3D point1 = stage1[0];
        Point3D point2 = stage1[1];
        
        glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
        glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
        
        // 计算轴线中心作为圆环中心
        glm::dvec3 torusCenter = (p1 + p2) * 0.5;
        
        // 添加圆环中心
        vertices->push_back(osg::Vec3(torusCenter.x, torusCenter.y, torusCenter.z));
    }
    else if (allStagePoints.size() == 3)
    {
        // 第三阶段：确定内圆半径，显示圆环中心
        const auto& stage1 = allStagePoints[0];
        
        assert(stage1.size() >= 2);
        
        Point3D point1 = stage1[0];
        Point3D point2 = stage1[1];
        
        glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
        glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
        
        // 计算轴线中心作为圆环中心
        glm::dvec3 torusCenter = (p1 + p2) * 0.5;
        
        // 添加圆环中心
        vertices->push_back(osg::Vec3(torusCenter.x, torusCenter.y, torusCenter.z));
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

void Torus3D_Geo::buildEdgeGeometries()
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
    
    // 从参数获取细分数量
    int segments = static_cast<int>(m_parameters.subdivisionLevel);
    
    if (allStagePoints.size() == 1) 
    {
        // 第一阶段：绘制轴线
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 2) 
        {
            Point3D point1 = stage1[0];
            Point3D point2 = stage1[1];
            
            vertices->push_back(osg::Vec3(point1.x(), point1.y(), point1.z())); // index 0
            vertices->push_back(osg::Vec3(point2.x(), point2.y(), point2.z())); // index 1
            
            // 连接轴线的两个端点
            indices->push_back(0);
            indices->push_back(1);
        }
    }
    else if (allStagePoints.size() == 2)
    {
        // 第二阶段：绘制主圆
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        assert(stage1.size() >= 2 && stage2.size() >= 1);
        
        Point3D point1 = stage1[0];
        Point3D point2 = stage1[1];
        Point3D point3 = stage2[0];
        
        glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
        glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
        glm::dvec3 p3 = glm::dvec3(point3.x(), point3.y(), point3.z());
        
        // 计算圆环参数
        glm::dvec3 torusCenter = (p1 + p2) * 0.5;
        glm::dvec3 axisDir = glm::normalize(p2 - p1);
        double majorRadius = glm::length(p2 - p1) * 0.5;
        
        // 确定圆环平面
        glm::dvec3 toP3 = p3 - torusCenter;
        glm::dvec3 radialDir = glm::normalize(toP3 - glm::dot(toP3, axisDir) * axisDir);
        glm::dvec3 tangentDir = glm::normalize(glm::cross(axisDir, radialDir));
        
        // 生成主圆上的点
        for (int i = 0; i < segments; i++) {
            double angle = 2.0 * M_PI * i / segments;
            
            glm::dvec3 circlePoint = torusCenter + majorRadius * (
                cos(angle) * radialDir + sin(angle) * tangentDir
            );
            
            vertices->push_back(osg::Vec3(circlePoint.x, circlePoint.y, circlePoint.z));
        }
        
        // 主圆连线
        for (int i = 0; i < segments; i++) 
        {
            int next = (i + 1) % segments;
            indices->push_back(i);
            indices->push_back(next);
        }
    }
    else if (allStagePoints.size() == 3)
    {
        // 第三阶段：绘制圆环的边线（不绘制面）
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        assert(stage1.size() >= 2 && stage2.size() >= 1 && stage3.size() >= 1);
        
        Point3D point1 = stage1[0];
        Point3D point2 = stage1[1];
        Point3D point3 = stage2[0];
        Point3D point4 = stage3[0];
        
        glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
        glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
        glm::dvec3 p3 = glm::dvec3(point3.x(), point3.y(), point3.z());
        glm::dvec3 p4 = glm::dvec3(point4.x(), point4.y(), point4.z());
        
        // 计算圆环参数
        glm::dvec3 torusCenter = (p1 + p2) * 0.5;
        glm::dvec3 axisDir = glm::normalize(p2 - p1);
        double majorRadius = glm::length(p2 - p1) * 0.5;
        
        // 确定圆环平面
        glm::dvec3 toP3 = p3 - torusCenter;
        glm::dvec3 radialDir = glm::normalize(toP3 - glm::dot(toP3, axisDir) * axisDir);
        glm::dvec3 tangentDir = glm::normalize(glm::cross(axisDir, radialDir));
        
        // 计算次半径（内环半径）
        glm::dvec3 toP4 = p4 - torusCenter;
        glm::dvec3 p4InPlane = toP4 - glm::dot(toP4, axisDir) * axisDir;
        double distanceToAxis = glm::length(p4InPlane);
        double minorRadius = std::abs(distanceToAxis - majorRadius);
        
        // 生成圆环线框
        int majorSegs = segments;
        int minorSegs = segments / 2;
        
        for (int i = 0; i < majorSegs; i++) {
            double majorAngle = 2.0 * M_PI * i / majorSegs;
            
            // 主圆上当前点的位置
            glm::dvec3 majorCenter = torusCenter + majorRadius * (
                cos(majorAngle) * radialDir + sin(majorAngle) * tangentDir
            );
            
            // 该点处的管子方向
            glm::dvec3 tubeRadial = cos(majorAngle) * radialDir + sin(majorAngle) * tangentDir;
            glm::dvec3 tubeTangent = glm::normalize(glm::cross(axisDir, tubeRadial));
            
            for (int j = 0; j < minorSegs; j++) {
                double minorAngle = 2.0 * M_PI * j / minorSegs;
                
                glm::dvec3 torusPoint = majorCenter + minorRadius * (
                    cos(minorAngle) * tubeTangent + sin(minorAngle) * axisDir
                );
                
                vertices->push_back(osg::Vec3(torusPoint.x, torusPoint.y, torusPoint.z));
            }
        }
        
        // 连接圆环线框
        for (int i = 0; i < majorSegs; i++) {
            for (int j = 0; j < minorSegs; j++) {
                int curr = i * minorSegs + j;
                int nextJ = i * minorSegs + (j + 1) % minorSegs;
                int nextI = ((i + 1) % majorSegs) * minorSegs + j;
                
                // 次方向连线
                indices->push_back(curr);
                indices->push_back(nextJ);
                
                // 主方向连线（只绘制一半，避免太密）
                if (j % 2 == 0) {
                    indices->push_back(curr);
                    indices->push_back(nextI);
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

void Torus3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getFaceGeometry();
    if (!geometry.valid())
    {
        return;
    }
    
    const auto& allStagePoints = mm_controlPoint()->getAllStageControlPoints();
    
    // 只在第三阶段绘制面
    if (allStagePoints.size() != 3) return;
    
    const auto& stage1 = allStagePoints[0];
    const auto& stage2 = allStagePoints[1];
    const auto& stage3 = allStagePoints[2];
    
    assert(stage1.size() >= 2 && stage2.size() >= 1 && stage3.size() >= 1);
    
    Point3D point1 = stage1[0];
    Point3D point2 = stage1[1];
    Point3D point3 = stage2[0];
    Point3D point4 = stage3[0];
    
    glm::dvec3 p1 = glm::dvec3(point1.x(), point1.y(), point1.z());
    glm::dvec3 p2 = glm::dvec3(point2.x(), point2.y(), point2.z());
    glm::dvec3 p3 = glm::dvec3(point3.x(), point3.y(), point3.z());
    glm::dvec3 p4 = glm::dvec3(point4.x(), point4.y(), point4.z());
    
    // 计算圆环参数
    glm::dvec3 torusCenter = (p1 + p2) * 0.5;
    glm::dvec3 axisDir = glm::normalize(p2 - p1);
    double majorRadius = glm::length(p2 - p1) * 0.5;
    
    // 确定圆环平面
    glm::dvec3 toP3 = p3 - torusCenter;
    glm::dvec3 radialDir = glm::normalize(toP3 - glm::dot(toP3, axisDir) * axisDir);
    glm::dvec3 tangentDir = glm::normalize(glm::cross(axisDir, radialDir));
    
    // 计算次半径（内环半径）
    glm::dvec3 toP4 = p4 - torusCenter;
    glm::dvec3 p4InPlane = toP4 - glm::dot(toP4, axisDir) * axisDir;
    double distanceToAxis = glm::length(p4InPlane);
    double minorRadius = std::abs(distanceToAxis - majorRadius);
    
    // 从参数获取细分数量
    int majorSegs = static_cast<int>(m_parameters.subdivisionLevel);
    int minorSegs = majorSegs / 2;
    
    // 创建顶点数组和法向量数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    // 生成圆环表面顶点和法向量
    for (int i = 0; i < majorSegs; i++) {
        double majorAngle = 2.0 * M_PI * i / majorSegs;
        
        // 主圆上当前点的位置
        glm::dvec3 majorCenter = torusCenter + majorRadius * (
            cos(majorAngle) * radialDir + sin(majorAngle) * tangentDir
        );
        
        // 该点处的管子方向
        glm::dvec3 tubeRadial = cos(majorAngle) * radialDir + sin(majorAngle) * tangentDir;
        glm::dvec3 tubeTangent = glm::normalize(glm::cross(axisDir, tubeRadial));
        
        for (int j = 0; j < minorSegs; j++) {
            double minorAngle = 2.0 * M_PI * j / minorSegs;
            
            glm::dvec3 localNormal = cos(minorAngle) * tubeTangent + sin(minorAngle) * axisDir;
            glm::dvec3 torusPoint = majorCenter + minorRadius * localNormal;
            
            vertices->push_back(osg::Vec3(torusPoint.x, torusPoint.y, torusPoint.z));
            normals->push_back(osg::Vec3(localNormal.x, localNormal.y, localNormal.z));
        }
    }
    
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 生成三角形面片（将四边形分解为三角形）
    osg::ref_ptr<osg::DrawElementsUInt> triangleIndices = 
        new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES);
        
    for (int i = 0; i < majorSegs; i++) {
        for (int j = 0; j < minorSegs; j++) {
            int curr = i * minorSegs + j;
            int nextJ = i * minorSegs + (j + 1) % minorSegs;
            int nextI = ((i + 1) % majorSegs) * minorSegs + j;
            int nextBoth = ((i + 1) % majorSegs) * minorSegs + (j + 1) % minorSegs;
            
            // 第一个三角形：curr -> nextJ -> nextBoth
            triangleIndices->push_back(curr);
            triangleIndices->push_back(nextJ);
            triangleIndices->push_back(nextBoth);
            
            // 第二个三角形：curr -> nextBoth -> nextI
            triangleIndices->push_back(curr);
            triangleIndices->push_back(nextBoth);
            triangleIndices->push_back(nextI);
        }
    }
    
    geometry->addPrimitiveSet(triangleIndices);
}


