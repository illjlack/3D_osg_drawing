#include "GeoOsgbIO.h"
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osg/Group>
#include <osg/Node>
#include <osg/UserDataContainer>
#include <osg/ValueObject>
#include <QDebug>
#include "../core/geometry/UndefinedGeo3D.h"
#include "../core/GeometryBase.h"
#include "../core/Enums3D.h"
#include "../util/GeometryFactory.h"
#include "LogManager.h"

// 场景根节点标识名
const std::string GeoOsgbIO::SCENE_ROOT_NAME = NodeTags3D::SCENE_ROOT;

// ============================================================================
// 公共接口实现
// ============================================================================

bool GeoOsgbIO::saveGeoList(const QString& filePath, const std::vector<Geo3D::Ptr>& geoList)
{
    if (geoList.empty()) {
        LOG_WARNING("保存的几何体列表为空", "文件IO");
        return false;
    }

    // 检查OSG插件是否可用
    if (!osgDB::Registry::instance()->getReaderWriterForExtension("osgb")) {
        LOG_ERROR("OSG osgb插件不可用，无法保存文件", "文件IO");
        return false;
    }

    // 创建场景根节点
    osg::ref_ptr<osg::Group> sceneRoot = new osg::Group();
    sceneRoot->setName(SCENE_ROOT_NAME);

    // 将每个几何体的OSG节点添加到场景根节点下
    for (Geo3D::Ptr geo : geoList) {
        if (!geo) continue;
        
        osg::ref_ptr<osg::Node> geoNode = geo->mm_node()->getOSGNode();
        if (geoNode.valid()) {
            // 在节点中保存几何体数据
            saveGeoDataToNode(geoNode.get(), geo);
            sceneRoot->addChild(geoNode.get());
            LOG_INFO(QString("添加几何体到场景: %1").arg(static_cast<int>(geo->getGeoType())), "文件IO");
        }
    }

    // 保存场景到文件
    std::string stdFilePath = filePath.toStdString();
    bool success = osgDB::writeNodeFile(*sceneRoot, stdFilePath);
    
    if (success) {
        LOG_INFO(QString("成功保存 %1 个几何体到文件: %2").arg(geoList.size()).arg(filePath), "文件IO");
    } else {
        LOG_ERROR(QString("保存文件失败: %1").arg(filePath), "文件IO");
    }

    return success;
}

std::vector<Geo3D::Ptr> GeoOsgbIO::loadGeoList(const QString& filePath)
{
    std::vector<Geo3D::Ptr> result;

    // 检查OSG插件是否可用
    if (!osgDB::Registry::instance()->getReaderWriterForExtension("osgb")) {
        LOG_ERROR("OSG osgb插件不可用，无法读取文件", "文件IO");
        return result;
    }

    // 读取文件
    std::string stdFilePath = filePath.toStdString();
    osg::ref_ptr<osg::Node> rootNode = osgDB::readNodeFile(stdFilePath);
    
    if (!rootNode.valid()) {
        LOG_ERROR(QString("无法读取文件: %1").arg(filePath), "文件IO");
        return result;
    }

    LOG_INFO(QString("成功读取文件: %1").arg(filePath), "文件IO");

    // 检查是否为场景根节点
    if (rootNode->getName() == SCENE_ROOT_NAME) {
        // 是我们软件保存的场景文件
        LOG_INFO("检测到场景文件，开始解析几何体", "文件IO");
        
        osg::Group* sceneGroup = dynamic_cast<osg::Group*>(rootNode.get());
        if (sceneGroup) {
            // 遍历每个子节点，创建对应的几何体对象
            for (unsigned int i = 0; i < sceneGroup->getNumChildren(); ++i) {
                osg::Node* childNode = sceneGroup->getChild(i);
                if (childNode) {
                    Geo3D::Ptr geo = loadGeoDataFromNode(childNode);
                    if (geo) {
                        // 将OSG节点设置给几何体
                        geo->mm_node()->setOSGNode(childNode);
                        result.push_back(geo);
                        LOG_INFO(QString("成功加载几何体: %1").arg(static_cast<int>(geo->getGeoType())), "文件IO");
                    }
                }
            }
        }
    } else {
        // 不是我们软件保存的文件，直接用未定义对象加载
        LOG_INFO("检测到外部文件，用未定义对象加载", "文件IO");
        
        Geo3D::Ptr undefinedGeo = GeometryFactory::createGeometry(Geo_Undefined3D);
        if (undefinedGeo) {
            undefinedGeo->mm_node()->setOSGNode(rootNode.get());
            result.push_back(undefinedGeo);
        }
    }

    LOG_INFO(QString("文件加载完成，共 %1 个几何体").arg(result.size()), "文件IO");
    return result;
}

// ============================================================================
// 私有辅助函数实现
// ============================================================================

void GeoOsgbIO::saveGeoDataToNode(osg::Node* node, Geo3D::Ptr geo)
{
    if (!node || !geo) return;

    // 获取或创建用户数据容器
    osg::UserDataContainer* userData = node->getOrCreateUserDataContainer();
    
    // 保存几何体类型
    userData->addUserObject(new osg::StringValueObject("GeoType", 
        QString::number(static_cast<int>(geo->getGeoType())).toStdString()));
    
    // 保存序列化的几何体数据
    userData->addUserObject(new osg::StringValueObject("GeoData", 
        geo->serialize().toStdString()));
        
    LOG_INFO("几何体数据已保存到OSG节点", "文件IO");
}

Geo3D::Ptr GeoOsgbIO::loadGeoDataFromNode(osg::Node* node)
{
    if (!node) return nullptr;

    osg::UserDataContainer* userData = node->getUserDataContainer();
    if (!userData) {
        LOG_WARNING("节点没有用户数据，无法识别几何体类型", "文件IO");
        return nullptr;
    }

    // 读取几何体类型
    osg::Object* geoTypeObj = userData->getUserObject("GeoType");
    osg::StringValueObject* geoTypeValue = dynamic_cast<osg::StringValueObject*>(geoTypeObj);
    if (!geoTypeValue) {
        LOG_WARNING("节点没有几何体类型数据", "文件IO");
        return nullptr;
    }

    // 解析几何体类型
    bool ok;
    int geoTypeInt = QString::fromStdString(geoTypeValue->getValue()).toInt(&ok);
    if (!ok) {
        LOG_ERROR("解析几何体类型失败", "文件IO");
        return nullptr;
    }
    GeoType3D geoType = static_cast<GeoType3D>(geoTypeInt);

    // 根据类型创建几何体对象
    Geo3D::Ptr geo = GeometryFactory::createGeometry(geoType);
    if (!geo) {
        LOG_ERROR(QString("创建几何体对象失败，类型: %1").arg(geoTypeInt), "文件IO");
        return nullptr;
    }

    // 读取序列化数据
    osg::Object* geoDataObj = userData->getUserObject("GeoData");
    osg::StringValueObject* geoDataValue = dynamic_cast<osg::StringValueObject*>(geoDataObj);
    if (geoDataValue) {
        QString serializedData = QString::fromStdString(geoDataValue->getValue());
        if (!geo->deserialize(serializedData)) {
            LOG_WARNING("反序列化几何体数据失败", "文件IO");
            // 不删除对象，继续使用默认参数
        }
    } else {
        LOG_WARNING("节点没有几何体数据，使用默认参数", "文件IO");
    }

    return geo;
} 


