#pragma once
#pragma execution_character_set("utf-8")

#include <glm/glm.hpp>
#include <osg/Geometry>
#include <osg/ref_ptr>

// 前向声明
class Geo3D;

// 拾取特征类型
enum class PickFeatureType {
    NONE = 0,
    VERTEX = 1,    // 顶点
    EDGE = 2,      // 边
    FACE = 3       // 面
};

// 拾取结果
struct PickResult {
    bool hasResult = false;
    Geo3D* geometry = nullptr;
    glm::vec3 worldPosition{0.0f};
    glm::vec3 surfaceNormal{0.0f};
    float distance = FLT_MAX;
    int screenX = 0;
    int screenY = 0;
    
    // 特征信息
    PickFeatureType featureType = PickFeatureType::NONE;
    int primitiveIndex = -1;    // 图元索引（顶点/边/面）
    
    // OSG几何体信息
    osg::ref_ptr<osg::Geometry> osgGeometry = nullptr;  // OSG几何体节点
    int osgPrimitiveIndex = -1;   // OSG图元索引（第几个三角形/线段等）
    
    // 捕捉信息
    bool isSnapped = false;
    glm::vec3 snapPosition{0.0f};
    
    // 构造函数
    PickResult() = default;
    
    // 重置
    void reset() {
        hasResult = false;
        geometry = nullptr;
        worldPosition = glm::vec3(0.0f);
        surfaceNormal = glm::vec3(0.0f);
        distance = FLT_MAX;
        screenX = 0;
        screenY = 0;
        featureType = PickFeatureType::NONE;
        primitiveIndex = -1;
        osgGeometry = nullptr;
        osgPrimitiveIndex = -1;
        isSnapped = false;
        snapPosition = glm::vec3(0.0f);
    }
}; 