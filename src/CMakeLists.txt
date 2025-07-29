# 源代码模块
# 包含所有子目录
add_subdirectory(util)
add_subdirectory(core)
add_subdirectory(ui)

# 主程序源文件
set(MAIN_SOURCES
    main.cpp
)

# 合并所有模块的源文件
set(ALL_MODULE_SOURCES
    ${MAIN_SOURCES}
    ${UTIL_SOURCES}
    ${CORE_SOURCES}
    ${UI_SOURCES}
)

set(ALL_MODULE_HEADERS
    ${UTIL_HEADERS}
    ${CORE_HEADERS}
    ${UI_HEADERS}
)

# 添加到父作用域
set(ALL_SOURCES ${ALL_MODULE_SOURCES} PARENT_SCOPE)
set(ALL_HEADERS ${ALL_MODULE_HEADERS} PARENT_SCOPE)

# 创建一个组合库，包含所有模块
add_library(3Drawing_All STATIC
    ${ALL_MODULE_SOURCES}
    ${ALL_MODULE_HEADERS}
)

# 设置包含目录，让所有模块都能访问彼此的头文件
target_include_directories(3Drawing_All PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/util
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/core/geo3D
    ${CMAKE_CURRENT_SOURCE_DIR}/core/geo3D/geometry
    ${CMAKE_CURRENT_SOURCE_DIR}/core/geo3D/managers
    ${CMAKE_CURRENT_SOURCE_DIR}/core/geo3D/buildings
    ${CMAKE_CURRENT_SOURCE_DIR}/core/camera
    ${CMAKE_CURRENT_SOURCE_DIR}/core/picking
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/interaction
)

# 链接所有子模块
target_link_libraries(3Drawing_All PUBLIC
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::OpenGL
    unofficial::osg::osg
    unofficial::osg::osgDB
    unofficial::osg::osgViewer
    unofficial::osg::osgGA
    unofficial::osg::osgManipulator
    unofficial::osg::osgUtil
    unofficial::osg::osgText
    osgQOpenGL
    glm::glm
    3Drawing_Util
    3Drawing_Core
    3Drawing_UI
    3Drawing_Geo3D
) 