# cmake -B build -DCMAKE_PREFIX_PATH=F:/thirdparty/frameworks/Qt5.15.12/5.15.2/msvc2019_64
# cmake -B build -DCMAKE_PREFIX_PATH=/home/liushisheng/Qt5.12.8/5.12.8/gcc_64 -DCMAKE_BUILD_TYPE=Debug
# cmake --build build 

cmake_minimum_required(VERSION 3.16)

project(3Drawing VERSION 1.0.0 LANGUAGES CXX)

# 引入第三方库工具函数
include(${CMAKE_SOURCE_DIR}/cmake/ThirdPartyUtils.cmake)

# ——— 基本配置 ———
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置调试模式编译标志
set(CMAKE_BUILD_TYPE Debug)

# 平台特定的编译标志
if(WIN32)
    # Windows特定标志：解决并行编译时PDB文件冲突
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS")
else()
    # Linux/Unix特定标志
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
endif()

# 添加GLM相关的编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLM_FORCE_CTOR_INIT")

# 设置可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Qt自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ——— 源文件收集 ———
# 主程序
set(MAIN_SOURCES src/main.cpp)

# UI模块
file(GLOB UI_SOURCES CONFIGURE_DEPENDS "src/ui/*.cpp")
file(GLOB UI_HEADERS CONFIGURE_DEPENDS "src/ui/*.h")

# Core基础模块
file(GLOB CORE_BASE_SOURCES CONFIGURE_DEPENDS "src/core/*.cpp")
file(GLOB CORE_BASE_HEADERS CONFIGURE_DEPENDS "src/core/*.h")

# Core Camera模块
file(GLOB CORE_CAMERA_SOURCES CONFIGURE_DEPENDS "src/core/camera/*.cpp")
file(GLOB CORE_CAMERA_HEADERS CONFIGURE_DEPENDS "src/core/camera/*.h")

# Core Managers模块
file(GLOB CORE_MANAGERS_SOURCES CONFIGURE_DEPENDS "src/core/managers/*.cpp")
file(GLOB CORE_MANAGERS_HEADERS CONFIGURE_DEPENDS "src/core/managers/*.h")

# Core Geometry模块
file(GLOB GEOMETRY_SOURCES CONFIGURE_DEPENDS "src/core/geometry/*.cpp")
file(GLOB GEOMETRY_HEADERS CONFIGURE_DEPENDS "src/core/geometry/*.h")

# Core Buildings模块
file(GLOB BUILDINGS_SOURCES CONFIGURE_DEPENDS "src/core/buildings/*.cpp")
file(GLOB BUILDINGS_HEADERS CONFIGURE_DEPENDS "src/core/buildings/*.h")

# Core Picking模块
file(GLOB PICKING_SOURCES CONFIGURE_DEPENDS "src/core/picking/*.cpp")
file(GLOB PICKING_HEADERS CONFIGURE_DEPENDS "src/core/picking/*.h")

# Util模块
file(GLOB UTIL_SOURCES CONFIGURE_DEPENDS "src/util/*.cpp")
file(GLOB UTIL_HEADERS CONFIGURE_DEPENDS "src/util/*.h")

# 合并所有源文件
set(ALL_SOURCES
    ${MAIN_SOURCES}
    ${UI_SOURCES}
    ${CORE_BASE_SOURCES}
    ${CORE_CAMERA_SOURCES}
    ${CORE_MANAGERS_SOURCES}
    ${GEOMETRY_SOURCES}
    ${BUILDINGS_SOURCES}
    ${PICKING_SOURCES}
    ${UTIL_SOURCES}
)

# 合并所有头文件
set(ALL_HEADERS
    ${UI_HEADERS}
    ${CORE_BASE_HEADERS}
    ${CORE_CAMERA_HEADERS}
    ${CORE_MANAGERS_HEADERS}
    ${GEOMETRY_HEADERS}
    ${BUILDINGS_HEADERS}
    ${PICKING_HEADERS}
    ${UTIL_HEADERS}
)

# ——— 可执行目标 ———
add_executable(${PROJECT_NAME} ${ALL_SOURCES} ${ALL_HEADERS})

# ——— 设置第三方库根目录 ———
if(WIN32)
    set(THIRD_PARTY_ROOT "F:/thirdparty")
else()
    set(THIRD_PARTY_ROOT "/home/liushisheng/thirdparty")
