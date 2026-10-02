# Keep the original MetaHookSv SDL feature selection and static CRT/VC-LTL settings.
# Install runtime DLLs beside MetaHook.exe; retain the vendor SDK installation rules.
set(CMAKE_INSTALL_BINDIR ".")
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_INSTALL ON CACHE BOOL "" FORCE)
foreach(option STATIC TEST_LIBRARY TESTS INSTALL_TESTS RENDER GPU JOYSTICK HAPTIC WASAPI UNINSTALL)
    set(SDL_${option} OFF CACHE BOOL "" FORCE)
endforeach()

# sdl2-compat consumes the SDL3::Headers target from this same build tree.
add_subdirectory(thirdparty/SDL3_fork)
set(SDL2COMPAT_INSTALL ON CACHE BOOL "" FORCE)
foreach(option TESTS STATIC INSTALL_CPACK INSTALL_SDL3)
    set(SDL2COMPAT_${option} OFF CACHE BOOL "" FORCE)
endforeach()
add_subdirectory(thirdparty/sdl2-compat-fork)
