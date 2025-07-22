#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// L型房屋几何体类
class LHouse3D_Geo : public Geo3D
{
public:
    LHouse3D_Geo();
    virtual ~LHouse3D_Geo() = default;
    
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
    void buildExtensionStageGeometry();
    void buildLHouseStageGeometry();
    void calculateLHouseParameters();
    bool isValidLHouseConfiguration() const;
    
private:
    float m_mainWidth = 4.0f;
    float m_mainLength = 6.0f;
    float m_extensionWidth = 3.0f;
    float m_extensionLength = 4.0f;
    float m_height = 3.0f;
    glm::vec3 m_baseCorner1 = glm::vec3(0.0f);
    glm::vec3 m_baseCorner2 = glm::vec3(0.0f);
    glm::vec3 m_extensionCorner = glm::vec3(0.0f);
    float m_calculatedMainWidth = 0.0f;
    float m_calculatedMainLength = 0.0f;
    float m_calculatedExtensionWidth = 0.0f;
    float m_calculatedExtensionLength = 0.0f;
    float m_calculatedHeight = 0.0f;
};
