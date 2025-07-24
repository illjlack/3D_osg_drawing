cmake_minimum_required(VERSION 3.16)

project(3Drawing VERSION 1.0.0 LANGUAGES CXX)

# ——— 构建选项 ———
option(BUILD_TESTS "Build test suite" OFF)
option(ENABLE_DEBUG_OUTPUT "Enable debug output" ON)
option(USE_VCPKG "Use vcpkg for dependency management" ON)

# ——— 基本配置 ———
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# 禁用有问题的 windeployqt 自动部署
set(DISABLE_WINDEPLOYQT TRUE CACHE BOOL "Disable automatic Qt deployment")

# 如果你使用 vcpkg，请在调用 cmake 时传入：
#   -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake
if(USE_VCPKG AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(WARNING
        "未指定 CMAKE_TOOLCHAIN_FILE；如果你使用 vcpkg，请添加：\n"
        "  -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake\n"
        "或者设置 USE_VCPKG=OFF 来禁用 vcpkg"
    )
endif()

# 设置 vcpkg 安装目录
if (USE_VCPKG AND DEFINED CMAKE_TOOLCHAIN_FILE)
#
#  set(VCPKG_INSTALLED_DIR "${CMAKE_BINARY_DIR}/../vcpkg_installed"
#      CACHE PATH "vcpkg installed directory")
#  set(VCPKG_OVERLAY_PORTS  "${CMAKE_SOURCE_DIR}/vcpkg_ports"
#      CACHE PATH "vcpkg overlay ports")

  # —— 打印调试信息 ——
  message(STATUS "================ vcpkg ==============")
  message(STATUS "vcpkg toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
  message(STATUS "vcpkg installed dir: ${VCPKG_INSTALLED_DIR}")
  message(STATUS "vcpkg overlay ports: ${VCPKG_OVERLAY_PORTS}")
  message(STATUS "vcpkg target triplet: ${VCPKG_TARGET_TRIPLET}")
  message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
  
  # 设置OSG插件路径
  if (WIN32)
    set(OSG_PLUGIN_PATH "${VCPKG_INSTALLED_DIR}/${_vcpkg_triplet}/plugins/osgPlugins-${OpenSceneGraph_VERSION}")
    set(OSG_LIBRARY_PATH "${VCPKG_INSTALLED_DIR}/${_vcpkg_triplet}/bin")
  else()
    set(OSG_PLUGIN_PATH "${VCPKG_INSTALLED_DIR}/${_vcpkg_triplet}/lib/osgPlugins-${OpenSceneGraph_VERSION}")
    set(OSG_LIBRARY_PATH "${VCPKG_INSTALLED_DIR}/${_vcpkg_triplet}/lib")
  endif()
  
  message(STATUS "OSG Plugin Path: ${OSG_PLUGIN_PATH}")
  message(STATUS "OSG Library Path: ${OSG_LIBRARY_PATH}")
endif()

# ——— 查找依赖 ———
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui OpenGL)
message(STATUS "找到 Qt5 版本：${Qt5_VERSION}，安装目录：${Qt5_DIR}")

find_package(unofficial-osg CONFIG REQUIRED)
message(STATUS "找到 OpenSceneGraph 版本：${OpenSceneGraph_VERSION}")

find_package(glm CONFIG REQUIRED)
# 获取 glm::glm 的包含目录属性并打印
get_target_property(_glm_inc glm::glm INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "glm 的头文件目录：${_glm_inc}")

# ——— 导入 osgQOpenGL 静态库 ———
add_library(osgQOpenGL STATIC IMPORTED GLOBAL)

# ——— 检测 32/64 位 & 操作系统 ———
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(_arch "x64")
else()
  set(_arch "x86")
endif()

if (WIN32)
  set(_os "windows")
elseif (APPLE)
  set(_os "osx")
elseif (UNIX)
  set(_os "linux")
endif()

# ——— 组合 vcpkg triplet ———
set(_vcpkg_triplet "${_arch}-${_os}")
message(STATUS "Using vcpkg triplet: ${_vcpkg_triplet}")

if (WIN32)
  # Windows 下带 d 后缀的 Debug lib
  set_target_properties(osgQOpenGL PROPERTIES
    IMPORTED_CONFIGURATIONS   "Debug;Release"
    IMPORTED_LOCATION_DEBUG   "${VCPKG_INSTALLED_DIR}/${_vcpkg_triplet}/debug/lib/osgQOpenGLd${CMAKE_STATIC_LIBRARY_SUFFIX}"
    IMPORTED_LOCATION_RELEASE "${VCPKG_INSTALLED_DIR}/${_vcpkg_triplet}/lib/osgQOpenGL${CMAKE_STATIC_LIBRARY_SUFFIX}"
    INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INSTALLED_DIR}/${_vcpkg_triplet}/include/osgQOpenGL"
  )
