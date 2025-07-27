#include "LHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"
#include "../../util/VertexShapeUtils.h"

LHouse3D_Geo::LHouse3D_Geo()
{
    m_geoType = Geo_LHouse3D;
    // 确保基类正确初始化
    initialize();
    
    // 房屋类特定的可见性设置：只显示点和线
    GeoParameters3D params = getParameters();
    params.showPoints = true;
    params.showEdges = true;
    params.showFaces = false;
    
    // 更新渲染参数
    if (mm_render()) {
        mm_render()->updateRenderingParameters(params);
    }
    // 同步参数到基类
    setParameters(params);
}

void LHouse3D_Geo::buildVertexGeometries()
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
        // 第一阶段：主体基座第一角点
        const auto& stage1 = allStagePoints[0];
        
        for (size_t i = 0; i < stage1.size(); ++i) {
            vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
        }
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：主体基座对角点，形成主体矩形
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 1 && stage2.size() >= 1) {
            Point3D A = stage1[0];  // 主体第一个角点
            Point3D C = stage2[0];  // 主体对角点
            
            // 计算主体矩形的其他两个顶点
            Point3D B = Point3D(C.x(), A.y(), A.z());  
            Point3D D = Point3D(A.x(), C.y(), A.z());  
            
            // 添加主体矩形4个顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
        }
    }
    else if (allStagePoints.size() == 3) {
        // 第三阶段：确定扩展部分位置，形成L型基座
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D E = stage3[0];  // 扩展部分的角点
            
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
            // 根据扩展点的位置确定扩展方向
            // 简化实现：假设扩展部分是从主体矩形的一边延伸出去的矩形
            Point3D F, G, H;
            
            // 判断扩展方向（基于扩展点相对于主体矩形的位置）
            if (E.x() > C.x()) {
                // 向右扩展
                F = Point3D(E.x(), C.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(C.x(), E.y(), E.z());
            }
            else if (E.x() < A.x()) {
                // 向左扩展
                F = Point3D(E.x(), A.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(A.x(), E.y(), E.z());
            }
            else if (E.y() > C.y()) {
                // 向上扩展
                F = Point3D(D.x(), E.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(E.x(), C.y(), E.z());
            }
            else {
                // 向下扩展
                F = Point3D(A.x(), E.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(E.x(), A.y(), E.z());
            }
            
            // 添加主体矩形4个顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 添加扩展部分顶点
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));
            vertices->push_back(osg::Vec3(G.x(), G.y(), G.z()));
            vertices->push_back(osg::Vec3(H.x(), H.y(), H.z()));
        }
    }
    else if (allStagePoints.size() >= 4) {
        // 第四阶段：确定房屋高度，形成完整的L型房屋
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D E = stage3[0];
            Point3D heightPoint = stage4[0];
            
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
            // 计算扩展部分的顶点
            Point3D F, G, H;
            if (E.x() > C.x()) {
                F = Point3D(E.x(), C.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(C.x(), E.y(), E.z());
            }
            else if (E.x() < A.x()) {
                F = Point3D(E.x(), A.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(A.x(), E.y(), E.z());
            }
            else if (E.y() > C.y()) {
                F = Point3D(D.x(), E.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(E.x(), C.y(), E.z());
            }
            else {
                F = Point3D(A.x(), E.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(E.x(), A.y(), E.z());
            }
            
            double height = heightPoint.z() - A.z();
            
            // 计算顶层顶点
            Point3D A2 = Point3D(A.x(), A.y(), A.z() + height);
            Point3D B2 = Point3D(B.x(), B.y(), B.z() + height);
            Point3D C2 = Point3D(C.x(), C.y(), C.z() + height);
            Point3D D2 = Point3D(D.x(), D.y(), D.z() + height);
            Point3D E2 = Point3D(E.x(), E.y(), E.z() + height);
            Point3D F2 = Point3D(F.x(), F.y(), F.z() + height);
            Point3D G2 = Point3D(G.x(), G.y(), G.z() + height);
            Point3D H2 = Point3D(H.x(), H.y(), H.z() + height);
            
            // 添加底层顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));
            vertices->push_back(osg::Vec3(G.x(), G.y(), G.z()));
            vertices->push_back(osg::Vec3(H.x(), H.y(), H.z()));
            
            // 添加顶层顶点
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            vertices->push_back(osg::Vec3(E2.x(), E2.y(), E2.z()));
            vertices->push_back(osg::Vec3(F2.x(), F2.y(), F2.z()));
            vertices->push_back(osg::Vec3(G2.x(), G2.y(), G2.z()));
            vertices->push_back(osg::Vec3(H2.x(), H2.y(), H2.z()));
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

void LHouse3D_Geo::buildEdgeGeometries()
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
        // 第二阶段：主体矩形的边
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
    else if (allStagePoints.size() == 3) {
        // 第三阶段：L型基座的边线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D E = stage3[0];
            
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
            Point3D F, G, H;
            if (E.x() > C.x()) {
                F = Point3D(E.x(), C.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(C.x(), E.y(), E.z());
            }
            else if (E.x() < A.x()) {
                F = Point3D(E.x(), A.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(A.x(), E.y(), E.z());
            }
            else if (E.y() > C.y()) {
                F = Point3D(D.x(), E.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(E.x(), C.y(), E.z());
            }
            else {
                F = Point3D(A.x(), E.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(E.x(), A.y(), E.z());
            }
            
            // 添加所有顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));  // 0
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));  // 1
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));  // 2
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));  // 3
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));  // 4
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));  // 5
            vertices->push_back(osg::Vec3(G.x(), G.y(), G.z()));  // 6
            vertices->push_back(osg::Vec3(H.x(), H.y(), H.z()));  // 7
            
            // 主体矩形的边
            indices->push_back(0); indices->push_back(1); // A-B
            indices->push_back(1); indices->push_back(2); // B-C
            indices->push_back(2); indices->push_back(3); // C-D
            indices->push_back(3); indices->push_back(0); // D-A
            
            // 扩展部分的边
            indices->push_back(4); indices->push_back(5); // E-F
            indices->push_back(5); indices->push_back(6); // F-G
            indices->push_back(6); indices->push_back(7); // G-H
            indices->push_back(7); indices->push_back(4); // H-E
            
            // 连接主体和扩展部分的边（根据扩展方向添加）
            if (E.x() > C.x()) {
                indices->push_back(2); indices->push_back(5); // C-F
                indices->push_back(2); indices->push_back(7); // C-H
            }
            else if (E.x() < A.x()) {
                indices->push_back(0); indices->push_back(5); // A-F
                indices->push_back(3); indices->push_back(7); // D-H
            }
            else if (E.y() > C.y()) {
                indices->push_back(3); indices->push_back(5); // D-F
                indices->push_back(2); indices->push_back(7); // C-H
            }
            else {
                indices->push_back(0); indices->push_back(5); // A-F
                indices->push_back(1); indices->push_back(7); // B-H
            }
        }
    }
    else if (allStagePoints.size() >= 4) {
        // 第四阶段：完整L型房屋的边线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D E = stage3[0];
            Point3D heightPoint = stage4[0];
            
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
            Point3D F, G, H;
            if (E.x() > C.x()) {
                F = Point3D(E.x(), C.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(C.x(), E.y(), E.z());
            }
            else if (E.x() < A.x()) {
                F = Point3D(E.x(), A.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(A.x(), E.y(), E.z());
            }
            else if (E.y() > C.y()) {
                F = Point3D(D.x(), E.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(E.x(), C.y(), E.z());
            }
            else {
                F = Point3D(A.x(), E.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(E.x(), A.y(), E.z());
            }
            
            double height = heightPoint.z() - A.z();
            
            Point3D A2 = Point3D(A.x(), A.y(), A.z() + height);
            Point3D B2 = Point3D(B.x(), B.y(), B.z() + height);
            Point3D C2 = Point3D(C.x(), C.y(), C.z() + height);
            Point3D D2 = Point3D(D.x(), D.y(), D.z() + height);
            Point3D E2 = Point3D(E.x(), E.y(), E.z() + height);
            Point3D F2 = Point3D(F.x(), F.y(), F.z() + height);
            Point3D G2 = Point3D(G.x(), G.y(), G.z() + height);
            Point3D H2 = Point3D(H.x(), H.y(), H.z() + height);
            
            // 添加16个顶点（底层8个 + 顶层8个）
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));   // 0
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));   // 1
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));   // 2
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));   // 3
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));   // 4
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));   // 5
            vertices->push_back(osg::Vec3(G.x(), G.y(), G.z()));   // 6
            vertices->push_back(osg::Vec3(H.x(), H.y(), H.z()));   // 7
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z())); // 8
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z())); // 9
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z())); // 10
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z())); // 11
            vertices->push_back(osg::Vec3(E2.x(), E2.y(), E2.z())); // 12
            vertices->push_back(osg::Vec3(F2.x(), F2.y(), F2.z())); // 13
            vertices->push_back(osg::Vec3(G2.x(), G2.y(), G2.z())); // 14
            vertices->push_back(osg::Vec3(H2.x(), H2.y(), H2.z())); // 15
            
            // 底层边线
            indices->push_back(0); indices->push_back(1);
            indices->push_back(1); indices->push_back(2);
            indices->push_back(2); indices->push_back(3);
            indices->push_back(3); indices->push_back(0);
            indices->push_back(4); indices->push_back(5);
            indices->push_back(5); indices->push_back(6);
            indices->push_back(6); indices->push_back(7);
            indices->push_back(7); indices->push_back(4);
            
            // 顶层边线
            indices->push_back(8); indices->push_back(9);
            indices->push_back(9); indices->push_back(10);
            indices->push_back(10); indices->push_back(11);
            indices->push_back(11); indices->push_back(8);
            indices->push_back(12); indices->push_back(13);
            indices->push_back(13); indices->push_back(14);
            indices->push_back(14); indices->push_back(15);
            indices->push_back(15); indices->push_back(12);
            
            // 垂直边线
            for (int i = 0; i < 8; ++i) {
                indices->push_back(i); indices->push_back(i + 8);
            }
            
            // 连接边线
            if (E.x() > C.x()) {
                indices->push_back(2); indices->push_back(5);
                indices->push_back(2); indices->push_back(7);
                indices->push_back(10); indices->push_back(13);
                indices->push_back(10); indices->push_back(15);
            }
            else if (E.x() < A.x()) {
                indices->push_back(0); indices->push_back(5);
                indices->push_back(3); indices->push_back(7);
                indices->push_back(8); indices->push_back(13);
                indices->push_back(11); indices->push_back(15);
            }
            else if (E.y() > C.y()) {
                indices->push_back(3); indices->push_back(5);
                indices->push_back(2); indices->push_back(7);
                indices->push_back(11); indices->push_back(13);
                indices->push_back(10); indices->push_back(15);
            }
            else {
                indices->push_back(0); indices->push_back(5);
                indices->push_back(1); indices->push_back(7);
                indices->push_back(8); indices->push_back(13);
                indices->push_back(9); indices->push_back(15);
            }
        }
    }
    
    // 设置顶点数组和索引
    geometry->setVertexArray(vertices);
    if (indices->size() > 0) {
        geometry->addPrimitiveSet(indices);
    }
}

