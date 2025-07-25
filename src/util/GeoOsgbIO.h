#pragma once
#pragma execution_character_set("utf-8")

#include <QString>
#include <vector>
#include "../core/GeometryBase.h"

// 前向声明
namespace osg {
    class Group;
    class Node;
    template<class T> class ref_ptr;
}

// OSGB文件保存与读取工具类
class GeoOsgbIO
{
public:
    // 保存Geo3D对象列表到osgb文件
    static bool saveGeoList(const QString& filePath, const std::vector<osg::ref_ptr<Geo3D>>& geoList);
    
    // 从osgb文件加载Geo3D对象列表
    static std::vector<osg::ref_ptr<Geo3D>> loadGeoList(const QString& filePath);

private:
    // 场景根节点标识名
    static const std::string SCENE_ROOT_NAME;
    
    // 在OSG节点中保存Geo3D对象信息
    static void saveGeoDataToNode(osg::Node* node, osg::ref_ptr<Geo3D> geo);
    
    // 从OSG节点中读取Geo3D对象信息
    static osg::ref_ptr<Geo3D> loadGeoDataFromNode(osg::Node* node);
    
    // 打印场景树信息（深搜遍历）
    static void printSceneTreeInfo(osg::Node* node, int depth = 0);
    
    // 预处理加载的场景
    static void preprocessSceneForPicking(osg::Node* rootNode, const QString& filePath);
}; 
