#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 椭球几何体类
class Ellipsoid3D_Geo : public Geo3D
{
public:
    Ellipsoid3D_Geo();
    virtual ~Ellipsoid3D_Geo() = default;
    
    // ==================== 多阶段绘制支持 ====================
    
    // 获取椭球的阶段描述符
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
    
    // 第一阶段：直径绘制相关方法
    void buildDiameterStageGeometry();
    void buildDiameterPreview();
    
    // 第二阶段：垂点选择相关方法  
    void buildEllipseStageGeometry();
    void buildEllipsePreview();
    Point3D calculatePerpendicularPoint(const Point3D& mousePos) const; // 计算中垂线上的投影点
    
    // 椭圆参数计算
    void calculateEllipseParameters();
    bool isValidEllipseConfiguration() const;
    
private:
    glm::vec3 m_radii;  // 椭球的三个轴半径
    int m_segments;      // 细分段数
    
    // 椭圆几何计算缓存
    glm::vec3 m_center;        // 椭圆中心
    glm::vec3 m_majorAxis;     // 长轴方向
    glm::vec3 m_minorAxis;     // 短轴方向
    float m_majorRadius;       // 长轴半径
    float m_minorRadius;       // 短轴半径
}; 