else()
  # Linux/macOS 下只有 libosgQOpenGL.a
  set(_osgQLib "${VCPKG_INSTALLED_DIR}/${_vcpkg_triplet}/lib/libosgQOpenGL${CMAKE_STATIC_LIBRARY_SUFFIX}")
  set_target_properties(osgQOpenGL PROPERTIES
    IMPORTED_CONFIGURATIONS   "Debug;Release"
    IMPORTED_LOCATION_DEBUG   "${_osgQLib}"
    IMPORTED_LOCATION_RELEASE "${_osgQLib}"
    INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INSTALLED_DIR}/${_vcpkg_triplet}/include/osgQOpenGL"
  )
endif()

# ——— 源文件收集 ———
# 按目录分组源文件
set(MAIN_SOURCES
    src/main.cpp
)
set(UI_SOURCES
    src/ui/OSGWidget.cpp
    src/ui/MainWindow.cpp
    src/ui/CoordinateSystemDialog.cpp
    src/ui/LogOutputWidget.cpp
    src/ui/StatusBar3D.cpp
    src/ui/PropertyEditor3D.cpp
    src/ui/ToolPanel3D.cpp
    src/ui/ImportInfoDialog.cpp
)
set(UI_HEADERS
    src/ui/OSGWidget.h
    src/ui/MainWindow.h
    src/ui/CoordinateSystemDialog.h
    src/ui/LogOutputWidget.h
    src/ui/StatusBar3D.h
    src/ui/PropertyEditor3D.h
    src/ui/ToolPanel3D.h
    src/ui/ImportInfoDialog.h
)
set(CORE_SOURCES
    src/core/Common3D.cpp
    src/core/GeometryBase.cpp
    src/core/ConstraintSystem.cpp
    src/core/world/CoordinateSystem3D.cpp
    src/core/world/CoordinateSystemRenderer.cpp
    src/core/world/Skybox.cpp
    src/core/camera/CameraController.cpp
    src/core/managers/GeoControlPointManager.cpp
    src/core/managers/GeoNodeManager.cpp
    src/core/managers/GeoRenderManager.cpp
    src/core/managers/GeoStateManager.cpp
)
set(CORE_HEADERS
    src/core/Common3D.h
    src/core/GeometryBase.h
    src/core/Enums3D.h
    src/core/ConstraintSystem.h
    src/core/world/CoordinateSystem3D.h
    src/core/world/CoordinateSystemRenderer.h
    src/core/world/Skybox.h
    src/core/camera/CameraController.h
    src/core/managers/GeoControlPointManager.h
    src/core/managers/GeoNodeManager.h
    src/core/managers/GeoRenderManager.h
    src/core/managers/GeoStateManager.h
)
set(GEOMETRY_SOURCES
    src/core/geometry/Point3D.cpp
    src/core/geometry/Line3D.cpp
    src/core/geometry/Triangle3D.cpp
    src/core/geometry/Quad3D.cpp
    src/core/geometry/Polygon3D.cpp
    src/core/geometry/Box3D.cpp
    src/core/geometry/Cube3D.cpp
    src/core/geometry/Sphere3D.cpp
    src/core/geometry/Cylinder3D.cpp
    src/core/geometry/Cone3D.cpp
    src/core/geometry/Torus3D.cpp
    src/core/geometry/Arc3D.cpp
    src/core/geometry/BezierCurve3D.cpp
    src/core/geometry/UndefinedGeo3D.cpp
    src/core/geometry/Ellipsoid3D.cpp
    src/core/geometry/Hemisphere3D.cpp
    src/core/geometry/Prism3D.cpp
)
set(GEOMETRY_HEADERS
    src/core/geometry/Point3D.h
    src/core/geometry/Line3D.h
    src/core/geometry/Triangle3D.h
    src/core/geometry/Quad3D.h
    src/core/geometry/Polygon3D.h
    src/core/geometry/Box3D.h
    src/core/geometry/Cube3D.h
    src/core/geometry/Sphere3D.h
    src/core/geometry/Cylinder3D.h
    src/core/geometry/Cone3D.h
    src/core/geometry/Torus3D.h
    src/core/geometry/Arc3D.h
    src/core/geometry/BezierCurve3D.h
    src/core/geometry/UndefinedGeo3D.h
    src/core/geometry/Ellipsoid3D.h
    src/core/geometry/Hemisphere3D.h
    src/core/geometry/Prism3D.h
)
set(PICKING_SOURCES
    src/core/picking/GeometryPickingSystem.cpp
    src/core/picking/PickingIndicator.cpp
)
set(PICKING_HEADERS
    src/core/picking/GeometryPickingSystem.h
    src/core/picking/PickingIndicator.h
    src/core/picking/PickingTypes.h
)
set(UTIL_SOURCES
    src/util/OSGUtils.cpp
    src/util/GeometryFactory.cpp
    src/util/MathUtils.cpp
    src/util/LogManager.cpp
    src/util/GeoOsgbIO.cpp
)
set(UTIL_HEADERS
    src/util/OSGUtils.h
    src/util/GeometryFactory.h
    src/util/MathUtils.h
    src/util/LogManager.h
    src/util/GeoOsgbIO.h
)

