#include "GeoOsgbIO.h"
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Group>
#include <osg/Node>
#include <osg/UserDataContainer>
#include <osg/ValueObject>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QProcessEnvironment>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "../core/geometry/UndefinedGeo3D.h"
#include "../core/GeometryBase.h"
#include "../core/Enums3D.h"
#include "LogManager.h"

// 初始化OSG插件系统
static bool initializeOSGPlugins()
{
    static bool initialized = false;
    if (initialized) return true;
    
    LOG_INFO("正在初始化OSG插件系统...", "文件IO");
    
    // 设置OSG插件路径
#ifdef OSG_PLUGIN_PATH
    QString pluginPath = QString::fromLocal8Bit(OSG_PLUGIN_PATH);
    LOG_INFO(QString("设置OSG插件路径: %1").arg(pluginPath), "文件IO");
    
    osgDB::Registry* registry = osgDB::Registry::instance();
    if (registry) {
        registry->setLibraryFilePathList(pluginPath.toStdString());
        LOG_INFO("OSG插件路径已设置", "文件IO");
        
        // 检查可用的插件
        QStringList commonFormats = {"osg", "osgb", "osgt", "ive", "obj", "3ds", "dae"};
        QStringList supportedFormats;
        for (const QString& format : commonFormats) {
            if (registry->getReaderWriterForExtension(format.toStdString())) {
                supportedFormats << format;
            }
        }
        LOG_INFO(QString("支持的常用格式: %1").arg(supportedFormats.join(", ")), "文件IO");
    }
#endif
    
    initialized = true;
    LOG_INFO("OSG插件系统初始化完成", "文件IO");
    return true;
}

// 保存单个Geo3D对象到osgb文件
bool GeoOsgbIO::saveToOsgb(const QString& path, Geo3D* geo)
{
    // 初始化OSG插件系统
    if (!initializeOSGPlugins()) {
        LOG_ERROR("OSG插件系统初始化失败", "文件IO");
        return false;
    }

    if (!geo || path.isEmpty()) return false;
    osg::ref_ptr<osg::Group> root = new osg::Group();
    // 附加类型信息到节点名称
    GeoType3D type = geo->getGeoType();
    root->setName(QString("GeoType:%1").arg(type).toStdString());
    // 添加几何体节点
    root->addChild(geo->getOSGNode());
    // 保存到osgb
    return osgDB::writeNodeFile(*root, path.toStdString());
}

// 保存场景数据（多个Geo3D对象）到osgb文件
bool GeoOsgbIO::saveSceneToOsgb(const QString& path, const SceneData& sceneData)
{
    // 初始化OSG插件系统
    if (!initializeOSGPlugins()) {
        LOG_ERROR("OSG插件系统初始化失败", "文件IO");
        return false;
    }

    if (path.isEmpty()) {
        LOG_ERROR("文件路径为空", "文件IO");
        return false;
    }

    // 验证场景数据
    if (!validateSceneData(sceneData)) {
        LOG_ERROR("场景数据验证失败", "文件IO");
        return false;
    }

    // 创建根节点
    osg::ref_ptr<osg::Group> root = new osg::Group();
    root->setName("SceneRoot");

    // 写入场景元数据
    if (!writeSceneMetadata(root.get(), sceneData)) {
        LOG_ERROR("写入场景元数据失败", "文件IO");
        return false;
    }

    // 写入对象标签
    if (!writeObjectTags(root.get(), sceneData.objectTags)) {
        LOG_ERROR("写入对象标签失败", "文件IO");
        return false;
    }

    // 添加所有几何对象
    for (Geo3D* geo : sceneData.objects) {
        if (geo) {
            // 为每个对象创建子组
            osg::ref_ptr<osg::Group> geoGroup = new osg::Group();
            GeoType3D type = geo->getGeoType();
            geoGroup->setName(QString("GeoType:%1").arg(type).toStdString());
            geoGroup->addChild(geo->getOSGNode());
            root->addChild(geoGroup.get());
        }
    }

    // 保存到osgb文件
    bool success = osgDB::writeNodeFile(*root, path.toStdString());
    if (success) {
        LOG_SUCCESS(QString("场景保存成功: %1").arg(path), "文件IO");
    } else {
        LOG_ERROR(QString("场景保存失败: %1").arg(path), "文件IO");
    }

    return success;
}

