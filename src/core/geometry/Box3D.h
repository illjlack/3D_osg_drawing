#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 长方体几何体类
class Box3D_Geo : public Geo3D
{
public:
    Box3D_Geo();
    virtual ~Box3D_Geo() = default;
    
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
    void buildCornerStageGeometry();
    void buildBoxStageGeometry();
    void calculateBoxParameters();
    bool isValidBoxConfiguration() const;
    
private:
    glm::vec3 m_size = glm::vec3(1.0f);
    glm::vec3 m_corner1 = glm::vec3(0.0f);
    glm::vec3 m_corner2 = glm::vec3(0.0f);
};
