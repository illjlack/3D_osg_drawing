# 核心模块
# 包含子目录
add_subdirectory(geo3D)
add_subdirectory(camera)
add_subdirectory(picking)

# 收集当前目录下的所有源文件和头文件
set(CORE_MODULE_SOURCES
    ConstraintSystem.cpp
)

set(CORE_MODULE_HEADERS
    Common3D.h
    Enums3D.h
    ConstraintSystem.h
)

# 合并所有子目录的源文件
set(CORE_ALL_SOURCES
    ${CORE_MODULE_SOURCES}
    ${GEO3D_SOURCES}
    ${CAMERA_SOURCES}
    ${PICKING_SOURCES}
)

set(CORE_ALL_HEADERS
    ${CORE_MODULE_HEADERS}
    ${GEO3D_HEADERS}
    ${CAMERA_HEADERS}
    ${PICKING_HEADERS}
)

# 添加到父作用域
set(CORE_SOURCES ${CORE_ALL_SOURCES} PARENT_SCOPE)
set(CORE_HEADERS ${CORE_ALL_HEADERS} PARENT_SCOPE)

# 创建核心静态库
add_library(3Drawing_Core STATIC
    ${CORE_ALL_SOURCES}
    ${CORE_ALL_HEADERS}
)

# 设置包含目录
target_include_directories(3Drawing_Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/geo3D
    ${CMAKE_CURRENT_SOURCE_DIR}/geo3D/geometry
    ${CMAKE_CURRENT_SOURCE_DIR}/geo3D/managers
    ${CMAKE_CURRENT_SOURCE_DIR}/geo3D/buildings
    ${CMAKE_CURRENT_SOURCE_DIR}/camera
    ${CMAKE_CURRENT_SOURCE_DIR}/picking
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/util
)

# 链接依赖
target_link_libraries(3Drawing_Core PUBLIC
    Qt5::Core
    Qt5::Widgets
    Qt5::OpenGL
    unofficial::osg::osg
    unofficial::osg::osgDB
    unofficial::osg::osgViewer
    unofficial::osg::osgGA
    unofficial::osg::osgManipulator
    unofficial::osg::osgUtil
    glm::glm
    3Drawing_Geo3D
    3Drawing_Camera
    3Drawing_Picking
) 