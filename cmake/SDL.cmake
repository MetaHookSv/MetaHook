# Keep the original MetaHookSv SDL feature selection and static CRT/VC-LTL settings.
# SDL is built for its runtime DLLs only: the vendor SDK installation rules
# (headers, import libraries, CMake package configs, pkg-config files,
# sdl2-config and license notices) stay disabled, and the two runtime DLLs are
# installed explicitly below.
set(CMAKE_INSTALL_BINDIR ".")
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
foreach(option STATIC TEST_LIBRARY TESTS INSTALL_TESTS RENDER GPU JOYSTICK HAPTIC WASAPI UNINSTALL)
    set(SDL_${option} OFF CACHE BOOL "" FORCE)
endforeach()

# sdl2-compat consumes the SDL3::Headers target from this same build tree.
add_subdirectory(thirdparty/SDL3_fork)
set(SDL2COMPAT_INSTALL OFF CACHE BOOL "" FORCE)
foreach(option TESTS STATIC INSTALL_CPACK INSTALL_SDL3)
    set(SDL2COMPAT_${option} OFF CACHE BOOL "" FORCE)
endforeach()
add_subdirectory(thirdparty/sdl2-compat-fork)

# Group the SDL libraries under one IDE filter.
foreach(sdl_target SDL_uclibc SDL2 SDL2_test SDL2main SDL3-shared)
    if(TARGET ${sdl_target})
        set_target_properties(${sdl_target} PROPERTIES FOLDER "SDL")
    endif()
endforeach()
unset(sdl_target)

# SDL_INSTALL only gates the vendor SDK rules; the runtime DLLs are the only SDL
# artifacts this build ships, installed beside MetaHook.exe.
install(FILES $<TARGET_FILE:SDL3-shared> DESTINATION ".")
install(FILES $<TARGET_FILE:SDL2> DESTINATION ".")
