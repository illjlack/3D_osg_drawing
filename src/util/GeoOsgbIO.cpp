#include "GeoOsgbIO.h"
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Group>
#include <osg/Node>
#include <osg/UserDataContainer>
#include <osg/ValueObject>
#include <osg/ComputeBoundsVisitor>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Drawable>
#include <osg/PagedLOD>
#include <osg/Material>
#include <osg/LineWidth>
#include <osg/Depth>
#include <osg/PolygonMode>
#include <osg/StateSet>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QProcessEnvironment>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cfloat>
#include "../core/geometry/UndefinedGeo3D.h"
#include "../core/GeometryBase.h"
#include "../core/Enums3D.h"
#include "LogManager.h"

// 打印OSG场景树结构
static void printSceneTree(osg::Node* node, int depth = 0, const QString& prefix = "")
{
    if (!node) return;
    
    QString indent = QString("  ").repeated(depth);
    QString nodeName = QString::fromStdString(node->getName());
    QString nodeType = QString::fromStdString(node->className());
    
    // 获取额外信息
    QString extraInfo;
    osg::Group* group = dynamic_cast<osg::Group*>(node);
    if (group) {
        extraInfo = QString("子节点数: %1").arg(group->getNumChildren());
    }
    
    osg::Geode* geode = dynamic_cast<osg::Geode*>(node);
    if (geode) {
        extraInfo = QString("Drawable数: %1").arg(geode->getNumDrawables());
    }
    
    osg::Geometry* geometry = dynamic_cast<osg::Geometry*>(node);
    if (geometry) {
        osg::Array* vertexArray = geometry->getVertexArray();
        if (vertexArray) {
            extraInfo = QString("顶点数: %1").arg(vertexArray->getNumElements());
        }
    }
    
    // 输出节点信息
    QString nodeInfo = QString("%1%2[%3] 名称:'%4' %5")
        .arg(indent)
        .arg(prefix)
        .arg(nodeType)
        .arg(nodeName.isEmpty() ? "无名称" : nodeName)
        .arg(extraInfo.isEmpty() ? "" : QString("(%1)").arg(extraInfo));
    
    LOG_INFO(nodeInfo, "场景树");
    
    // 递归遍历子节点
    if (group) {
        for (unsigned int i = 0; i < group->getNumChildren(); ++i) {
            QString childPrefix = QString("├─ 子节点[%1]: ").arg(i);
            if (i == group->getNumChildren() - 1) {
                childPrefix = QString("└─ 子节点[%1]: ").arg(i);
            }
            printSceneTree(group->getChild(i), depth + 1, childPrefix);
        }
    }
    
    // 如果是Geode，遍历Drawable
    if (geode) {
        for (unsigned int i = 0; i < geode->getNumDrawables(); ++i) {
            osg::Drawable* drawable = geode->getDrawable(i);
            if (drawable) {
                QString drawablePrefix = QString("├─ Drawable[%1]: ").arg(i);
                if (i == geode->getNumDrawables() - 1) {
                    drawablePrefix = QString("└─ Drawable[%1]: ").arg(i);
                }
                
                QString drawableType = QString::fromStdString(drawable->className());
                QString drawableInfo = QString("%1  %2[%3]")
                    .arg(indent)
                    .arg(drawablePrefix)
                    .arg(drawableType);
                
                // 如果是Geometry，添加更多信息
                osg::Geometry* geom = dynamic_cast<osg::Geometry*>(drawable);
                if (geom) {
                    osg::Array* vertexArray = geom->getVertexArray();
                    if (vertexArray) {
                        drawableInfo += QString(" 顶点数: %1").arg(vertexArray->getNumElements());
                    }
                    drawableInfo += QString(" 图元集数: %1").arg(geom->getNumPrimitiveSets());
                }
                
                LOG_INFO(drawableInfo, "场景树");
            }
        }
    }
}

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
            root->addChild(geo->mm_node()->getOSGNode());
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
            geoGroup->addChild(geo->mm_node()->getOSGNode());
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
    
    // 对于包含 PagedLOD 的场景，尝试强制加载数据
    osg::Group* rootGroup = dynamic_cast<osg::Group*>(node.get());
    if (rootGroup) {
        // 获取文件的基础目录
        QFileInfo fileInfo(path);
        QString baseDir = fileInfo.absolutePath();
        forceLoadPagedLODData(rootGroup, baseDir);
    }
    
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
        geo = new UndefinedGeo3D_Geo();
        LOG_INFO("创建未定义几何体对象", "文件IO");
        
        // 对于未定义几何体，将导入的节点挂载到变换节点下，并设置面拾取掩码
        auto transformNode = geo->mm_node()->getTransformNode();
        if (transformNode.valid()) {
            // 设置导入节点的NodeMask为面拾取
            node->setNodeMask(NODE_MASK_FACE);
            // 为导入的节点设置用户数据，指向父几何体对象
            node->setUserData(geo);
            // 挂载到变换节点下
            transformNode->addChild(node.get());
            LOG_INFO("将导入节点挂载到变换节点下，设置为面拾取掩码", "文件IO");
        } else {
            // 备用方案：挂载到根节点
            geo->mm_node()->getOSGNode()->addChild(node.get());
            LOG_WARNING("变换节点不存在，挂载到根节点", "文件IO");
        }
    } else {
        geo = createGeo3D(static_cast<DrawMode3D>(type));
        if (!geo) {
            geo = new UndefinedGeo3D_Geo();
            LOG_WARNING(QString("无法创建类型 %1 的几何体，使用默认类型").arg(type), "文件IO");
            
            // 对于创建失败后使用默认类型的情况，也执行相同的挂载逻辑
            auto transformNode = geo->mm_node()->getTransformNode();
            if (transformNode.valid()) {
                node->setNodeMask(NODE_MASK_FACE);
                node->setUserData(geo);
                transformNode->addChild(node.get());
                LOG_INFO("将导入节点挂载到变换节点下（备用UndefinedGeo3D）", "文件IO");
            } else {
                geo->mm_node()->getOSGNode()->addChild(node.get());
            }
        } else {
            LOG_INFO(QString("创建几何体对象，类型: %1").arg(type), "文件IO");
            // 对于已知类型，挂载到根节点
            geo->mm_node()->getOSGNode()->addChild(node.get());
        }
    }
    
    geo->setGeoType(type);
    
    // 对于未定义几何体，设置其状态为完成，使其可以被选中拾取
    if (type == Geo_UndefinedGeo3D) {
        geo->mm_state()->setStateComplete();
        LOG_INFO("设置未定义几何体状态为完成，可被选中拾取", "文件IO");
    }
    
    // 优化渲染质量
    optimizeRenderingQuality(geo->mm_node()->getOSGNode());
    
    // 显示相机设置建议
    suggestCameraSettings();
    
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
    
    // 递归查找几何对象和drawable节点
    std::vector<osg::ref_ptr<osg::Node>> drawableNodes;
    // 获取文件的基础目录用于解析相对路径
    QString baseDir = fileInfo.absolutePath();
    recursiveFindGeoObjects(root, sceneData.objects, drawableNodes, baseDir);
    
    // 处理未分类的drawable节点，创建未定义几何对象
    if (!drawableNodes.empty()) {
        for (auto& drawableNode : drawableNodes) {
            UndefinedGeo3D_Geo* geo = new UndefinedGeo3D_Geo();
            geo->setGeoType(Geo_UndefinedGeo3D);
            
            // 将drawable节点挂载到未定义几何体的变换节点下
            auto transformNode = geo->mm_node()->getTransformNode();
            if (transformNode.valid()) {
                drawableNode->setNodeMask(NODE_MASK_FACE);
                drawableNode->setUserData(geo);
                transformNode->addChild(drawableNode.get());
                LOG_INFO("将drawable节点挂载到未定义几何体的变换节点下", "文件IO");
            } else {
                geo->mm_node()->getOSGNode()->addChild(drawableNode.get());
                LOG_WARNING("变换节点不存在，挂载到根节点", "文件IO");
            }
            
            // 设置状态为完成，使其可以被选中拾取
            geo->mm_state()->setStateComplete();
            
            // 优化渲染质量
            optimizeRenderingQuality(geo->mm_node()->getOSGNode());
            
            // 添加到场景数据
            sceneData.objects.push_back(geo);
            
            LOG_INFO("创建未定义几何体对象用于drawable节点", "文件IO");
        }
    }
    
    // 如果成功加载了对象，显示相机设置建议
    if (!sceneData.objects.empty()) {
        suggestCameraSettings();
    }
    
    LOG_SUCCESS(QString("成功加载场景文件: %1，包含 %2 个对象").arg(path).arg(sceneData.objects.size()), "文件IO");
    return sceneData;
}

