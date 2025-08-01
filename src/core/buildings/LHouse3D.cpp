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
        // 第一阶段：确定第一个顶点
        const auto& stage1 = allStagePoints[0];
        
        for (size_t i = 0; i < stage1.size(); ++i) {
            vertices->push_back(osg::Vec3(stage1[i].x(), stage1[i].y(), stage1[i].z()));
        }
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：确定第二个顶点
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
        // 第三阶段：确定第三个顶点
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
        // 第四阶段：确定第四个顶点
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        if (stage1.size() >= 1) vertices->push_back(osg::Vec3(stage1[0].x(), stage1[0].y(), stage1[0].z()));
        if (stage2.size() >= 1) vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z()));
        if (stage3.size() >= 1) vertices->push_back(osg::Vec3(stage3[0].x(), stage3[0].y(), stage3[0].z()));
        if (stage4.size() >= 1) vertices->push_back(osg::Vec3(stage4[0].x(), stage4[0].y(), stage4[0].z()));
    }
    else if (allStagePoints.size() == 5) {
        // 第五阶段：确定第五个顶点
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        const auto& stage5 = allStagePoints[4];
        
        if (stage1.size() >= 1) vertices->push_back(osg::Vec3(stage1[0].x(), stage1[0].y(), stage1[0].z()));
        if (stage2.size() >= 1) vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z()));
        if (stage3.size() >= 1) vertices->push_back(osg::Vec3(stage3[0].x(), stage3[0].y(), stage3[0].z()));
        if (stage4.size() >= 1) vertices->push_back(osg::Vec3(stage4[0].x(), stage4[0].y(), stage4[0].z()));
        if (stage5.size() >= 1) vertices->push_back(osg::Vec3(stage5[0].x(), stage5[0].y(), stage5[0].z()));
    }
    else if (allStagePoints.size() == 6) {
        // 第六阶段：确定第六个顶点，形成L型基座
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        const auto& stage5 = allStagePoints[4];
        const auto& stage6 = allStagePoints[5];
        
        if (stage1.size() >= 1) vertices->push_back(osg::Vec3(stage1[0].x(), stage1[0].y(), stage1[0].z()));
        if (stage2.size() >= 1) vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z()));
        if (stage3.size() >= 1) vertices->push_back(osg::Vec3(stage3[0].x(), stage3[0].y(), stage3[0].z()));
        if (stage4.size() >= 1) vertices->push_back(osg::Vec3(stage4[0].x(), stage4[0].y(), stage4[0].z()));
        if (stage5.size() >= 1) vertices->push_back(osg::Vec3(stage5[0].x(), stage5[0].y(), stage5[0].z()));
        if (stage6.size() >= 1) vertices->push_back(osg::Vec3(stage6[0].x(), stage6[0].y(), stage6[0].z()));
    }
    else if (allStagePoints.size() >= 7) {
        // 第七阶段：确定墙体高度，形成完整的L型房屋
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        const auto& stage5 = allStagePoints[4];
        const auto& stage6 = allStagePoints[5];
        const auto& stage7 = allStagePoints[6];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1 && 
            stage5.size() >= 1 && stage6.size() >= 1 && stage7.size() >= 1) {
            
            // L型基座的6个顶点
            Point3D A = stage1[0];
            Point3D B = stage2[0];
            Point3D C = stage3[0];
            Point3D D = stage4[0];
            Point3D E = stage5[0];
            Point3D F = stage6[0];
            Point3D wallHeightPoint = stage7[0];
            
            // 计算墙体高度
            double wallHeight = wallHeightPoint.z() - A.z();
            
            // 计算墙体顶层的6个顶点
            Point3D A2 = Point3D(A.x(), A.y(), A.z() + wallHeight);
            Point3D B2 = Point3D(B.x(), B.y(), B.z() + wallHeight);
            Point3D C2 = Point3D(C.x(), C.y(), C.z() + wallHeight);
            Point3D D2 = Point3D(D.x(), D.y(), D.z() + wallHeight);
            Point3D E2 = Point3D(E.x(), E.y(), E.z() + wallHeight);
            Point3D F2 = Point3D(F.x(), F.y(), F.z() + wallHeight);
            
            // 添加底层6个顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));
            
            // 添加顶层6个顶点
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));
            vertices->push_back(osg::Vec3(E2.x(), E2.y(), E2.z()));
            vertices->push_back(osg::Vec3(F2.x(), F2.y(), F2.z()));
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
        // 第三阶段：三个点，绘制线段
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
        // 第四阶段：四个点，继续连线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1) {
            vertices->push_back(osg::Vec3(stage1[0].x(), stage1[0].y(), stage1[0].z()));
            vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z()));
            vertices->push_back(osg::Vec3(stage3[0].x(), stage3[0].y(), stage3[0].z()));
            vertices->push_back(osg::Vec3(stage4[0].x(), stage4[0].y(), stage4[0].z()));
            
            indices->push_back(0); indices->push_back(1);
            indices->push_back(1); indices->push_back(2);
            indices->push_back(2); indices->push_back(3);
        }
    }
    else if (allStagePoints.size() == 5) {
        // 第五阶段：五个点，继续连线
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
            vertices->push_back(osg::Vec3(stage5[0].x(), stage5[0].y(), stage5[0].z()));
            
            indices->push_back(0); indices->push_back(1);
            indices->push_back(1); indices->push_back(2);
            indices->push_back(2); indices->push_back(3);
            indices->push_back(3); indices->push_back(4);
        }
    }
    else if (allStagePoints.size() == 6) {
        // 第六阶段：六个点，形成L型基座边线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        const auto& stage5 = allStagePoints[4];
        const auto& stage6 = allStagePoints[5];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1 && stage5.size() >= 1 && stage6.size() >= 1) {
            vertices->push_back(osg::Vec3(stage1[0].x(), stage1[0].y(), stage1[0].z()));
            vertices->push_back(osg::Vec3(stage2[0].x(), stage2[0].y(), stage2[0].z()));
            vertices->push_back(osg::Vec3(stage3[0].x(), stage3[0].y(), stage3[0].z()));
            vertices->push_back(osg::Vec3(stage4[0].x(), stage4[0].y(), stage4[0].z()));
            vertices->push_back(osg::Vec3(stage5[0].x(), stage5[0].y(), stage5[0].z()));
            vertices->push_back(osg::Vec3(stage6[0].x(), stage6[0].y(), stage6[0].z()));
            
            // 连接6个点形成L型闭合图形
            indices->push_back(0); indices->push_back(1);
            indices->push_back(1); indices->push_back(2);
            indices->push_back(2); indices->push_back(3);
            indices->push_back(3); indices->push_back(4);
            indices->push_back(4); indices->push_back(5);
            indices->push_back(5); indices->push_back(0); // 闭合
        }
    }
    else if (allStagePoints.size() >= 7) {
        // 第七阶段：确定墙体高度，完整的L型房屋边线
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        const auto& stage4 = allStagePoints[3];
        const auto& stage5 = allStagePoints[4];
        const auto& stage6 = allStagePoints[5];
        const auto& stage7 = allStagePoints[6];
        
        if (stage1.size() >= 1 && stage2.size() >= 1 && stage3.size() >= 1 && stage4.size() >= 1 && 
            stage5.size() >= 1 && stage6.size() >= 1 && stage7.size() >= 1) {
            
            // L型基座的6个顶点
            Point3D A = stage1[0];
            Point3D B = stage2[0];
            Point3D C = stage3[0];
            Point3D D = stage4[0];
            Point3D E = stage5[0];
            Point3D F = stage6[0];
            Point3D wallHeightPoint = stage7[0];
            
            // 计算墙体高度
            double wallHeight = wallHeightPoint.z() - A.z();
            
            // 计算墙体顶层的6个顶点
            Point3D A2 = Point3D(A.x(), A.y(), A.z() + wallHeight);
            Point3D B2 = Point3D(B.x(), B.y(), B.z() + wallHeight);
            Point3D C2 = Point3D(C.x(), C.y(), C.z() + wallHeight);
            Point3D D2 = Point3D(D.x(), D.y(), D.z() + wallHeight);
            Point3D E2 = Point3D(E.x(), E.y(), E.z() + wallHeight);
            Point3D F2 = Point3D(F.x(), F.y(), F.z() + wallHeight);
            
            // 添加底层6个顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));  // 0
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));  // 1
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));  // 2
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));  // 3
            vertices->push_back(osg::Vec3(E.x(), E.y(), E.z()));  // 4
            vertices->push_back(osg::Vec3(F.x(), F.y(), F.z()));  // 5
            
            // 添加顶层6个顶点
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z()));  // 6
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z()));  // 7
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z()));  // 8
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z()));  // 9
            vertices->push_back(osg::Vec3(E2.x(), E2.y(), E2.z()));  // 10
            vertices->push_back(osg::Vec3(F2.x(), F2.y(), F2.z()));  // 11
            
            // 底层L型闭合边线
            indices->push_back(0); indices->push_back(1);
            indices->push_back(1); indices->push_back(2);
            indices->push_back(2); indices->push_back(3);
            indices->push_back(3); indices->push_back(4);
            indices->push_back(4); indices->push_back(5);
            indices->push_back(5); indices->push_back(0);
            
            // 顶层L型闭合边线
            indices->push_back(6); indices->push_back(7);
            indices->push_back(7); indices->push_back(8);
            indices->push_back(8); indices->push_back(9);
            indices->push_back(9); indices->push_back(10);
            indices->push_back(10); indices->push_back(11);
            indices->push_back(11); indices->push_back(6);
            
            // 6条垂直边线
            indices->push_back(0); indices->push_back(6);
            indices->push_back(1); indices->push_back(7);
            indices->push_back(2); indices->push_back(8);
            indices->push_back(3); indices->push_back(9);
            indices->push_back(4); indices->push_back(10);
            indices->push_back(5); indices->push_back(11);
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
            
            // 添加底面（分解为两个三角形）
            osg::ref_ptr<osg::Vec3Array> triangleVertices = new osg::Vec3Array();
            triangleVertices->push_back((*vertices)[0]);
            triangleVertices->push_back((*vertices)[1]);
            triangleVertices->push_back((*vertices)[2]);
            triangleVertices->push_back((*vertices)[0]);
            triangleVertices->push_back((*vertices)[2]);
            triangleVertices->push_back((*vertices)[3]);
            
            vertices->clear();
            for (int i = 0; i < 6; i++) {
                vertices->push_back((*triangleVertices)[i]);
            }
            
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 6));
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
            
            // 转换四边形为三角形
            osg::ref_ptr<osg::Vec3Array> triangleVertices = new osg::Vec3Array();
            
            // 主体矩形（顶点0-3）-> 2个三角形
            triangleVertices->push_back((*vertices)[0]); // A
            triangleVertices->push_back((*vertices)[1]); // B
            triangleVertices->push_back((*vertices)[2]); // C
            triangleVertices->push_back((*vertices)[0]); // A
            triangleVertices->push_back((*vertices)[2]); // C
            triangleVertices->push_back((*vertices)[3]); // D
            
            // 扩展矩形（顶点4-7）-> 2个三角形
            triangleVertices->push_back((*vertices)[4]); // E
            triangleVertices->push_back((*vertices)[5]); // F
            triangleVertices->push_back((*vertices)[6]); // G
            triangleVertices->push_back((*vertices)[4]); // E
            triangleVertices->push_back((*vertices)[6]); // G
            triangleVertices->push_back((*vertices)[7]); // H
            
            vertices->clear();
            for (int i = 0; i < triangleVertices->size(); i++) {
                vertices->push_back((*triangleVertices)[i]);
            }
            
            // 添加两个矩形面
            // 转换为三角形（2个四边形 = 4个三角形）
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 12)); // 主体和扩展（4个三角形）
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
            
            // 转换四边形为三角形
            osg::ref_ptr<osg::Vec3Array> triangleVertices = new osg::Vec3Array();
            
            // 转换4个四边形为8个三角形
            for (int i = 0; i < 4; i++) {
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
            
            // 保留剩余的顶点（墙面等）
            for (int i = 16; i < vertices->size(); i++) {
                triangleVertices->push_back((*vertices)[i]);
            }
            
            vertices->clear();
            for (int i = 0; i < triangleVertices->size(); i++) {
                vertices->push_back((*triangleVertices)[i]);
            }
            
            // 添加基本面
            // 转换为三角形（4个四边形 = 8个三角形）
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 24)); // 所有面
            
            // 添加更多的墙面（这里简化处理，实际上L型房屋的墙面构建比较复杂）
            // 可以根据需要添加更多的墙面
        }
    }
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
} 

