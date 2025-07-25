#include "GeoOsgbIO.h"
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Group>
#include <osg/Node>
#include <osg/UserDataContainer>
#include <osg/ValueObject>
#include <osg/PagedLOD>
#include <osg/Geode>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
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

bool GeoOsgbIO::saveGeoList(const QString& filePath, const std::vector<osg::ref_ptr<Geo3D>>& geoList)
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
    for (osg::ref_ptr<Geo3D> geo : geoList) {
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

std::vector<osg::ref_ptr<Geo3D>> GeoOsgbIO::loadGeoList(const QString& filePath)
{
    std::vector<osg::ref_ptr<Geo3D>> result;

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

    // 打印场景树结构信息
    LOG_INFO("=== 场景树结构信息 ===", "场景树");
    printSceneTreeInfo(rootNode.get());
    LOG_INFO("=== 场景树结构结束 ===", "场景树");

    // 预处理场景以优化拾取性能
    preprocessSceneForPicking(rootNode.get(), filePath);

    // 检查是否为场景根节点
    if (rootNode->getName() == SCENE_ROOT_NAME) {
        // 是该程序保存的场景文件
        LOG_INFO("检测到场景文件，开始解析几何体", "文件IO");
        
        osg::Group* sceneGroup = dynamic_cast<osg::Group*>(rootNode.get());
        if (sceneGroup) {
            // 遍历每个子节点，创建对应的几何体对象
            for (unsigned int i = 0; i < sceneGroup->getNumChildren(); ++i) {
                osg::Node* childNode = sceneGroup->getChild(i);
                if (childNode) {
                    osg::ref_ptr<Geo3D> geo = loadGeoDataFromNode(childNode);
                    if (geo) {
                        // 使用新的完整恢复方法
                        geo->restoreFromFileNode(childNode);
                        result.push_back(geo);
                        LOG_INFO(QString("成功加载几何体: %1").arg(static_cast<int>(geo->getGeoType())), "文件IO");
                    }
                }
            }
        }
    } else {
        // 不是该程序保存的文件，直接用未定义对象加载
        LOG_INFO("检测到外部文件，用未定义对象加载", "文件IO");
        
        osg::ref_ptr<Geo3D> undefinedGeo = GeometryFactory::createGeometry(Geo_Undefined3D);
        if (undefinedGeo) {
            // 使用新的完整恢复方法
            undefinedGeo->restoreFromFileNode(rootNode.get());
            result.push_back(undefinedGeo);
        }
    }

    LOG_INFO(QString("文件加载完成，共 %1 个几何体").arg(result.size()), "文件IO");
    return result;
}

// ============================================================================
// 私有辅助函数实现
// ============================================================================

void GeoOsgbIO::saveGeoDataToNode(osg::Node* node, osg::ref_ptr<Geo3D> geo)
{
    if (!node || !geo) return;

    // 获取或创建用户数据容器
    osg::UserDataContainer* userData = node->getOrCreateUserDataContainer();
    
    // 保存几何体类型（用于文件读取时创建对象）
    userData->addUserObject(new osg::StringValueObject("GeoType", 
        QString::number(static_cast<int>(geo->getGeoType())).toStdString()));
    
    // 保存对象内部数据（参数+控制点，用于对象内部恢复）
    QString internalData = QString::fromStdString(geo->getParameters().toString()) + "|" + 
                          QString::fromStdString(geo->mm_controlPoint()->serializeControlPoints());
    userData->addUserObject(new osg::StringValueObject("GeoData", 
        internalData.toStdString()));
        
    LOG_INFO(QString("保存几何体数据: 类型=%1, 内部数据='%2'")
        .arg(static_cast<int>(geo->getGeoType()))
        .arg(internalData), "文件IO");
}

osg::ref_ptr<Geo3D> GeoOsgbIO::loadGeoDataFromNode(osg::Node* node)
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
    osg::ref_ptr<Geo3D> geo = GeometryFactory::createGeometry(geoType);
    if (!geo) {
        LOG_ERROR(QString("创建几何体对象失败，类型: %1").arg(geoTypeInt), "文件IO");
        return nullptr;
    }

    LOG_INFO(QString("成功创建几何体对象，类型: %1").arg(geoTypeInt), "文件IO");
    return geo;
}

