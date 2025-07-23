#include "Box3D.h"
#include "../managers/GeoControlPointManager.h"
#include <osg/Geometry>
#include <osg/Array>
#include <osg/PrimitiveSet>

Box3D_Geo::Box3D_Geo()
{
    m_geoType = Geo_Box3D;
    // 确保基类正确初始化
    initialize();
}

void Box3D_Geo::buildVertexGeometries()
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
        // 第一阶段：确定一条边 (1-2个点)
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 1) {
            // 添加第一个顶点
            vertices->push_back(osg::Vec3(stage1[0].x(), stage1[0].y(), stage1[0].z()));
        }
        
        if (stage1.size() >= 2) {
            // 添加第二个顶点
            vertices->push_back(osg::Vec3(stage1[1].x(), stage1[1].y(), stage1[1].z()));
        }
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：确定底面 (3个点，形成矩形底面的4个顶点)
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 2 && stage2.size() >= 1) {
            Point3D A = stage1[0];  // 底面第一个顶点
            Point3D B = stage1[1];  // 底面第二个顶点
            Point3D C = stage2[0];  // 底面第三个顶点（通过垂直约束得到）
            
            // 计算底面第四个顶点D，使ABCD形成矩形
            Point3D D = Point3D(
                A.x() + (C.x() - B.x()),
                A.y() + (C.y() - B.y()),
                A.z() + (C.z() - B.z())
            );
            
            // 添加底面4个顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
        }
    }
    else if (allStagePoints.size() >= 3) {
        // 第三阶段：确定高度 (4个点，形成完整长方体的8个顶点)
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 2 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D A = stage1[0];
            Point3D B = stage1[1];
            Point3D C = stage2[0];
            Point3D D = Point3D(
                A.x() + (C.x() - B.x()),
                A.y() + (C.y() - B.y()),
                A.z() + (C.z() - B.z())
            );
            
            // 高度向量（从底面中心到高度点的向量）
            Point3D bottomCenter = Point3D(
                (A.x() + B.x() + C.x() + D.x()) / 4.0f,
                (A.y() + B.y() + C.y() + D.y()) / 4.0f,
                (A.z() + B.z() + C.z() + D.z()) / 4.0f
            );
            
            Point3D heightPoint = stage3[0];
            Point3D heightVector = Point3D(
                heightPoint.x() - bottomCenter.x(),
                heightPoint.y() - bottomCenter.y(),
                heightPoint.z() - bottomCenter.z()
            );
            
            // 计算顶面4个顶点
            Point3D A2 = Point3D(A.x() + heightVector.x(), A.y() + heightVector.y(), A.z() + heightVector.z());
            Point3D B2 = Point3D(B.x() + heightVector.x(), B.y() + heightVector.y(), B.z() + heightVector.z());
            Point3D C2 = Point3D(C.x() + heightVector.x(), C.y() + heightVector.y(), C.z() + heightVector.z());
            Point3D D2 = Point3D(D.x() + heightVector.x(), D.y() + heightVector.y(), D.z() + heightVector.z());
            
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
    
    // 设置顶点数组
    geometry->setVertexArray(vertices);
    
    // 添加点绘制原语
    if (vertices->size() > 0) {
        geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size()));
    }
}

