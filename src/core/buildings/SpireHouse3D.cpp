#include "SpireHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"
#include "../../util/VertexShapeUtils.h"

SpireHouse3D_Geo::SpireHouse3D_Geo()
{
    m_geoType = Geo_SpireHouse3D;
    // 确保基类正确初始化
    initialize();
    
    // 房屋类特定的可见性设置：只显示点和线
    GeoParameters3D params = getParameters();
    params.showPoints = true;
    params.showEdges = true;
    params.showFaces = false;

    setParameters(params);
}

void SpireHouse3D_Geo::buildVertexGeometries()
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
        // 第一阶段：确定第一个角点
        const auto& stage1 = allStagePoints[0];
        
        for (size_t i = 0; i < stage1.size(); ++i) {
            vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
        }
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：确定第二个角点
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        for (size_t i = 0; i < stage1.size(); ++i) {
            vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
        }
        for (size_t i = 0; i < stage2.size(); ++i) {
            vertices->push_back(osg::Vec3(stage2[i].x(), stage2[i].y(), stage2[i].z()));
        }
    }
    else if (allStagePoints.size() == 3) {
        // 第三阶段：确定第三个角点
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        for (size_t i = 0; i < stage1.size(); ++i) {
            vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
        }
        for (size_t i = 0; i < stage2.size(); ++i) {
            vertices->push_back(osg::Vec3(stage2[i].x(), stage2[i].y(), stage2[i].z()));
        }
        for (size_t i = 0; i < stage3.size(); ++i) {
            vertices->push_back(osg::Vec3(stage3[i].x(), stage3[i].y(), stage3[i].z()));
        }
    }
    else if (allStagePoints.size() == 4) {
        // 第四阶段：确定第四个角点，形成四边形基座
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        // 添加四个角点
        if (stage1.size() >= 1) vertices->push_back(osg::Vec3(stage1[0].x(), stage1[0].y(), stage1[0].z()));
        if (stage2.size() >= 1) vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z()));
        if (stage3.size() >= 1) vertices->push_back(osg::Vec3(stage3[0].x(), stage3[0].y(), stage3[0].z()));
        if (stage4.size() >= 1) vertices->push_back(osg::Vec3(stage4[0].x(), stage4[0].y(), stage4[0].z()));
    }
    else if (allStagePoints.size() == 5) {
        // 第五阶段：确定尖顶位置
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        const auto& stage5 = allStagePoints[4];
        
        // 添加四个角点
        if (stage1.size() >= 1) vertices->push_back(osg::Vec3(stage1[0].x(), stage1[0].y(), stage1[0].z()));
        if (stage2.size() >= 1) vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z()));
        if (stage3.size() >= 1) vertices->push_back(osg::Vec3(stage3[0].x(), stage3[0].y(), stage3[0].z()));
        if (stage4.size() >= 1) vertices->push_back(osg::Vec3(stage4[0].x(), stage4[0].y(), stage4[0].z()));
        
        // 添加尖顶点
        if (stage5.size() >= 1) vertices->push_back(osg::Vec3(stage5[0].x(), stage5[0].y(), stage5[0].z()));
    }
    else if (allStagePoints.size() >= 6) {
        // 第六阶段：确定地面，形成完整的尖顶房屋
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        const auto& stage5 = allStagePoints[4];
        const auto& stage6 = allStagePoints[5];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1 && stage5.size() >= 1 && stage6.size() >= 1) {
            Point3D A = stage1[0];  // 第一个角点
            Point3D B = stage2[0];  // 第二个角点
            Point3D C = stage3[0];  // 第三个角点
            Point3D D = stage4[0];  // 第四个角点
            Point3D spirePoint = stage5[0];  // 尖顶点
            Point3D groundPoint = stage6[0];  // 地面高度点
            
            // 计算地面高度
            double groundHeight = groundPoint.z() - A.z();
            
            // 计算房屋墙体顶面的四个顶点
            Point3D A2 = Point3D(A.x(), A.y(), A.z() + groundHeight);
            Point3D B2 = Point3D(B.x(), B.y(), B.z() + groundHeight);
            Point3D C2 = Point3D(C.x(), C.y(), C.z() + groundHeight);
            Point3D D2 = Point3D(D.x(), D.y(), D.z() + groundHeight);
            
            // 添加底面4个顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 添加墙体顶面4个顶点
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            
            // 添加尖顶点
            vertices->push_back(osg::Vec3(spirePoint.x(), spirePoint.y(), spirePoint.z()));
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

void SpireHouse3D_Geo::buildEdgeGeometries()
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
        // 第一阶段：只有一个点，无边线
        // 不绘制任何边线
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：两个点，绘制一条边
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 1 && stage2.size() >= 1) {
            vertices->push_back(osg::Vec3(stage1[0].x(), stage1[0].y(), stage1[0].z()));
            vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z()));
            
            indices->push_back(0); indices->push_back(1);
        }
    }
    else if (allStagePoints.size() == 3) {
        // 第三阶段：三个点，绘制两条边
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1) {
            vertices->push_back(osg::Vec3(stage1[0].x(), stage1[0].y(), stage1[0].z()));
            vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z()));
            vertices->push_back(osg::Vec3(stage3[0].x(), stage3[0].y(), stage3[0].z()));
            
            indices->push_back(0); indices->push_back(1);
            indices->push_back(1); indices->push_back(2);
        }
    }
    else if (allStagePoints.size() == 4) {
        // 第四阶段：四个点，形成四边形基座
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1) {
            vertices->push_back(osg::Vec3(stage1[0].x(), stage1[0].y(), stage1[0].z()));
            vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z()));
            vertices->push_back(osg::Vec3(stage3[0].x(), stage3[0].y(), stage3[0].z()));
            vertices->push_back(osg::Vec3(stage4[0].x(), stage4[0].y(), stage4[0].z()));
            
            // 连接四个点形成四边形
            indices->push_back(0); indices->push_back(1);
            indices->push_back(1); indices->push_back(2);
            indices->push_back(2); indices->push_back(3);
            indices->push_back(3); indices->push_back(0);
        }
    }
    else if (allStagePoints.size() == 5) {
        // 第五阶段：添加尖顶点并连接到基座各点
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        const auto& stage5 = allStagePoints[4];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1 && stage5.size() >= 1) {
            vertices->push_back(osg::Vec3(stage1[0].x(), stage1[0].y(), stage1[0].z()));
            vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z()));
            vertices->push_back(osg::Vec3(stage3[0].x(), stage3[0].y(), stage3[0].z()));
            vertices->push_back(osg::Vec3(stage4[0].x(), stage4[0].y(), stage4[0].z()));
            vertices->push_back(osg::Vec3(stage5[0].x(), stage5[0].y(), stage5[0].z())); // 尖顶点
            
            // 底面四边形
            indices->push_back(0); indices->push_back(1);
            indices->push_back(1); indices->push_back(2);
            indices->push_back(2); indices->push_back(3);
            indices->push_back(3); indices->push_back(0);
            
            // 尖顶到基座各点的连线
            indices->push_back(4); indices->push_back(0);
            indices->push_back(4); indices->push_back(1);
            indices->push_back(4); indices->push_back(2);
            indices->push_back(4); indices->push_back(3);
        }
    }
    else if (allStagePoints.size() >= 6) {
        // 第六阶段：确定地面，完整的尖顶房屋边线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        const auto& stage5 = allStagePoints[4];
        const auto& stage6 = allStagePoints[5];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1 && stage5.size() >= 1 && stage6.size() >= 1) {
            Point3D A = stage1[0];  // 第一个角点
            Point3D B = stage2[0];  // 第二个角点
            Point3D C = stage3[0];  // 第三个角点
            Point3D D = stage4[0];  // 第四个角点
            Point3D spirePoint = stage5[0];  // 尖顶点
            Point3D groundPoint = stage6[0];  // 地面高度点
            
            // 计算地面高度
            double groundHeight = groundPoint.z() - A.z();
            
            // 计算房屋墙体顶面的四个顶点
            Point3D A2 = Point3D(A.x(), A.y(), A.z() + groundHeight);
            Point3D B2 = Point3D(B.x(), B.y(), B.z() + groundHeight);
            Point3D C2 = Point3D(C.x(), C.y(), C.z() + groundHeight);
            Point3D D2 = Point3D(D.x(), D.y(), D.z() + groundHeight);
            
            // 添加所有顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));    // 0
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));    // 1
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));    // 2
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));    // 3
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z())); // 4
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z())); // 5
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z())); // 6
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z())); // 7
            vertices->push_back(osg::Vec3(spirePoint.x(), spirePoint.y(), spirePoint.z())); // 8
            
            // 底面四边形
            indices->push_back(0); indices->push_back(1);
            indices->push_back(1); indices->push_back(2);
            indices->push_back(2); indices->push_back(3);
            indices->push_back(3); indices->push_back(0);
            
            // 顶面四边形
            indices->push_back(4); indices->push_back(5);
            indices->push_back(5); indices->push_back(6);
            indices->push_back(6); indices->push_back(7);
            indices->push_back(7); indices->push_back(4);
            
            // 4条垂直边
            indices->push_back(0); indices->push_back(4);
            indices->push_back(1); indices->push_back(5);
            indices->push_back(2); indices->push_back(6);
            indices->push_back(3); indices->push_back(7);
            
            // 尖顶到顶面各点的连线
            indices->push_back(8); indices->push_back(4);
            indices->push_back(8); indices->push_back(5);
            indices->push_back(8); indices->push_back(6);
            indices->push_back(8); indices->push_back(7);
        }
    }
    else if (allStagePoints.size() == 3) {
        // 第三阶段：墙体边线（废弃的旧逻辑）
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D heightPoint = stage3[0];
            
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
            double wallHeight = heightPoint.z() - A.z();
            
            Point3D A2 = Point3D(A.x(), A.y(), A.z() + wallHeight);
            Point3D B2 = Point3D(B.x(), B.y(), B.z() + wallHeight);
            Point3D C2 = Point3D(C.x(), C.y(), C.z() + wallHeight);
            Point3D D2 = Point3D(D.x(), D.y(), D.z() + wallHeight);
            
            // 添加8个顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            
            // 底面4条边
            indices->push_back(0); indices->push_back(1);
            indices->push_back(1); indices->push_back(2);
            indices->push_back(2); indices->push_back(3);
            indices->push_back(3); indices->push_back(0);
            
            // 顶面4条边
            indices->push_back(4); indices->push_back(5);
            indices->push_back(5); indices->push_back(6);
            indices->push_back(6); indices->push_back(7);
            indices->push_back(7); indices->push_back(4);
            
            // 4条垂直边
            indices->push_back(0); indices->push_back(4);
            indices->push_back(1); indices->push_back(5);
            indices->push_back(2); indices->push_back(6);
            indices->push_back(3); indices->push_back(7);
        }
    }
    else if (allStagePoints.size() >= 4) {
        // 第四阶段：完整尖塔房屋的边线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D heightPoint = stage3[0];
            Point3D spirePoint = stage4[0];
            
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
            double wallHeight = heightPoint.z() - A.z();
            
            Point3D A2 = Point3D(A.x(), A.y(), A.z() + wallHeight);
            Point3D B2 = Point3D(B.x(), B.y(), B.z() + wallHeight);
            Point3D C2 = Point3D(C.x(), C.y(), C.z() + wallHeight);
            Point3D D2 = Point3D(D.x(), D.y(), D.z() + wallHeight);
            
            // 添加9个顶点（8个墙体顶点 + 1个尖顶点）
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));   // 0
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));   // 1
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));   // 2
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));   // 3
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z())); // 4
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z())); // 5
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z())); // 6
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z())); // 7
            vertices->push_back(osg::Vec3(spirePoint.x(), spirePoint.y(), spirePoint.z())); // 8
            
            // 底面4条边
            indices->push_back(0); indices->push_back(1);
            indices->push_back(1); indices->push_back(2);
            indices->push_back(2); indices->push_back(3);
            indices->push_back(3); indices->push_back(0);
            
            // 墙体顶面4条边
            indices->push_back(4); indices->push_back(5);
            indices->push_back(5); indices->push_back(6);
            indices->push_back(6); indices->push_back(7);
            indices->push_back(7); indices->push_back(4);
            
            // 4条垂直边
            indices->push_back(0); indices->push_back(4);
            indices->push_back(1); indices->push_back(5);
            indices->push_back(2); indices->push_back(6);
            indices->push_back(3); indices->push_back(7);
            
            // 尖顶到墙体顶面4个角的连线
            indices->push_back(8); indices->push_back(4);
            indices->push_back(8); indices->push_back(5);
            indices->push_back(8); indices->push_back(6);
            indices->push_back(8); indices->push_back(7);
        }
    }
    
    // 设置顶点数组和索引
    geometry->setVertexArray(vertices);
    if (indices->size() > 0) {
        geometry->addPrimitiveSet(indices);
    }
}

