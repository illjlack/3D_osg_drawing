#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 穹顶房屋几何体类
class DomeHouse3D_Geo : public Geo3D
{
public:
    DomeHouse3D_Geo();
    virtual ~DomeHouse3D_Geo() = default;
    
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
    void buildBaseStageGeometry();
    void buildDomeHouseStageGeometry();
    void calculateDomeHouseParameters();
    bool isValidDomeHouseConfiguration() const;
    
private:
    float m_radius = 3.0f;
    float m_height = 4.0f;
    float m_domeHeight = 2.0f;
    int m_segments = 16;
    glm::vec3 m_center = glm::vec3(0.0f);
    float m_calculatedRadius = 0.0f;
    float m_calculatedHeight = 0.0f;
    float m_calculatedDomeHeight = 0.0f;
}; 