void Box3D_Geo::buildEdgeGeometries()
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
        // 第一阶段：1条边
        const auto& stage1 = allStagePoints[0];
        
        if (stage1.size() >= 2) {
            // 添加第一条边AB
            vertices->push_back(osg::Vec3(stage1[0].x(), stage1[0].y(), stage1[0].z()));
            vertices->push_back(osg::Vec3(stage1[1].x(), stage1[1].y(), stage1[1].z()));
            indices->push_back(0);
            indices->push_back(1);
        }
    }
    else if (allStagePoints.size() == 2) {
        // 第二阶段：底面矩形的4条边
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 2 && stage2.size() >= 1) {
            Point3D A = stage1[0];
            Point3D B = stage1[1];
            Point3D C = stage2[0];
            Point3D D = Point3D(
                A.x() + (C.x() - B.x()),
                A.y() + (C.y() - B.y()),
                A.z() + (C.z() - B.z())
            );
            
            // 添加4个顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));  // 0
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));  // 1
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));  // 2
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));  // 3
            
            // 底面4条边
            indices->push_back(0); indices->push_back(1); // A-B
            indices->push_back(1); indices->push_back(2); // B-C
            indices->push_back(2); indices->push_back(3); // C-D
            indices->push_back(3); indices->push_back(0); // D-A
        }
    }
    else if (allStagePoints.size() >= 3) {
        // 第三阶段：完整长方体的12条边
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 2 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D A = stage1[0];
            Point3D B = stage1[1];
            Point3D C = stage2[0];
            Point3D D = Point3D(
                A.x() + (C.x() - B.x()),
                A.y() + (C.y() - B.y()),
                A.z() + (C.z() - B.z())
            );
            
            Point3D bottomCenter = Point3D(
                (A.x() + B.x() + C.x() + D.x()) / 4.0f,
                (A.y() + B.y() + C.y() + D.y()) / 4.0f,
                (A.z() + B.z() + C.z() + D.z()) / 4.0f
            );
            
            Point3D heightPoint = stage3[0];
            Point3D heightVector = Point3D(
                heightPoint.x() - bottomCenter.x(),
                heightPoint.y() - bottomCenter.y(),
                heightPoint.z() - bottomCenter.z()
            );
            
            Point3D A2 = Point3D(A.x() + heightVector.x(), A.y() + heightVector.y(), A.z() + heightVector.z());
            Point3D B2 = Point3D(B.x() + heightVector.x(), B.y() + heightVector.y(), B.z() + heightVector.z());
            Point3D C2 = Point3D(C.x() + heightVector.x(), C.y() + heightVector.y(), C.z() + heightVector.z());
            Point3D D2 = Point3D(D.x() + heightVector.x(), D.y() + heightVector.y(), D.z() + heightVector.z());
            
            // 添加8个顶点
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));    // 0
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));    // 1
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));    // 2
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));    // 3
            vertices->push_back(osg::Vec3(A2.x(), A2.y(), A2.z())); // 4
            vertices->push_back(osg::Vec3(B2.x(), B2.y(), B2.z())); // 5
            vertices->push_back(osg::Vec3(C2.x(), C2.y(), C2.z())); // 6
            vertices->push_back(osg::Vec3(D2.x(), D2.y(), D2.z())); // 7
            
            // 底面4条边
            indices->push_back(0); indices->push_back(1); // A-B
            indices->push_back(1); indices->push_back(2); // B-C
            indices->push_back(2); indices->push_back(3); // C-D
            indices->push_back(3); indices->push_back(0); // D-A
            
            // 顶面4条边
            indices->push_back(4); indices->push_back(5); // A2-B2
            indices->push_back(5); indices->push_back(6); // B2-C2
            indices->push_back(6); indices->push_back(7); // C2-D2
            indices->push_back(7); indices->push_back(4); // D2-A2
            
            // 垂直4条边
            indices->push_back(0); indices->push_back(4); // A-A2
            indices->push_back(1); indices->push_back(5); // B-B2
            indices->push_back(2); indices->push_back(6); // C-C2
            indices->push_back(3); indices->push_back(7); // D-D2
        }
    }
    
    // 设置顶点数组和索引
    geometry->setVertexArray(vertices);
    geometry->addPrimitiveSet(indices);
}

void Box3D_Geo::buildFaceGeometries()
{
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getFaceGeometry();
    if (!geometry.valid())
    {
        return;
    }
    
    const auto& allStagePoints = mm_controlPoint()->getAllStageControlPoints();
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    if (allStagePoints.size() == 2) {
        // 第二阶段：底面1个面
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        
        if (stage1.size() >= 2 && stage2.size() >= 1) {
            Point3D A = stage1[0];
            Point3D B = stage1[1];
            Point3D C = stage2[0];
            Point3D D = Point3D(
                A.x() + (C.x() - B.x()),
                A.y() + (C.y() - B.y()),
                A.z() + (C.z() - B.z())
            );
            
            // 添加底面四边形顶点（按逆时针顺序）
            vertices->push_back(osg::Vec3(A.x(), A.y(), A.z()));
            vertices->push_back(osg::Vec3(B.x(), B.y(), B.z()));
            vertices->push_back(osg::Vec3(C.x(), C.y(), C.z()));
            vertices->push_back(osg::Vec3(D.x(), D.y(), D.z()));
            
            // 添加底面
            geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));
        }
    }
    else if (allStagePoints.size() >= 3) {
        // 第三阶段：完整长方体的6个面
        const auto& stage1 = allStagePoints[0];
        const auto& stage2 = allStagePoints[1];
        const auto& stage3 = allStagePoints[2];
        
        if (stage1.size() >= 2 && stage2.size() >= 1 && stage3.size() >= 1) {
            Point3D A = stage1[0];
            Point3D B = stage1[1];
            Point3D C = stage2[0];
            Point3D D = Point3D(
                A.x() + (C.x() - B.x()),
                A.y() + (C.y() - B.y()),
                A.z() + (C.z() - B.z())
            );
            
            Point3D bottomCenter = Point3D(
                (A.x() + B.x() + C.x() + D.x()) / 4.0f,
                (A.y() + B.y() + C.y() + D.y()) / 4.0f,
                (A.z() + B.z() + C.z() + D.z()) / 4.0f
            );
            
            Point3D heightPoint = stage3[0];
            Point3D heightVector = Point3D(
                heightPoint.x() - bottomCenter.x(),
                heightPoint.y() - bottomCenter.y(),
                heightPoint.z() - bottomCenter.z()
            );
            
            Point3D A2 = Point3D(A.x() + heightVector.x(), A.y() + heightVector.y(), A.z() + heightVector.z());
            Point3D B2 = Point3D(B.x() + heightVector.x(), B.y() + heightVector.y(), B.z() + heightVector.z());
            Point3D C2 = Point3D(C.x() + heightVector.x(), C.y() + heightVector.y(), C.z() + heightVector.z());
            Point3D D2 = Point3D(D.x() + heightVector.x(), D.y() + heightVector.y(), D.z() + heightVector.z());
            
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
