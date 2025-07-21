#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 立方体几何体类
class Cube3D_Geo : public Geo3D
{
public:
    Cube3D_Geo();
    virtual ~Cube3D_Geo() = default;
    
    // ==================== 多阶段绘制支持 ====================
    
    // 获取立方体的阶段描述符
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
    
    // 第一阶段：起始角点绘制方法
    void buildCornerStageGeometry();
    void buildCornerPreview();
    
    // 第二阶段：立方体绘制方法
    void buildCubeStageGeometry();
    void buildCubePreview();
    
    // 立方体参数计算
    void calculateCubeParameters();
    bool isValidCubeConfiguration() const;
    
private:
    float m_size = 1.0f;
    glm::vec3 m_corner1 = glm::vec3(0.0f);
    glm::vec3 m_corner2 = glm::vec3(0.0f);
    float m_calculatedSize = 0.0f;
};
