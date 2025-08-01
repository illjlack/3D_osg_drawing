#include "CourtHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"
#include "../../util/VertexShapeUtils.h"

CourtHouse3D_Geo::CourtHouse3D_Geo()
{
    m_geoType = Geo_CourtHouse3D;
    // 确保基类正确初始化
    initialize();
    
    // 房屋类特定的可见性设置：只显示点和线
    GeoParameters3D params = getParameters();
    params.showPoints = true;
    params.showEdges = true;
    params.showFaces = false;
    
    setParameters(params);
}

void CourtHouse3D_Geo::buildVertexGeometries()
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
        // 第一阶段：确定外围多边形
        const auto& stage1 = allStagePoints[0];
        
        for (size_t i = 0; i < stage1.size(); ++i) {
            vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
        }
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：确定内围多边形
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        // 添加外围多边形顶点
        for (size_t i = 0; i < stage1.size(); ++i) {
            vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
        }
        
        // 添加内围多边形顶点
        for (size_t i = 0; i < stage2.size(); ++i) {
            vertices->push_back(osg::Vec3(stage2[i].x(), stage2[i].y(), stage2[i].z()));
        }
    }
    else if (allStagePoints.size() >= 3) {
        // 第三阶段：确定墙体高度，形成完整的回型房屋
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 4 && stage2.size() >= 3 && stage3.size() >= 1) {
            Point3D wallHeightPoint = stage3[0];
            
            // 计算墙体高度（以外围第一个点为基准）
            double wallHeight = wallHeightPoint.z() - stage1[0].z();
            
            // 添加外围底面顶点
            for (size_t i = 0; i < stage1.size(); ++i) {
                vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
            }
            
            // 添加内围底面顶点
            for (size_t i = 0; i < stage2.size(); ++i) {
                vertices->push_back(osg::Vec3(stage2[i].x(), stage2[i].y(), stage2[i].z()));
            }
            
            // 添加外围顶面顶点
            for (size_t i = 0; i < stage1.size(); ++i) {
                Point3D topPoint = Point3D(stage1[i].x(), stage1[i].y(), stage1[i].z() + wallHeight);
                vertices->push_back(osg::Vec3(topPoint.x(), topPoint.y(), topPoint.z()));
            }
            
            // 添加内围顶面顶点
            for (size_t i = 0; i < stage2.size(); ++i) {
                Point3D topPoint = Point3D(stage2[i].x(), stage2[i].y(), stage2[i].z() + wallHeight);
                vertices->push_back(osg::Vec3(topPoint.x(), topPoint.y(), topPoint.z()));
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

void CourtHouse3D_Geo::buildEdgeGeometries()
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
        // 第一阶段：外围多边形的边线
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 4) {
            // 添加外围多边形顶点
            for (size_t i = 0; i < stage1.size(); ++i) {
                vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
            }
            
            // 添加外围多边形的边线索引
            for (size_t i = 0; i < stage1.size(); ++i) {
                indices->push_back(i);
                indices->push_back((i + 1) % stage1.size());
            }
        }
        else if (stage1.size() >= 2) {
            // 少于4个点时，画已有的线段
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
        // 第二阶段：添加内围多边形边线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 4 && stage2.size() >= 3) {
            // 添加外围多边形顶点
            for (size_t i = 0; i < stage1.size(); ++i) {
                vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
            }
            
            // 添加内围多边形顶点
            for (size_t i = 0; i < stage2.size(); ++i) {
                vertices->push_back(osg::Vec3(stage2[i].x(), stage2[i].y(), stage2[i].z()));
            }
            
            // 外围多边形边线
            for (size_t i = 0; i < stage1.size(); ++i) {
                indices->push_back(i);
                indices->push_back((i + 1) % stage1.size());
            }
            
            // 内围多边形边线
            int innerStart = stage1.size();
            for (size_t i = 0; i < stage2.size(); ++i) {
                indices->push_back(innerStart + i);
                indices->push_back(innerStart + (i + 1) % stage2.size());
            }
        }
    }
    else if (allStagePoints.size() >= 3) {
        // 第三阶段：确定墙体高度，完整的回型房屋边线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 4 && stage2.size() >= 3 && stage3.size() >= 1) {
            Point3D wallHeightPoint = stage3[0];
            
            // 计算墙体高度
            double wallHeight = wallHeightPoint.z() - stage1[0].z();
            
            // 添加外围底面顶点
            for (size_t i = 0; i < stage1.size(); ++i) {
                vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
            }
            
            // 添加内围底面顶点
            for (size_t i = 0; i < stage2.size(); ++i) {
                vertices->push_back(osg::Vec3(stage2[i].x(), stage2[i].y(), stage2[i].z()));
            }
            
            // 添加外围顶面顶点
            for (size_t i = 0; i < stage1.size(); ++i) {
                Point3D topPoint = Point3D(stage1[i].x(), stage1[i].y(), stage1[i].z() + wallHeight);
                vertices->push_back(osg::Vec3(topPoint.x(), topPoint.y(), topPoint.z()));
            }
            
            // 添加内围顶面顶点
            for (size_t i = 0; i < stage2.size(); ++i) {
                Point3D topPoint = Point3D(stage2[i].x(), stage2[i].y(), stage2[i].z() + wallHeight);
                vertices->push_back(osg::Vec3(topPoint.x(), topPoint.y(), topPoint.z()));
            }
            
            int outerBottomStart = 0;
            int innerBottomStart = stage1.size();
            int outerTopStart = stage1.size() + stage2.size();
            int innerTopStart = stage1.size() + stage2.size() + stage1.size();
            
            // 外围底面多边形边线
            for (size_t i = 0; i < stage1.size(); ++i) {
                indices->push_back(outerBottomStart + i);
                indices->push_back(outerBottomStart + (i + 1) % stage1.size());
            }
            
            // 内围底面多边形边线
            for (size_t i = 0; i < stage2.size(); ++i) {
                indices->push_back(innerBottomStart + i);
                indices->push_back(innerBottomStart + (i + 1) % stage2.size());
            }
            
            // 外围顶面多边形边线
            for (size_t i = 0; i < stage1.size(); ++i) {
                indices->push_back(outerTopStart + i);
                indices->push_back(outerTopStart + (i + 1) % stage1.size());
            }
            
            // 内围顶面多边形边线
            for (size_t i = 0; i < stage2.size(); ++i) {
                indices->push_back(innerTopStart + i);
                indices->push_back(innerTopStart + (i + 1) % stage2.size());
            }
            
            // 外围垂直边线（底面到顶面）
            for (size_t i = 0; i < stage1.size(); ++i) {
                indices->push_back(outerBottomStart + i);
                indices->push_back(outerTopStart + i);
            }
            
            // 内围垂直边线（底面到顶面）
            for (size_t i = 0; i < stage2.size(); ++i) {
                indices->push_back(innerBottomStart + i);
                indices->push_back(innerTopStart + i);
            }
        }
    }
    
    // 设置顶点数组和索引
    geometry->setVertexArray(vertices);
    if (indices->size() > 0) {
        geometry->addPrimitiveSet(indices);
    }
}

