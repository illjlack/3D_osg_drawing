#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 尖顶房屋几何体类
class SpireHouse3D_Geo : public Geo3D
{
public:
    SpireHouse3D_Geo();
    virtual ~SpireHouse3D_Geo() = default;
    
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
    void buildSpireHouseStageGeometry();
    void calculateSpireHouseParameters();
    bool isValidSpireHouseConfiguration() const;
    
private:
    float m_width = 4.0f;
    float m_length = 6.0f;
    float m_height = 3.0f;
    float m_spireHeight = 2.0f;
    glm::vec3 m_baseCorner1 = glm::vec3(0.0f);
    glm::vec3 m_baseCorner2 = glm::vec3(0.0f);
    float m_calculatedWidth = 0.0f;
    float m_calculatedLength = 0.0f;
    float m_calculatedHeight = 0.0f;
    float m_calculatedSpireHeight = 0.0f;
}; 