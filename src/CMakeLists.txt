# 收集源文件并按目录分组
file(GLOB_RECURSE ALL_SOURCES "*.cpp")
file(GLOB_RECURSE ALL_HEADERS "*.h")

# 按目录分组源文件
set(MAIN_SOURCES main.cpp)
set(UI_SOURCES 
    ui/OSGWidget.cpp
    ui/MainWindow.cpp
)
set(UI_HEADERS
    ui/OSGWidget.h
    ui/MainWindow.h
)
set(CORE_SOURCES
    core/Common3D.cpp
    core/GeometryBase.cpp
    core/Enums3D.h
)
set(CORE_HEADERS
    core/Common3D.h
    core/GeometryBase.h
    core/Enums3D.h
)
set(GEOMETRY_SOURCES
    core/geometry/Point3D.cpp
    core/geometry/Line3D.cpp
    core/geometry/Triangle3D.cpp
    core/geometry/Quad3D.cpp
    core/geometry/Polygon3D.cpp
    core/geometry/Box3D.cpp
    core/geometry/Cube3D.cpp
    core/geometry/Sphere3D.cpp
    core/geometry/Cylinder3D.cpp
    core/geometry/Cone3D.cpp
    core/geometry/Torus3D.cpp
    core/geometry/Arc3D.cpp
    core/geometry/BezierCurve3D.cpp
)
set(GEOMETRY_HEADERS
    core/geometry/Point3D.h
    core/geometry/Line3D.h
    core/geometry/Triangle3D.h
    core/geometry/Quad3D.h
    core/geometry/Polygon3D.h
    core/geometry/Box3D.h
    core/geometry/Cube3D.h
    core/geometry/Sphere3D.h
    core/geometry/Cylinder3D.h
    core/geometry/Cone3D.h
    core/geometry/Torus3D.h
    core/geometry/Arc3D.h
    core/geometry/BezierCurve3D.h
)
set(PICKING_SOURCES
    core/picking/PickingSystem.cpp
    core/picking/PickingIntegration.cpp
    core/picking/PickingIndicator.cpp
)
set(PICKING_HEADERS
    core/picking/PickingSystem.h
    core/picking/PickingIntegration.h
    core/picking/PickingIndicator.h
)
set(UTIL_SOURCES
    util/OSGUtils.cpp
    util/GeometryFactory.cpp
    util/IndicatorFactory.cpp
    util/MathUtils.cpp
)
set(UTIL_HEADERS
    util/OSGUtils.h
    util/GeometryFactory.h
    util/IndicatorFactory.h
    util/MathUtils.h
)

# 合并所有源文件
set(ALL_SOURCES
    ${MAIN_SOURCES}
    ${UI_SOURCES}
    ${CORE_SOURCES}
    ${GEOMETRY_SOURCES}
    ${PICKING_SOURCES}
    ${UTIL_SOURCES}
)

set(ALL_HEADERS
    ${UI_HEADERS}
    ${CORE_HEADERS}
    ${GEOMETRY_HEADERS}
    ${PICKING_HEADERS}
    ${UTIL_HEADERS}
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${ALL_SOURCES} ${ALL_HEADERS})

# 设置VS文件夹结构
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "3Drawing")

# 设置源文件分组
source_group("Main" FILES ${MAIN_SOURCES})
source_group("UI" FILES ${UI_SOURCES} ${UI_HEADERS})
source_group("Core" FILES ${CORE_SOURCES} ${CORE_HEADERS})
source_group("Core\\Geometry" FILES ${GEOMETRY_SOURCES} ${GEOMETRY_HEADERS})
source_group("Core\\Picking" FILES ${PICKING_SOURCES} ${PICKING_HEADERS})
source_group("Util" FILES ${UTIL_SOURCES} ${UTIL_HEADERS})

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    OpenSceneGraph::osg
    OpenSceneGraph::osgDB
    OpenSceneGraph::osgViewer
    OpenSceneGraph::osgGA
    OpenSceneGraph::osgUtil
    OpenSceneGraph::osgText
    OpenSceneGraph::osgManipulator
    osgQt::osgQt
    glm::glm
)

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/util
)

# Windows特定设置
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    
    # 自动部署Qt
    find_program(WINDEPLOYQT NAMES windeployqt HINTS ${Qt6_DIR}/../../../bin)
    if(WINDEPLOYQT)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${WINDEPLOYQT} --no-compiler-runtime --no-translations --no-system-d3d-compiler $<TARGET_FILE:${PROJECT_NAME}>
            COMMENT "Deploying Qt libraries..."
        )
    endif()
endif()

# 设置编译选项
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
) 