void CourtHouse3D_Geo::buildFaceGeometries()
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
        // 第一阶段：只显示外围多边形底面
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 4) {
            // 添加外围多边形的三角扇形面，计算中心点
            double centerX = 0.0, centerY = 0.0, centerZ = 0.0;
            for (size_t i = 0; i < stage1.size(); ++i) {
                centerX += stage1[i].x();
                centerY += stage1[i].y();
                centerZ += stage1[i].z();
            }
            Point3D center = Point3D(centerX / (double)stage1.size(), 
                                   centerY / (double)stage1.size(), 
                                   centerZ / (double)stage1.size());
            
            // 添加中心点
            vertices->push_back(osg::Vec3(center.x(), center.y(), center.z()));
            
            // 添加多边形顶点
            for (size_t i = 0; i < stage1.size(); ++i) {
                vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
            }
            
            // 使用三角扇形
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, stage1.size() + 1));
        }
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：显示外围底面，但中间挖空内围部分
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 4 && stage2.size() >= 3) {
            // 简化处理：分别绘制外围和内围的边界线作为面的边界表示
            // 这里可以后续优化为真正的挖空多边形
            for (size_t i = 0; i < stage1.size(); ++i) {
                vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
            }
            
            // 外围多边形的线框表示
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP, 0, stage1.size()));
        }
    }
    else if (allStagePoints.size() >= 3) {
        // 第三阶段：完整的回型房屋面
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 4 && stage2.size() >= 3 && stage3.size() >= 1) {
            Point3D wallHeightPoint = stage3[0];
            double wallHeight = wallHeightPoint.z() - stage1[0].z();
            
            // 外围墙体面（每个面分解为两个三角形）
            for (size_t i = 0; i < stage1.size(); ++i) {
                size_t next = (i + 1) % stage1.size();
                
                Point3D A = stage1[i];
                Point3D B = stage1[next];
                Point3D A_top = Point3D(A.x(), A.y(), A.z() + wallHeight);
                Point3D B_top = Point3D(B.x(), B.y(), B.z() + wallHeight);
                
                // 外壁面（逆时针）
                // 三角形1: A -> B -> A_top
                vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
                vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
                vertices->push_back(osg::Vec3(A_top.x(), A_top.y(), A_top.z()));
                
                // 三角形2: B -> B_top -> A_top
                vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
                vertices->push_back(osg::Vec3(B_top.x(), B_top.y(), B_top.z()));
                vertices->push_back(osg::Vec3(A_top.x(), A_top.y(), A_top.z()));
            }
            
            // 内围墙体面（每个面分解为两个三角形）
            for (size_t i = 0; i < stage2.size(); ++i) {
                size_t next = (i + 1) % stage2.size();
                
                Point3D A = stage2[i];
                Point3D B = stage2[next];
                Point3D A_top = Point3D(A.x(), A.y(), A.z() + wallHeight);
                Point3D B_top = Point3D(B.x(), B.y(), B.z() + wallHeight);
                
                // 内壁面（顺时针，因为是内侧）
                // 三角形1: A -> A_top -> B
                vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
                vertices->push_back(osg::Vec3(A_top.x(), A_top.y(), A_top.z()));
                vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
                
                // 三角形2: B -> A_top -> B_top
                vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
                vertices->push_back(osg::Vec3(A_top.x(), A_top.y(), A_top.z()));
                vertices->push_back(osg::Vec3(B_top.x(), B_top.y(), B_top.z()));
            }
            
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, vertices->size()));
        }
    }
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
} 