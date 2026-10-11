#include "../src/FatalError.h"
#include "ICommandLine.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#define CHECK(x)                                                      \
    do                                                                \
    {                                                                 \
        if (!(x))                                                     \
        {                                                             \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); \
            std::exit(EXIT_FAILURE);                                  \
        }                                                             \
    } while (0)

static int        win32Calls = 0;
static int        sdlCalls   = 0;
static int WINAPI Win32MessageBox(HWND, const char*, const char*, UINT)
{
    ++win32Calls;
    return IDCANCEL;
}
static int __cdecl SDLMessageBox(uint32_t, const char*, const char*, SDL_Window*)
{
    ++sdlCalls;
    return -7;
}

int main()
{
    g_pfnFatalErrorMessageBoxA   = Win32MessageBox;
    g_pfnFatalErrorSDLMessageBox = SDLMessageBox;
    CommandLine()->CreateCmdLine("MetaHook.exe");
    CHECK(!MH_RedirectFatalError("Fatal Error", "disabled"));
    CHECK(IDCANCEL == MH_FatalErrorMessageBoxA(nullptr, "disabled", "Fatal Error", MB_ICONERROR));
    CHECK(-7 == MH_FatalErrorSDLMessageBox(MH_SDL_MESSAGEBOX_ERROR, "Error", "disabled", nullptr));
    CHECK(1 == win32Calls);
    CHECK(1 == sdlCalls);

    CommandLine()->CreateCmdLine("MetaHook.exe -redirect_fatalerror_to_log");
    CHECK(IDOK == MH_FatalErrorMessageBoxA(nullptr, "Win32 fatal\nsecond line", "Fatal Error", MB_ICONERROR));
    CHECK(IDOK == MH_FatalErrorMessageBoxA(nullptr, "Legacy fatal", "Fatal Error", MB_OK));
    CHECK(0 == MH_FatalErrorSDLMessageBox(MH_SDL_MESSAGEBOX_ERROR, "SDL Error", "SDL fatal", nullptr));
    CHECK(MH_RedirectFatalError(nullptr, nullptr));
    CHECK(1 == win32Calls);
    CHECK(1 == sdlCalls);

    CHECK(IDCANCEL == MH_FatalErrorMessageBoxA(nullptr, "warning", "Warning", MB_ICONWARNING));
    CHECK(IDCANCEL == MH_FatalErrorMessageBoxA(nullptr, "confirm", "Error", MB_YESNO | MB_ICONERROR));
    CHECK(-7 == MH_FatalErrorSDLMessageBox(0x20, "Warning", "warning", nullptr));
    CHECK(3 == win32Calls);
    CHECK(2 == sdlCalls);

    wchar_t executable[MAX_PATH];
    CHECK(GetModuleFileNameW(nullptr, executable, MAX_PATH));
    const auto directory = std::filesystem::path(executable).parent_path();
    // The output belongs beside the executable even if the process changes cwd.
    const auto previousDirectory = std::filesystem::current_path();
    std::filesystem::current_path(std::filesystem::temp_directory_path());
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i)
        threads.emplace_back([] { CHECK(MH_RedirectFatalError("Thread", "Concurrent fatal")); });
    for (auto& thread : threads)
        thread.join();
    std::filesystem::current_path(previousDirectory);

    const auto            suffix = "-" + std::to_string(GetCurrentProcessId()) + ".log";
    std::filesystem::path logPath;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        const auto name = entry.path().filename().string();
        if (name.starts_with("fatalerror-") && name.ends_with(suffix))
        {
            CHECK(logPath.empty());
            logPath = entry.path();
        }
    }
    CHECK(!logPath.empty());
    std::ifstream     file(logPath, std::ios::binary);
    const std::string contents((std::istreambuf_iterator<char>(file)), {});
    file.close();
    CHECK(std::string::npos != contents.find("Fatal Error\r\nWin32 fatal\nsecond line"));
    CHECK(std::string::npos != contents.find("Legacy fatal"));
    CHECK(std::string::npos != contents.find("SDL Error\r\nSDL fatal"));
    CHECK(std::string::npos == contents.find("disabled"));
    CHECK(std::string::npos == contents.find("warning"));
    size_t count = 0;
    for (size_t offset = 0; (offset = contents.find("Concurrent fatal", offset)) != std::string::npos; ++offset)
        ++count;
    CHECK(8 == count);

    // An unwritable log must not fall back to a blocking dialog.
    HANDLE lock = CreateFileW(logPath.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    CHECK(INVALID_HANDLE_VALUE != lock);
    CHECK(IDOK == MH_FatalErrorMessageBoxA(nullptr, "locked log", "Fatal Error", MB_ICONERROR));
    CHECK(0 == MH_FatalErrorSDLMessageBox(MH_SDL_MESSAGEBOX_ERROR, "Error", "locked log", nullptr));
    CHECK(3 == win32Calls);
    CHECK(2 == sdlCalls);
    CloseHandle(lock);
    CHECK(DeleteFileW(logPath.c_str()));
    std::puts("fatal error redirection tests passed");
}
