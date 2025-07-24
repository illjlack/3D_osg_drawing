#include "CoordinateSystemRenderer.h"
#include <osg/StateSet>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osgText/Text>
#include <QDebug>

CoordinateSystemRenderer::CoordinateSystemRenderer(QObject* parent)
    : QObject(parent)
    , m_coordSystem(CoordinateSystem3D::getInstance())
{
    // 创建根节点
    m_coordSystemNode = new osg::Group();
    m_coordSystemNode->setName("CoordinateSystem");
    
    // 创建子节点
    m_axisNode = new osg::Group();
    m_axisNode->setName("Axis");
    m_gridNode = new osg::Group();
    m_gridNode->setName("Grid");
    m_scaleNode = new osg::Group();
    m_scaleNode->setName("Scale");
    m_textNode = new osg::Group();
    m_textNode->setName("Text");
    
    // 添加到根节点
    m_coordSystemNode->addChild(m_axisNode);
    m_coordSystemNode->addChild(m_gridNode);
    m_coordSystemNode->addChild(m_scaleNode);
    m_coordSystemNode->addChild(m_textNode);
    
    // 连接信号
    connect(m_coordSystem, &CoordinateSystem3D::coordinateSystemTypeChanged,
            this, &CoordinateSystemRenderer::onCoordinateSystemChanged);
    connect(m_coordSystem, &CoordinateSystem3D::axisVisibleChanged,
            this, &CoordinateSystemRenderer::onCoordinateSystemChanged);
    connect(m_coordSystem, &CoordinateSystem3D::gridVisibleChanged,
            this, &CoordinateSystemRenderer::onCoordinateSystemChanged);
    connect(m_coordSystem, &CoordinateSystem3D::scaleUnitChanged,
            this, &CoordinateSystemRenderer::onCoordinateSystemChanged);
    connect(m_coordSystem, &CoordinateSystem3D::scaleIntervalChanged,
            this, &CoordinateSystemRenderer::onCoordinateSystemChanged);
    connect(m_coordSystem, &CoordinateSystem3D::axisLengthChanged,
            this, &CoordinateSystemRenderer::onCoordinateSystemChanged);
    connect(m_coordSystem, &CoordinateSystem3D::axisThicknessChanged,
            this, &CoordinateSystemRenderer::onCoordinateSystemChanged);
    connect(m_coordSystem, &CoordinateSystem3D::gridSpacingChanged,
            this, &CoordinateSystemRenderer::onCoordinateSystemChanged);
    connect(m_coordSystem, &CoordinateSystem3D::gridThicknessChanged,
            this, &CoordinateSystemRenderer::onCoordinateSystemChanged);
    
    // 新增：监听天空盒范围变化
    connect(m_coordSystem, &CoordinateSystem3D::skyboxRangeChanged,
            this, &CoordinateSystemRenderer::onCoordinateSystemChanged);
    
    // 初始化坐标系
    updateCoordinateSystem();
}

void CoordinateSystemRenderer::updateCoordinateSystem()
{
    // 清空现有内容
    m_axisNode->removeChildren(0, m_axisNode->getNumChildren());
    m_gridNode->removeChildren(0, m_gridNode->getNumChildren());
    m_scaleNode->removeChildren(0, m_scaleNode->getNumChildren());
    m_textNode->removeChildren(0, m_textNode->getNumChildren());
    
    CoordinateSystemType3D type = m_coordSystem->getCoordinateSystemType();
    
    // 根据类型创建坐标系
    switch (type)
    {
        case CoordSystem_None3D:
            // 不显示坐标系
            break;
            
        case CoordSystem_Axis3D:
            // 只显示坐标轴
            if (m_coordSystem->isAxisVisible(Axis_X3D))
                m_axisNode->addChild(createAxis(Axis_X3D));
            if (m_coordSystem->isAxisVisible(Axis_Y3D))
                m_axisNode->addChild(createAxis(Axis_Y3D));
            if (m_coordSystem->isAxisVisible(Axis_Z3D))
                m_axisNode->addChild(createAxis(Axis_Z3D));
            break;
            
        case CoordSystem_Grid3D:
            // 只显示网格
            if (m_coordSystem->isGridVisible())
                m_gridNode->addChild(createGrid());
            break;
            
        case CoordSystem_Both3D:
            // 显示坐标轴和网格
            if (m_coordSystem->isAxisVisible(Axis_X3D))
                m_axisNode->addChild(createAxis(Axis_X3D));
            if (m_coordSystem->isAxisVisible(Axis_Y3D))
                m_axisNode->addChild(createAxis(Axis_Y3D));
            if (m_coordSystem->isAxisVisible(Axis_Z3D))
                m_axisNode->addChild(createAxis(Axis_Z3D));
            if (m_coordSystem->isGridVisible())
                m_gridNode->addChild(createGrid());
            break;
    }
    
    qDebug() << "坐标系已更新，类型:" << type;
}