// 递归查找几何对象的辅助函数
void GeoOsgbIO::recursiveFindGeoObjects(osg::Group* parentGroup, 
                                       std::vector<Geo3D*>& geoObjects,
                                       std::vector<osg::ref_ptr<osg::Node>>& drawableNodes,
                                       const QString& baseDir)
{
    if (!parentGroup) return;
    
    // 遍历所有子节点
    for (unsigned int i = 0; i < parentGroup->getNumChildren(); ++i) {
        osg::Node* child = parentGroup->getChild(i);
        if (!child) continue;
        
        std::string name = child->getName();
        
        // 检查是否有类型定义
        if (name.find("GeoType:") == 0) {
            // 这是一个几何对象节点，创建对应的Geo3D对象
            int typeValue = atoi(name.substr(8).c_str());
            GeoType3D type = static_cast<GeoType3D>(typeValue);
            
            // 创建几何对象
            Geo3D* geo = nullptr;
            if (type == Geo_UndefinedGeo3D) {
                geo = new UndefinedGeo3D_Geo();
            } else {
                geo = createGeo3D(static_cast<DrawMode3D>(type));
                if (!geo) {
                    geo = new UndefinedGeo3D_Geo();
                }
            }
            
            if (geo) {
                // 挂载子节点
                geo->mm_node()->setOSGNode(child);
                geo->setGeoType(type);
                
                // 添加到几何对象列表
                geoObjects.push_back(geo);
                
                LOG_INFO(QString("加载几何对象，类型: %1").arg(type), "文件IO");
            }
            
            // 跳过这个子树，不再递归
            continue;
        }
        
        // 特殊处理 PagedLOD 节点
        osg::PagedLOD* pagedLOD = dynamic_cast<osg::PagedLOD*>(child);
        if (pagedLOD) {
            LOG_INFO(QString("发现 PagedLOD 节点: %1, 子节点数: %2")
                     .arg(QString::fromStdString(pagedLOD->getName()))
                     .arg(pagedLOD->getNumChildren()), "文件IO");
            
            // 优先加载最精细的LOD层级（通常索引0是最精细的）
            for (unsigned int lod = 0; lod < pagedLOD->getNumFileNames(); ++lod) {
                std::string filename = pagedLOD->getFileName(lod);
                if (!filename.empty()) {
                    QString qFilename = QString::fromStdString(filename);
                    
                    // 拼接完整路径
                    QString fullPath;
                    if (qFilename.startsWith("./") || (!qFilename.contains(":/") && !qFilename.startsWith("/"))) {
                        // 相对路径，需要拼接基础目录
                        if (qFilename.startsWith("./")) {
                            qFilename = qFilename.mid(2); // 移除 "./"
                        }
                        fullPath = baseDir + "/" + qFilename;
                    } else {
                        // 绝对路径，直接使用
                        fullPath = qFilename;
                    }
                    
                    LOG_INFO(QString("尝试加载 PagedLOD 文件: %1 -> %2")
                             .arg(QString::fromStdString(filename))
                             .arg(fullPath), "文件IO");
                    
                    // 尝试加载文件
                    osg::ref_ptr<osg::Node> loadedNode = osgDB::readNodeFile(fullPath.toStdString());
                    if (loadedNode.valid()) {
                        LOG_INFO(QString("成功加载 PagedLOD 文件: %1 (LOD级别: %2)").arg(fullPath).arg(lod), "文件IO");
                        
                        // 将加载的节点作为子节点处理
                        osg::Group* loadedGroup = dynamic_cast<osg::Group*>(loadedNode.get());
                        if (loadedGroup) {
                            recursiveFindGeoObjects(loadedGroup, geoObjects, drawableNodes, baseDir);
                        } else {
                            // 检查是否为包含drawable的节点
                            osg::Geode* loadedGeode = dynamic_cast<osg::Geode*>(loadedNode.get());
                            if (loadedGeode && loadedGeode->getNumDrawables() > 0) {
                                drawableNodes.push_back(loadedNode.get());
                                LOG_INFO("将加载的 PagedLOD 内容添加到drawable节点列表", "文件IO");
                            } else if (loadedNode.valid()) {
                                // 其他类型的节点也加入待处理列表
                                drawableNodes.push_back(loadedNode.get());
                                LOG_INFO("将加载的 PagedLOD 节点添加到待处理列表", "文件IO");
                            }
                        }
                        
                        // 成功加载最精细的层级后，跳出循环，不再加载其他层级
                        LOG_INFO(QString("已加载最精细的LOD层级 %1，跳过其他层级").arg(lod), "文件IO");
                        break;
                    } else {
                        LOG_WARNING(QString("无法加载 PagedLOD 文件: %1").arg(fullPath), "文件IO");
                    }
                }
            }
            
            // 继续处理 PagedLOD 节点的现有子节点
            recursiveFindGeoObjects(pagedLOD, geoObjects, drawableNodes, baseDir);
            continue;
        }
        
        // 检查是否为Group节点，如果是则继续递归
        osg::Group* childGroup = dynamic_cast<osg::Group*>(child);
        if (childGroup) {
            recursiveFindGeoObjects(childGroup, geoObjects, drawableNodes, baseDir);
        } else {
            // 检查是否为Drawable节点
            osg::Geode* geode = dynamic_cast<osg::Geode*>(child);
            if (geode && geode->getNumDrawables() > 0) {
                // 这是一个包含drawable的节点，添加到待处理列表
                drawableNodes.push_back(child);
                LOG_INFO("发现drawable节点，添加到待处理列表", "文件IO");
            } else if (geode) {
                // 即使 Geode 为空，也记录一下
                LOG_INFO(QString("发现空的 Geode 节点: %1").arg(QString::fromStdString(geode->getName())), "文件IO");
            } else {
                // 其他类型的节点，如果不是基本的变换节点，也可能包含几何数据
                LOG_INFO(QString("发现其他类型节点: %1 (%2)")
                         .arg(QString::fromStdString(child->className()))
                         .arg(QString::fromStdString(child->getName())), "文件IO");
                
                // 将未知类型的节点也加入待处理列表，以防遗漏
                drawableNodes.push_back(child);
            }
        }
    }
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

// 强制加载 PagedLOD 数据的辅助函数
void GeoOsgbIO::forceLoadPagedLODData(osg::Group* group, const QString& baseDir)
{
    if (!group) return;
    
    // 遍历所有子节点
    for (unsigned int i = 0; i < group->getNumChildren(); ++i) {
        osg::Node* child = group->getChild(i);
        if (!child) continue;
        
        // 检查是否为 PagedLOD 节点
        osg::PagedLOD* pagedLOD = dynamic_cast<osg::PagedLOD*>(child);
        if (pagedLOD) {
            LOG_INFO(QString("强制加载 PagedLOD 节点: %1")
                     .arg(QString::fromStdString(pagedLOD->getName())), "文件IO");
            
            // 优先加载最高精度的 LOD 级别数据（从最后一个开始，通常是最精细的）
            int startLOD = static_cast<int>(pagedLOD->getNumFileNames()) - 1;
            for (int lod = startLOD; lod >= 0; --lod) {
                std::string filename = pagedLOD->getFileName(lod);
                if (!filename.empty()) {
                    QString qFilename = QString::fromStdString(filename);
                    
                    // 拼接完整路径
                    QString fullPath;
                    if (qFilename.startsWith("./") || (!qFilename.contains(":/") && !qFilename.startsWith("/"))) {
                        // 相对路径，需要拼接基础目录
                        if (qFilename.startsWith("./")) {
                            qFilename = qFilename.mid(2); // 移除 "./"
                        }
                        fullPath = baseDir + "/" + qFilename;
                    } else {
                        // 绝对路径，直接使用
                        fullPath = qFilename;
                    }
                    
                    LOG_INFO(QString("强制加载 PagedLOD 文件: %1 -> %2")
                             .arg(QString::fromStdString(filename))
                             .arg(fullPath), "文件IO");
                    
                    // 尝试加载文件
                    osg::ref_ptr<osg::Node> loadedNode = osgDB::readNodeFile(fullPath.toStdString());
                    if (loadedNode.valid()) {
                        // 将加载的节点添加为 PagedLOD 的子节点
                        pagedLOD->addChild(loadedNode.get(), 0.0, FLT_MAX);
                        LOG_INFO(QString("成功强制加载 PagedLOD 文件: %1 (LOD级别: %2)").arg(fullPath).arg(lod), "文件IO");
                        
                        // 成功加载最精细的层级后，跳出循环，不再加载其他层级
                        LOG_INFO(QString("已强制加载最精细的LOD层级 %1，跳过其他层级").arg(lod), "文件IO");
                        break;
                    } else {
                        LOG_WARNING(QString("无法强制加载 PagedLOD 文件: %1").arg(fullPath), "文件IO");
                    }
                }
            }
        }
        
        // 递归处理子节点
        osg::Group* childGroup = dynamic_cast<osg::Group*>(child);
        if (childGroup) {
            forceLoadPagedLODData(childGroup, baseDir);
        }
    }
}

// 优化渲染质量的辅助函数
void GeoOsgbIO::optimizeRenderingQuality(osg::Node* node)
{
    if (!node) return;
    
    LOG_INFO("开始优化渲染质量", "文件IO");
    
    // 获取或创建状态集
    osg::ref_ptr<osg::StateSet> stateSet = node->getOrCreateStateSet();
    
    // 1. 启用抗锯齿
    stateSet->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    stateSet->setMode(GL_POLYGON_SMOOTH, osg::StateAttribute::ON);
    
    // 2. 设置更细的线条宽度
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth();
    lineWidth->setWidth(1.0); // 设置线条宽度为1像素
    stateSet->setAttributeAndModes(lineWidth.get(), osg::StateAttribute::ON);
    
    // 3. 优化材质设置
    osg::ref_ptr<osg::Material> material = new osg::Material();
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0.3, 0.3, 0.3, 1.0));
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0.8, 0.8, 0.8, 1.0));
    material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.2, 0.2, 0.2, 1.0));
    material->setShininess(osg::Material::FRONT_AND_BACK, 32.0);
    stateSet->setAttributeAndModes(material.get(), osg::StateAttribute::ON);
    
    // 4. 启用深度测试
    osg::ref_ptr<osg::Depth> depth = new osg::Depth();
    depth->setFunction(osg::Depth::LEQUAL);
    depth->setRange(0.0, 1.0);
    stateSet->setAttributeAndModes(depth.get(), osg::StateAttribute::ON);
    
    // 5. 设置更精细的多边形模式
    osg::ref_ptr<osg::PolygonMode> polygonMode = new osg::PolygonMode();
    polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
    stateSet->setAttributeAndModes(polygonMode.get(), osg::StateAttribute::ON);
    
    // 6. 启用背面剔除以提高性能
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
    
    // 7. 设置渲染提示为高质量
    stateSet->setRenderingHint(osg::StateSet::DEFAULT_BIN);
    
    LOG_INFO("渲染质量优化完成", "文件IO");
    
    // 递归优化子节点
    osg::Group* group = dynamic_cast<osg::Group*>(node);
    if (group) {
        for (unsigned int i = 0; i < group->getNumChildren(); ++i) {
            optimizeRenderingQuality(group->getChild(i));
        }
    }
}