void GeoOsgbIO::printSceneTreeInfo(osg::Node* node, int depth)
{
    if (!node) return;

    // 生成缩进字符串
    QString indent = QString("  ").repeated(depth);
    
    // 获取节点基本信息
    QString nodeName = QString::fromStdString(node->getName());
    QString nodeClass = QString::fromStdString(node->className());
    if (nodeName.isEmpty()) {
        nodeName = "<无名称>";
    }
    
    // 获取节点mask信息
    osg::Node::NodeMask nodeMask = node->getNodeMask();
    QString maskInfo = QString("mask:0x%1").arg(nodeMask, 0, 16);
    
    // 打印节点信息
    LOG_INFO(QString("%1[%2] '%3' %4").arg(indent).arg(nodeClass).arg(nodeName).arg(maskInfo), "场景树");
    
    // 打印节点的用户数据键值对
    osg::UserDataContainer* userData = node->getUserDataContainer();
    if (userData && userData->getNumUserObjects() > 0) {
        LOG_INFO(QString("%1  用户数据 (%2个键值对):").arg(indent).arg(userData->getNumUserObjects()), "场景树");
        
        for (unsigned int i = 0; i < userData->getNumUserObjects(); ++i) {
            osg::Object* userObj = userData->getUserObject(i);
            if (userObj) {
                QString objName = QString::fromStdString(userObj->getName());
                QString objValue = "<未知类型>";
                
                // 尝试转换为不同的值对象类型
                if (osg::StringValueObject* stringVal = dynamic_cast<osg::StringValueObject*>(userObj)) {
                    objValue = QString("'%1'").arg(QString::fromStdString(stringVal->getValue()));
                } else if (osg::IntValueObject* intVal = dynamic_cast<osg::IntValueObject*>(userObj)) {
                    objValue = QString::number(intVal->getValue());
                } else if (osg::FloatValueObject* floatVal = dynamic_cast<osg::FloatValueObject*>(userObj)) {
                    objValue = QString::number(floatVal->getValue());
                } else if (osg::DoubleValueObject* doubleVal = dynamic_cast<osg::DoubleValueObject*>(userObj)) {
                    objValue = QString::number(doubleVal->getValue());
                } else if (osg::BoolValueObject* boolVal = dynamic_cast<osg::BoolValueObject*>(userObj)) {
                    objValue = boolVal->getValue() ? "true" : "false";
                } else {
                    // 对于其他类型，显示类名
                    objValue = QString("<%1>").arg(QString::fromStdString(userObj->className()));
                }
                
                LOG_INFO(QString("%1    %2 = %3").arg(indent).arg(objName).arg(objValue), "场景树");
            }
        }
    }
    
    // 如果是Group节点，递归打印子节点
    osg::Group* group = dynamic_cast<osg::Group*>(node);
    if (group) {
        unsigned int numChildren = group->getNumChildren();
        if (numChildren > 0) {
            LOG_INFO(QString("%1  子节点数量: %2").arg(indent).arg(numChildren), "场景树");
            for (unsigned int i = 0; i < numChildren; ++i) {
                printSceneTreeInfo(group->getChild(i), depth + 1);
            }
        }
    }
}

