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
        // 第一阶段：基座第一角点
        const auto& stage1 = allStagePoints[0];
        
        for (size_t i = 0; i < stage1.size(); ++i) {
            vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
        }
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：基座对角点，形成矩形基座
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 1 && stage2.size() >= 1) {
            Point3D A = stage1[0];  // 第一个角点
            Point3D C = stage2[0];  // 对角点
            
            // 计算矩形的其他两个顶点
            Point3D B = Point3D(C.x(), A.y(), A.z());  // 保持y坐标与A相同
            Point3D D = Point3D(A.x(), C.y(), A.z());  // 保持x坐标与A相同
            
            // 添加底面4个顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
        }
    }
    else if (allStagePoints.size() >= 3) {
        // 第三阶段：确定房屋高度，形成完整的平房
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D heightPoint = stage3[0];
            
            // 计算矩形底面的四个顶点
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
            // 计算高度向量
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
        // 第二阶段：基座矩形的边
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 1 && stage2.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
            // 添加4个顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 添加4条边的索引
            indices->push_back(0); indices->push_back(1); // A-B
            indices->push_back(1); indices->push_back(2); // B-C
            indices->push_back(2); indices->push_back(3); // C-D
            indices->push_back(3); indices->push_back(0); // D-A
        }
    }
    else if (allStagePoints.size() >= 3) {
        // 第三阶段：完整平房的边线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D heightPoint = stage3[0];
            
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
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
        // 第二阶段：只显示底面
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
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
            
            // 添加底面
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));
        }
    }
    else if (allStagePoints.size() >= 3) {
        // 第三阶段：完整的平房（6个面）
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D heightPoint = stage3[0];
            
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
            double height = heightPoint.z() - A.z();
            
            Point3D A2 = Point3D(A.x(), A.y(), A.z() + height);
            Point3D B2 = Point3D(B.x(), B.y(), B.z() + height);
            Point3D C2 = Point3D(C.x(), C.y(), C.z() + height);
            Point3D D2 = Point3D(D.x(), D.y(), D.z() + height);
            
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
            
            // 添加6个四边形面
            for (int i = 0; i < 6; i++) {
                geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, i * 4, 4));
            }
        }
    }
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
} 

