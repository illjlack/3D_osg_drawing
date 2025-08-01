#include "DomeHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"
#include "../../util/VertexShapeUtils.h"
#include <cmath>

DomeHouse3D_Geo::DomeHouse3D_Geo()
{
    m_geoType = Geo_DomeHouse3D;
    // 确保基类正确初始化
    initialize();
    
    // 房屋类特定的可见性设置：只显示点和线
    GeoParameters3D params = getParameters();
    params.showPoints = true;
    params.showEdges = true;
    params.showFaces = false;
    
    setParameters(params);
}

void DomeHouse3D_Geo::buildVertexGeometries()
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
        // 第一阶段：确定圆形基座（通过3个点确定圆）
        const auto& stage1 = allStagePoints[0];
        
        for (size_t i = 0; i < stage1.size(); ++i) {
            vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
        }
        
        // 如果有3个点，计算圆心和半径，生成圆形基座的更多点
        if (stage1.size() >= 3) {
            Point3D p1 = stage1[0];
            Point3D p2 = stage1[1];
            Point3D p3 = stage1[2];
            
            // 计算圆心（简化算法，假设三点在同一平面）
            Point3D center = Point3D(
                (p1.x() + p2.x() + p3.x()) / 3.0,
                (p1.y() + p2.y() + p3.y()) / 3.0,
                (p1.z() + p2.z() + p3.z()) / 3.0
            );
            
            // 计算半径（取到第一个点的距离）
            double radius = std::sqrt(
                std::pow(p1.x() - center.x(), 2) +
                std::pow(p1.y() - center.y(), 2) +
                std::pow(p1.z() - center.z(), 2)
            );
            
            // 生成圆形基座的其他点
            int numPoints = 16; // 16边形近似圆
            for (int i = 3; i < numPoints; ++i) {
                double angle = 2.0 * M_PI * i / numPoints;
                double x = center.x() + radius * std::cos(angle);
                double y = center.y() + radius * std::sin(angle);
                double z = center.z();
                vertices->push_back(osg::Vec3(x, y, z));
            }
        }
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：确定穹顶高度
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 3 && stage2.size() >= 1) {
            Point3D p1 = stage1[0];
            Point3D p2 = stage1[1];
            Point3D p3 = stage1[2];
            Point3D heightPoint = stage2[0];
            
            // 计算圆心和半径
            Point3D center = Point3D(
                (p1.x() + p2.x() + p3.x()) / 3.0,
                (p1.y() + p2.y() + p3.y()) / 3.0,
                (p1.z() + p2.z() + p3.z()) / 3.0
            );
            
            double radius = std::sqrt(
                std::pow(p1.x() - center.x(), 2) +
                std::pow(p1.y() - center.y(), 2) +
                std::pow(p1.z() - center.z(), 2)
            );
            
            double domeHeight = heightPoint.z() - center.z();
            
            // 添加圆形基座的点
            int numBasePoints = 16;
            for (int i = 0; i < numBasePoints; ++i) {
                double angle = 2.0 * M_PI * i / numBasePoints;
                double x = center.x() + radius * std::cos(angle);
                double y = center.y() + radius * std::sin(angle);
                double z = center.z();
                vertices->push_back(osg::Vec3(x, y, z));
            }
            
            // 添加穹顶的点（半球）
            int numRings = 8; // 穹顶分为8层
            for (int ring = 1; ring <= numRings; ++ring) {
                double ringHeight = domeHeight * ring / numRings;
                double ringRadius = radius * std::sqrt(1.0 - std::pow((double)ring / numRings, 2));
                
                if (ring == numRings) {
                    // 最顶层只有一个点
                    vertices->push_back(osg::Vec3(center.x(), center.y(), center.z() + domeHeight));
                } else {
                    for (int i = 0; i < numBasePoints; ++i) {
                        double angle = 2.0 * M_PI * i / numBasePoints;
                        double x = center.x() + ringRadius * std::cos(angle);
                        double y = center.y() + ringRadius * std::sin(angle);
                        double z = center.z() + ringHeight;
                        vertices->push_back(osg::Vec3(x, y, z));
                    }
                }
            }
        }
    }
    else if (allStagePoints.size() >= 3) {
        // 第三阶段：确定地面，形成完整的圆顶房屋
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 3 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D p1 = stage1[0];
            Point3D p2 = stage1[1];
            Point3D p3 = stage1[2];
            Point3D heightPoint = stage2[0];
            Point3D groundPoint = stage3[0];
            
            // 计算圆心和半径
            Point3D center = Point3D(
                (p1.x() + p2.x() + p3.x()) / 3.0,
                (p1.y() + p2.y() + p3.y()) / 3.0,
                (p1.z() + p2.z() + p3.z()) / 3.0
            );
            
            double radius = std::sqrt(
                std::pow(p1.x() - center.x(), 2) +
                std::pow(p1.y() - center.y(), 2) +
                std::pow(p1.z() - center.z(), 2)
            );
            
            double domeHeight = heightPoint.z() - center.z();
            double groundHeight = groundPoint.z() - center.z();
            
            // 添加地面圆形的点
            int numBasePoints = 16;
            for (int i = 0; i < numBasePoints; ++i) {
                double angle = 2.0 * M_PI * i / numBasePoints;
                double x = center.x() + radius * std::cos(angle);
                double y = center.y() + radius * std::sin(angle);
                double z = center.z() + groundHeight;
                vertices->push_back(osg::Vec3(x, y, z));
            }
            
            // 添加原基座圆形的点（墙体底部）
            for (int i = 0; i < numBasePoints; ++i) {
                double angle = 2.0 * M_PI * i / numBasePoints;
                double x = center.x() + radius * std::cos(angle);
                double y = center.y() + radius * std::sin(angle);
                double z = center.z();
                vertices->push_back(osg::Vec3(x, y, z));
            }
            
            // 添加穹顶的点（半球）
            int numRings = 8; // 穹顶分为8层
            for (int ring = 1; ring <= numRings; ++ring) {
                double ringHeight = domeHeight * ring / numRings;
                double ringRadius = radius * std::sqrt(1.0 - std::pow((double)ring / numRings, 2));
                
                if (ring == numRings) {
                    // 最顶层只有一个点
                    vertices->push_back(osg::Vec3(center.x(), center.y(), center.z() + domeHeight));
                } else {
                    for (int i = 0; i < numBasePoints; ++i) {
                        double angle = 2.0 * M_PI * i / numBasePoints;
                        double x = center.x() + ringRadius * std::cos(angle);
                        double y = center.y() + ringRadius * std::sin(angle);
                        double z = center.z() + ringHeight;
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

void DomeHouse3D_Geo::buildEdgeGeometries()
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
        // 第一阶段：圆形基座的边线
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 3) {
            Point3D p1 = stage1[0];
            Point3D p2 = stage1[1];
            Point3D p3 = stage1[2];
            
            // 计算圆心和半径
            Point3D center = Point3D(
                (p1.x() + p2.x() + p3.x()) / 3.0,
                (p1.y() + p2.y() + p3.y()) / 3.0,
                (p1.z() + p2.z() + p3.z()) / 3.0
            );
            
            double radius = std::sqrt(
                std::pow(p1.x() - center.x(), 2) +
                std::pow(p1.y() - center.y(), 2) +
                std::pow(p1.z() - center.z(), 2)
            );
            
            // 生成圆形基座的边线
            int numPoints = 16;
            for (int i = 0; i < numPoints; ++i) {
                double angle = 2.0 * M_PI * i / numPoints;
                double x = center.x() + radius * std::cos(angle);
                double y = center.y() + radius * std::sin(angle);
                double z = center.z();
                vertices->push_back(osg::Vec3(x, y, z));
            }
            
            // 添加圆形的边线索引
            for (int i = 0; i < numPoints; ++i) {
                indices->push_back(i);
                indices->push_back((i + 1) % numPoints);
            }
        }
        else if (stage1.size() >= 2) {
            // 少于3个点时，画已有的线段
            for (size_t i = 0; i < stage1.size(); ++i) {
                vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
            }
            for (size_t i = 0; i < stage1.size() - 1; ++i) {
                indices->push_back(i);
                indices->push_back(i + 1);
            }
        }
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：添加穹顶边线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 3 && stage2.size() >= 1) {
            Point3D p1 = stage1[0];
            Point3D p2 = stage1[1];
            Point3D p3 = stage1[2];
            Point3D heightPoint = stage2[0];
            
            Point3D center = Point3D(
                (p1.x() + p2.x() + p3.x()) / 3.0,
                (p1.y() + p2.y() + p3.y()) / 3.0,
                (p1.z() + p2.z() + p3.z()) / 3.0
            );
            
            double radius = std::sqrt(
                std::pow(p1.x() - center.x(), 2) +
                std::pow(p1.y() - center.y(), 2) +
                std::pow(p1.z() - center.z(), 2)
            );
            
            double domeHeight = heightPoint.z() - center.z();
            
            // 添加圆形基座的顶点
            int numBasePoints = 16;
            for (int i = 0; i < numBasePoints; ++i) {
                double angle = 2.0 * M_PI * i / numBasePoints;
                double x = center.x() + radius * std::cos(angle);
                double y = center.y() + radius * std::sin(angle);
                double z = center.z();
                vertices->push_back(osg::Vec3(x, y, z));
            }
            
            // 添加穹顶的顶点
            int numRings = 8;
            
            for (int ring = 1; ring <= numRings; ++ring) {
                double ringHeight = domeHeight * ring / numRings;
                double ringRadius = radius * std::sqrt(1.0 - std::pow((double)ring / numRings, 2));
                
                if (ring == numRings) {
                    // 最顶层点
                    vertices->push_back(osg::Vec3(center.x(), center.y(), center.z() + domeHeight));
                } else {
                    for (int i = 0; i < numBasePoints; ++i) {
                        double angle = 2.0 * M_PI * i / numBasePoints;
                        double x = center.x() + ringRadius * std::cos(angle);
                        double y = center.y() + ringRadius * std::sin(angle);
                        double z = center.z() + ringHeight;
                        vertices->push_back(osg::Vec3(x, y, z));
                    }
                }
            }
            
            // 添加基座圆形边线
            for (int i = 0; i < numBasePoints; ++i) {
                indices->push_back(i);
                indices->push_back((i + 1) % numBasePoints);
            }
            
            // 添加穹顶的经线（从基座到顶点的线）
            int topIndex = vertices->size() - 1; // 最顶层点的索引
            for (int i = 0; i < numBasePoints; ++i) {
                // 从基座点到顶点的经线
                int baseIndex = i;
                int prevRingIndex = baseIndex;
                
                for (int ring = 1; ring < numRings; ++ring) {
                    int currentRingIndex = numBasePoints + (ring - 1) * numBasePoints + i;
                    indices->push_back(prevRingIndex);
                    indices->push_back(currentRingIndex);
                    prevRingIndex = currentRingIndex;
                }
                
                // 连接到顶点
                indices->push_back(prevRingIndex);
                indices->push_back(topIndex);
            }
            
            // 添加穹顶的纬线（每一层的圆形边线）
            for (int ring = 1; ring < numRings; ++ring) {
                int ringStartIndex = numBasePoints + (ring - 1) * numBasePoints;
                for (int i = 0; i < numBasePoints; ++i) {
                    indices->push_back(ringStartIndex + i);
                    indices->push_back(ringStartIndex + (i + 1) % numBasePoints);
                }
            }
        }
    }
    else if (allStagePoints.size() >= 3) {
        // 第三阶段：确定地面，完整圆顶房屋的边线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 3 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D p1 = stage1[0];
            Point3D p2 = stage1[1];
            Point3D p3 = stage1[2];
            Point3D heightPoint = stage2[0];
            Point3D groundPoint = stage3[0];
            
            Point3D center = Point3D(
                (p1.x() + p2.x() + p3.x()) / 3.0,
                (p1.y() + p2.y() + p3.y()) / 3.0,
                (p1.z() + p2.z() + p3.z()) / 3.0
            );
            
            double radius = std::sqrt(
                std::pow(p1.x() - center.x(), 2) +
                std::pow(p1.y() - center.y(), 2) +
                std::pow(p1.z() - center.z(), 2)
            );
            
            double domeHeight = heightPoint.z() - center.z();
            double groundHeight = groundPoint.z() - center.z();
            
            // 添加地面圆形的顶点
            int numBasePoints = 16;
            for (int i = 0; i < numBasePoints; ++i) {
                double angle = 2.0 * M_PI * i / numBasePoints;
                double x = center.x() + radius * std::cos(angle);
                double y = center.y() + radius * std::sin(angle);
                double z = center.z() + groundHeight;
                vertices->push_back(osg::Vec3(x, y, z));
            }
            
            // 添加墙体基座圆形的顶点
            for (int i = 0; i < numBasePoints; ++i) {
                double angle = 2.0 * M_PI * i / numBasePoints;
                double x = center.x() + radius * std::cos(angle);
                double y = center.y() + radius * std::sin(angle);
                double z = center.z();
                vertices->push_back(osg::Vec3(x, y, z));
            }
            
            // 添加穹顶的顶点
            int numRings = 8;
            
            for (int ring = 1; ring <= numRings; ++ring) {
                double ringHeight = domeHeight * ring / numRings;
                double ringRadius = radius * std::sqrt(1.0 - std::pow((double)ring / numRings, 2));
                
                if (ring == numRings) {
                    // 最顶层点
                    vertices->push_back(osg::Vec3(center.x(), center.y(), center.z() + domeHeight));
                } else {
                    for (int i = 0; i < numBasePoints; ++i) {
                        double angle = 2.0 * M_PI * i / numBasePoints;
                        double x = center.x() + ringRadius * std::cos(angle);
                        double y = center.y() + ringRadius * std::sin(angle);
                        double z = center.z() + ringHeight;
                        vertices->push_back(osg::Vec3(x, y, z));
                    }
                }
            }
            
            // 地面圆形边线
            for (int i = 0; i < numBasePoints; ++i) {
                indices->push_back(i);
                indices->push_back((i + 1) % numBasePoints);
            }
            
            // 墙体基座圆形边线
            for (int i = 0; i < numBasePoints; ++i) {
                indices->push_back(numBasePoints + i);
                indices->push_back(numBasePoints + (i + 1) % numBasePoints);
            }
            
            // 垂直边线（地面到基座）
            for (int i = 0; i < numBasePoints; ++i) {
                indices->push_back(i);
                indices->push_back(numBasePoints + i);
            }
            
            // 添加穹顶的经线（从基座到顶点的线）
            int topIndex = vertices->size() - 1; // 最顶层点的索引
            int baseStartIndex = numBasePoints; // 墙体基座起始索引
            
            for (int i = 0; i < numBasePoints; ++i) {
                // 从墙体基座点到顶点的经线
                int baseIndex = baseStartIndex + i;
                int prevRingIndex = baseIndex;
                
                for (int ring = 1; ring < numRings; ++ring) {
                    int currentRingIndex = 2 * numBasePoints + (ring - 1) * numBasePoints + i;
                    indices->push_back(prevRingIndex);
                    indices->push_back(currentRingIndex);
                    prevRingIndex = currentRingIndex;
                }
                
                // 连接到顶点
                indices->push_back(prevRingIndex);
                indices->push_back(topIndex);
            }
            
            // 添加穹顶的纬线（每一层的圆形边线）
            for (int ring = 1; ring < numRings; ++ring) {
                int ringStartIndex = 2 * numBasePoints + (ring - 1) * numBasePoints;
                for (int i = 0; i < numBasePoints; ++i) {
                    indices->push_back(ringStartIndex + i);
                    indices->push_back(ringStartIndex + (i + 1) % numBasePoints);
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

void DomeHouse3D_Geo::buildFaceGeometries()
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
        // 第一阶段：只显示圆形基座底面
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 3) {
            Point3D p1 = stage1[0];
            Point3D p2 = stage1[1];
            Point3D p3 = stage1[2];
            
            Point3D center = Point3D(
                (p1.x() + p2.x() + p3.x()) / 3.0,
                (p1.y() + p2.y() + p3.y()) / 3.0,
                (p1.z() + p2.z() + p3.z()) / 3.0
            );
            
            double radius = std::sqrt(
                std::pow(p1.x() - center.x(), 2) +
                std::pow(p1.y() - center.y(), 2) +
                std::pow(p1.z() - center.z(), 2)
            );
            
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
    else if (allStagePoints.size() == 2) {
        // 第二阶段：圆顶房屋（底面 + 穹顶，无地面）
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 3 && stage2.size() >= 1) {
            Point3D p1 = stage1[0];
            Point3D p2 = stage1[1];
            Point3D p3 = stage1[2];
            Point3D heightPoint = stage2[0];
            
            Point3D center = Point3D(
                (p1.x() + p2.x() + p3.x()) / 3.0,
                (p1.y() + p2.y() + p3.y()) / 3.0,
                (p1.z() + p2.z() + p3.z()) / 3.0
            );
            
            double radius = std::sqrt(
                std::pow(p1.x() - center.x(), 2) +
                std::pow(p1.y() - center.y(), 2) +
                std::pow(p1.z() - center.z(), 2)
            );
            
            double domeHeight = heightPoint.z() - center.z();
            
            int numBasePoints = 16;
            int numRings = 8;
            
            // 构建穹顶的所有顶点
            std::vector<std::vector<int>> ringIndices(numRings + 1);
            
            // 基座圆周点
            for (int i = 0; i < numBasePoints; ++i) {
                double angle = 2.0 * M_PI * i / numBasePoints;
                double x = center.x() + radius * std::cos(angle);
                double y = center.y() + radius * std::sin(angle);
                double z = center.z();
                vertices->push_back(osg::Vec3(x, y, z));
                ringIndices[0].push_back(vertices->size() - 1);
            }
            
            // 基座中心点（用于底面三角形扇形）
            vertices->push_back(osg::Vec3(center.x(), center.y(), center.z()));
            int centerIndex = vertices->size() - 1;
            
            // 穹顶各层点
            for (int ring = 1; ring <= numRings; ++ring) {
                double ringHeight = domeHeight * ring / numRings;
                double ringRadius = radius * std::sqrt(1.0 - std::pow((double)ring / numRings, 2));
                
                if (ring == numRings) {
                    // 最顶层只有一个点
                    vertices->push_back(osg::Vec3(center.x(), center.y(), center.z() + domeHeight));
                    ringIndices[ring].push_back(vertices->size() - 1);
                } else {
                    for (int i = 0; i < numBasePoints; ++i) {
                        double angle = 2.0 * M_PI * i / numBasePoints;
                        double x = center.x() + ringRadius * std::cos(angle);
                        double y = center.y() + ringRadius * std::sin(angle);
                        double z = center.z() + ringHeight;
                        vertices->push_back(osg::Vec3(x, y, z));
                        ringIndices[ring].push_back(vertices->size() - 1);
                    }
                }
            }
            
            // 添加底面（三角形扇形）
            for (int i = 0; i < numBasePoints; ++i) {
                vertices->push_back(vertices->at(ringIndices[0][i]));      // 基座点
                vertices->push_back(vertices->at(ringIndices[0][(i + 1) % numBasePoints])); // 下一个基座点
                vertices->push_back(vertices->at(centerIndex));           // 中心点
            }
            
            // 添加穹顶面片
            for (int ring = 0; ring < numRings - 1; ++ring) {
                for (int i = 0; i < numBasePoints; ++i) {
                    int currentRing = ring;
                    int nextRing = ring + 1;
                    int currentPoint = i;
                    int nextPoint = (i + 1) % numBasePoints;
                    
                    // 四边形分解为两个三角形
                    // 三角形1
                    vertices->push_back(vertices->at(ringIndices[currentRing][currentPoint]));
                    vertices->push_back(vertices->at(ringIndices[currentRing][nextPoint]));
                    vertices->push_back(vertices->at(ringIndices[nextRing][currentPoint]));
                    
                    // 三角形2
                    vertices->push_back(vertices->at(ringIndices[currentRing][nextPoint]));
                    vertices->push_back(vertices->at(ringIndices[nextRing][nextPoint]));
                    vertices->push_back(vertices->at(ringIndices[nextRing][currentPoint]));
                }
            }
            
            // 最后一层（连接到顶点的三角形）
            int lastRing = numRings - 1;
            int topIndex = ringIndices[numRings][0];
            for (int i = 0; i < numBasePoints; ++i) {
                vertices->push_back(vertices->at(ringIndices[lastRing][i]));
                vertices->push_back(vertices->at(ringIndices[lastRing][(i + 1) % numBasePoints]));
                vertices->push_back(vertices->at(topIndex));
            }
            
            // 添加图元集合
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, vertices->size()));
        }
    }
    else if (allStagePoints.size() >= 3) {
        // 第三阶段：完整的圆顶房屋（底面 + 穹顶 + 地面）
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 3 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D p1 = stage1[0];
            Point3D p2 = stage1[1];
            Point3D p3 = stage1[2];
            Point3D heightPoint = stage2[0];
            Point3D groundPoint = stage3[0];
            
            Point3D center = Point3D(
                (p1.x() + p2.x() + p3.x()) / 3.0,
                (p1.y() + p2.y() + p3.y()) / 3.0,
                (p1.z() + p2.z() + p3.z()) / 3.0
            );
            
            double radius = std::sqrt(
                std::pow(p1.x() - center.x(), 2) +
                std::pow(p1.y() - center.y(), 2) +
                std::pow(p1.z() - center.z(), 2)
            );
            
            double domeHeight = heightPoint.z() - center.z();
            double groundHeight = groundPoint.z() - center.z();
            
            int numBasePoints = 16;
            int numRings = 8;
            
            // 构建穹顶的所有顶点
            std::vector<std::vector<int>> ringIndices(numRings + 1);
            
            // 地面圆周点
            std::vector<int> groundIndices;
            for (int i = 0; i < numBasePoints; ++i) {
                double angle = 2.0 * M_PI * i / numBasePoints;
                double x = center.x() + radius * std::cos(angle);
                double y = center.y() + radius * std::sin(angle);
                double z = center.z() + groundHeight;
                vertices->push_back(osg::Vec3(x, y, z));
                groundIndices.push_back(vertices->size() - 1);
            }
            
            // 墙体基座圆周点
            for (int i = 0; i < numBasePoints; ++i) {
                double angle = 2.0 * M_PI * i / numBasePoints;
                double x = center.x() + radius * std::cos(angle);
                double y = center.y() + radius * std::sin(angle);
                double z = center.z();
                vertices->push_back(osg::Vec3(x, y, z));
                ringIndices[0].push_back(vertices->size() - 1);
            }
            
            // 穹顶各层点
            for (int ring = 1; ring <= numRings; ++ring) {
                double ringHeight = domeHeight * ring / numRings;
                double ringRadius = radius * std::sqrt(1.0 - std::pow((double)ring / numRings, 2));
                
                if (ring == numRings) {
                    // 最顶层点
                    vertices->push_back(osg::Vec3(center.x(), center.y(), center.z() + domeHeight));
                    ringIndices[ring].push_back(vertices->size() - 1);
                } else {
                    for (int i = 0; i < numBasePoints; ++i) {
                        double angle = 2.0 * M_PI * i / numBasePoints;
                        double x = center.x() + ringRadius * std::cos(angle);
                        double y = center.y() + ringRadius * std::sin(angle);
                        double z = center.z() + ringHeight;
                        vertices->push_back(osg::Vec3(x, y, z));
                        ringIndices[ring].push_back(vertices->size() - 1);
                    }
                }
            }
            
            // 添加地面圆形面片（三角形扇形）
            vertices->push_back(osg::Vec3(center.x(), center.y(), center.z() + groundHeight));
            int groundCenterIndex = vertices->size() - 1;
            
            for (int i = 0; i < numBasePoints; ++i) {
                vertices->push_back(vertices->at(groundCenterIndex));
                vertices->push_back(vertices->at(groundIndices[i]));
                vertices->push_back(vertices->at(groundIndices[(i + 1) % numBasePoints]));
            }
            
            // 添加墙体底面圆形面片（三角形扇形）
            vertices->push_back(osg::Vec3(center.x(), center.y(), center.z()));
            int baseCenterIndex = vertices->size() - 1;
            
            for (int i = 0; i < numBasePoints; ++i) {
                vertices->push_back(vertices->at(baseCenterIndex));
                vertices->push_back(vertices->at(ringIndices[0][(i + 1) % numBasePoints]));
                vertices->push_back(vertices->at(ringIndices[0][i]));
            }
            
            // 添加墙体侧面面片（连接地面和基座）
            for (int i = 0; i < numBasePoints; ++i) {
                int next_i = (i + 1) % numBasePoints;
                
                // 四边形分解为两个三角形
                // 三角形1: 地面[i] -> 地面[next_i] -> 基座[i]
                vertices->push_back(vertices->at(groundIndices[i]));
                vertices->push_back(vertices->at(groundIndices[next_i]));
                vertices->push_back(vertices->at(ringIndices[0][i]));
                
                // 三角形2: 地面[next_i] -> 基座[next_i] -> 基座[i]
                vertices->push_back(vertices->at(groundIndices[next_i]));
                vertices->push_back(vertices->at(ringIndices[0][next_i]));
                vertices->push_back(vertices->at(ringIndices[0][i]));
            }
            
            // 添加穹顶面片
            for (int ring = 0; ring < numRings - 1; ++ring) {
                for (int i = 0; i < numBasePoints; ++i) {
                    int next_i = (i + 1) % numBasePoints;
                    
                    // 四边形分解为两个三角形
                    // 三角形1
                    vertices->push_back(vertices->at(ringIndices[ring][i]));
                    vertices->push_back(vertices->at(ringIndices[ring][next_i]));
                    vertices->push_back(vertices->at(ringIndices[ring + 1][i]));
                    
                    // 三角形2
                    vertices->push_back(vertices->at(ringIndices[ring][next_i]));
                    vertices->push_back(vertices->at(ringIndices[ring + 1][next_i]));
                    vertices->push_back(vertices->at(ringIndices[ring + 1][i]));
                }
            }
            
            // 最顶层的三角形面片
            int topRing = numRings - 1;
            int topIndex = ringIndices[numRings][0];
            for (int i = 0; i < numBasePoints; ++i) {
                vertices->push_back(vertices->at(ringIndices[topRing][i]));
                vertices->push_back(vertices->at(ringIndices[topRing][(i + 1) % numBasePoints]));
                vertices->push_back(vertices->at(topIndex));
            }
            
            // 添加所有三角形
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, vertices->size()));
        }
    }
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
} 

