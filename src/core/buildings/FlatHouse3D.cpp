#include "FlatHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"
#include "../../util/VertexShapeUtils.h"

FlatHouse3D_Geo::FlatHouse3D_Geo()
{
    m_geoType = Geo_FlatHouse3D;
    // 确保基类正确初始化
    initialize();
    
    // 房屋类特定的可见性设置：只显示点和线
    GeoParameters3D params = getParameters();
    params.showPoints = true;
    params.showEdges = true;
    params.showFaces = false;
    
    setParameters(params);
}

void FlatHouse3D_Geo::buildVertexGeometries()
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
    else if (allStagePoints.size() >= 5) {
        // 第五阶段：确定地面高度，形成完整的平顶房屋
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        const auto& stage5 = allStagePoints[4];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1 && stage5.size() >= 1) {
            Point3D A = stage1[0];  // 第一个角点
            Point3D B = stage2[0];  // 第二个角点
            Point3D C = stage3[0];  // 第三个角点
            Point3D D = stage4[0];  // 第四个角点
            Point3D heightPoint = stage5[0];  // 地面高度点
            
            // 计算高度
            double height = heightPoint.z() - A.z();
            
            // 计算顶面的四个顶点
            Point3D A2 = Point3D(A.x(), A.y(), A.z() + height);
            Point3D B2 = Point3D(B.x(), B.y(), B.z() + height);
            Point3D C2 = Point3D(C.x(), C.y(), C.z() + height);
            Point3D D2 = Point3D(D.x(), D.y(), D.z() + height);
            
            // 添加底面4个顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 添加顶面4个顶点
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
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

void FlatHouse3D_Geo::buildEdgeGeometries()
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
    else if (allStagePoints.size() >= 5) {
        // 第五阶段：完整平顶房屋的边线
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
            Point3D heightPoint = stage5[0];
            
            double height = heightPoint.z() - A.z();
            
            Point3D A2 = Point3D(A.x(), A.y(), A.z() + height);
            Point3D B2 = Point3D(B.x(), B.y(), B.z() + height);
            Point3D C2 = Point3D(C.x(), C.y(), C.z() + height);
            Point3D D2 = Point3D(D.x(), D.y(), D.z() + height);
            
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
    
    // 设置顶点数组和索引
    geometry->setVertexArray(vertices);
    if (indices->size() > 0) {
        geometry->addPrimitiveSet(indices);
    }
}

void FlatHouse3D_Geo::buildFaceGeometries()
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
        // 第二阶段：无面
        // 不绘制任何面
    }
    else if (allStagePoints.size() == 3) {
        // 第三阶段：无面
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
    else if (allStagePoints.size() >= 5) {
        // 第五阶段：完整的平顶房屋（6个面）
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
            Point3D heightPoint = stage5[0];
            
            double height = heightPoint.z() - A.z();
            
            Point3D A2 = Point3D(A.x(), A.y(), A.z() + height);
            Point3D B2 = Point3D(B.x(), B.y(), B.z() + height);
            Point3D C2 = Point3D(C.x(), C.y(), C.z() + height);
            Point3D D2 = Point3D(D.x(), D.y(), D.z() + height);
            
            // 底面 (A, B, C, D) - 分解为两个三角形
            // 三角形1: A, B, C
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            // 三角形2: A, C, D
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 顶面 (A2, D2, C2, B2) - 分解为两个三角形
            // 三角形1: A2, D2, C2
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            // 三角形2: A2, C2, B2
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            
            // 前面 (A, B, B2, A2) - 分解为两个三角形
            // 三角形1: A, B, B2
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            // 三角形2: A, B2, A2
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            
            // 右面 (B, C, C2, B2) - 分解为两个三角形
            // 三角形1: B, C, C2
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            // 三角形2: B, C2, B2
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            
            // 后面 (C, D, D2, C2) - 分解为两个三角形
            // 三角形1: C, D, D2
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            // 三角形2: C, D2, C2
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            
            // 左面 (D, A, A2, D2) - 分解为两个三角形
            // 三角形1: D, A, A2
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            // 三角形2: D, A2, D2
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            
            // 添加所有三角形面片（6个面 × 2个三角形 = 12个三角形）
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 36));
        }
    }
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
} 

