# 工具类模块
# 收集当前目录下的所有源文件和头文件

set(UTIL_MODULE_SOURCES
    OSGUtils.cpp
    GeometryFactory.cpp
    MathUtils.cpp
    LogManager.cpp
    GeoOsgbIO.cpp
    VertexShapeUtils.cpp
)

set(UTIL_MODULE_HEADERS
    OSGUtils.h
    GeometryFactory.h
    MathUtils.h
    LogManager.h
    GeoOsgbIO.h
    VertexShapeUtils.h
)

# 添加到父作用域
set(UTIL_SOURCES ${UTIL_MODULE_SOURCES} PARENT_SCOPE)
set(UTIL_HEADERS ${UTIL_MODULE_HEADERS} PARENT_SCOPE)

# 创建工具类静态库
add_library(3Drawing_Util STATIC
    ${UTIL_MODULE_SOURCES}
    ${UTIL_MODULE_HEADERS}
)

# 设置包含目录
target_include_directories(3Drawing_Util PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/geo3D
)

# 链接依赖
target_link_libraries(3Drawing_Util PUBLIC
    Qt5::Core
    Qt5::Widgets
    unofficial::osg::osg
    unofficial::osg::osgDB
    unofficial::osg::osgUtil
    unofficial::osg::osgViewer
    unofficial::osg::osgText
    glm::glm
) 