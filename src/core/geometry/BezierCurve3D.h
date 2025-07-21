#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 贝塞尔曲线几何体类
class BezierCurve3D_Geo : public Geo3D
{
public:
    BezierCurve3D_Geo();
    virtual ~BezierCurve3D_Geo() = default;
    
    // ==================== 多阶段绘制支持 ====================
    
    // 获取贝塞尔曲线的阶段描述符
    virtual std::vector<StageDescriptor> getStageDescriptors() const override;
    
    // 重写事件处理以支持多阶段绘制
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void keyPressEvent(QKeyEvent* event) override;

protected:
    // 传统几何体构建方法
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;
    
    // ==================== 多阶段几何构建方法 ====================
    
    // 为不同阶段构建特定的几何体
    virtual void buildStageVertexGeometries(int stage) override;
    virtual void buildStageEdgeGeometries(int stage) override;
    virtual void buildStageFaceGeometries(int stage) override;
    
    // 构建当前阶段的临时预览几何体
    virtual void buildCurrentStagePreviewGeometries() override;
    
    // 绘制完成检查和控制点验证
    virtual bool isDrawingComplete() const override;
    virtual bool areControlPointsValid() const override;
    
private:
    // ==================== 阶段特定的辅助方法 ====================
    
    // 贝塞尔曲线绘制相关方法
    void buildBezierCurveStageGeometry();
    void buildBezierCurvePreview();
    
    // 贝塞尔曲线参数计算
    bool isValidBezierConfiguration() const;
    
private:
    // 贝塞尔曲线点缓存
    std::vector<Point3D> m_bezierPoints;
};
