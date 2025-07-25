#include "BuildingFactory.h"
#include "GableHouse3D.h"
#include "SpireHouse3D.h"
#include "DomeHouse3D.h"
#include "FlatHouse3D.h"
#include "LHouse3D.h"

osg::ref_ptr<Geo3D> BuildingFactory::createBuilding(BuildingType3D type)
{
    switch (type)
    {
    case Building_GableHouse3D:
        return createGableHouse();
    case Building_SpireHouse3D:
        return createSpireHouse();
    case Building_DomeHouse3D:
        return createDomeHouse();
    case Building_FlatHouse3D:
        return createFlatHouse();
    case Building_LHouse3D:
        return createLHouse();
    default:
        return nullptr;
    }
}

GableHouse3D_Geo* BuildingFactory::createGableHouse()
{
    return new GableHouse3D_Geo();
}

SpireHouse3D_Geo* BuildingFactory::createSpireHouse()
{
    return new SpireHouse3D_Geo();
}

DomeHouse3D_Geo* BuildingFactory::createDomeHouse()
{
    return new DomeHouse3D_Geo();
}

FlatHouse3D_Geo* BuildingFactory::createFlatHouse()
{
    return new FlatHouse3D_Geo();
}

LHouse3D_Geo* BuildingFactory::createLHouse()
{
    return new LHouse3D_Geo();
}

std::string BuildingFactory::getBuildingName(BuildingType3D type)
{
    switch (type)
    {
    case Building_GableHouse3D:
        return "人字形房屋";
    case Building_SpireHouse3D:
        return "尖顶房屋";
    case Building_FlatHouse3D:
        return "平顶房屋";
    case Building_ParapetHouse3D:
        return "带女儿墙的平顶房屋";
    case Building_OverlapHouse3D:
        return "搭边房屋";
    case Building_DomeHouse3D:
        return "穹顶房屋";
    case Building_LHouse3D:
        return "L型房屋";
    case Building_CourtyardHouse3D:
        return "回型房屋";
    case Building_GableSpireHouse3D:
        return "人字尖点房屋";
    case Building_ArcHouse3D:
        return "弧顶房屋";
    default:
        return "未知建筑";
    }
}

BuildingType3D BuildingFactory::getBuildingType(const std::string& name)
{
    if (name == "人字形房屋") return Building_GableHouse3D;
    if (name == "尖顶房屋") return Building_SpireHouse3D;
    if (name == "平顶房屋") return Building_FlatHouse3D;
    if (name == "带女儿墙的平顶房屋") return Building_ParapetHouse3D;
    if (name == "搭边房屋") return Building_OverlapHouse3D;
    if (name == "穹顶房屋") return Building_DomeHouse3D;
    if (name == "L型房屋") return Building_LHouse3D;
    if (name == "回型房屋") return Building_CourtyardHouse3D;
    if (name == "人字尖点房屋") return Building_GableSpireHouse3D;
    if (name == "弧顶房屋") return Building_ArcHouse3D;
    
    return Building_GableHouse3D;  // 默认返回人字形房屋
} 

