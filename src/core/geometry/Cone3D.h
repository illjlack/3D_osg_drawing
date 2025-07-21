#pragma once

#include "../GeometryBase.h"

// 圆锥体几何体类
class Cone3D_Geo : public Geo3D
{
public:
    Cone3D_Geo();
    virtual ~Cone3D_Geo() = default;
    
    // ==================== 多阶段绘制支持 ====================
    
    // 获取圆锥的阶段描述符
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
    
    // 第一阶段：底面直径绘制相关方法
    void buildBaseDiameterStageGeometry();
    void buildBaseDiameterPreview();
    
    // 第二阶段：高度点选择相关方法
    void buildConeStageGeometry();
    void buildConePreview();
    Point3D calculatePerpendicularHeightPoint(const Point3D& mousePos) const; // 计算中垂线上的投影点
    
    // 圆锥参数计算
    void calculateConeParameters();
    bool isValidConeConfiguration() const;
    
private:
    float m_radius = 1.0f;
    float m_height = 1.0f;
    int m_segments = 16;
    glm::vec3 m_axis = glm::vec3(0.0f, 0.0f, 1.0f);
    
    // 圆锥几何计算缓存
    glm::vec3 m_baseCenter;      // 底面中心
    glm::vec3 m_apexPoint;       // 顶点
    glm::vec3 m_baseNormal;      // 底面法向量
    float m_calculatedRadius;    // 计算得出的半径
    float m_calculatedHeight;    // 计算得出的高度
};
