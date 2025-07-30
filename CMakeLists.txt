# cmake -B build -DCMAKE_PREFIX_PATH=F:/thirdparty/frameworks/Qt5.15.12/5.15.2/msvc2019_64
# cmake --build build --config Debug

cmake_minimum_required(VERSION 3.16)

project(3Drawing VERSION 1.0.0 LANGUAGES CXX)

# 引入第三方库工具函数
include(${CMAKE_SOURCE_DIR}/cmake/ThirdPartyUtils.cmake)

# ——— 基本配置 ———
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
set(THIRD_PARTY_ROOT "F:/thirdparty")
set(THIRD_PARTY_INCLUDE_DIR "${THIRD_PARTY_ROOT}/include")

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

# 链接Qt5
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::OpenGL
)

# 链接OSG和其他第三方库
link_third_party(${PROJECT_NAME} osg ROOT "${THIRD_PARTY_ROOT}" REQUIRED STATIC)
link_third_party(${PROJECT_NAME} osgDB ROOT "${THIRD_PARTY_ROOT}" REQUIRED STATIC)
link_third_party(${PROJECT_NAME} osgViewer ROOT "${THIRD_PARTY_ROOT}" REQUIRED STATIC)
link_third_party(${PROJECT_NAME} osgGA ROOT "${THIRD_PARTY_ROOT}" REQUIRED STATIC)
link_third_party(${PROJECT_NAME} osgUtil ROOT "${THIRD_PARTY_ROOT}" REQUIRED STATIC)
link_third_party(${PROJECT_NAME} osgText ROOT "${THIRD_PARTY_ROOT}" REQUIRED STATIC)
link_third_party(${PROJECT_NAME} osgQOpenGL ROOT "${THIRD_PARTY_ROOT}" REQUIRED STATIC)
link_third_party(${PROJECT_NAME} glm ROOT "${THIRD_PARTY_ROOT}" REQUIRED STATIC)