#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 环面几何体类
class Torus3D_Geo : public Geo3D
{
public:
    Torus3D_Geo();
    virtual ~Torus3D_Geo() = default;
    
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
    void buildTorusStageGeometry();
    void calculateTorusParameters();
    bool isValidTorusConfiguration() const;
    
private:
    float m_majorRadius = 2.0f;
    float m_minorRadius = 0.5f;
    int m_majorSegments = 16;
    int m_minorSegments = 8;
    glm::vec3 m_center = glm::vec3(0.0f);
    glm::vec3 m_normal = glm::vec3(0.0f, 0.0f, 1.0f);
    float m_calculatedMajorRadius = 0.0f;
    float m_calculatedMinorRadius = 0.0f;
};
