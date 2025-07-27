#include "Ellipsoid3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"
#include "../../util/VertexShapeUtils.h"
#include <cmath>

Ellipsoid3D_Geo::Ellipsoid3D_Geo()
{
    m_geoType = Geo_Ellipsoid3D;
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

void Ellipsoid3D_Geo::buildVertexGeometries()
{
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
    
    if (allStagePoints.size() == 1) {
        // 第一阶段：椭球中心
        const auto& stage1 = allStagePoints[0];
        
        for (size_t i = 0; i < stage1.size(); ++i) {
            vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
        }
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：中心 + 第一轴端点
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 1 && stage2.size() >= 1) {
            Point3D center = stage1[0];
            Point3D axis1End = stage2[0];
            
            // 添加中心点和第一轴端点
            vertices->push_back(osg::Vec3(center.x(), center.y(), center.z()));
            vertices->push_back(osg::Vec3(axis1End.x(), axis1End.y(), axis1End.z()));
        }
    }
    else if (allStagePoints.size() == 3) {
        // 第三阶段：中心 + 两个轴端点
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D center = stage1[0];
            Point3D axis1End = stage2[0];
            Point3D axis2End = stage3[0];
            
            vertices->push_back(osg::Vec3(center.x(), center.y(), center.z()));
            vertices->push_back(osg::Vec3(axis1End.x(), axis1End.y(), axis1End.z()));
            vertices->push_back(osg::Vec3(axis2End.x(), axis2End.y(), axis2End.z()));
        }
    }
    else if (allStagePoints.size() >= 4) {
        // 第四阶段：完整椭球体
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1) {
            Point3D center = stage1[0];
            Point3D axis1End = stage2[0];
            Point3D axis2End = stage3[0];
            Point3D axis3End = stage4[0];
            
            // 计算三个轴的长度
            double a = glm::length(glm::dvec3(axis1End.x() - center.x(), axis1End.y() - center.y(), axis1End.z() - center.z()));
            double b = glm::length(glm::dvec3(axis2End.x() - center.x(), axis2End.y() - center.y(), axis2End.z() - center.z()));
            double c = glm::length(glm::dvec3(axis3End.x() - center.x(), axis3End.y() - center.y(), axis3End.z() - center.z()));
            
            // 生成椭球表面的采样点
            int latitudes = 8;   // 纬度分割数
            int longitudes = 16; // 经度分割数
            
            for (int lat = 0; lat <= latitudes; ++lat) {
                double phi = M_PI * lat / latitudes - M_PI / 2; // -π/2 到 π/2
                
                for (int lng = 0; lng < longitudes; ++lng) {
                    double theta = 2.0 * M_PI * lng / longitudes; // 0 到 2π
                    
                    // 椭球参数方程
                    double x = center.x() + a * std::cos(phi) * std::cos(theta);
                    double y = center.y() + b * std::cos(phi) * std::sin(theta);
                    double z = center.z() + c * std::sin(phi);
                    
                    vertices->push_back(osg::Vec3(x, y, z));
                }
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

void Ellipsoid3D_Geo::buildEdgeGeometries()
{
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
    
    if (allStagePoints.size() == 1) {
        // 第一阶段：只有中心点，无边线
        // 不绘制任何边线
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：中心到第一轴端点的线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 1 && stage2.size() >= 1) {
            Point3D center = stage1[0];
            Point3D axis1End = stage2[0];
            
            vertices->push_back(osg::Vec3(center.x(), center.y(), center.z()));
            vertices->push_back(osg::Vec3(axis1End.x(), axis1End.y(), axis1End.z()));
            
            indices->push_back(0);
            indices->push_back(1);
        }
    }
    else if (allStagePoints.size() == 3) {
        // 第三阶段：显示两个轴
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D center = stage1[0];
            Point3D axis1End = stage2[0];
            Point3D axis2End = stage3[0];
            
            vertices->push_back(osg::Vec3(center.x(), center.y(), center.z()));
            vertices->push_back(osg::Vec3(axis1End.x(), axis1End.y(), axis1End.z()));
            vertices->push_back(osg::Vec3(axis2End.x(), axis2End.y(), axis2End.z()));
            
            // 第一轴
            indices->push_back(0); indices->push_back(1);
            // 第二轴
            indices->push_back(0); indices->push_back(2);
        }
    }
    else if (allStagePoints.size() >= 4) {
        // 第四阶段：完整椭球的线框
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1) {
            Point3D center = stage1[0];
            Point3D axis1End = stage2[0];
            Point3D axis2End = stage3[0];
            Point3D axis3End = stage4[0];
            
            // 计算三个轴的长度
            double a = glm::length(glm::dvec3(axis1End.x() - center.x(), axis1End.y() - center.y(), axis1End.z() - center.z()));
            double b = glm::length(glm::dvec3(axis2End.x() - center.x(), axis2End.y() - center.y(), axis2End.z() - center.z()));
            double c = glm::length(glm::dvec3(axis3End.x() - center.x(), axis3End.y() - center.y(), axis3End.z() - center.z()));
            
            // 绘制椭球的经线和纬线
            int latitudes = 8;
            int longitudes = 16;
            
            // 生成所有顶点
            for (int lat = 0; lat <= latitudes; ++lat) {
                double phi = M_PI * lat / latitudes - M_PI / 2;
                
                for (int lng = 0; lng < longitudes; ++lng) {
                    double theta = 2.0 * M_PI * lng / longitudes;
                    
                    double x = center.x() + a * std::cos(phi) * std::cos(theta);
                    double y = center.y() + b * std::cos(phi) * std::sin(theta);
                    double z = center.z() + c * std::sin(phi);
                    
                    vertices->push_back(osg::Vec3(x, y, z));
                }
            }
            
            // 添加纬线
            for (int lat = 0; lat <= latitudes; ++lat) {
                for (int lng = 0; lng < longitudes; ++lng) {
                    int current = lat * longitudes + lng;
                    int next = lat * longitudes + (lng + 1) % longitudes;
                    
                    indices->push_back(current);
                    indices->push_back(next);
                }
            }
            
            // 添加经线
            for (int lng = 0; lng < longitudes; ++lng) {
                for (int lat = 0; lat < latitudes; ++lat) {
                    int current = lat * longitudes + lng;
                    int next = (lat + 1) * longitudes + lng;
                    
                    indices->push_back(current);
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

void Ellipsoid3D_Geo::buildFaceGeometries()
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
    
    if (allStagePoints.size() < 4) {
        // 前三个阶段不显示面
        // 不绘制任何面
    }
    else if (allStagePoints.size() >= 4) {
        // 第四阶段：完整的椭球面
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1) {
            Point3D center = stage1[0];
            Point3D axis1End = stage2[0];
            Point3D axis2End = stage3[0];
            Point3D axis3End = stage4[0];
            
            // 计算三个轴的长度
            double a = glm::length(glm::dvec3(axis1End.x() - center.x(), axis1End.y() - center.y(), axis1End.z() - center.z()));
            double b = glm::length(glm::dvec3(axis2End.x() - center.x(), axis2End.y() - center.y(), axis2End.z() - center.z()));
            double c = glm::length(glm::dvec3(axis3End.x() - center.x(), axis3End.y() - center.y(), axis3End.z() - center.z()));
            
            int latitudes = 8;
            int longitudes = 16;
            
            // 生成椭球表面的所有顶点
            std::vector<osg::Vec3> ellipsoidVertices;
            for (int lat = 0; lat <= latitudes; ++lat) {
                double phi = M_PI * lat / latitudes - M_PI / 2;
                
                for (int lng = 0; lng < longitudes; ++lng) {
                    double theta = 2.0 * M_PI * lng / longitudes;
                    
                    double x = center.x() + a * std::cos(phi) * std::cos(theta);
                    double y = center.y() + b * std::cos(phi) * std::sin(theta);
                    double z = center.z() + c * std::sin(phi);
                    
                    ellipsoidVertices.push_back(osg::Vec3(x, y, z));
                }
            }
            
            // 生成四边形面片
            for (int lat = 0; lat < latitudes; ++lat) {
                for (int lng = 0; lng < longitudes; ++lng) {
                    int next_lng = (lng + 1) % longitudes;
                    
                    // 当前四边形的四个顶点
                    int v1 = lat * longitudes + lng;           // 当前层当前经度
                    int v2 = lat * longitudes + next_lng;      // 当前层下一经度
                    int v3 = (lat + 1) * longitudes + next_lng; // 下一层下一经度
                    int v4 = (lat + 1) * longitudes + lng;     // 下一层当前经度
                    
                    // 添加四边形顶点
                    vertices->push_back(ellipsoidVertices[v1]);
                    vertices->push_back(ellipsoidVertices[v2]);
                    vertices->push_back(ellipsoidVertices[v3]);
                    vertices->push_back(ellipsoidVertices[v4]);
                    
                    // 添加四边形面
                    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 
                        vertices->size() - 4, 4));
                }
            }
        }
    }
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
} 