void GeoOsgbIO::preprocessSceneForPicking(osg::Node* rootNode, const QString& filePath)
{
    if (!rootNode) return;
    
    LOG_INFO("开始预处理场景，优化拾取性能", "文件IO");
    
    // 获取原始文件的目录路径，用于解析相对路径
    QFileInfo fileInfo(filePath);
    QString baseDir = fileInfo.absolutePath();
    LOG_INFO(QString("基础目录路径: %1").arg(baseDir), "场景预处理");
    
    // 1. 创建更新访问器强制更新PagedLOD
    class PagedLODUpdateVisitor : public osg::NodeVisitor
    {
    public:
        PagedLODUpdateVisitor(const QString& baseDir) : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), m_baseDir(baseDir) {}
        
        virtual void apply(osg::PagedLOD& pagedLOD) override
        {
            // 强制加载第一个LOD级别
            if (pagedLOD.getNumRanges() > 0) {
                pagedLOD.setRange(0, 0.0f, 1e30f); // 设置一个很大的范围确保加载
                LOG_INFO(QString("更新PagedLOD节点: %1").arg(QString::fromStdString(pagedLOD.getName())), "场景预处理");
                
                // 检查PagedLOD的文件信息
                for (unsigned int i = 0; i < pagedLOD.getNumFileNames(); ++i) {
                    QString fileName = QString::fromStdString(pagedLOD.getFileName(i));
                    
                    // 显示原始路径和解析后的路径
                    QFileInfo fileInfo(fileName);
                    if (fileInfo.isRelative()) {
                        QDir baseDir(m_baseDir);
                        QString fullPath = baseDir.absoluteFilePath(fileName);
                        LOG_INFO(QString("  LOD级别 %1 相对路径: %2 -> %3").arg(i).arg(fileName).arg(fullPath), "场景预处理");
                        
                        // 检查文件是否存在
                        if (QFileInfo::exists(fullPath)) {
                            LOG_INFO("    文件存在", "场景预处理");
                        } else {
                            LOG_WARNING("    文件不存在", "场景预处理");
                        }
                    } else {
                        LOG_INFO(QString("  LOD级别 %1 绝对路径: %2").arg(i).arg(fileName), "场景预处理");
                    }
                }
                
                // 检查当前加载的子节点
                LOG_INFO(QString("  当前子节点数量: %1").arg(pagedLOD.getNumChildren()), "场景预处理");
            }
            traverse(pagedLOD);
        }
        
        virtual void apply(osg::Geode& geode) override
        {
            // 检查Geode是否有有效的Drawable
            unsigned int numDrawables = geode.getNumDrawables();
            if (numDrawables == 0) {
                LOG_WARNING(QString("发现空的Geode节点: %1").arg(QString::fromStdString(geode.getName())), "场景预处理");
            } else {
                LOG_INFO(QString("Geode节点包含 %1 个Drawable").arg(numDrawables), "场景预处理");
            }
            traverse(geode);
        }
        
    private:
        QString m_baseDir;
    };
    
    // 2. 应用PagedLOD更新访问器
    PagedLODUpdateVisitor updateVisitor(baseDir);
    rootNode->accept(updateVisitor);
    
    // 3. 强制重新计算包围盒
    LOG_INFO("重新计算场景包围盒", "场景预处理");
    rootNode->dirtyBound();
    osg::BoundingSphere bound = rootNode->getBound();
    
    if (bound.valid()) {
        LOG_INFO(QString("场景包围盒: 中心(%1,%2,%3) 半径:%4")
            .arg(QString::number(bound.center().x(), 'f', 2))
            .arg(QString::number(bound.center().y(), 'f', 2))
            .arg(QString::number(bound.center().z(), 'f', 2))
            .arg(QString::number(bound.radius(), 'f', 2)), "场景预处理");
    } else {
        LOG_WARNING("场景包围盒仍然无效", "场景预处理");
    }
    
    // 4. 设置节点拾取优化标志
    class PickingOptimizeVisitor : public osg::NodeVisitor
    {
    public:
        PickingOptimizeVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
        
        virtual void apply(osg::Node& node) override
        {
            // 确保节点可以被拾取
            node.setNodeMask(0xffffffff);
            
            // 为节点设置用户数据以便拾取识别
            if (node.getName().empty()) {
                node.setName("PickableNode_" + std::to_string(reinterpret_cast<uintptr_t>(&node)));
            }
            
            traverse(node);
        }
    };
    
    // 5. 应用拾取优化访问器
    PickingOptimizeVisitor pickingVisitor;
    rootNode->accept(pickingVisitor);
    
    // 6. 强制加载PagedLOD数据
    LOG_INFO("尝试强制加载PagedLOD数据", "场景预处理");
    class ForceLoadVisitor : public osg::NodeVisitor
    {
    public:
        ForceLoadVisitor(const QString& baseDir) : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), m_baseDir(baseDir) {}
        
        virtual void apply(osg::PagedLOD& pagedLOD) override
        {
            // 尝试手动加载第一个文件
            if (pagedLOD.getNumFileNames() > 0) {
                std::string fileName = pagedLOD.getFileName(0);
                if (!fileName.empty()) {
                    QString qFileName = QString::fromStdString(fileName);
                    QString fullPath;
                    
                    // 检查是否为相对路径
                    QFileInfo fileInfo(qFileName);
                    if (fileInfo.isRelative()) {
                        // 是相对路径，与基础目录组合
                        QDir baseDir(m_baseDir);
                        fullPath = baseDir.absoluteFilePath(qFileName);
                        LOG_INFO(QString("相对路径 '%1' 解析为: %2").arg(qFileName).arg(fullPath), "场景预处理");
                    } else {
                        // 是绝对路径，直接使用
                        fullPath = qFileName;
                        LOG_INFO(QString("使用绝对路径: %1").arg(fullPath), "场景预处理");
                    }
                    
                    // 检查文件是否存在
                    if (!QFileInfo::exists(fullPath)) {
                        LOG_WARNING(QString("文件不存在: %1").arg(fullPath), "场景预处理");
                        traverse(pagedLOD);
                        return;
                    }
                    
                    LOG_INFO(QString("尝试手动加载文件: %1").arg(fullPath), "场景预处理");
                    osg::ref_ptr<osg::Node> loadedNode = osgDB::readNodeFile(fullPath.toStdString());
                    if (loadedNode.valid()) {
                        // 如果手动加载成功，替换或添加到PagedLOD
                        if (pagedLOD.getNumChildren() == 0) {
                            pagedLOD.addChild(loadedNode.get());
                            LOG_INFO("手动加载的节点已添加到PagedLOD", "场景预处理");
                        }
                    } else {
                        LOG_WARNING(QString("手动加载失败: %1").arg(fullPath), "场景预处理");
                    }
                }
            }
            traverse(pagedLOD);
        }
        
    private:
        QString m_baseDir;
    };
    
    ForceLoadVisitor forceLoadVisitor(baseDir);
    rootNode->accept(forceLoadVisitor);
    
    // 7. 最终统计和诊断
    LOG_INFO("=== 场景统计信息 ===", "场景预处理");
    class StatisticsVisitor : public osg::NodeVisitor
    {
    public:
        StatisticsVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
            totalNodes(0), pagedLODNodes(0), geodeNodes(0), geometryNodes(0), validDrawables(0) {}
        
        virtual void apply(osg::Node& node) override
        {
            totalNodes++;
            traverse(node);
        }
        
        virtual void apply(osg::PagedLOD& pagedLOD) override
        {
            pagedLODNodes++;
            traverse(pagedLOD);
        }
        
        virtual void apply(osg::Geode& geode) override
        {
            geodeNodes++;
            unsigned int numDrawables = geode.getNumDrawables();
            if (numDrawables > 0) {
                validDrawables += numDrawables;
                LOG_INFO(QString("Geode节点 '%1' 包含 %2 个Drawable").arg(QString::fromStdString(geode.getName())).arg(numDrawables), "场景预处理");
            }
            traverse(geode);
        }
        
        virtual void apply(osg::Geometry& geometry) override
        {
            geometryNodes++;
            // osg::Geometry本身就是Drawable
            osg::Array* vertexArray = geometry.getVertexArray();
            if (vertexArray && vertexArray->getNumElements() > 0) {
                validDrawables++;
                LOG_INFO(QString("Geometry节点 '%1' 包含 %2 个顶点，mask:0x%3")
                    .arg(QString::fromStdString(geometry.getName()))
                    .arg(vertexArray->getNumElements())
                    .arg(geometry.getNodeMask(), 0, 16), "场景预处理");
                
                // 检查几何数据的详细信息
                unsigned int numPrimitiveSets = geometry.getNumPrimitiveSets();
                if (numPrimitiveSets > 0) {
                    LOG_INFO(QString("  包含 %1 个图元集").arg(numPrimitiveSets), "场景预处理");
                } else {
                    LOG_WARNING(QString("  没有图元集数据"), "场景预处理");
                }
            } else {
                LOG_WARNING(QString("Geometry节点 '%1' 没有顶点数据").arg(QString::fromStdString(geometry.getName())), "场景预处理");
            }
            traverse(geometry);
        }
        
        int totalNodes;
        int pagedLODNodes;
        int geodeNodes;
        int geometryNodes;
        int validDrawables;
    };
    
    StatisticsVisitor statsVisitor;
    rootNode->accept(statsVisitor);
    
    LOG_INFO(QString("总节点数: %1").arg(statsVisitor.totalNodes), "场景预处理");
    LOG_INFO(QString("PagedLOD节点数: %1").arg(statsVisitor.pagedLODNodes), "场景预处理");
    LOG_INFO(QString("Geode节点数: %1").arg(statsVisitor.geodeNodes), "场景预处理");
    LOG_INFO(QString("Geometry节点数: %1").arg(statsVisitor.geometryNodes), "场景预处理");
    LOG_INFO(QString("有效Drawable数: %1").arg(statsVisitor.validDrawables), "场景预处理");
    
    if (statsVisitor.validDrawables == 0) {
        LOG_ERROR("场景中没有任何可渲染的几何数据，这是拾取失败的主要原因！", "场景预处理");
        
        // 提供更详细的诊断信息
        if (statsVisitor.geometryNodes > 0) {
            LOG_INFO("但是发现了Geometry节点，可能是节点mask设置问题", "场景预处理");
        }
    } else {
        LOG_INFO("场景包含有效的几何数据，拾取应该可以正常工作", "场景预处理");
    }
    LOG_INFO("=== 场景统计结束 ===", "场景预处理");
    
    LOG_INFO("场景预处理完成", "文件IO");
} 