// 相机设置优化建议
void GeoOsgbIO::suggestCameraSettings()
{
    LOG_INFO("========================= 相机设置优化建议 =========================", "文件IO");
    LOG_INFO("为了获得更好的显示效果，建议进行以下相机设置：", "文件IO");
    LOG_INFO("", "文件IO");
    LOG_INFO("1. 近裁面距离 (Near Plane):", "文件IO");
    LOG_INFO("   - 当前可能设置为: 1.0 或更大", "文件IO");
    LOG_INFO("   - 建议设置为: 0.001 到 0.01", "文件IO");
    LOG_INFO("   - 代码示例: camera->setProjectionMatrixAsPerspective(30.0, aspectRatio, 0.001, 10000.0);", "文件IO");
    LOG_INFO("", "文件IO");
    LOG_INFO("2. 远裁面距离 (Far Plane):", "文件IO");
    LOG_INFO("   - 建议设置为场景包围盒对角线长度的 2-5 倍", "文件IO");
    LOG_INFO("   - 对于建筑模型通常设置为: 1000.0 到 10000.0", "文件IO");
    LOG_INFO("", "文件IO");
    LOG_INFO("3. 视野角度 (Field of View):", "文件IO");
    LOG_INFO("   - 建议设置为: 30-45 度（更小的角度可以减少透视变形）", "文件IO");
    LOG_INFO("", "文件IO");
    LOG_INFO("4. 相机操控器设置:", "文件IO");
    LOG_INFO("   - 最小距离: 0.1", "文件IO");
    LOG_INFO("   - 最大距离: 1000.0", "文件IO");
    LOG_INFO("   - 代码示例: cameraManipulator->setMinimumDistance(0.1);", "文件IO");
    LOG_INFO("   - 代码示例: cameraManipulator->setMaximumDistance(1000.0);", "文件IO");
    LOG_INFO("", "文件IO");
    LOG_INFO("5. 启用多重采样抗锯齿 (MSAA):", "文件IO");
    LOG_INFO("   - 在创建图形上下文时设置采样数: traits->samples = 4;", "文件IO");
    LOG_INFO("", "文件IO");
    LOG_INFO("6. 优化光照设置:", "文件IO");
    LOG_INFO("   - 启用光照: viewer->getLight()->setLightNum(0);", "文件IO");
    LOG_INFO("   - 设置环境光: viewer->getLight()->setAmbient(osg::Vec4(0.3, 0.3, 0.3, 1.0));", "文件IO");
    LOG_INFO("======================================================================", "文件IO");
} 


