#include <string>
#include <string_view>
#include <filesystem>

#pragma once

inline std::string getFilePath(std::string_view path) 
{
    std::filesystem::path executablePath = std::filesystem::current_path();
    if (executablePath.filename() == "build")
        executablePath = executablePath.parent_path();

    std::string fullPath = (executablePath / path).string();
    return fullPath;
}

inline std::string getRelativeFilePath(std::string fullPath)
{
    std::filesystem::path executablePath = std::filesystem::current_path();
    if (executablePath.filename() == "build")
        executablePath = executablePath.parent_path();

    std::string relativePath = fullPath.erase(0, executablePath.string().size());
    return relativePath;
}