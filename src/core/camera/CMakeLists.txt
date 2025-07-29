# 相机控制模块
# 收集当前目录下的所有源文件和头文件

set(CAMERA_MODULE_SOURCES
    CameraController.cpp
)

set(CAMERA_MODULE_HEADERS
    CameraController.h
)

# 添加到父作用域
set(CAMERA_SOURCES ${CAMERA_MODULE_SOURCES} PARENT_SCOPE)
set(CAMERA_HEADERS ${CAMERA_MODULE_HEADERS} PARENT_SCOPE)

# 创建相机控制静态库
add_library(3Drawing_Camera STATIC
    ${CAMERA_MODULE_SOURCES}
    ${CAMERA_MODULE_HEADERS}
)

# 设置包含目录
target_include_directories(3Drawing_Camera PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/util
)

# 链接依赖
target_link_libraries(3Drawing_Camera PUBLIC
    Qt5::Core
    Qt5::Widgets
    Qt5::OpenGL
    unofficial::osg::osg
    unofficial::osg::osgGA
    unofficial::osg::osgViewer
    unofficial::osg::osgManipulator
    glm::glm
) 