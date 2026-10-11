#pragma once

#include <Windows.h>
#include <cstdint>

struct SDL_Window;
using SDLShowSimpleMessageBoxFn = int(__cdecl*)(uint32_t, const char*, const char*, SDL_Window*);

// SDL2's public ABI; the launcher does not link against SDL.
constexpr uint32_t MH_SDL_MESSAGEBOX_ERROR = 0x00000010;

extern decltype(&MessageBoxA)    g_pfnFatalErrorMessageBoxA;
extern SDLShowSimpleMessageBoxFn g_pfnFatalErrorSDLMessageBox;

// Returns true when redirection is requested, including if writing fails.
bool        MH_RedirectFatalError(const char* title, const char* message);
int WINAPI  MH_FatalErrorMessageBoxA(HWND window, const char* message, const char* title, UINT flags);
int __cdecl MH_FatalErrorSDLMessageBox(uint32_t flags, const char* title, const char* message, SDL_Window* window);
