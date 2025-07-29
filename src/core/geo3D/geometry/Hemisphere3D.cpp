#include "Hemisphere3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "util/MathUtils.h"
#include "util/VertexShapeUtils.h"
#include <cmath>

Hemisphere3D_Geo::Hemisphere3D_Geo()
{
    m_geoType = Geo_Hemisphere3D;
    // 确保基类正确初始化
    initialize();
    
    // 立体几何特定的可见性设置：显示线面
    GeoParameters3D params = getParameters();
    params.showPoints = false;
    params.showEdges = true;
    params.showFaces = true;
    
    setParameters(params);
}

void Hemisphere3D_Geo::buildVertexGeometries()
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
        // 第一阶段：底面圆心
        const auto& stage1 = allStagePoints[0];
        
        for (size_t i = 0; i < stage1.size(); ++i) {
            vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
        }
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：圆心 + 半径点
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 1 && stage2.size() >= 1) {
            Point3D center = stage1[0];
            Point3D radiusPoint = stage2[0];
            
            // 计算半径
            double radius = glm::length(glm::dvec3(
                radiusPoint.x() - center.x(),
                radiusPoint.y() - center.y(),
                radiusPoint.z() - center.z()
            ));
            
            // 添加圆心
            vertices->push_back(osg::Vec3(center.x(), center.y(), center.z()));
            
            // 生成底面圆周的采样点
            int numPoints = 16;
            for (int i = 0; i < numPoints; ++i) {
                double angle = 2.0 * M_PI * i / numPoints;
                double x = center.x() + radius * std::cos(angle);
                double y = center.y() + radius * std::sin(angle);
                double z = center.z();
                vertices->push_back(osg::Vec3(x, y, z));
            }
        }
    }
    else if (allStagePoints.size() >= 3) {
        // 第三阶段：完整的半球体
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D center = stage1[0];
            Point3D radiusPoint = stage2[0];
            Point3D directionPoint = stage3[0];
            
            // 计算半径
            double radius = glm::length(glm::dvec3(
                radiusPoint.x() - center.x(),
                radiusPoint.y() - center.y(),
                radiusPoint.z() - center.z()
            ));
            
            // 确定半球朝向（向上或向下）
            bool upward = directionPoint.z() > center.z();
            
            // 生成半球表面的采样点
            int latitudes = 8;   // 纬度分割数
            int longitudes = 16; // 经度分割数
            
            // 添加底面圆周点
            for (int lng = 0; lng < longitudes; ++lng) {
                double theta = 2.0 * M_PI * lng / longitudes;
                double x = center.x() + radius * std::cos(theta);
                double y = center.y() + radius * std::sin(theta);
                double z = center.z();
                vertices->push_back(osg::Vec3(x, y, z));
            }
            
            // 添加半球表面点
            for (int lat = 1; lat <= latitudes; ++lat) {
                double phi = M_PI * lat / (2 * latitudes); // 0 到 π/2
                if (!upward) phi = -phi; // 向下的半球
                
                double cosLat = std::cos(phi);
                double sinLat = std::sin(phi);
                
                if (lat == latitudes) {
                    // 顶点
                    double x = center.x();
                    double y = center.y();
                    double z = center.z() + (upward ? radius : -radius);
                    vertices->push_back(osg::Vec3(x, y, z));
                } else {
                    // 中间层的点
                    for (int lng = 0; lng < longitudes; ++lng) {
                        double theta = 2.0 * M_PI * lng / longitudes;
                        double x = center.x() + radius * cosLat * std::cos(theta);
                        double y = center.y() + radius * cosLat * std::sin(theta);
                        double z = center.z() + radius * sinLat;
                        vertices->push_back(osg::Vec3(x, y, z));
                    }
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

void Hemisphere3D_Geo::buildEdgeGeometries()
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
        // 第一阶段：只有圆心，无边线
        // 不绘制任何边线
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：圆心到半径点的线 + 底面圆周
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 1 && stage2.size() >= 1) {
            Point3D center = stage1[0];
            Point3D radiusPoint = stage2[0];
            
            double radius = glm::length(glm::dvec3(
                radiusPoint.x() - center.x(),
                radiusPoint.y() - center.y(),
                radiusPoint.z() - center.z()
            ));
            
            // 添加圆心
            vertices->push_back(osg::Vec3(center.x(), center.y(), center.z()));
            
            // 生成底面圆周点
            int numPoints = 16;
            for (int i = 0; i < numPoints; ++i) {
                double angle = 2.0 * M_PI * i / numPoints;
                double x = center.x() + radius * std::cos(angle);
                double y = center.y() + radius * std::sin(angle);
                double z = center.z();
                vertices->push_back(osg::Vec3(x, y, z));
            }
            
            // 添加半径线
            vertices->push_back(osg::Vec3(radiusPoint.x(), radiusPoint.y(), radiusPoint.z()));
            indices->push_back(0); // 圆心
            indices->push_back(vertices->size() - 1); // 半径点
            
            // 添加底面圆周线
            for (int i = 0; i < numPoints; ++i) {
                indices->push_back(1 + i);
                indices->push_back(1 + (i + 1) % numPoints);
            }
        }
    }
    else if (allStagePoints.size() >= 3) {
        // 第三阶段：完整半球的线框
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D center = stage1[0];
            Point3D radiusPoint = stage2[0];
            Point3D directionPoint = stage3[0];
            
            double radius = glm::length(glm::dvec3(
                radiusPoint.x() - center.x(),
                radiusPoint.y() - center.y(),
                radiusPoint.z() - center.z()
            ));
            
            bool upward = directionPoint.z() > center.z();
            
            int latitudes = 8;
            int longitudes = 16;
            
            // 生成半球的所有顶点
            // 底面圆周点（第0层）
            for (int lng = 0; lng < longitudes; ++lng) {
                double theta = 2.0 * M_PI * lng / longitudes;
                double x = center.x() + radius * std::cos(theta);
                double y = center.y() + radius * std::sin(theta);
                double z = center.z();
                vertices->push_back(osg::Vec3(x, y, z));
            }
            
            // 半球表面点（第1层到第latitudes-1层）
            for (int lat = 1; lat < latitudes; ++lat) {
                double phi = M_PI * lat / (2 * latitudes);
                if (!upward) phi = -phi;
                
                double cosLat = std::cos(phi);
                double sinLat = std::sin(phi);
                
                for (int lng = 0; lng < longitudes; ++lng) {
                    double theta = 2.0 * M_PI * lng / longitudes;
                    double x = center.x() + radius * cosLat * std::cos(theta);
                    double y = center.y() + radius * cosLat * std::sin(theta);
                    double z = center.z() + radius * sinLat;
                    vertices->push_back(osg::Vec3(x, y, z));
                }
            }
            
            // 顶点（第latitudes层，只有一个点）
            double x = center.x();
            double y = center.y();
            double z = center.z() + (upward ? radius : -radius);
            vertices->push_back(osg::Vec3(x, y, z));
            int topIndex = vertices->size() - 1; // 顶点索引
            
            // 添加底面圆周线
            for (int i = 0; i < longitudes; ++i) {
                indices->push_back(i);
                indices->push_back((i + 1) % longitudes);
            }
            
            // 添加经线（从底面到顶点）
            for (int lng = 0; lng < longitudes; ++lng) {
                int prevRingIndex = lng; // 底面圆周点索引
                
                // 连接各层
                for (int lat = 1; lat < latitudes; ++lat) {
                    int currentRingIndex = longitudes + (lat - 1) * longitudes + lng;
                    indices->push_back(prevRingIndex);
                    indices->push_back(currentRingIndex);
                    prevRingIndex = currentRingIndex;
                }
                
                // 连接到顶点
                indices->push_back(prevRingIndex);
                indices->push_back(topIndex);
            }
            
            // 添加纬线（每一层的圆形边线）
            for (int lat = 1; lat < latitudes; ++lat) {
                int ringStartIndex = longitudes + (lat - 1) * longitudes;
                for (int i = 0; i < longitudes; ++i) {
                    indices->push_back(ringStartIndex + i);
                    indices->push_back(ringStartIndex + (i + 1) % longitudes);
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

void Hemisphere3D_Geo::buildFaceGeometries()
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
    
    if (allStagePoints.size() == 1) {
        // 第一阶段：无面
        // 不绘制任何面
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：只显示底面圆
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 1 && stage2.size() >= 1) {
            Point3D center = stage1[0];
            Point3D radiusPoint = stage2[0];
            
            double radius = glm::length(glm::dvec3(
                radiusPoint.x() - center.x(),
                radiusPoint.y() - center.y(),
                radiusPoint.z() - center.z()
            ));
            
            // 添加圆心
            vertices->push_back(osg::Vec3(center.x(), center.y(), center.z()));
            
            // 添加圆周点
            int numPoints = 16;
            for (int i = 0; i < numPoints; ++i) {
                double angle = 2.0 * M_PI * i / numPoints;
                double x = center.x() + radius * std::cos(angle);
                double y = center.y() + radius * std::sin(angle);
                double z = center.z();
                vertices->push_back(osg::Vec3(x, y, z));
            }
            
            // 添加圆形底面（三角形扇形）
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, numPoints + 1));
        }
    }
    else if (allStagePoints.size() >= 3) {
        // 第三阶段：完整的半球面
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D center = stage1[0];
            Point3D radiusPoint = stage2[0];
            Point3D directionPoint = stage3[0];
            
            double radius = glm::length(glm::dvec3(
                radiusPoint.x() - center.x(),
                radiusPoint.y() - center.y(),
                radiusPoint.z() - center.z()
            ));
            
            bool upward = directionPoint.z() > center.z();
            
            int latitudes = 8;
            int longitudes = 16;
            
            // 构建半球表面的所有顶点
            std::vector<std::vector<int>> ringIndices(latitudes + 1);
            
            // 底面圆周点
            for (int lng = 0; lng < longitudes; ++lng) {
                double theta = 2.0 * M_PI * lng / longitudes;
                double x = center.x() + radius * std::cos(theta);
                double y = center.y() + radius * std::sin(theta);
                double z = center.z();
                vertices->push_back(osg::Vec3(x, y, z));
                ringIndices[0].push_back(vertices->size() - 1);
            }
            
            // 半球各层点
            for (int lat = 1; lat <= latitudes; ++lat) {
                double phi = M_PI * lat / (2 * latitudes);
                if (!upward) phi = -phi;
                
                double cosLat = std::cos(phi);
                double sinLat = std::sin(phi);
                
                if (lat == latitudes) {
                    // 顶点
                    double x = center.x();
                    double y = center.y();
                    double z = center.z() + (upward ? radius : -radius);
                    vertices->push_back(osg::Vec3(x, y, z));
                    ringIndices[lat].push_back(vertices->size() - 1);
                } else {
                    for (int lng = 0; lng < longitudes; ++lng) {
                        double theta = 2.0 * M_PI * lng / longitudes;
                        double x = center.x() + radius * cosLat * std::cos(theta);
                        double y = center.y() + radius * cosLat * std::sin(theta);
                        double z = center.z() + radius * sinLat;
                        vertices->push_back(osg::Vec3(x, y, z));
                        ringIndices[lat].push_back(vertices->size() - 1);
                    }
                }
            }
            
            // 添加圆形底面（三角形扇形）
            vertices->push_back(osg::Vec3(center.x(), center.y(), center.z())); // 圆心
            int centerIndex = vertices->size() - 1;
            
            for (int i = 0; i < longitudes; ++i) {
                vertices->push_back(osg::Vec3(center.x(), center.y(), center.z())); // 圆心
                vertices->push_back(vertices->at(ringIndices[0][i])); // 当前底面点
                vertices->push_back(vertices->at(ringIndices[0][(i + 1) % longitudes])); // 下一个底面点
                
                geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 
                    vertices->size() - 3, 3));
            }
            
            // 添加半球面片
            for (int lat = 0; lat < latitudes - 1; ++lat) {
                for (int lng = 0; lng < longitudes; ++lng) {
                    int next_lng = (lng + 1) % longitudes;
                    
                    // 构建三角形面片（将四边形分解为两个三角形）
                    osg::Vec3 v1 = vertices->at(ringIndices[lat][lng]);
                    osg::Vec3 v2 = vertices->at(ringIndices[lat][next_lng]);
                    osg::Vec3 v3 = vertices->at(ringIndices[lat + 1][next_lng]);
                    osg::Vec3 v4 = vertices->at(ringIndices[lat + 1][lng]);
                    
                    // 第一个三角形：v1 -> v2 -> v3
                    vertices->push_back(v1);
                    vertices->push_back(v2);
                    vertices->push_back(v3);
                    
                    // 第二个三角形：v1 -> v3 -> v4
                    vertices->push_back(v1);
                    vertices->push_back(v3);
                    vertices->push_back(v4);
                }
            }
            
            // 添加所有四边形转换的三角形面片
            int quadTriangleCount = (latitudes - 1) * longitudes * 2 * 3; // 每个四边形=2个三角形，每个三角形=3个顶点
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 
                vertices->size() - quadTriangleCount, quadTriangleCount));
            
            // 最顶层的三角形面片
            int topRing = latitudes - 1;
            int topIndex = ringIndices[latitudes][0]; // 顶点索引
            for (int lng = 0; lng < longitudes; ++lng) {
                int next_lng = (lng + 1) % longitudes;
                
                vertices->push_back(vertices->at(ringIndices[topRing][lng]));
                vertices->push_back(vertices->at(ringIndices[topRing][next_lng]));
                vertices->push_back(vertices->at(topIndex));
                
                geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 
                    vertices->size() - 3, 3));
            }
        }
    }
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
} 