void SpireHouse3D_Geo::buildFaceGeometries()
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
    
    if (allStagePoints.size() <= 3) {
        // 第一至三阶段：无面
        // 不绘制任何面
    }
    else if (allStagePoints.size() == 4) {
        // 第四阶段：显示底面
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1) {
            Point3D A = stage1[0];
            Point3D B = stage2[0];
            Point3D C = stage3[0];
            Point3D D = stage4[0];
            
            // 添加底面四边形顶点（分解为两个三角形）
            // 第一个三角形：A, B, C
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            
            // 第二个三角形：A, C, D
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 添加底面三角形
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 6));
        }
    }
    else if (allStagePoints.size() == 5) {
        // 第五阶段：显示底面和尖顶锥形
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        const auto& stage5 = allStagePoints[4];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1 && stage5.size() >= 1) {
            Point3D A = stage1[0];
            Point3D B = stage2[0];
            Point3D C = stage3[0];
            Point3D D = stage4[0];
            Point3D spirePoint = stage5[0];
            
            // 底面四边形（分解为两个三角形）
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 尖顶锥形四个侧面（4个三角形）
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(spirePoint.x(), spirePoint.y(), spirePoint.z()));
            
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(spirePoint.x(), spirePoint.y(), spirePoint.z()));
            
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(spirePoint.x(), spirePoint.y(), spirePoint.z()));
            
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(spirePoint.x(), spirePoint.y(), spirePoint.z()));
            
            // 添加所有三角形（底面2个 + 侧面4个 = 6个三角形）
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 18));
        }
    }
    else if (allStagePoints.size() >= 6) {
        // 第六阶段：完整的尖顶房屋，添加墙体
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        const auto& stage5 = allStagePoints[4];
        const auto& stage6 = allStagePoints[5];
        
        if (stage1.size() >= 1 && stage2.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
            // 添加底面四边形顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 添加底面（分解为两个三角形）
            // 第一个三角形：0, 1, 2
            // 第二个三角形：0, 2, 3
            // 需要调整顶点顺序以符合三角形
            osg::ref_ptr<osg::Vec3Array> triangleVertices = new osg::Vec3Array();
            triangleVertices->push_back((*vertices)[0]); // A
            triangleVertices->push_back((*vertices)[1]); // B  
            triangleVertices->push_back((*vertices)[2]); // C
            triangleVertices->push_back((*vertices)[0]); // A
            triangleVertices->push_back((*vertices)[2]); // C
            triangleVertices->push_back((*vertices)[3]); // D
            
            vertices->clear();
            for (int i = 0; i < 6; i++) {
                vertices->push_back((*triangleVertices)[i]);
            }
            
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 6));
        }
    }
    else if (allStagePoints.size() == 3) {
        // 第三阶段：墙体（6个面）
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D heightPoint = stage3[0];
            
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
            double wallHeight = heightPoint.z() - A.z();
            
            Point3D A2 = Point3D(A.x(), A.y(), A.z() + wallHeight);
            Point3D B2 = Point3D(B.x(), B.y(), B.z() + wallHeight);
            Point3D C2 = Point3D(C.x(), C.y(), C.z() + wallHeight);
            Point3D D2 = Point3D(D.x(), D.y(), D.z() + wallHeight);
            
            // 底面 (A, B, C, D)
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 顶面 (A2, D2, C2, B2) - 反向顺序保证法向量正确
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            
            // 前面 (A, B, B2, A2)
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            
            // 右面 (B, C, C2, B2)
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            
            // 后面 (C, D, D2, C2)
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            
            // 左面 (D, A, A2, D2)
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            
            // 添加所有面（分解为三角形）
            // 总共6个面，每个面4个顶点，需要转换为每个面6个顶点（2个三角形）
            osg::ref_ptr<osg::Vec3Array> triangleVertices = new osg::Vec3Array();
            
            for (int i = 0; i < 6; i++) {
                int base = i * 4;
                // 第一个三角形：base+0, base+1, base+2
                triangleVertices->push_back((*vertices)[base + 0]);
                triangleVertices->push_back((*vertices)[base + 1]);
                triangleVertices->push_back((*vertices)[base + 2]);
                
                // 第二个三角形：base+0, base+2, base+3
                triangleVertices->push_back((*vertices)[base + 0]);
                triangleVertices->push_back((*vertices)[base + 2]);
                triangleVertices->push_back((*vertices)[base + 3]);
            }
            
            vertices->clear();
            for (int i = 0; i < triangleVertices->size(); i++) {
                vertices->push_back((*triangleVertices)[i]);
            }
            
            // 添加所有三角形面片（6个面 × 2个三角形 = 12个三角形）
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 36));
        }
    }
    else if (allStagePoints.size() >= 4) {
        // 第四阶段：完整的尖塔房屋（墙体 + 锥形尖顶）
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D heightPoint = stage3[0];
            Point3D spirePoint = stage4[0];
            
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
            double wallHeight = heightPoint.z() - A.z();
            
            Point3D A2 = Point3D(A.x(), A.y(), A.z() + wallHeight);
            Point3D B2 = Point3D(B.x(), B.y(), B.z() + wallHeight);
            Point3D C2 = Point3D(C.x(), C.y(), C.z() + wallHeight);
            Point3D D2 = Point3D(D.x(), D.y(), D.z() + wallHeight);
            
            // 底面 (A, B, C, D)
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 前面 (A, B, B2, A2)
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            
            // 右面 (B, C, C2, B2)
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            
            // 后面 (C, D, D2, C2)
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            
            // 左面 (D, A, A2, D2)
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            
            // 尖顶4个三角形面
            // 前面三角形 (A2, B2, spirePoint)
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            vertices->push_back(osg::Vec3(spirePoint.x(), spirePoint.y(), spirePoint.z()));
            
            // 右面三角形 (B2, C2, spirePoint)
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            vertices->push_back(osg::Vec3(spirePoint.x(), spirePoint.y(), spirePoint.z()));
            
            // 后面三角形 (C2, D2, spirePoint)
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            vertices->push_back(osg::Vec3(spirePoint.x(), spirePoint.y(), spirePoint.z()));
            
            // 左面三角形 (D2, A2, spirePoint)
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            vertices->push_back(osg::Vec3(spirePoint.x(), spirePoint.y(), spirePoint.z()));
            
            // 添加墙体面（分解四边形为三角形）
            // 需要将前面的1个底面+4个墙面四边形转换为三角形
            osg::ref_ptr<osg::Vec3Array> wallTriangles = new osg::Vec3Array();
            
            // 转换5个四边形面为三角形（每个四边形 = 2个三角形）
            for (int i = 0; i < 5; i++) {
                int base = i * 4;
                // 第一个三角形：base+0, base+1, base+2
                wallTriangles->push_back((*vertices)[base + 0]);
                wallTriangles->push_back((*vertices)[base + 1]);
                wallTriangles->push_back((*vertices)[base + 2]);
                
                // 第二个三角形：base+0, base+2, base+3
                wallTriangles->push_back((*vertices)[base + 0]);
                wallTriangles->push_back((*vertices)[base + 2]);
                wallTriangles->push_back((*vertices)[base + 3]);
            }
            
            // 复制尖顶三角形（保持原有的4个三角形）
            for (int i = 20; i < 32; i++) { // 4个三角形 × 3个顶点 = 12个顶点
                wallTriangles->push_back((*vertices)[i]);
            }
            
            vertices->clear();
            for (int i = 0; i < wallTriangles->size(); i++) {
                vertices->push_back((*wallTriangles)[i]);
            }
            
            // 添加墙体三角形面片（5个面 × 2个三角形 = 10个三角形）
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 30));
            
            // 添加尖顶面（4个三角形）
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 30, 12));
        }
    }
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
} 

