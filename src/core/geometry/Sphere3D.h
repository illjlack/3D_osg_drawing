#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 球体几何体类
class Sphere3D_Geo : public Geo3D
{
public:
    Sphere3D_Geo();
    virtual ~Sphere3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取球体的阶段描述符
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
    
    // 第一阶段：球心绘制方法
    void buildCenterStageGeometry();
    void buildCenterPreview();
    
    // 第二阶段：球体绘制方法
    void buildSphereStageGeometry();
    void buildSpherePreview();
    
    // 球体参数计算
    void calculateSphereParameters();
    bool isValidSphereConfiguration() const;
    
private:
    float m_radius = 1.0f;
    int m_segments = 16;
    glm::vec3 m_center = glm::vec3(0.0f);
    float m_calculatedRadius = 0.0f;
};