# 建筑物相关文件
set(BUILDINGS_SOURCES
    src/core/buildings/BuildingFactory.cpp
    src/core/buildings/DomeHouse3D.cpp
    src/core/buildings/FlatHouse3D.cpp
    src/core/buildings/GableHouse3D.cpp
    src/core/buildings/LHouse3D.cpp
    src/core/buildings/SpireHouse3D.cpp
)
set(BUILDINGS_HEADERS
    src/core/buildings/BuildingFactory.h
    src/core/buildings/DomeHouse3D.h
    src/core/buildings/FlatHouse3D.h
    src/core/buildings/GableHouse3D.h
    src/core/buildings/LHouse3D.h
    src/core/buildings/SpireHouse3D.h
)

# 合并所有源文件和头文件
set(ALL_SOURCES
    ${MAIN_SOURCES}
    ${UI_SOURCES}
    ${CORE_SOURCES}
    ${GEOMETRY_SOURCES}
    ${BUILDINGS_SOURCES}
    ${PICKING_SOURCES}
    ${UTIL_SOURCES}
)
set(ALL_HEADERS
    ${UI_HEADERS}
    ${CORE_HEADERS}
    ${GEOMETRY_HEADERS}
    ${BUILDINGS_HEADERS}
    ${PICKING_HEADERS}
    ${UTIL_HEADERS}
)

