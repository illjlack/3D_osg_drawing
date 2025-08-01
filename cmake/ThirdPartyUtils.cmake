function(link_third_party target lib_name)
    set(options REQUIRED STATIC SHARED)
    set(oneValueArgs ROOT INCLUDE_SUBDIR LIB_NAME ARCH_SUBDIR)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "" ${ARGN})

    # 第三方根路径
    set(ROOT_PATH "${ARG_ROOT}")
    if(NOT ROOT_PATH)
        set(ROOT_PATH "${CMAKE_SOURCE_DIR}/third_party/install")
    endif()

    # 平台架构路径（如 x64）
    set(ARCH_SUBDIR "${ARG_ARCH_SUBDIR}")
    if(NOT ARCH_SUBDIR)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(ARCH_SUBDIR "x64")
        else()
            set(ARCH_SUBDIR "x86")
        endif()
    endif()

    # 头文件路径
    set(INCLUDE_PATH "${ROOT_PATH}/include")
    if(ARG_INCLUDE_SUBDIR)
        set(INCLUDE_PATH "${INCLUDE_PATH}/${ARG_INCLUDE_SUBDIR}")
    endif()
    target_include_directories(${target} PRIVATE "${INCLUDE_PATH}")

    # 要查找的库名
    if(ARG_LIB_NAME)
        set(LIB_TO_FIND "${ARG_LIB_NAME}")
    else()
        set(LIB_TO_FIND "${lib_name}")
    endif()

    # 动态选择 debug/release 库路径
    if(WIN32)
        if(ARG_SHARED)
            # Windows 动态库在 bin 目录
            set(LIB_PATH_BASE "${ROOT_PATH}/bin/${ARCH_SUBDIR}")
        else()
            # Windows 静态库在 lib 目录
            set(LIB_PATH_BASE "${ROOT_PATH}/lib/${ARCH_SUBDIR}")
        endif()
    else()
        # 其他系统使用默认路径
        set(LIB_PATH_BASE "${ROOT_PATH}/lib/${ARCH_SUBDIR}")
    endif()
    
    set(DEBUG_LIB_PATH "${LIB_PATH_BASE}/debug")
    set(RELEASE_LIB_PATH "${LIB_PATH_BASE}/release")

    # 生成查找名
    if(WIN32)
        if(ARG_STATIC)
            set(debug_names "${LIB_TO_FIND}d.lib")
            set(release_names "${LIB_TO_FIND}.lib")
        elseif(ARG_SHARED)
            set(debug_names "${LIB_TO_FIND}d.dll")
            set(release_names "${LIB_TO_FIND}.dll")
        else()
            set(debug_names "${LIB_TO_FIND}d.lib")
            set(release_names "${LIB_TO_FIND}.lib")
        endif()
    else()
        if(ARG_STATIC)
            set(debug_names "lib${LIB_TO_FIND}d.a")
            set(release_names "lib${LIB_TO_FIND}.a")
        elseif(ARG_SHARED)
            set(debug_names "lib${LIB_TO_FIND}d.so")
            set(release_names "lib${LIB_TO_FIND}.so")
        else()
            set(debug_names "lib${LIB_TO_FIND}d.a")
            set(release_names "lib${LIB_TO_FIND}.a")
        endif()
    endif()

    # 查找 debug 和 release 库
    find_library(${lib_name}_DEBUG
        NAMES ${debug_names}
        PATHS "${DEBUG_LIB_PATH}"
        NO_DEFAULT_PATH
    )

    find_library(${lib_name}_RELEASE
        NAMES ${release_names}
        PATHS "${RELEASE_LIB_PATH}"
        NO_DEFAULT_PATH
    )

    # 报错处理
    if(ARG_REQUIRED)
        if(NOT ${lib_name}_DEBUG)
            message(FATAL_ERROR "❌ 找不到 Debug 库 '${lib_name} (${debug_names}) '，路径: ${DEBUG_LIB_PATH}")
        endif()
        if(NOT ${lib_name}_RELEASE)
            message(FATAL_ERROR "❌ 找不到 Release 库 '${lib_name}(${release_names})'，路径: ${RELEASE_LIB_PATH}")
        endif()
    endif()

    # 链接
    if(${lib_name}_DEBUG OR ${lib_name}_RELEASE)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_link_libraries(${target} PRIVATE ${${lib_name}_DEBUG})
            message(STATUS "✅ 成功链接 ${lib_name}，Debug: ${${lib_name}_DEBUG}")
        else()
            target_link_libraries(${target} PRIVATE ${${lib_name}_RELEASE})
            message(STATUS "✅ 成功链接 ${lib_name}，Release: ${${lib_name}_RELEASE}")
        endif()
	
    endif()

endfunction()

