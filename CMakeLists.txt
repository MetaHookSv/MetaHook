cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(MetaHook LANGUAGES C CXX RC)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "MetaHook requires Windows MSVC x86. Configure with -A Win32.")
endif()
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported configurations" FORCE)
option(METAHOOK_BUILD_SDL "Build and install SDL2/SDL3 runtimes and SDK headers" ON)

include(cmake/Dependencies.cmake)
metahook_prepare_dependencies()
include("${VC_LTL_Root}/VC-LTL helper for cmake.cmake")

# The parent already applies VC-LTL to all dependency targets.
set(USE_VCLTL OFF CACHE BOOL "Separate vendor VC-LTL integration" FORCE)
set(CAPSTONE_ARCHITECTURE_DEFAULT OFF CACHE BOOL "" FORCE)
set(CAPSTONE_X86_SUPPORT ON CACHE BOOL "" FORCE)
set(CAPSTONE_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(CAPSTONE_BUILD_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
foreach(option BUILD_SHARED BUILD_TESTS BUILD_CSTOOL INSTALL BUILD_DIET)
    set(CAPSTONE_${option} OFF CACHE BOOL "" FORCE)
endforeach()
add_subdirectory(thirdparty/capstone_fork EXCLUDE_FROM_ALL)
# Group the vendored Capstone library under one IDE filter.
set_target_properties(capstone-static PROPERTIES FOLDER "capstone")
# This Capstone version does not publish its public include directory.
target_include_directories(capstone-static INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/capstone_fork/include")
add_subdirectory(thirdparty/MemoryModulePP EXCLUDE_FROM_ALL)
if(METAHOOK_BUILD_SDL)
    include(cmake/SDL.cmake)
endif()

add_executable(MetaHook WIN32
    src/commandline.cpp
    src/GameData.cpp
    src/LoadDllNotification.cpp
    src/launcher.cpp
    src/LoadBlob.cpp
    src/metahook.cpp
    src/registry.cpp
    src/sys_launcher.cpp
    src/Z.cpp
    src/Launcher.rc
    include/HLSDK/common/interface.cpp
    thirdparty/Detours_fork/src/detours.cpp
    thirdparty/Detours_fork/src/disasm.cpp
    thirdparty/Detours_fork/src/modules.cpp
)
target_compile_features(MetaHook PRIVATE cxx_std_20)
target_include_directories(MetaHook PRIVATE
    include include/Interface include/HLSDK/common include/HLSDK/cl_dll
    include/HLSDK/engine include/HLSDK/pm_shared include/HLSDK/public
    thirdparty/Detours_fork/src
    thirdparty/rapidjson/include
    thirdparty/Chocobo1Hash/src
)
target_compile_definitions(MetaHook PRIVATE WIN32 _WINDOWS _MBCS
    $<$<CONFIG:Debug>:_DEBUG> $<$<CONFIG:Release>:NDEBUG>)
target_compile_options(MetaHook PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:/MP;/W3;/permissive;/wd4005;/wd4091;/wd4311;/wd4312;/wd4819;/wd4996;/Zi>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Debug>>:/GS>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Release>>:/GS->
)
target_link_libraries(MetaHook PRIVATE capstone-static MemoryModule ntdll ws2_32)
target_link_options(MetaHook PRIVATE /BASE:0x1400000 /LARGEADDRESSAWARE /INCREMENTAL:NO
    /DEBUG /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' /MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/src/MetaHook.manifest
    $<$<CONFIG:Debug>:/SAFESEH:NO>
    $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
set_target_properties(MetaHook PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

option(METAHOOK_SYNC_GAMEDATA "Download and validate gamedata before building" ON)
set(METAHOOK_GAMEDATA_DIR "${PROJECT_BINARY_DIR}/assets/svencoop/metahook/gamedata" CACHE PATH "Validated gamedata directory")
if(METAHOOK_SYNC_GAMEDATA)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_custom_target(MetaHookGameData
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/sync-gamedata.py"
            --manifest "${CMAKE_CURRENT_SOURCE_DIR}/scripts/manifests/metahook.json"
            --target-dir "${METAHOOK_GAMEDATA_DIR}" --temp-root "${PROJECT_BINARY_DIR}/gamedata-sync"
        COMMENT "Synchronizing and validating MetaHook gamedata"
        VERBATIM
    )
    add_dependencies(MetaHook MetaHookGameData)
endif()
install(TARGETS MetaHook RUNTIME DESTINATION .)
install(FILES "$<TARGET_PDB_FILE:MetaHook>" DESTINATION .)
install(DIRECTORY "${METAHOOK_GAMEDATA_DIR}/" DESTINATION svencoop/metahook/gamedata OPTIONAL)
