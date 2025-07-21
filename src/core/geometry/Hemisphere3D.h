#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 半球几何体类
class Hemisphere3D_Geo : public Geo3D
{
public:
    Hemisphere3D_Geo();
    virtual ~Hemisphere3D_Geo() = default;
    
    // ==================== 多阶段绘制支持 ====================
    virtual std::vector<StageDescriptor> getStageDescriptors() const override;
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void keyPressEvent(QKeyEvent* event) override;

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;
    virtual void buildStageVertexGeometries(int stage) override;
    virtual void buildStageEdgeGeometries(int stage) override;
    virtual void buildStageFaceGeometries(int stage) override;
    virtual void buildCurrentStagePreviewGeometries() override;
    virtual bool isDrawingComplete() const override;
    virtual bool areControlPointsValid() const override;

private:
    void buildCenterStageGeometry();
    void buildHemisphereStageGeometry();
    void calculateHemisphereParameters();
    bool isValidHemisphereConfiguration() const;
    
private:
    float m_radius = 1.0f;
    int m_segments = 16;
    glm::vec3 m_center = glm::vec3(0.0f);
    glm::vec3 m_normal = glm::vec3(0.0f, 0.0f, 1.0f);
    float m_calculatedRadius = 0.0f;
}; 