#include "GeoOsgbIO.h"
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Group>
#include <osg/Node>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include "../core/geometry/UndefinedGeo3D.h"
#include "../core/GeometryBase.h"
#include "../core/Enums3D.h"
#include "LogManager.h"

// 保存Geo3D对象到osgb文件
bool GeoOsgbIO::saveToOsgb(const QString& path, Geo3D* geo)
{
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

// 从osgb文件读取Geo3D对象
Geo3D* GeoOsgbIO::loadFromOsgb(const QString& path)
{
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