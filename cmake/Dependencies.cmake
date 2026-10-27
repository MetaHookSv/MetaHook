function(metahook_prepare_dependencies)
    set(modules Detours_fork capstone_fork rapidjson Chocobo1Hash Musa.Veil_fork MemoryModulePP)
    if(METAHOOK_BUILD_SDL)
        list(APPEND modules SDL3 sdl2-compat)
    endif()
    foreach(module IN LISTS modules)
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
endfunction()
