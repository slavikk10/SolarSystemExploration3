#include <string>
#include <string_view>
#include <filesystem>

#pragma once

inline std::string getFilePath(std::string_view path) {
    std::filesystem::path executablePath = std::filesystem::current_path();
    if (executablePath.filename() == "build") {
        executablePath = executablePath.parent_path();
    }

    std::string fullPath = (executablePath / path).string();

    return fullPath;
}