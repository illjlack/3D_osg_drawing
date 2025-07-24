#pragma once
#pragma execution_character_set("utf-8")

#include "CoordinateSystem3D.h"
#include <osg/Node>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/LineWidth>
#include <osg/Material>
#include <osgText/Text>
#include <osg/Group>
#include <osg/Vec3>
#include <osg/Vec4>
#include <QObject>

// 坐标系渲染器类
class CoordinateSystemRenderer : public QObject
{
    Q_OBJECT

public:
    explicit CoordinateSystemRenderer(QObject* parent = nullptr);
    ~CoordinateSystemRenderer() = default;

    // 获取坐标系节点
    osg::ref_ptr<osg::Node> getCoordinateSystemNode() const { return m_coordSystemNode; }
    
    // 更新坐标系显示
    void updateCoordinateSystem();

private:
    // 创建坐标轴
    osg::ref_ptr<osg::Node> createAxis(CoordinateAxis3D axis);
    
    // 创建网格
    osg::ref_ptr<osg::Node> createGrid();
    
    // 创建刻度标记
    osg::ref_ptr<osg::Node> createScaleMarks(CoordinateAxis3D axis);
    
    // 创建文本标签
    osg::ref_ptr<osgText::Text> createTextLabel(const QString& text, const osg::Vec3& position, 
                                           const osg::Vec4& color = osg::Vec4(1.0, 1.0, 1.0, 1.0));

private:
    // 坐标系统实例
    CoordinateSystem3D* m_coordSystem;
    
    // 坐标系根节点
    osg::ref_ptr<osg::Group> m_coordSystemNode;
    
    // 坐标轴节点
    osg::ref_ptr<osg::Group> m_axisNode;
    
    // 网格节点
    osg::ref_ptr<osg::Group> m_gridNode;
    
    // 刻度节点
    osg::ref_ptr<osg::Group> m_scaleNode;
    
    // 文本节点
    osg::ref_ptr<osg::Group> m_textNode;

private slots:
    void onCoordinateSystemChanged();
}; 

