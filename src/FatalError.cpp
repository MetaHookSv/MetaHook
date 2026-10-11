#include "FatalError.h"
#include "ICommandLine.h"

#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>

decltype(&MessageBoxA)    g_pfnFatalErrorMessageBoxA   = MessageBoxA;
SDLShowSimpleMessageBoxFn g_pfnFatalErrorSDLMessageBox = nullptr;

static std::wstring MH_FatalErrorLogPath()
{
    // Anchor to the launcher rather than cwd: engines can change cwd at runtime.
    constexpr size_t MAX_WINDOWS_PATH_LENGTH = 32768;
    wchar_t          executable[MAX_WINDOWS_PATH_LENGTH];
    DWORD            length = GetModuleFileNameW(nullptr, executable, ARRAYSIZE(executable));
    if (!length || length == ARRAYSIZE(executable))
        return {};

    std::wstring path(executable, length);
    const auto   separator = path.find_last_of(L"\\/");
    if (separator == std::wstring::npos)
        return {};
    path.resize(separator + 1);

    SYSTEMTIME time;
    GetLocalTime(&time);
    wchar_t filename[96];
    swprintf_s(filename, L"fatalerror-%04u%02u%02u-%02u%02u%02u-%03u-%lu.log",
               time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
               time.wSecond, time.wMilliseconds, GetCurrentProcessId());
    return path + filename;
}

bool MH_RedirectFatalError(const char* title, const char* message)
{
    if (!CommandLine()->CheckParm("-redirect_fatalerror_to_log"))
        return false;

    static std::mutex                 mutex;
    const std::lock_guard<std::mutex> lock(mutex);
    static const std::wstring         path = MH_FatalErrorLogPath();

    // Always suppress the dialog when requested, even on a read-only directory.
    HANDLE file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                              OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        OutputDebugStringA("MetaHook: Could not open fatal error log.\n");
        return true;
    }

    SYSTEMTIME time;
    GetLocalTime(&time);
    char timestamp[64];
    snprintf(timestamp, sizeof(timestamp), "[%04u-%02u-%02u %02u:%02u:%02u.%03u]\r\n",
             time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
             time.wSecond, time.wMilliseconds);
    const std::string record = std::string(timestamp) + (title ? title : "Fatal Error") + "\r\n" +
        (message ? message : "") + "\r\n\r\n";
    DWORD written = 0;
    if (!WriteFile(file, record.data(), static_cast<DWORD>(record.size()), &written, nullptr) || written != record.size())
        OutputDebugStringA("MetaHook: Could not write fatal error log.\n");
    FlushFileBuffers(file);
    CloseHandle(file);
    return true;
}

int WINAPI MH_FatalErrorMessageBoxA(HWND window, const char* message, const char* title, UINT flags)
{
    const bool error = (flags & MB_ICONMASK) == MB_ICONERROR ||
        (title && (!_stricmp(title, "Fatal Error") || !_stricmp(title, "Error")));
    if ((flags & MB_TYPEMASK) == MB_OK && error && MH_RedirectFatalError(title, message))
        return IDOK;
    return g_pfnFatalErrorMessageBoxA(window, message, title, flags);
}

int __cdecl MH_FatalErrorSDLMessageBox(uint32_t flags, const char* title, const char* message, SDL_Window* window)
{
    if ((flags & MH_SDL_MESSAGEBOX_ERROR) && MH_RedirectFatalError(title, message))
        return 0; // SDL2 reports success with zero, unlike SDL3.
    return g_pfnFatalErrorSDLMessageBox(flags, title, message, window);
}
