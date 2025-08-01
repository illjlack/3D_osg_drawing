#include "GableHouse3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"
#include "../../util/VertexShapeUtils.h"

GableHouse3D_Geo::GableHouse3D_Geo()
{
    m_geoType = Geo_GableHouse3D;
    // 确保基类正确初始化
    initialize();
    
    // 房屋类特定的可见性设置：显示点、线和面
    GeoParameters3D params = getParameters();
    params.showPoints = true;
    params.showEdges = true;
    params.showFaces = false;
    
    setParameters(params);
}

void GableHouse3D_Geo::buildVertexGeometries()
{
    // 清理顶点几何体
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
    
    if (allStagePoints.size() == 1) {
        // 第一阶段：底面4个点
        const auto& stage1 = allStagePoints[0];
        
        for (size_t i = 0; i < stage1.size() && i < 4; ++i) {
            vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
        }
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：底面4个点 + 屋脊第一个点
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        // 添加底面4个点
        for (size_t i = 0; i < stage1.size() && i < 4; ++i) {
            vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
        }
        
        // 添加屋脊第一个点
        if (stage2.size() >= 1) {
            vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z()));
        }
    }
    else if (allStagePoints.size() == 3) {
        // 第三阶段：底面4个点 + 屋脊2个点
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        // 添加底面4个点
        for (size_t i = 0; i < stage1.size() && i < 4; ++i) {
            vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
        }
        
        // 添加屋脊2个点
        if (stage2.size() >= 1) {
            vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z()));
        }
        if (stage3.size() >= 1) {
            vertices->push_back(osg::Vec3(stage3[0].x(), stage3[0].y(), stage3[0].z()));
        }
    }
    else if (allStagePoints.size() >= 4) {
        // 第四阶段：确定地面，完整的人字形房屋
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        // 添加底面4个点
        for (size_t i = 0; i < stage1.size() && i < 4; ++i) {
            vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
        }
        
        // 添加屋脊2个点
        if (stage2.size() >= 1) {
            vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z()));
        }
        if (stage3.size() >= 1) {
            vertices->push_back(osg::Vec3(stage3[0].x(), stage3[0].y(), stage3[0].z()));
        }
        
        // 第四阶段新增：添加地面4个顶点
        if (stage4.size() >= 4) {
            // 如果stage4有4个点，直接添加
            for (size_t i = 0; i < 4; ++i) {
                vertices->push_back(osg::Vec3(stage4[i].x(), stage4[i].y(), stage4[i].z()));
            }
        } else if (stage4.size() >= 1) {
            // 如果stage4只有1个点，基于这个点生成地面4个角点
            Point3D centerGround = stage4[0];
            
            // 基于底面4个点生成地面4个点
            Point3D A = stage1[0], B = stage1[1], C = stage1[2], D = stage1[3];
            
            Point3D G = Point3D(A.x(), A.y(), centerGround.z()); // 地面前左
            Point3D H = Point3D(B.x(), B.y(), centerGround.z()); // 地面前右  
            Point3D I = Point3D(C.x(), C.y(), centerGround.z()); // 地面后右
            Point3D J = Point3D(D.x(), D.y(), centerGround.z()); // 地面后左
            
            vertices->push_back(osg::Vec3(G.x(), G.y(), G.z())); 
            vertices->push_back(osg::Vec3(H.x(), H.y(), H.z())); 
            vertices->push_back(osg::Vec3(I.x(), I.y(), I.z())); 
            vertices->push_back(osg::Vec3(J.x(), J.y(), J.z()));
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

void GableHouse3D_Geo::buildEdgeGeometries()
{
    // 清理边几何体
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
    
    if (allStagePoints.size() == 1) {
        // 第一阶段：底面矩形的边
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 4) {
            // 添加底面4个顶点
            for (size_t i = 0; i < 4; ++i) {
                vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
            }
            
            // 添加底面4条边的索引
            for (int i = 0; i < 4; ++i) {
                indices->push_back(i);
                indices->push_back((i + 1) % 4);
            }
        }
        else if (stage1.size() >= 2) {
            // 少于4个点时，只画已有的线段
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
        // 第二阶段：底面边 + 屋脊第一个点到底面4个点的连线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 4 && stage2.size() >= 1) {
            // 添加底面4个顶点
            for (size_t i = 0; i < 4; ++i) {
                vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
            }
            
            // 添加屋脊第一个点
            vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z()));
            
            // 添加底面4条边
            for (int i = 0; i < 4; ++i) {
                indices->push_back(i);
                indices->push_back((i + 1) % 4);
            }
            
            // 添加从屋脊点到底面4个顶点的连线（形成四面体的边）
            for (int i = 0; i < 4; ++i) {
                indices->push_back(4); // 屋脊点索引
                indices->push_back(i); // 底面顶点索引
            }
        }
    }
    else if (allStagePoints.size() == 3) {
        // 第三阶段：完整的人字形房屋边线（从第二阶段演变：C,D改连屋脊2）
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 4 && stage2.size() >= 1 && stage3.size() >= 1) {
            // 添加底面4个顶点：A(0)前左, B(1)前右, C(2)后右, D(3)后左
            for (size_t i = 0; i < 4; ++i) {
                vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
            }
            
            // 添加屋脊2个点：E(4)屋脊1, F(5)屋脊2  
            vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z())); // E索引4(A,B连)
            vertices->push_back(osg::Vec3(stage3[0].x(), stage3[0].y(), stage3[0].z())); // F索引5(C,D连)
            
            // 人字形屋顶的主要边线：
            // 1-4. 底面4条边
            indices->push_back(0); indices->push_back(1); // AB (前边)
            indices->push_back(1); indices->push_back(2); // BC (右边)
            indices->push_back(2); indices->push_back(3); // CD (后边)
            indices->push_back(3); indices->push_back(0); // DA (左边)
            
            // 5. 屋脊线
            indices->push_back(4); indices->push_back(5); // EF (屋脊线)
            
            // 6-9. AB连接E，CD连接F的脊线
            indices->push_back(0); indices->push_back(4); // A-E (A连屋脊1)
            indices->push_back(1); indices->push_back(4); // B-E (B连屋脊1)
            indices->push_back(2); indices->push_back(5); // C-F (C连屋脊2)
            indices->push_back(3); indices->push_back(5); // D-F (D连屋脊2)
        }
    }
    else if (allStagePoints.size() >= 4) {
        // 第四阶段：确定地面，完整的人字形房屋边线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        if (stage1.size() >= 4 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1) {
            // 添加底面4个顶点：A(0)前左, B(1)前右, C(2)后右, D(3)后左
            for (size_t i = 0; i < 4; ++i) {
                vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
            }
            
            // 添加屋脊2个点：E(4)屋脊1, F(5)屋脊2  
            vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z())); // E索引4(A,B连)
            vertices->push_back(osg::Vec3(stage3[0].x(), stage3[0].y(), stage3[0].z())); // F索引5(C,D连)
            
            // 添加地面4个顶点：G(6)地面前左, H(7)地面前右, I(8)地面后右, J(9)地面后左
            if (stage4.size() >= 4) {
                // 如果stage4有4个点，直接添加
                for (size_t i = 0; i < 4; ++i) {
                    vertices->push_back(osg::Vec3(stage4[i].x(), stage4[i].y(), stage4[i].z()));
                }
            } else {
                // 如果stage4只有1个点，基于底面点直接垂直投影到地面高度
                Point3D centerGround = stage4[0];
                Point3D A = stage1[0], B = stage1[1], C = stage1[2], D = stage1[3];
                
                Point3D G = Point3D(A.x(), A.y(), centerGround.z()); // 地面前左
                Point3D H = Point3D(B.x(), B.y(), centerGround.z()); // 地面前右  
                Point3D I = Point3D(C.x(), C.y(), centerGround.z()); // 地面后右
                Point3D J = Point3D(D.x(), D.y(), centerGround.z()); // 地面后左
                
                vertices->push_back(osg::Vec3(G.x(), G.y(), G.z())); // 索引6
                vertices->push_back(osg::Vec3(H.x(), H.y(), H.z())); // 索引7
                vertices->push_back(osg::Vec3(I.x(), I.y(), I.z())); // 索引8
                vertices->push_back(osg::Vec3(J.x(), J.y(), J.z())); // 索引9
            }
            
            // 人字形屋顶的主要边线：
            // 1-4. 底面4条边
            indices->push_back(0); indices->push_back(1); // AB (前边)
            indices->push_back(1); indices->push_back(2); // BC (右边)
            indices->push_back(2); indices->push_back(3); // CD (后边)
            indices->push_back(3); indices->push_back(0); // DA (左边)
            
            // 5. 屋脊线
            indices->push_back(4); indices->push_back(5); // EF (屋脊线)
            
            // 6-9. AB连接E，CD连接F的脊线
            indices->push_back(0); indices->push_back(4); // A-E (A连屋脊1)
            indices->push_back(1); indices->push_back(4); // B-E (B连屋脊1)
            indices->push_back(2); indices->push_back(5); // C-F (C连屋脊2)
            indices->push_back(3); indices->push_back(5); // D-F (D连屋脊2)
            
            // 第四阶段新增：地面边线和底面到地面的垂直连线
            // 10-13. 地面4条边 (G-H-I-J围成地面)
            indices->push_back(6); indices->push_back(7); // GH (地面前边)
            indices->push_back(7); indices->push_back(8); // HI (地面右边)
            indices->push_back(8); indices->push_back(9); // IJ (地面后边)
            indices->push_back(9); indices->push_back(6); // JG (地面左边)
            
            // 14-17. 底面4个顶点到地面对应点的垂直连线
            indices->push_back(0); indices->push_back(6); // A-G (前左柱)
            indices->push_back(1); indices->push_back(7); // B-H (前右柱)
            indices->push_back(2); indices->push_back(8); // C-I (后右柱)
            indices->push_back(3); indices->push_back(9); // D-J (后左柱)
        }
    }
    
    // 设置顶点数组和索引
    geometry->setVertexArray(vertices);
    if (indices->size() > 0) {
        geometry->addPrimitiveSet(indices);
    }
}

