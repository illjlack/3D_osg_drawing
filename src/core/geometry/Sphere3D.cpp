#include "Sphere3D.h"
#include "../managers/GeoControlPointManager.h"
#include <osg/Geometry>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"
#include "../../util/OSGUtils.h"
#include <cassert>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 四点确定球心和半径的辅助函数
bool calculateSphereCenterAndRadius(const glm::dvec3& p1, const glm::dvec3& p2, 
                                   const glm::dvec3& p3, const glm::dvec3& p4,
                                   glm::dvec3& center, double& radius)
{
    // 四点确定球心和半径的算法
    // 设球心为 (x, y, z)，半径为 r
    // 四个点都满足 (xi - x)² + (yi - y)² + (zi - z)² = r²
    
    // 通过消除r²，得到三个线性方程
    glm::dmat3 A;
    glm::dvec3 b;
    
    // 方程1: (p1-p2) · (x,y,z) = 0.5 * (|p1|² - |p2|²)
    glm::dvec3 d12 = p1 - p2;
    A[0] = glm::dvec3(2*d12.x, 2*d12.y, 2*d12.z);
    b[0] = glm::dot(p1, p1) - glm::dot(p2, p2);
    
    // 方程2: (p1-p3) · (x,y,z) = 0.5 * (|p1|² - |p3|²)
    glm::dvec3 d13 = p1 - p3;
    A[1] = glm::dvec3(2*d13.x, 2*d13.y, 2*d13.z);
    b[1] = glm::dot(p1, p1) - glm::dot(p3, p3);
    
    // 方程3: (p1-p4) · (x,y,z) = 0.5 * (|p1|² - |p4|²)
    glm::dvec3 d14 = p1 - p4;
    A[2] = glm::dvec3(2*d14.x, 2*d14.y, 2*d14.z);
    b[2] = glm::dot(p1, p1) - glm::dot(p4, p4);
    
    // 解线性方程组 A * center = b
    double det = glm::determinant(A);
    if (std::abs(det) < 1e-10) {
        return false; // 四点共面，无法确定唯一球
    }
    
    glm::dmat3 invA = glm::inverse(A);
    center = invA * b;
    
    // 计算半径
    radius = glm::distance(center, p1);
    
    // 验证其他三个点是否在同一球面上
    double tol = 1e-6;
    if (std::abs(glm::distance(center, p2) - radius) > tol ||
        std::abs(glm::distance(center, p3) - radius) > tol ||
        std::abs(glm::distance(center, p4) - radius) > tol) {
        return false;
    }
    
    return true;
}

Sphere3D_Geo::Sphere3D_Geo()
{
    m_geoType = Geo_Sphere3D;
    // 确保基类正确初始化
    initialize();
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
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
    
    // 添加点绘制原语
    if (vertices->size() > 0) 
    {
        geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size()));
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
        
        if (calculateSphereCenterAndRadius(p1, p2, p3, p4, center, radius)) 
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
