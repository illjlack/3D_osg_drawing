# 用户界面模块
# 包含子目录
add_subdirectory(interaction)

# 收集当前目录下的所有源文件和头文件
set(UI_MODULE_SOURCES
    OSGWidget.cpp
    MainWindow.cpp
    LogOutputWidget.cpp
    StatusBar3D.cpp
    PropertyEditor3D.cpp
    ToolPanel3D.cpp
)

set(UI_MODULE_HEADERS
    OSGWidget.h
    MainWindow.h
    LogOutputWidget.h
    StatusBar3D.h
    PropertyEditor3D.h
    ToolPanel3D.h
)

# 合并子目录的源文件
set(UI_ALL_SOURCES
    ${UI_MODULE_SOURCES}
    ${INTERACTION_SOURCES}
)

set(UI_ALL_HEADERS
    ${UI_MODULE_HEADERS}
    ${INTERACTION_HEADERS}
)

# 添加到父作用域
set(UI_SOURCES ${UI_ALL_SOURCES} PARENT_SCOPE)
set(UI_HEADERS ${UI_ALL_HEADERS} PARENT_SCOPE)

# 创建用户界面静态库
add_library(3Drawing_UI STATIC
    ${UI_ALL_SOURCES}
    ${UI_ALL_HEADERS}
)

# 设置包含目录
target_include_directories(3Drawing_UI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/interaction
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/geo3D
    ${CMAKE_SOURCE_DIR}/src/core/geo3D/geometry
    ${CMAKE_SOURCE_DIR}/src/core/geo3D/managers
    ${CMAKE_SOURCE_DIR}/src/core/camera
    ${CMAKE_SOURCE_DIR}/src/core/picking
    ${CMAKE_SOURCE_DIR}/src/util
)

# 链接依赖
target_link_libraries(3Drawing_UI PUBLIC
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
    osgQOpenGL
    glm::glm
    3Drawing_Interaction
) 