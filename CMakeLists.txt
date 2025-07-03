cmake_minimum_required(VERSION 3.20)

project(3Drawing VERSION 1.0.0 LANGUAGES CXX)

# ——— 基本配置 ———
set(CMAKE_CXX_STANDARD            17)
set(CMAKE_CXX_STANDARD_REQUIRED   ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# ——— Qt5 路径设置 ———
# 优先使用全局安装的Qt5
set(QT5_PREFIX "D:/Qt5.15/5.15.2/msvc2019_64" CACHE PATH "Qt5 installation prefix")
if(EXISTS "${QT5_PREFIX}")
    set(CMAKE_PREFIX_PATH "${QT5_PREFIX};${CMAKE_PREFIX_PATH}")
    message(STATUS "使用全局Qt5: ${QT5_PREFIX}")
else()
    message(WARNING "未找到全局Qt5，将尝试使用vcpkg的Qt5")
endif()

# 禁用有问题的 windeployqt 自动部署
set(DISABLE_WINDEPLOYQT TRUE CACHE BOOL "Disable automatic Qt deployment")

# 如果你使用 vcpkg，请在调用 cmake 时传入：
#   -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake
if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  message(WARNING
    "未指定 CMAKE_TOOLCHAIN_FILE；如果你使用 vcpkg，请添加：\n"
    "  -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake"
  )
endif()

# 设置vcpkg使用全局缓存，避免重复安装
if (DEFINED CMAKE_TOOLCHAIN_FILE)
  set(VCPKG_INSTALLED_DIR "${CMAKE_BINARY_DIR}/../vcpkg_installed" CACHE PATH "vcpkg installed directory")
  set(VCPKG_OVERLAY_PORTS "${CMAKE_SOURCE_DIR}/vcpkg_ports" CACHE PATH "vcpkg overlay ports")
endif()

# ——— 查找依赖 ———
find_package(Qt5      REQUIRED COMPONENTS Core Widgets Gui OpenGL)
find_package(unofficial-osg CONFIG REQUIRED)
find_package(glm      CONFIG REQUIRED)

# 创建 osgQOpenGL 的 IMPORTED target，支持 Debug 和 Release 自动切换
add_library(osgQOpenGL STATIC IMPORTED GLOBAL)
set_target_properties(osgQOpenGL PROPERTIES
  IMPORTED_CONFIGURATIONS "Debug;Release"
  IMPORTED_LOCATION_DEBUG "${VCPKG_INSTALLED_DIR}/x64-windows/debug/lib/osgQOpenGLd.lib"
  IMPORTED_LOCATION_RELEASE "${VCPKG_INSTALLED_DIR}/x64-windows/lib/osgQOpenGL.lib"
  INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INSTALLED_DIR}/x64-windows/include/osgQOpenGL"
)

# ——— 源文件收集 ———
# 按目录分组源文件
set(MAIN_SOURCES 
    src/main.cpp
)
set(UI_SOURCES 
    src/ui/OSGWidget.cpp
    src/ui/MainWindow.cpp
)
set(UI_HEADERS
    src/ui/OSGWidget.h
    src/ui/MainWindow.h
)
set(CORE_SOURCES
    src/core/Common3D.cpp
    src/core/GeometryBase.cpp
)
set(CORE_HEADERS
    src/core/Common3D.h
    src/core/GeometryBase.h
    src/core/Enums3D.h
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
)
set(PICKING_SOURCES
    src/core/picking/PickingSystem.cpp
    src/core/picking/PickingIntegration.cpp
    src/core/picking/PickingIndicator.cpp
)
set(PICKING_HEADERS
    src/core/picking/PickingSystem.h
    src/core/picking/PickingIntegration.h
    src/core/picking/PickingIndicator.h
)
set(UTIL_SOURCES
    src/util/OSGUtils.cpp
    src/util/GeometryFactory.cpp
    src/util/IndicatorFactory.cpp
    src/util/MathUtils.cpp
)
set(UTIL_HEADERS
    src/util/OSGUtils.h
    src/util/GeometryFactory.h
    src/util/IndicatorFactory.h
    src/util/MathUtils.h
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

# ——— 可执行目标 ———
add_executable(${PROJECT_NAME}
  ${ALL_SOURCES}
  ${ALL_HEADERS}
)

# ——— 链接 & 包含 ———
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

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/util
    ${VCPKG_INSTALLED_DIR}/x64-windows/include
)

# ——— 编译开关 ———
target_compile_definitions(${PROJECT_NAME} PRIVATE
  $<$<CONFIG:Debug>:DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
  QT_NO_DEBUG_OUTPUT
)

if (MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE
    /W3 /MP /utf-8
    $<$<CONFIG:Release>:/Ox>
  )
else()
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-g>
  )
endif()

# ——— 平台特定设置 ———
if (WIN32)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
  )

  # 只有在明确启用时才执行 windeployqt
  if(NOT DISABLE_WINDEPLOYQT)
    if(EXISTS "${QT5_PREFIX}/bin/windeployqt.exe")
      set(WINDEPLOYQT "${QT5_PREFIX}/bin/windeployqt.exe")
    else()
      find_program(WINDEPLOYQT NAMES windeployqt HINTS ${Qt5_DIR}/../../../bin)
    endif()

    if (WINDEPLOYQT)
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${WINDEPLOYQT} --no-compiler-runtime --no-translations --no-system-d3d-compiler $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "Deploying Qt with windeployqt…"
      )
    endif()
  else()
    message(STATUS "已禁用 windeployqt 自动部署")
  endif()
endif()

if (APPLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist"
  )

  find_program(MACDEPLOYQT NAMES macdeployqt HINTS ${Qt5_DIR}/../../../bin)
  if (MACDEPLOYQT)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${MACDEPLOYQT} $<TARGET_FILE:${PROJECT_NAME}>
      COMMENT "Deploying Qt with macdeployqt…"
    )
  endif()
endif()

# ——— 安装 & 打包 ———
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include(CPack)
set(CPACK_PACKAGE_NAME               "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR             "3Drawing Team")
set(CPACK_PACKAGE_VERSION            "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "OpenSceneGraph-based 3D Drawing Application"
)

if (WIN32)
  set(CPACK_GENERATOR "NSIS;ZIP")
elseif(APPLE)
  set(CPACK_GENERATOR "DragNDrop;TGZ")
else()
  set(CPACK_GENERATOR "DEB;RPM;TGZ")
endif()

# ——— 构建信息 ———
message(STATUS "—— 3Drawing Configuration ——")
message(STATUS "  Build Type     : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard   : ${CMAKE_CXX_STANDARD}")
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

# 设置VS文件夹结构
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "3Drawing")

# 设置源文件分组
source_group("Main" FILES ${MAIN_SOURCES})
source_group("UI" FILES ${UI_SOURCES} ${UI_HEADERS})
source_group("Core" FILES ${CORE_SOURCES} ${CORE_HEADERS})
source_group("Core\\Geometry" FILES ${GEOMETRY_SOURCES} ${GEOMETRY_HEADERS})
source_group("Core\\Picking" FILES ${PICKING_SOURCES} ${PICKING_HEADERS})
source_group("Util" FILES ${UTIL_SOURCES} ${UTIL_HEADERS})
