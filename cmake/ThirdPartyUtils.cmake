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
    set(LIB_PATH_BASE "${ROOT_PATH}/lib/${ARCH_SUBDIR}")
    
    set(DEBUG_LIB_PATH "${LIB_PATH_BASE}/debug")
    set(RELEASE_LIB_PATH "${LIB_PATH_BASE}/release")

    # 生成查找名
    if(WIN32)
        # Windows下无论静态库还是动态库，find_library都查找.lib文件
        # 动态库的.lib是导入库，静态库的.lib是静态库本体
        set(debug_names "${LIB_TO_FIND}d.lib")
        set(release_names "${LIB_TO_FIND}.lib")
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

    # Windows动态库运行时处理
    if(WIN32 AND ARG_SHARED)
        # 查找对应的.dll文件
        set(DLL_PATH_BASE "${ROOT_PATH}/bin/${ARCH_SUBDIR}")
        set(DEBUG_DLL_PATH "${DLL_PATH_BASE}/debug")
        set(RELEASE_DLL_PATH "${DLL_PATH_BASE}/release")
        
        # 生成.dll和.pdb文件名
        set(debug_dll_names "${LIB_TO_FIND}d.dll")
        set(release_dll_names "${LIB_TO_FIND}.dll")
        set(debug_pdb_names "${LIB_TO_FIND}d.pdb")
        set(release_pdb_names "${LIB_TO_FIND}.pdb")
        
        # 查找.dll文件
        find_file(${lib_name}_DEBUG_DLL
            NAMES ${debug_dll_names}
            PATHS "${DEBUG_DLL_PATH}"
            NO_DEFAULT_PATH
        )
        
        find_file(${lib_name}_RELEASE_DLL
            NAMES ${release_dll_names}
            PATHS "${RELEASE_DLL_PATH}"
            NO_DEFAULT_PATH
        )
        
        # 查找.pdb文件
        find_file(${lib_name}_DEBUG_PDB
            NAMES ${debug_pdb_names}
            PATHS "${DEBUG_DLL_PATH}"
            NO_DEFAULT_PATH
        )
        
        find_file(${lib_name}_RELEASE_PDB
            NAMES ${release_pdb_names}
            PATHS "${RELEASE_DLL_PATH}"
            NO_DEFAULT_PATH
        )
        
        # 添加post-build步骤来复制.dll和.pdb文件到输出目录
        if(${lib_name}_DEBUG_DLL)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${${lib_name}_DEBUG_DLL}"
                "$<TARGET_FILE_DIR:${target}>"
                COMMENT "复制 ${lib_name} Debug DLL到输出目录"
            )
            message(STATUS "🔄 将在构建后复制 Debug DLL: ${${lib_name}_DEBUG_DLL}")
            
            # 如果找到对应的.pdb文件，也复制过去
            if(${lib_name}_DEBUG_PDB)
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${${lib_name}_DEBUG_PDB}"
                    "$<TARGET_FILE_DIR:${target}>"
                    COMMENT "复制 ${lib_name} Debug PDB到输出目录"
                )
                message(STATUS "🔍 将在构建后复制 Debug PDB: ${${lib_name}_DEBUG_PDB}")
            endif()
        endif()
        
        if(${lib_name}_RELEASE_DLL)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${${lib_name}_RELEASE_DLL}"
                "$<TARGET_FILE_DIR:${target}>"
                COMMENT "复制 ${lib_name} Release DLL到输出目录"
            )
            message(STATUS "🔄 将在构建后复制 Release DLL: ${${lib_name}_RELEASE_DLL}")
            
            # 如果找到对应的.pdb文件，也复制过去
            if(${lib_name}_RELEASE_PDB)
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${${lib_name}_RELEASE_PDB}"
                    "$<TARGET_FILE_DIR:${target}>"
                    COMMENT "复制 ${lib_name} Release PDB到输出目录"
                )
                message(STATUS "🔍 将在构建后复制 Release PDB: ${${lib_name}_RELEASE_PDB}")
            endif()
        endif()
    endif()

endfunction()