void GableHouse3D_Geo::buildFaceGeometries()
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
        // 第一阶段：只显示底面
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 4) {
            // 添加底面四边形顶点（分解为两个三角形）
            Point3D A = stage1[0];
            Point3D B = stage1[1]; 
            Point3D C = stage1[2];
            Point3D D = stage1[3];
            
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
    else if (allStagePoints.size() == 2) {
        // 第二阶段：地面四个点都连接屋脊1（5个面：底面 + 4个三角形侧面）
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 4 && stage2.size() >= 1) {
            Point3D A = stage1[0];  // 前左
            Point3D B = stage1[1];  // 前右  
            Point3D C = stage1[2];  // 后右
            Point3D D = stage1[3];  // 后左
            Point3D E = stage2[0];  // 屋脊1（所有底面点都连接到这里）
            
            // 面1: 底面 (A, B, C, D) - 分解为两个三角形
            // 第一个三角形：A, B, C
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            // 第二个三角形：A, C, D
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 四个三角形侧面（地面四个点都连接屋脊1）
            // 面2: A, B, E (前面)
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));
            
            // 面3: B, C, E (右面)
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));
            
            // 面4: C, D, E (后面)
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));
            
            // 面5: D, A, E (左面)
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));
            
            // 添加5个面
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 6)); // 底面（2个三角形）
            for (int i = 0; i < 4; i++) {
                geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 6 + i * 3, 3)); // 4个三角形侧面
            }
        }
    }
    else if (allStagePoints.size() == 3) {
        // 第三阶段：完整的人字形房屋（从第二阶段演变：C,D改连屋脊2）
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 4 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D A = stage1[0];  // 前左
            Point3D B = stage1[1];  // 前右
            Point3D C = stage1[2];  // 后右
            Point3D D = stage1[3];  // 后左
            Point3D E = stage2[0];  // 屋脊1（A,B连接）
            Point3D F = stage3[0];  // 屋脊2（C,D连接）
            
            // 面1: 底面 (A, B, C, D) - 分解为两个三角形
            // 第一个三角形：A, B, C
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            // 第二个三角形：A, C, D
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 面2: 前三角形端面 (A, B, E) - A,B连屋脊1
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));
            
            // 面3: 后三角形端面 (C, D, F) - C,D连屋脊2  
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));
            
            // 面4: 左斜屋顶面 (A, E, F, D) - 分解为两个三角形
            // 第一个三角形：A, E, F
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));
            // 第二个三角形：A, F, D
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 面5: 右斜屋顶面 (B, E, F, C) - 分解为两个三角形
            // 第一个三角形：B, E, F
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));
            // 第二个三角形：B, F, C
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            
            // 添加所有面的图元
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 6));   // 底面（2个三角形）
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 6, 3));   // 前三角形端面
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 9, 3));   // 后三角形端面
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 12, 6));  // 左斜屋顶面（2个三角形）
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 18, 6));  // 右斜屋顶面（2个三角形）
        }
    }
    else if (allStagePoints.size() >= 4) {
        // 第四阶段：确定地面，完整的人字形房屋
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        if (stage1.size() >= 4 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D A = stage1[0];  // 前左
            Point3D B = stage1[1];  // 前右
            Point3D C = stage1[2];  // 后右
            Point3D D = stage1[3];  // 后左
            Point3D E = stage2[0];  // 屋脊1（A,B连接）
            Point3D F = stage3[0];  // 屋脊2（C,D连接）
            
            // 面1: 底面 (A, B, C, D) - 分解为两个三角形
            // 第一个三角形：A, B, C
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            // 第二个三角形：A, C, D
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 面2: 前三角形端面 (A, B, E) - A,B连屋脊1
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));
            
            // 面3: 后三角形端面 (C, D, F) - C,D连屋脊2  
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));
            
            // 面4: 左斜屋顶面 (A, E, F, D) - 分解为两个三角形
            // 第一个三角形：A, E, F
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));
            // 第二个三角形：A, F, D
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 面5: 右斜屋顶面 (B, E, F, C) - 分解为两个三角形
            // 第一个三角形：B, E, F
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));
            // 第二个三角形：B, F, C
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            
            // 第四阶段新增：地面面（如果有地面数据）
            if (stage4.size() >= 1) {
                // 添加地面4个顶点
                Point3D G, H, I, J; // 地面4个角点
                
                if (stage4.size() >= 4) {
                    // 如果stage4有4个点，直接使用
                    G = stage4[0]; H = stage4[1]; I = stage4[2]; J = stage4[3];
                } else {
                    // 如果stage4只有1个点，基于这个点生成地面4个角点
                    Point3D centerGround = stage4[0];
                    
                    G = Point3D(A.x(), A.y(), centerGround.z()); // 地面前左
                    H = Point3D(B.x(), B.y(), centerGround.z()); // 地面前右  
                    I = Point3D(C.x(), C.y(), centerGround.z()); // 地面后右
                    J = Point3D(D.x(), D.y(), centerGround.z()); // 地面后左
                }
                
                // 面6: 地面 (G, H, I, J) - 分解为两个三角形
                // 第一个三角形：G, H, I
                vertices->push_back(osg::Vec3(G.x(), G.y(), G.z()));
                vertices->push_back(osg::Vec3(H.x(), H.y(), H.z()));
                vertices->push_back(osg::Vec3(I.x(), I.y(), I.z()));
                // 第二个三角形：G, I, J
                vertices->push_back(osg::Vec3(G.x(), G.y(), G.z()));
                vertices->push_back(osg::Vec3(I.x(), I.y(), I.z()));
                vertices->push_back(osg::Vec3(J.x(), J.y(), J.z()));
                
                // 面7-10: 4个侧面（底面到地面的连接面）
                // 面7: 前侧面 (A, B, H, G) - 分解为两个三角形
                vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
                vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
                vertices->push_back(osg::Vec3(H.x(), H.y(), H.z()));
                vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
                vertices->push_back(osg::Vec3(H.x(), H.y(), H.z()));
                vertices->push_back(osg::Vec3(G.x(), G.y(), G.z()));
                
                // 面8: 右侧面 (B, C, I, H) - 分解为两个三角形
                vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
                vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
                vertices->push_back(osg::Vec3(I.x(), I.y(), I.z()));
                vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
                vertices->push_back(osg::Vec3(I.x(), I.y(), I.z()));
                vertices->push_back(osg::Vec3(H.x(), H.y(), H.z()));
                
                // 面9: 后侧面 (C, D, J, I) - 分解为两个三角形
                vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
                vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
                vertices->push_back(osg::Vec3(J.x(), J.y(), J.z()));
                vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
                vertices->push_back(osg::Vec3(J.x(), J.y(), J.z()));
                vertices->push_back(osg::Vec3(I.x(), I.y(), I.z()));
                
                // 面10: 左侧面 (D, A, G, J) - 分解为两个三角形
                vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
                vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
                vertices->push_back(osg::Vec3(G.x(), G.y(), G.z()));
                vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
                vertices->push_back(osg::Vec3(G.x(), G.y(), G.z()));
                vertices->push_back(osg::Vec3(J.x(), J.y(), J.z()));
            }
            
            // 添加基础面的图元
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 6));   // 底面（2个三角形）
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 6, 3));   // 前三角形端面
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 9, 3));   // 后三角形端面
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 12, 6));  // 左斜屋顶面（2个三角形）
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 18, 6));  // 右斜屋顶面（2个三角形）
            
            // 第四阶段新增的面（仅当有地面数据时）
            if (stage4.size() >= 1) {
                geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 24, 6));  // 地面（2个三角形）
                geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 30, 6));  // 前侧面（2个三角形）
                geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 36, 6));  // 右侧面（2个三角形）
                geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 42, 6));  // 后侧面（2个三角形）
                geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 48, 6));  // 左侧面（2个三角形）
            }
        }
    }
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
} 

