#pragma once
#pragma execution_character_set("utf-8")

#include <QString>
#include "../core/GeometryBase.h"

// OSGB文件保存与读取工具类
class GeoOsgbIO
{
public:
    // 保存Geo3D对象到osgb文件
    static bool saveToOsgb(const QString& path, Geo3D* geo);
    // 从osgb文件读取Geo3D对象
    static Geo3D* loadFromOsgb(const QString& path);
}; 