osg::ref_ptr<osg::Node> CoordinateSystemRenderer::createAxis(CoordinateAxis3D axis)
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    
    // 设置顶点
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 使用天空盒范围来创建坐标轴，确保覆盖整个空间
    const CoordinateSystem3D::CoordinateRange& skyboxRange = m_coordSystem->getSkyboxRange();
    double axisLength = m_coordSystem->getAxisLength();
    
    // 确保坐标轴长度不超过天空盒范围
    double maxSkyboxRange = skyboxRange.maxRange();
    axisLength = std::min(axisLength, maxSkyboxRange * 0.9); // 使用天空盒范围的90%
    
    osg::Vec4 axisColor;
    
    switch (axis)
    {
        case Axis_X3D:
            // X轴从负方向延伸到正方向
            vertices->push_back(osg::Vec3(-axisLength, 0, 0));
            vertices->push_back(osg::Vec3(axisLength, 0, 0));
            axisColor = osg::Vec4(1.0, 0.0, 0.0, 1.0); // 红色
            break;
        case Axis_Y3D:
            // Y轴从负方向延伸到正方向
            vertices->push_back(osg::Vec3(0, -axisLength, 0));
            vertices->push_back(osg::Vec3(0, axisLength, 0));
            axisColor = osg::Vec4(0.0, 1.0, 0.0, 1.0); // 绿色
            break;
        case Axis_Z3D:
            // Z轴从负方向延伸到正方向
            vertices->push_back(osg::Vec3(0, 0, -axisLength));
            vertices->push_back(osg::Vec3(0, 0, axisLength));
            axisColor = osg::Vec4(0.0, 0.0, 1.0, 1.0); // 蓝色
            break;
        default:
            return nullptr;
    }
    
    colors->push_back(axisColor);
    colors->push_back(axisColor);
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2));
    
    // 设置线宽
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth(m_coordSystem->getAxisThickness());
    geometry->getOrCreateStateSet()->setAttributeAndModes(lineWidth);
    
    geode->addDrawable(geometry);
    
    // 添加轴标签
    QString axisLabel;
    switch (axis)
    {
        case Axis_X3D: axisLabel = "X"; break;
        case Axis_Y3D: axisLabel = "Y"; break;
        case Axis_Z3D: axisLabel = "Z"; break;
    }
    
    osg::Vec3 labelPos;
    switch (axis)
    {
        case Axis_X3D: labelPos = osg::Vec3(axisLength * 1.1, 0, 0); break;
        case Axis_Y3D: labelPos = osg::Vec3(0, axisLength * 1.1, 0); break;
        case Axis_Z3D: labelPos = osg::Vec3(0, 0, axisLength * 1.1); break;
    }
    
    osg::ref_ptr<osgText::Text> text = createTextLabel(axisLabel, labelPos, axisColor);
    geode->addDrawable(text);
    
    return geode;
}