endif()
set(THIRD_PARTY_INCLUDE_DIR "${THIRD_PARTY_ROOT}/include")

if(WIN32)
    # 根据构建类型设置插件目录
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OSG_PLUGINS_DIR "${THIRD_PARTY_ROOT}/plugins/x64/debug/osgPlugins-3.6.5")
    message(STATUS "使用Debug版本OSG插件: ${OSG_PLUGINS_DIR}")
    else()
    set(OSG_PLUGINS_DIR "${THIRD_PARTY_ROOT}/plugins/x64/release/osgPlugins-3.6.5")
    message(STATUS "使用Release版本OSG插件: ${OSG_PLUGINS_DIR}")
    endif()
else()
    # 根据构建类型设置插件目录
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OSG_PLUGINS_DIR "${THIRD_PARTY_ROOT}/lib/x64/debug/osgPlugins-3.6.5")
    message(STATUS "使用Debug版本OSG插件: ${OSG_PLUGINS_DIR}")
    else()
    set(OSG_PLUGINS_DIR "${THIRD_PARTY_ROOT}/lib/x64/release/osgPlugins-3.6.5")
    message(STATUS "使用Release版本OSG插件: ${OSG_PLUGINS_DIR}")
    endif()
endif()

# 添加插件目录的编译定义（使用字符串字面量）
add_compile_definitions(OSG_PLUGIN_PATH="${OSG_PLUGINS_DIR}")

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/util
    ${THIRD_PARTY_ROOT}/include
)

# 查找Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui OpenGL)

# 链接Qt5和OpenGL
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::OpenGL
)

# 链接OSG和其他第三方库 - 使用动态库
link_third_party(${PROJECT_NAME} osgQOpenGL ROOT "${THIRD_PARTY_ROOT}" REQUIRED STATIC)# 没编osgqt的动态库
link_third_party(${PROJECT_NAME} osgViewer ROOT "${THIRD_PARTY_ROOT}" REQUIRED SHARED)
link_third_party(${PROJECT_NAME} osgGA ROOT "${THIRD_PARTY_ROOT}" REQUIRED SHARED)
link_third_party(${PROJECT_NAME} osgText ROOT "${THIRD_PARTY_ROOT}" REQUIRED SHARED)
link_third_party(${PROJECT_NAME} osgDB ROOT "${THIRD_PARTY_ROOT}" REQUIRED SHARED)
link_third_party(${PROJECT_NAME} osgUtil ROOT "${THIRD_PARTY_ROOT}" REQUIRED SHARED)
link_third_party(${PROJECT_NAME} osg ROOT "${THIRD_PARTY_ROOT}" REQUIRED SHARED)
link_third_party(${PROJECT_NAME} OpenThreads ROOT "${THIRD_PARTY_ROOT}" REQUIRED SHARED)
link_third_party(${PROJECT_NAME} glm ROOT "${THIRD_PARTY_ROOT}" REQUIRED STATIC)

# 根据操作系统添加不同的系统库
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        OpenGL32
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        GL
        dl
        pthread
        fontconfig
        freetype
        expat
        z
        jpeg
        png
        tiff
        Xrandr
        Xinerama
        Xi
        Xcursor
        X11
    )
endif()

# 设置运行时库路径，确保程序能找到OSG动态库
set_target_properties(${PROJECT_NAME} PROPERTIES
    BUILD_RPATH_USE_ORIGIN TRUE
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

# 根据构建类型设置不同的运行时库路径
target_link_directories(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:${THIRD_PARTY_ROOT}/lib/x64/debug>
    $<$<CONFIG:Release>:${THIRD_PARTY_ROOT}/lib/x64/release>
)

# 设置不同配置的RPATH
set_property(TARGET ${PROJECT_NAME} PROPERTY
    BUILD_RPATH 
    $<$<CONFIG:Debug>:${THIRD_PARTY_ROOT}/lib/x64/debug>
    $<$<CONFIG:Release>:${THIRD_PARTY_ROOT}/lib/x64/release>
)

set_property(TARGET ${PROJECT_NAME} PROPERTY
    INSTALL_RPATH
    $<$<CONFIG:Debug>:${THIRD_PARTY_ROOT}/lib/x64/debug>
    $<$<CONFIG:Release>:${THIRD_PARTY_ROOT}/lib/x64/release>
)
