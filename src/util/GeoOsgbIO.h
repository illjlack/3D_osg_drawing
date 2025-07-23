#pragma once
#pragma execution_character_set("utf-8")

#include <QString>
#include <QDateTime>
#include <vector>
#include <map>
#include "../core/GeometryBase.h"

// 前向声明
namespace osg {
    class Group;
    class Node;
    template<class T> class ref_ptr;
}

// 场景对象属性标记
struct SceneObjectTag
{
    QString name;           // 对象名称
    QString description;    // 对象描述
    QString category;       // 对象分类
    QString material;       // 材质信息
    QString texture;        // 纹理信息
    bool visible;           // 是否可见
    bool selectable;        // 是否可选择
    float opacity;          // 透明度
    int layer;              // 图层
    QDateTime createTime;   // 创建时间
    QDateTime modifyTime;   // 修改时间
    
    SceneObjectTag()
        : visible(true)
        , selectable(true)
        , opacity(1.0f)
        , layer(0)
    {}
};

// 场景数据结构
struct SceneData
{
    QString sceneName;      // 场景名称
    QString description;    // 场景描述
    QString author;         // 作者
    QString version;        // 版本
    QDateTime createTime;   // 创建时间
    QDateTime modifyTime;   // 修改时间
    
    // 场景设置
    bool showGrid;          // 是否显示网格
    bool showAxis;          // 是否显示坐标轴
    bool showBoundingBox;   // 是否显示包围盒
    float backgroundColor[4]; // 背景颜色
    
    // 对象列表
    std::vector<Geo3D*> objects;
    std::map<Geo3D*, SceneObjectTag> objectTags;
    
    SceneData()
        : showGrid(true)
        , showAxis(true)
        , showBoundingBox(false)
    {
        backgroundColor[0] = 0.2f;
        backgroundColor[1] = 0.2f;
        backgroundColor[2] = 0.2f;
        backgroundColor[3] = 1.0f;
    }
};

// OSGB文件保存与读取工具类
class GeoOsgbIO
{
public:
    // 保存单个Geo3D对象到osgb文件
    static bool saveToOsgb(const QString& path, Geo3D* geo);
    
    // 保存场景数据（多个Geo3D对象）到osgb文件
    static bool saveSceneToOsgb(const QString& path, const SceneData& sceneData);
    
    // 从osgb文件读取单个Geo3D对象
    static Geo3D* loadFromOsgb(const QString& path);
    
    // 从osgb文件读取场景数据（多个Geo3D对象）
    static SceneData loadSceneFromOsgb(const QString& path);
    
    // 辅助函数
    static QString getSceneNameFromFile(const QString& path);
    static bool validateSceneData(const SceneData& sceneData);
    static void updateSceneModifyTime(SceneData& sceneData);
    
private:
    // 内部辅助函数
    static bool writeSceneMetadata(osg::Group* root, const SceneData& sceneData);
    static bool readSceneMetadata(osg::Group* root, SceneData& sceneData);
    static bool writeObjectTags(osg::Group* root, const std::map<Geo3D*, SceneObjectTag>& tags);
    static bool readObjectTags(osg::Group* root, std::map<Geo3D*, SceneObjectTag>& tags);
    static QString serializeTag(const SceneObjectTag& tag);
    static bool deserializeTag(const QString& data, SceneObjectTag& tag);
    
    // 递归查找几何对象的辅助函数
    static void recursiveFindGeoObjects(osg::Group* parentGroup, 
                                       std::vector<Geo3D*>& geoObjects,
                                       std::vector<osg::ref_ptr<osg::Node>>& drawableNodes,
                                       const QString& baseDir = QString());
    
    // 强制加载 PagedLOD 数据的辅助函数
    static void forceLoadPagedLODData(osg::Group* group, const QString& baseDir = QString());
    
    // 优化渲染质量的辅助函数
    static void optimizeRenderingQuality(osg::Node* node);
    
    // 相机设置优化建议
    static void suggestCameraSettings();
}; 