osg::ref_ptr<osg::Node> CoordinateSystemRenderer::createGrid()
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    
    // 设置顶点
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 使用天空盒范围来创建网格，确保覆盖整个空间
    const CoordinateSystem3D::CoordinateRange& skyboxRange = m_coordSystem->getSkyboxRange();
    double spacing = m_coordSystem->getGridSpacing();
    osg::Vec4 gridColor(0.5, 0.5, 0.5, 0.3); // 半透明灰色，降低透明度
    
    // 根据网格平面可见性设置创建网格
    if (m_coordSystem->isGridPlaneVisible(GridPlane_XY3D))
    {
        // 创建XY平面网格 (Z=0平面)
        for (double x = skyboxRange.minX; x <= skyboxRange.maxX; x += spacing)
        {
            vertices->push_back(osg::Vec3(x, skyboxRange.minY, 0));
            vertices->push_back(osg::Vec3(x, skyboxRange.maxY, 0));
            colors->push_back(gridColor);
            colors->push_back(gridColor);
        }
        
        for (double y = skyboxRange.minY; y <= skyboxRange.maxY; y += spacing)
        {
            vertices->push_back(osg::Vec3(skyboxRange.minX, y, 0));
            vertices->push_back(osg::Vec3(skyboxRange.maxX, y, 0));
            colors->push_back(gridColor);
            colors->push_back(gridColor);
        }
    }
    
    if (m_coordSystem->isGridPlaneVisible(GridPlane_YZ3D))
    {
        // 创建YZ平面网格 (X=0平面)
        for (double y = skyboxRange.minY; y <= skyboxRange.maxY; y += spacing)
        {
            vertices->push_back(osg::Vec3(0, y, skyboxRange.minZ));
            vertices->push_back(osg::Vec3(0, y, skyboxRange.maxZ));
            colors->push_back(gridColor);
            colors->push_back(gridColor);
        }
        
        for (double z = skyboxRange.minZ; z <= skyboxRange.maxZ; z += spacing)
        {
            vertices->push_back(osg::Vec3(0, skyboxRange.minY, z));
            vertices->push_back(osg::Vec3(0, skyboxRange.maxY, z));
            colors->push_back(gridColor);
            colors->push_back(gridColor);
        }
    }
    
    if (m_coordSystem->isGridPlaneVisible(GridPlane_XZ3D))
    {
        // 创建XZ平面网格 (Y=0平面)
        for (double x = skyboxRange.minX; x <= skyboxRange.maxX; x += spacing)
        {
            vertices->push_back(osg::Vec3(x, 0, skyboxRange.minZ));
            vertices->push_back(osg::Vec3(x, 0, skyboxRange.maxZ));
            colors->push_back(gridColor);
            colors->push_back(gridColor);
        }
        
        for (double z = skyboxRange.minZ; z <= skyboxRange.maxZ; z += spacing)
        {
            vertices->push_back(osg::Vec3(skyboxRange.minX, 0, z));
            vertices->push_back(osg::Vec3(skyboxRange.maxX, 0, z));
            colors->push_back(gridColor);
            colors->push_back(gridColor);
        }
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size()));
    
    // 设置线宽和透明度
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth(m_coordSystem->getGridThickness());
    geometry->getOrCreateStateSet()->setAttributeAndModes(lineWidth);
    
    // 启用混合
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setAttribute(new osg::BlendFunc());
    
    geode->addDrawable(geometry);
    
    return geode;
}

osg::ref_ptr<osg::Node> CoordinateSystemRenderer::createScaleMarks(CoordinateAxis3D axis)
{
    // 这里可以实现刻度标记的创建
    // 暂时返回空节点
    return new osg::Group();
}

osg::ref_ptr<osgText::Text> CoordinateSystemRenderer::createTextLabel(const QString& text, const osg::Vec3& position, const osg::Vec4& color)
{
    osg::ref_ptr<osgText::Text> textNode = new osgText::Text();
    textNode->setText(text.toStdString());
    textNode->setPosition(position);
    textNode->setColor(color);
    textNode->setCharacterSize(m_coordSystem->getActualFontSize()); // 使用坐标系统的字体大小设置
    textNode->setAxisAlignment(osgText::Text::SCREEN);
    textNode->setCharacterSizeMode(osgText::Text::SCREEN_COORDS);
    
    return textNode;
}

void CoordinateSystemRenderer::onCoordinateSystemChanged()
{
    updateCoordinateSystem();
} 

