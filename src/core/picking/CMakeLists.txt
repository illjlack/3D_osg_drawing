# 拾取系统模块
# 收集当前目录下的所有源文件和头文件

set(PICKING_MODULE_SOURCES
    GeometryPickingSystem.cpp
    PickingIndicator.cpp
)

set(PICKING_MODULE_HEADERS
    GeometryPickingSystem.h
    PickingIndicator.h
    PickingTypes.h
)

# 添加到父作用域
set(PICKING_SOURCES ${PICKING_MODULE_SOURCES} PARENT_SCOPE)
set(PICKING_HEADERS ${PICKING_MODULE_HEADERS} PARENT_SCOPE)

# 创建拾取系统静态库
add_library(3Drawing_Picking STATIC
    ${PICKING_MODULE_SOURCES}
    ${PICKING_MODULE_HEADERS}
)

# 设置包含目录
target_include_directories(3Drawing_Picking PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/geo3D
    ${CMAKE_SOURCE_DIR}/src/core/geo3D/geometry
    ${CMAKE_SOURCE_DIR}/src/util
)

# 链接依赖
target_link_libraries(3Drawing_Picking PUBLIC
    unofficial::osg::osg
    unofficial::osg::osgDB
    unofficial::osg::osgUtil
    unofficial::osg::osgViewer
    glm::glm
) 