// 从osgb文件读取单个Geo3D对象
Geo3D* GeoOsgbIO::loadFromOsgb(const QString& path)
{
    // 初始化OSG插件系统
    if (!initializeOSGPlugins()) {
        LOG_ERROR("OSG插件系统初始化失败", "文件IO");
        return nullptr;
    }
    
    if (path.isEmpty()) {
        LOG_ERROR("文件路径为空", "文件IO");
        return nullptr;
    }
    
    // 检查文件是否存在
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        LOG_ERROR(QString("文件不存在: %1").arg(path), "文件IO");
        return nullptr;
    }
    
    if (!fileInfo.isReadable()) {
        LOG_ERROR(QString("文件不可读: %1").arg(path), "文件IO");
        return nullptr;
    }
    
    LOG_INFO(QString("尝试加载文件: %1").arg(path), "文件IO");
    
    // 检查OSG插件系统
    osgDB::Registry* registry = osgDB::Registry::instance();
    if (!registry) {
        LOG_ERROR("OSG插件注册表未初始化", "文件IO");
        return nullptr;
    }
    
    // 获取文件扩展名
    QString extension = fileInfo.suffix().toLower();
    LOG_INFO(QString("文件扩展名: %1").arg(extension), "文件IO");
    
    // 检查是否有对应的插件
    osgDB::ReaderWriter* reader = registry->getReaderWriterForExtension(extension.toStdString());
    if (!reader) {
        LOG_ERROR(QString("没有找到处理扩展名 '%1' 的OSG插件").arg(extension), "文件IO");
        return nullptr;
    }
    
    // 尝试读取文件
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(path.toStdString());
    if (!node) {
        LOG_ERROR(QString("osgDB::readNodeFile 返回空指针，文件: %1").arg(path), "文件IO");
        return nullptr;
    }
    
    LOG_SUCCESS(QString("成功读取OSG节点，节点名称: %1").arg(QString::fromStdString(node->getName())), "文件IO");
    
    // 解析类型
    std::string name = node->getName();
    GeoType3D type = Geo_UndefinedGeo3D;
    if (name.find("GeoType:") == 0) {
        int t = atoi(name.substr(8).c_str());
        type = static_cast<GeoType3D>(t);
        LOG_INFO(QString("从节点名称解析出几何类型: %1").arg(t), "文件IO");
    } else {
        LOG_INFO("未找到几何类型信息，使用默认类型", "文件IO");
    }
    
    // 创建对应类型的Geo3D对象
    Geo3D* geo = nullptr;
    if (type == Geo_UndefinedGeo3D) {
        geo = new UndefinedGeo3D();
        LOG_INFO("创建未定义几何体对象", "文件IO");
    } else {
        geo = createGeo3D(static_cast<DrawMode3D>(type));
        if (!geo) {
            geo = new UndefinedGeo3D();
            LOG_WARNING(QString("无法创建类型 %1 的几何体，使用默认类型").arg(type), "文件IO");
        } else {
            LOG_INFO(QString("创建几何体对象，类型: %1").arg(type), "文件IO");
        }
    }
    
    // 挂载节点
    geo->getOSGNode()->addChild(node.get());
    geo->setGeoType(type);
    
    LOG_SUCCESS(QString("成功加载文件: %1").arg(path), "文件IO");
    return geo;
}

// 从osgb文件读取场景数据（多个Geo3D对象）
SceneData GeoOsgbIO::loadSceneFromOsgb(const QString& path)
{
    SceneData sceneData;
    
    // 初始化OSG插件系统
    if (!initializeOSGPlugins()) {
        LOG_ERROR("OSG插件系统初始化失败", "文件IO");
        return sceneData;
    }
    
    if (path.isEmpty()) {
        LOG_ERROR("文件路径为空", "文件IO");
        return sceneData;
    }
    
    // 检查文件是否存在
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        LOG_ERROR(QString("文件不存在: %1").arg(path), "文件IO");
        return sceneData;
    }
    
    if (!fileInfo.isReadable()) {
        LOG_ERROR(QString("文件不可读: %1").arg(path), "文件IO");
        return sceneData;
    }
    
    LOG_INFO(QString("尝试加载场景文件: %1").arg(path), "文件IO");
    
    // 尝试读取文件
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(path.toStdString());
    if (!node) {
        LOG_ERROR(QString("osgDB::readNodeFile 返回空指针，文件: %1").arg(path), "文件IO");
        return sceneData;
    }
    
    // 检查是否为场景根节点
    osg::Group* root = dynamic_cast<osg::Group*>(node.get());
    if (!root) {
        LOG_ERROR("文件不是有效的场景文件", "文件IO");
        return sceneData;
    }
    
    // 读取场景元数据
    if (!readSceneMetadata(root, sceneData)) {
        LOG_WARNING("读取场景元数据失败，使用默认值", "文件IO");
    }
    
    // 读取对象标签
    if (!readObjectTags(root, sceneData.objectTags)) {
        LOG_WARNING("读取对象标签失败", "文件IO");
    }
    
    // 遍历所有子节点，查找几何对象
    for (unsigned int i = 0; i < root->getNumChildren(); ++i) {
        osg::Node* child = root->getChild(i);
        if (!child) continue;
        
        std::string name = child->getName();
        if (name.find("GeoType:") == 0) {
            // 这是一个几何对象节点
            int typeValue = atoi(name.substr(8).c_str());
            GeoType3D type = static_cast<GeoType3D>(typeValue);
            
            // 创建几何对象
            Geo3D* geo = nullptr;
            if (type == Geo_UndefinedGeo3D) {
                geo = new UndefinedGeo3D();
            } else {
                geo = createGeo3D(static_cast<DrawMode3D>(type));
                if (!geo) {
                    geo = new UndefinedGeo3D();
                }
            }
            
            if (geo) {
                // 挂载子节点
                geo->getOSGNode()->addChild(child);
                geo->setGeoType(type);
                
                // 添加到场景数据
                sceneData.objects.push_back(geo);
                
                LOG_INFO(QString("加载几何对象，类型: %1").arg(type), "文件IO");
            }
        }
    }
    
    LOG_SUCCESS(QString("成功加载场景文件: %1，包含 %2 个对象").arg(path).arg(sceneData.objects.size()), "文件IO");
    return sceneData;
}

