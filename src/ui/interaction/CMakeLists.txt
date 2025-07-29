# 交互模块
# 收集当前目录下的所有源文件和头文件

set(INTERACTION_MODULE_SOURCES
    InteractionModeManager.cpp
    InputInteractionMode.cpp
)

set(INTERACTION_MODULE_HEADERS
    InteractionModeManager.h
    InputInteractionMode.h
)

# 添加到父作用域
set(INTERACTION_SOURCES ${INTERACTION_MODULE_SOURCES} PARENT_SCOPE)
set(INTERACTION_HEADERS ${INTERACTION_MODULE_HEADERS} PARENT_SCOPE)

# 创建交互静态库
add_library(3Drawing_Interaction STATIC
    ${INTERACTION_MODULE_SOURCES}
    ${INTERACTION_MODULE_HEADERS}
)

# 设置包含目录
target_include_directories(3Drawing_Interaction PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/geo3D
    ${CMAKE_SOURCE_DIR}/src/core/geo3D/geometry
    ${CMAKE_SOURCE_DIR}/src/core/geo3D/managers
    ${CMAKE_SOURCE_DIR}/src/util
)

# 链接依赖
target_link_libraries(3Drawing_Interaction PUBLIC
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    unofficial::osg::osg
    unofficial::osg::osgGA
    glm::glm
) 