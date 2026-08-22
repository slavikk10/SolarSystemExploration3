#include <string>
#include <string_view>
#include <filesystem>
#ifdef __APPLE__
    #include <mach-o/dyld.h>
#elifdef _WIN32
    #include <windows.h>
#elifdef __linux__
    #include <unistd.h>
#endif

#pragma once

inline std::string getFilePath(std::string_view path)
{
    char execPath[1024];
    uint32_t size = sizeof(execPath);

#ifdef __APPLE__
    _NSGetExecutablePath(execPath, &size);
#elifdef _WIN32
    GetModuleFileName(nullptr, execPath, &size);
#elifdef __linux__
    readlink("/proc/self/exe", execPath, size);
#endif

    std::filesystem::path executablePath = execPath;

    std::string fullPath = ((executablePath.parent_path().filename() == "build" ? executablePath.parent_path().parent_path() : executablePath.parent_path()) / path).string();
    return fullPath;
}

inline std::string getRelativeFilePath(std::string fullPath)
{
    char execPath[1024];
    uint32_t size = sizeof(execPath);

#ifdef __APPLE__
    _NSGetExecutablePath(execPath, &size);
#elifdef _WIN32
    GetModuleFileName(nullptr, execPath, &size);
#elifdef __linux__
    readlink("/proc/self/exe", execPath, size);
#endif

    std::filesystem::path executablePath = execPath;
    return std::filesystem::relative(fullPath, executablePath).string();
}