// 获取场景名称
QString GeoOsgbIO::getSceneNameFromFile(const QString& path)
{
    SceneData sceneData = loadSceneFromOsgb(path);
    return sceneData.sceneName;
}

// 验证场景数据
bool GeoOsgbIO::validateSceneData(const SceneData& sceneData)
{
    if (sceneData.sceneName.isEmpty()) {
        LOG_ERROR("场景名称为空", "文件IO");
        return false;
    }
    
    if (sceneData.objects.empty()) {
        LOG_WARNING("场景中没有几何对象", "文件IO");
    }
    
    // 检查对象标签的一致性
    for (Geo3D* obj : sceneData.objects) {
        if (!obj) {
            LOG_ERROR("场景数据中包含空对象", "文件IO");
            return false;
        }
    }
    
    return true;
}

// 更新场景修改时间
void GeoOsgbIO::updateSceneModifyTime(SceneData& sceneData)
{
    sceneData.modifyTime = QDateTime::currentDateTime();
}

// 写入场景元数据
bool GeoOsgbIO::writeSceneMetadata(osg::Group* root, const SceneData& sceneData)
{
    if (!root) return false;
    
    // 创建用户数据容器
    osg::ref_ptr<osg::UserDataContainer> udc = root->getOrCreateUserDataContainer();
    
    // 序列化场景数据为JSON
    QJsonObject sceneJson;
    sceneJson["sceneName"] = sceneData.sceneName;
    sceneJson["description"] = sceneData.description;
    sceneJson["author"] = sceneData.author;
    sceneJson["version"] = sceneData.version;
    sceneJson["createTime"] = sceneData.createTime.toString(Qt::ISODate);
    sceneJson["modifyTime"] = sceneData.modifyTime.toString(Qt::ISODate);
    sceneJson["showGrid"] = sceneData.showGrid;
    sceneJson["showAxis"] = sceneData.showAxis;
    sceneJson["showBoundingBox"] = sceneData.showBoundingBox;
    
    QJsonArray bgColor;
    for (int i = 0; i < 4; ++i) {
        bgColor.append(sceneData.backgroundColor[i]);
    }
    sceneJson["backgroundColor"] = bgColor;
    
    QJsonDocument doc(sceneJson);
    QString jsonString = doc.toJson(QJsonDocument::Compact);
    
    // 存储到用户数据容器
    udc->setUserValue("SceneMetadata", jsonString.toStdString());
    
    return true;
}

