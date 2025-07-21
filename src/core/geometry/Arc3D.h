#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 圆弧几何体类
class Arc3D_Geo : public Geo3D
{
public:
    Arc3D_Geo();
    virtual ~Arc3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取圆弧的阶段描述符
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
    
    // 第一阶段：圆心和半径绘制方法
    void buildCenterRadiusStageGeometry();
    void buildCenterRadiusPreview();
    
    // 第二阶段：角度范围绘制方法
    void buildArcRangeStageGeometry(); 
    void buildArcRangePreview();
    
    // 圆弧参数计算
    void calculateArcParameters();
    bool isValidArcConfiguration() const;
    Point3D calculateArcPoint(const Point3D& mousePos) const; // 计算圆弧上的投影点
    
private:
    float m_radius = 0.0f;
    float m_startAngle = 0.0f;
    float m_endAngle = 0.0f;
    float m_sweepAngle = 0.0f;
    glm::vec3 m_normal = glm::vec3(0.0f);
    glm::vec3 m_center = glm::vec3(0.0f);
    glm::vec3 m_uAxis = glm::vec3(0.0f);
    glm::vec3 m_vAxis = glm::vec3(0.0f);
    std::vector<glm::vec3> m_arcPoints;
}; 