void LHouse3D_Geo::buildFaceGeometries()
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
        // 第二阶段：只显示主体底面
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 1 && stage2.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
            // 添加主体底面四边形顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 添加底面
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));
        }
    }
    else if (allStagePoints.size() == 3) {
        // 第三阶段：L型底面
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D E = stage3[0];
            
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
            Point3D F, G, H;
            if (E.x() > C.x()) {
                F = Point3D(E.x(), C.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(C.x(), E.y(), E.z());
            }
            else if (E.x() < A.x()) {
                F = Point3D(E.x(), A.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(A.x(), E.y(), E.z());
            }
            else if (E.y() > C.y()) {
                F = Point3D(D.x(), E.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(E.x(), C.y(), E.z());
            }
            else {
                F = Point3D(A.x(), E.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(E.x(), A.y(), E.z());
            }
            
            // 主体矩形
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 扩展矩形
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));
            vertices->push_back(osg::Vec3(G.x(), G.y(), G.z()));
            vertices->push_back(osg::Vec3(H.x(), H.y(), H.z()));
            
            // 添加两个矩形面
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4)); // 主体
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 4, 4)); // 扩展
        }
    }
    else if (allStagePoints.size() >= 4) {
        // 第四阶段：完整的L型房屋
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1) {
            Point3D A = stage1[0];
            Point3D C = stage2[0];
            Point3D E = stage3[0];
            Point3D heightPoint = stage4[0];
            
            Point3D B = Point3D(C.x(), A.y(), A.z());
            Point3D D = Point3D(A.x(), C.y(), A.z());
            
            Point3D F, G, H;
            if (E.x() > C.x()) {
                F = Point3D(E.x(), C.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(C.x(), E.y(), E.z());
            }
            else if (E.x() < A.x()) {
                F = Point3D(E.x(), A.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(A.x(), E.y(), E.z());
            }
            else if (E.y() > C.y()) {
                F = Point3D(D.x(), E.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(E.x(), C.y(), E.z());
            }
            else {
                F = Point3D(A.x(), E.y(), E.z());
                G = Point3D(E.x(), E.y(), E.z());
                H = Point3D(E.x(), A.y(), E.z());
            }
            
            double height = heightPoint.z() - A.z();
            
            Point3D A2 = Point3D(A.x(), A.y(), A.z() + height);
            Point3D B2 = Point3D(B.x(), B.y(), B.z() + height);
            Point3D C2 = Point3D(C.x(), C.y(), C.z() + height);
            Point3D D2 = Point3D(D.x(), D.y(), D.z() + height);
            Point3D E2 = Point3D(E.x(), E.y(), E.z() + height);
            Point3D F2 = Point3D(F.x(), F.y(), F.z() + height);
            Point3D G2 = Point3D(G.x(), G.y(), G.z() + height);
            Point3D H2 = Point3D(H.x(), H.y(), H.z() + height);
            
            // 构建L型房屋的所有面（这是一个复杂的几何体，需要仔细构建面）
            // 简化处理：分别构建主体部分和扩展部分的6个面，然后处理连接处
            
            // 主体底面
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 主体顶面
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            
            // 扩展部分底面
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));
            vertices->push_back(osg::Vec3(G.x(), G.y(), G.z()));
            vertices->push_back(osg::Vec3(H.x(), H.y(), H.z()));
            
            // 扩展部分顶面
            vertices->push_back(osg::Vec3(E2.x(), E2.y(), E2.z()));
            vertices->push_back(osg::Vec3(H2.x(), H2.y(), H2.z()));
            vertices->push_back(osg::Vec3(G2.x(), G2.y(), G2.z()));
            vertices->push_back(osg::Vec3(F2.x(), F2.y(), F2.z()));
            
            // 添加基本面
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));   // 主体底面
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 4, 4));   // 主体顶面
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 8, 4));   // 扩展底面
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 12, 4));  // 扩展顶面
            
            // 添加更多的墙面（这里简化处理，实际上L型房屋的墙面构建比较复杂）
            // 可以根据需要添加更多的墙面
        }
    }
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
} 

