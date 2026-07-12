#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>

#include <load.hpp>
#include <filesystem.hpp>

void createSaveFileMacOS(std::string name, std::string contents, std::string extension=".txt")
{
    std::filesystem::path p = std::filesystem::path(std::getenv("HOME")) / "Library/Application Support/com.vyacheslavc.SolarSystemExploration3";

    // create directory if it doesn't exist yet
    if (!(std::filesystem::exists(p) && std::filesystem::is_directory(p)))
        std::filesystem::create_directories(p);

    std::ofstream savefile(p / (name + extension), std::ios::out | std::ios::trunc); // truncation to delete file contents before writing
    
    // write to savefile
    // -----------------
    savefile << contents;

    // close the savefile
    // ------------------
    savefile.close();
}

void createSaveFileWindows(std::string name, std::string contents, std::string extension=".txt")
{
    std::filesystem::path p = std::filesystem::path(std::getenv("LOCALAPPDATA")) / "SolarSystemExploration3";

    // create directory if it doesn't exist yet
    if (!(std::filesystem::exists(p) && std::filesystem::is_directory(p)))
        std::filesystem::create_directories(p);

    std::ofstream savefile(p / (name + extension), std::ios::out | std::ios::trunc); // truncation to delete file contents before writing
    
    // write to savefile
    // -----------------
    savefile << contents;

    // close the savefile
    // ------------------
    savefile.close();
}

void createSaveFileLinux(std::string name, std::string contents, std::string extension=".txt")
{
    std::filesystem::path p = std::filesystem::path(std::getenv("HOME")) / ".local/share/SolarSystemExploration3";

    // create directory if it doesn't exist yet
    if (!(std::filesystem::exists(p) && std::filesystem::is_directory(p)))
        std::filesystem::create_directories(p);

    std::ofstream savefile(p / (name + extension), std::ios::out | std::ios::trunc); // truncation to delete file contents before writing
    
    // write to savefile
    // -----------------
    savefile << contents;

    // close the savefile
    // ------------------
    savefile.close();
}

void createSaveFile(std::string name, std::string contents, std::string extension=".txt")
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    createSaveFileWindows(name, contents, extension);
#elif __APPLE__
    createSaveFileMacOS(name, contents, extension);
#elif __linux__
    createSaveFileLinux(name, contents, extension);
#endif
}
