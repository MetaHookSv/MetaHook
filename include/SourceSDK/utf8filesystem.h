#ifndef UTF8FILESYSTEM_H
#define UTF8FILESYSTEM_H

#include <windows.h>
#include <string>

// VGUI paths are UTF-8, including the strings returned by ILocalize's ANSI methods.
namespace filesystem_utf8
{
inline std::wstring ToWide(const char *path)
{
    if (!path)
        return {};

    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, nullptr, 0);
    if (!count)
        return {};

    std::wstring result(count, L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, &result[0], count))
        return {};
    result.pop_back();
    return result;
}

inline std::string FromWide(const wchar_t *path)
{
    if (!path)
        return {};

    int count = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
    if (!count)
        return {};

    std::string result(count, '\0');
    if (!WideCharToMultiByte(CP_UTF8, 0, path, -1, &result[0], count, nullptr, nullptr))
        return {};
    result.pop_back();
    return result;
}

inline HANDLE FindFirst(const char *pattern, WIN32_FIND_DATAW *data)
{
    const auto wide = ToWide(pattern);
    return wide.empty() ? INVALID_HANDLE_VALUE : FindFirstFileW(wide.c_str(), data);
}

inline DWORD GetAttributes(const char *path)
{
    const auto wide = ToWide(path);
    return wide.empty() ? INVALID_FILE_ATTRIBUTES : GetFileAttributesW(wide.c_str());
}

inline bool CreateFolderWide(const std::wstring &path)
{
    if (CreateDirectoryW(path.c_str(), nullptr))
        return true;
    DWORD error = GetLastError();
    if (error == ERROR_ALREADY_EXISTS)
    {
        DWORD attributes = GetFileAttributesW(path.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
    }
    if (error != ERROR_PATH_NOT_FOUND)
        return false;

    const auto separator = path.find_last_of(L"\\/");
    return separator != std::wstring::npos && separator > 0 &&
        CreateFolderWide(path.substr(0, separator)) && CreateDirectoryW(path.c_str(), nullptr);
}

inline bool CreateFolder(const char *path)
{
    const auto wide = ToWide(path);
    return !wide.empty() && CreateFolderWide(wide);
}

inline std::string GetWorkingDirectory()
{
    DWORD count = GetCurrentDirectoryW(0, nullptr);
    if (!count)
        return {};

    std::wstring path(count, L'\0');
    DWORD length = GetCurrentDirectoryW(count, &path[0]);
    if (!length || length >= count)
        return {};
    return FromWide(path.c_str());
}
}

#endif