// 读取场景元数据
bool GeoOsgbIO::readSceneMetadata(osg::Group* root, SceneData& sceneData)
{
    if (!root) return false;
    
    osg::UserDataContainer* udc = root->getUserDataContainer();
    if (!udc) return false;
    
    std::string jsonString;
    if (!udc->getUserValue("SceneMetadata", jsonString)) {
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(jsonString).toUtf8());
    if (!doc.isObject()) return false;
    
    QJsonObject sceneJson = doc.object();
    
    sceneData.sceneName = sceneJson["sceneName"].toString();
    sceneData.description = sceneJson["description"].toString();
    sceneData.author = sceneJson["author"].toString();
    sceneData.version = sceneJson["version"].toString();
    QString createTimeStr = sceneJson["createTime"].toString();
    QString modifyTimeStr = sceneJson["modifyTime"].toString();
    sceneData.createTime = QDateTime::fromString(createTimeStr, Qt::ISODate);
    sceneData.modifyTime = QDateTime::fromString(modifyTimeStr, Qt::ISODate);
    sceneData.showGrid = sceneJson["showGrid"].toBool();
    sceneData.showAxis = sceneJson["showAxis"].toBool();
    sceneData.showBoundingBox = sceneJson["showBoundingBox"].toBool();
    
    QJsonArray bgColor = sceneJson["backgroundColor"].toArray();
    for (int i = 0; i < 4 && i < bgColor.size(); ++i) {
        sceneData.backgroundColor[i] = bgColor[i].toDouble();
    }
    
    return true;
}

// 写入对象标签
bool GeoOsgbIO::writeObjectTags(osg::Group* root, const std::map<Geo3D*, SceneObjectTag>& tags)
{
    if (!root) return false;
    
    osg::ref_ptr<osg::UserDataContainer> udc = root->getOrCreateUserDataContainer();
    
    QJsonArray tagsArray;
    for (const auto& pair : tags) {
        QJsonObject tagJson;
        tagJson["objectPtr"] = QString::number(reinterpret_cast<quintptr>(pair.first));
        tagJson["tagData"] = serializeTag(pair.second);
        tagsArray.append(tagJson);
    }
    
    QJsonDocument doc(tagsArray);
    QString jsonString = doc.toJson(QJsonDocument::Compact);
    
    udc->setUserValue("ObjectTags", jsonString.toStdString());
    
    return true;
}

// 读取对象标签
bool GeoOsgbIO::readObjectTags(osg::Group* root, std::map<Geo3D*, SceneObjectTag>& tags)
{
    if (!root) return false;
    
    osg::UserDataContainer* udc = root->getUserDataContainer();
    if (!udc) return false;
    
    std::string jsonString;
    if (!udc->getUserValue("ObjectTags", jsonString)) {
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(jsonString).toUtf8());
    if (!doc.isArray()) return false;
    
    QJsonArray tagsArray = doc.array();
    for (const QJsonValue& value : tagsArray) {
        QJsonObject tagJson = value.toObject();
        quintptr objectPtr = tagJson["objectPtr"].toString().toULongLong();
        Geo3D* obj = reinterpret_cast<Geo3D*>(objectPtr);
        
        SceneObjectTag tag;
        if (deserializeTag(tagJson["tagData"].toString(), tag)) {
            tags[obj] = tag;
        }
    }
    
    return true;
}

// 序列化标签
QString GeoOsgbIO::serializeTag(const SceneObjectTag& tag)
{
    QJsonObject tagJson;
    tagJson["name"] = tag.name;
    tagJson["description"] = tag.description;
    tagJson["category"] = tag.category;
    tagJson["material"] = tag.material;
    tagJson["texture"] = tag.texture;
    tagJson["visible"] = tag.visible;
    tagJson["selectable"] = tag.selectable;
    tagJson["opacity"] = tag.opacity;
    tagJson["layer"] = tag.layer;
    tagJson["createTime"] = tag.createTime.toString(Qt::ISODate);
    tagJson["modifyTime"] = tag.modifyTime.toString(Qt::ISODate);
    
    QJsonDocument doc(tagJson);
    return doc.toJson(QJsonDocument::Compact);
}

// 反序列化标签
bool GeoOsgbIO::deserializeTag(const QString& data, SceneObjectTag& tag)
{
    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    if (!doc.isObject()) return false;
    
    QJsonObject tagJson = doc.object();
    
    tag.name = tagJson["name"].toString();
    tag.description = tagJson["description"].toString();
    tag.category = tagJson["category"].toString();
    tag.material = tagJson["material"].toString();
    tag.texture = tagJson["texture"].toString();
    tag.visible = tagJson["visible"].toBool();
    tag.selectable = tagJson["selectable"].toBool();
    tag.opacity = tagJson["opacity"].toDouble();
    tag.layer = tagJson["layer"].toInt();
    QString tagCreateTimeStr = tagJson["createTime"].toString();
    QString tagModifyTimeStr = tagJson["modifyTime"].toString();
    tag.createTime = QDateTime::fromString(tagCreateTimeStr, Qt::ISODate);
    tag.modifyTime = QDateTime::fromString(tagModifyTimeStr, Qt::ISODate);
    
    return true;
} 