#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include "../GeometryBase.h"

// 前向声明
class GableHouse3D_Geo;
class SpireHouse3D_Geo;
class DomeHouse3D_Geo;
class FlatHouse3D_Geo;
class LHouse3D_Geo;

// 建筑工厂类
class BuildingFactory
{
public:
    // 根据建筑类型创建建筑
    static Geo3D::Ptr createBuilding(BuildingType3D type);
    
    // 创建具体类型的建筑
    static Geo3D::Ptr createGableHouse();
    static Geo3D::Ptr createSpireHouse();
    static Geo3D::Ptr createDomeHouse();
    static Geo3D::Ptr createFlatHouse();
    static Geo3D::Ptr createLHouse();
    
    // 辅助函数
    static std::string getBuildingName(BuildingType3D type);
    static BuildingType3D getBuildingType(const std::string& name);
    
private:
    BuildingFactory() = delete;  // 禁止实例化
}; 