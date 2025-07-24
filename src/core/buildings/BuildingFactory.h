#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"

// 前向声明
class Geo3D;
class GableHouse3D_Geo;
class SpireHouse3D_Geo;
class DomeHouse3D_Geo;
class FlatHouse3D_Geo;
class LHouse3D_Geo;

// 建筑类型枚举已在Enums3D.h中定义

// 建筑工厂类
class BuildingFactory
{
public:
    // 根据建筑类型创建建筑
    static Geo3D* createBuilding(BuildingType3D type);
    
    // 创建具体类型的建筑
    static GableHouse3D_Geo* createGableHouse();
    static SpireHouse3D_Geo* createSpireHouse();
    static DomeHouse3D_Geo* createDomeHouse();
    static FlatHouse3D_Geo* createFlatHouse();
    static LHouse3D_Geo* createLHouse();
    
    // 辅助函数
    static std::string getBuildingName(BuildingType3D type);
    static BuildingType3D getBuildingType(const std::string& name);
    
private:
    BuildingFactory() = delete;  // 禁止实例化
}; 

