cmake_minimum_required(VERSION 3.20)

project(3Drawing VERSION 1.0.0 LANGUAGES CXX)

# ——— 基本配置 ———
set(CMAKE_CXX_STANDARD            17)
set(CMAKE_CXX_STANDARD_REQUIRED   ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
find_package(Qt5      REQUIRED COMPONENTS Core Widgets Gui)
find_package(OpenSceneGraph CONFIG REQUIRED
  COMPONENTS osg osgDB osgViewer osgManipulator osgGA
)
find_package(osgQt    CONFIG REQUIRED)
find_package(glm      CONFIG REQUIRED)

# ——— 源文件 & 资源 ———
file(GLOB_RECURSE SRC_LIST CONFIGURE_DEPENDS
  src/*.cpp src/*.cxx
)
file(GLOB_RECURSE HDR_LIST CONFIGURE_DEPENDS
  src/*.h   src/*.hpp
)
set(RCC_LIST Drawing.qrc)

# ——— 可执行目标 ———
add_executable(${PROJECT_NAME}
  ${SRC_LIST}
  ${HDR_LIST}
  ${RCC_LIST}
)

# ——— 链接 & 包含 ———
target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt5::Core
  Qt5::Widgets
  Qt5::Gui

  OpenSceneGraph::osg
  OpenSceneGraph::osgDB
  OpenSceneGraph::osgViewer
  OpenSceneGraph::osgManipulator
  OpenSceneGraph::osgGA

  osgQt::osgQt

  glm::glm
)

# Qt MOC/UIC/RCC 已自动启用，无需额外设置

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

# ——— Windows 下自动部署 Qt ———
if (WIN32)
  find_program(WINDEPLOYQT NAMES windeployqt HINTS ${Qt5_DIR}/../../../bin)
  if (WINDEPLOYQT)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${WINDEPLOYQT} $<TARGET_FILE:${PROJECT_NAME}>
      COMMENT "Deploying Qt with windeployqt…"
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
endif()

# ——— 构建信息 ———
message(STATUS "—— 3Drawing Configuration ———")
message(STATUS "  Build Type     : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard   : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt Version     : ${Qt5_VERSION}")
message(STATUS "  OSG Version    : ${OpenSceneGraph_VERSION}")
message(STATUS "  osgQt Target   : osgQt::osgQt")
message(STATUS "  glm Target     : glm::glm")
message(STATUS "———————————— End ——————————")

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 设置包含目录
include_directories(${CMAKE_SOURCE_DIR}/src)

# 添加子目录
add_subdirectory(src)

# 设置VS文件夹结构
set_property(GLOBAL PROPERTY USE_FOLDERS ON)