# ——— 可执行目标 ———
add_executable(${PROJECT_NAME}
    ${ALL_SOURCES}
    ${ALL_HEADERS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::OpenGL

    unofficial::osg::osg
    unofficial::osg::osgDB
    unofficial::osg::osgViewer
    unofficial::osg::osgManipulator
    unofficial::osg::osgGA
    unofficial::osg::osgUtil
    unofficial::osg::osgText

    osgQOpenGL

    glm::glm
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/world
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/util
    ${VCPKG_INSTALLED_DIR}/x64-windows/include
)

# 设置OSG插件路径环境变量
if (USE_VCPKG AND DEFINED CMAKE_TOOLCHAIN_FILE)
    # 设置OSG插件路径环境变量
    set(OSG_PLUGIN_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/osgPlugins-3.6.5")
    set(OSG_LIBRARY_PATH "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
    
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        OSG_PLUGIN_PATH="${OSG_PLUGIN_PATH}"
        OSG_LIBRARY_PATH="${OSG_LIBRARY_PATH}"
    )
    
    # 在运行时设置环境变量
    if (WIN32)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            VS_DEBUGGER_ENVIRONMENT "OSG_PLUGIN_PATH=${OSG_PLUGIN_PATH};OSG_LIBRARY_PATH=${OSG_LIBRARY_PATH}"
        )
    endif()
    
    message(STATUS "设置OSG插件路径: ${OSG_PLUGIN_PATH}")
    message(STATUS "设置OSG库路径: ${OSG_LIBRARY_PATH}")
endif()

# ——— 编译选项 ———
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    $<$<BOOL:${ENABLE_DEBUG_OUTPUT}>:ENABLE_DEBUG_OUTPUT>
    QT_NO_DEBUG_OUTPUT
    _CRT_SECURE_NO_WARNINGS  # 禁用MSVC安全警告
    GLM_FORCE_CTOR_INIT  # 强制GLM构造函数初始化
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3 /MP /utf-8 /permissive-
        $<$<CONFIG:Release>:/Ox /GL>
        $<$<CONFIG:Debug>:/Zi /Od>
    )
    # 链接时优化
    set_target_properties(${PROJECT_NAME} PROPERTIES
        $<$<CONFIG:Release>:LINK_FLAGS "/LTCG">
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unknown-pragmas
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
        $<$<CONFIG:Debug>:-g -O0>
    )
endif()

# ——— 平台特定设置 ———
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

    if(NOT DISABLE_WINDEPLOYQT)
        find_program(WINDEPLOYQT NAMES windeployqt HINTS ${Qt5_DIR}/../../../bin)
        if(WINDEPLOYQT)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${WINDEPLOYQT} --no-compiler-runtime --no-translations --no-system-d3d-compiler $<TARGET_FILE:${PROJECT_NAME}>
                COMMENT "Deploying Qt with windeployqt…"
            )
        endif()
    else()
        message(STATUS "已禁用 windeployqt 自动部署")
    endif()
endif()

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist"
    )
    find_program(MACDEPLOYQT NAMES macdeployqt HINTS ${Qt5_DIR}/../../../bin)
    if(MACDEPLOYQT)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${MACDEPLOYQT} $<TARGET_FILE:${PROJECT_NAME}>
            COMMENT "Deploying Qt with macdeployqt…"
        )
    endif()
endif()

# ——— 安装 & 打包 ———
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CPack)
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "3Drawing Team")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenSceneGraph-based 3D Drawing Application")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
else()
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
endif()

# 构建信息输出
message(STATUS "—— 3Drawing Configuration ——")
message(STATUS "  Build Type     : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard   : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Tests    : ${BUILD_TESTS}")
message(STATUS "  Debug Output   : ${ENABLE_DEBUG_OUTPUT}")
message(STATUS "  Use vcpkg      : ${USE_VCPKG}")
message(STATUS "  Qt Version     : ${Qt5_VERSION}")
message(STATUS "  Qt Prefix      : ${QT5_PREFIX}")
message(STATUS "  OSG Version    : ${OpenSceneGraph_VERSION}")
message(STATUS "  glm Target     : glm::glm")
message(STATUS "  Platform       : ${CMAKE_SYSTEM_NAME}")
message(STATUS "———————— End ————————")

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 设置 VS 文件夹结构
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "3Drawing")

# 设置源文件分组
source_group("Main" FILES ${MAIN_SOURCES})
source_group("UI" FILES ${UI_SOURCES} ${UI_HEADERS})
source_group("Core" FILES ${CORE_SOURCES} ${CORE_HEADERS})
source_group("Core\\World" FILES 
    src/core/world/CoordinateSystem3D.cpp
    src/core/world/CoordinateSystem3D.h
    src/core/world/CoordinateSystemRenderer.cpp
    src/core/world/CoordinateSystemRenderer.h
    src/core/world/Skybox.cpp
    src/core/world/Skybox.h
)
source_group("Core\\Geometry" FILES ${GEOMETRY_SOURCES} ${GEOMETRY_HEADERS})
source_group("Core\\Picking" FILES ${PICKING_SOURCES} ${PICKING_HEADERS})
source_group("Util" FILES ${UTIL_SOURCES} ${UTIL_HEADERS})
