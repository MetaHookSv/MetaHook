cmake_minimum_required(VERSION 3.21)
project(MetaHookGameDataTests LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "MetaHook tests require Windows MSVC x86 (-A Win32).")
endif()
enable_testing()
set(METAHOOK_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
add_library(client_orig SHARED alias_fixture.cpp)
add_library(client_org SHARED alias_fixture.cpp)
target_compile_definitions(client_org PRIVATE FIXTURE_VALUE=2)
add_executable(gamedata_alias_tests gamedata_alias_tests.cpp)
target_compile_features(gamedata_alias_tests PRIVATE cxx_std_20)
target_compile_options(gamedata_alias_tests PRIVATE /UNDEBUG /wd4005 /wd4996)
target_include_directories(gamedata_alias_tests PRIVATE
    "${METAHOOK_ROOT}/include" "${METAHOOK_ROOT}/include/Interface"
    "${METAHOOK_ROOT}/include/HLSDK/common" "${METAHOOK_ROOT}/include/HLSDK/cl_dll"
    "${METAHOOK_ROOT}/include/HLSDK/engine" "${METAHOOK_ROOT}/include/HLSDK/pm_shared"
    "${METAHOOK_ROOT}/include/HLSDK/public" "${METAHOOK_ROOT}/thirdparty/rapidjson/include"
    "${METAHOOK_ROOT}/thirdparty/Chocobo1Hash/src")
add_dependencies(gamedata_alias_tests client_orig client_org)
add_test(NAME gamedata_alias COMMAND gamedata_alias_tests)

add_executable(blob_import_tests blob_import_tests.cpp)
target_compile_definitions(blob_import_tests PRIVATE METAHOOK_BLOB_SUPPORT)
target_compile_options(blob_import_tests PRIVATE /wd4996)
target_include_directories(blob_import_tests PRIVATE
    "${METAHOOK_ROOT}/include" "${METAHOOK_ROOT}/include/Interface"
    "${METAHOOK_ROOT}/include/HLSDK/common" "${METAHOOK_ROOT}/include/HLSDK/cl_dll"
    "${METAHOOK_ROOT}/include/HLSDK/engine" "${METAHOOK_ROOT}/include/HLSDK/pm_shared"
    "${METAHOOK_ROOT}/include/HLSDK/public")
add_test(NAME blob_imports COMMAND blob_import_tests)
