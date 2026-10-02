set(METAHOOK_DEPENDENCY_CACHE_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache" CACHE PATH "Downloaded dependency cache")
set(VC_LTL_Root "${METAHOOK_DEPENDENCY_CACHE_DIR}/VC-LTL-5.3.1" CACHE PATH "VC-LTL binary package root")

function(metahook_prepare_dependencies)
    foreach(module Detours_fork capstone_fork rapidjson Chocobo1Hash Musa.Veil_fork MemoryModulePP)
        if(NOT EXISTS "${PROJECT_SOURCE_DIR}/thirdparty/${module}/.git")
            find_package(Git REQUIRED)
            execute_process(
                COMMAND "${GIT_EXECUTABLE}" -C "${PROJECT_SOURCE_DIR}" submodule update --init -- "thirdparty/${module}"
                RESULT_VARIABLE result
            )
            if(NOT result EQUAL 0)
                message(FATAL_ERROR "Cannot initialize ${module}. MemoryModulePP's initial commit must be available locally or published first.")
            endif()
        endif()
    endforeach()

    set(version "5.3.1")
    set(expected_hash "7a18799ed3aa84a225610a5447a56bc534c5c98ccb8dec05caba0e3f633431ad")
    set(marker "${VC_LTL_Root}/.verified-sha256")
    file(MAKE_DIRECTORY "${METAHOOK_DEPENDENCY_CACHE_DIR}")
    # Debug and Release configurations share the package cache.
    file(LOCK "${METAHOOK_DEPENDENCY_CACHE_DIR}/VC-LTL.lock" GUARD FUNCTION TIMEOUT 300)
    if(EXISTS "${marker}" AND EXISTS "${VC_LTL_Root}/VC-LTL helper for cmake.cmake"
        AND EXISTS "${VC_LTL_Root}/TargetPlatform/6.0.6000.0/lib/Win32/libucrt.lib")
        file(READ "${marker}" verified_hash)
        string(STRIP "${verified_hash}" verified_hash)
        if(verified_hash STREQUAL expected_hash)
            message(STATUS "VC-LTL ${version} is ready")
            return()
        endif()
    endif()

    set(archive "${METAHOOK_DEPENDENCY_CACHE_DIR}/VC-LTL-${version}.7z")
    if(EXISTS "${archive}")
        file(SHA256 "${archive}" actual_hash)
        if(NOT actual_hash STREQUAL expected_hash)
            message(FATAL_ERROR "VC-LTL archive failed SHA-256 verification: ${archive}. Remove the invalid archive and configure again.")
        endif()
    else()
        set(download "${archive}.download")
        file(DOWNLOAD
            "https://github.com/Chuyu-Team/VC-LTL5/releases/download/v${version}/VC-LTL-Binary.7z"
            "${download}" EXPECTED_HASH "SHA256=${expected_hash}"
            TLS_VERIFY ON STATUS status SHOW_PROGRESS TIMEOUT 300 INACTIVITY_TIMEOUT 60
        )
        list(GET status 0 result)
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "VC-LTL download failed: ${status}")
        endif()
        file(RENAME "${download}" "${archive}")
    endif()
    file(MAKE_DIRECTORY "${VC_LTL_Root}")
    file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${VC_LTL_Root}")
    if(NOT EXISTS "${VC_LTL_Root}/VC-LTL helper for cmake.cmake"
        OR NOT EXISTS "${VC_LTL_Root}/TargetPlatform/6.0.6000.0/lib/Win32/libucrt.lib")
        message(FATAL_ERROR "VC-LTL extraction is incomplete: ${VC_LTL_Root}")
    endif()
    file(WRITE "${marker}" "${expected_hash}\n")
endfunction()
