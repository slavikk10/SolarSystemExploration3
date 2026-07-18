#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>

#include <load.hpp>
#include <filesystem.hpp>
#include <date.hpp>

struct PlayerSaveData
{
    ObjectState state;
    float fuelLeft;
    glm::dmat4 rotationMatrix;

    bool isEmpty;

    PlayerSaveData() {isEmpty = true;}
    PlayerSaveData(ObjectState s, float fl, glm::dmat4 rm) {state = s; fuelLeft = fl; rotationMatrix = rm; isEmpty = false;}
};

std::string getAppDataPath()
{
    if (GLM_PLATFORM == GLM_PLATFORM_WINDOWS)
        return std::getenv("LOCALAPPDATA") + std::string("/SolarSystemExploration3");
    else if (GLM_PLATFORM == GLM_PLATFORM_APPLE)
        return std::getenv("HOME") + std::string("/Library/Application Support/com.vyacheslavc.SolarSystemExploration3");
    else if (GLM_PLATFORM == GLM_PLATFORM_LINUX)
        return std::getenv("HOME") + std::string("/.local/share/SolarSystemExploration3");
}

void createSaveFile(std::string name, std::string contents, std::string extension=".txt")
{
    std::filesystem::path p = std::filesystem::path(getAppDataPath());

    // create directory if it doesn't exist yet
    // ----------------------------------------
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

template <typename T>
void createSaveFileBinary(std::string name, T contents, std::string extension=".txt")
{
    std::filesystem::path p = std::filesystem::path(getAppDataPath());

    // create directory if it doesn't exist yet
    // ----------------------------------------
    if (!(std::filesystem::exists(p) && std::filesystem::is_directory(p)))
        std::filesystem::create_directories(p);

    std::ofstream savefile(p / (name + extension), std::ios::out | std::ios::trunc | std::ios::binary); // truncation to delete file contents before writing
    
    // write to savefile
    // -----------------
    savefile.write(reinterpret_cast<char*>(&contents), sizeof(contents));

    // close the savefile
    // ------------------
    savefile.close();
}

PlayerSaveData readPlayerSaveFileBinary(std::string name, std::string extension=".txt")
{
    std::filesystem::path p = std::filesystem::path(getAppDataPath());
    if (!std::filesystem::exists(p))
        return PlayerSaveData();

    std::ifstream savefile(p / (name + extension), std::ios::in | std::ios::binary);

    PlayerSaveData output;
    
    // read from savefile
    // ------------------
    savefile.read(reinterpret_cast<char*>(&output), sizeof(output));

    // close the savefile
    // ------------------
    savefile.close();

    // return the output
    // -----------------
    return output;
}

ObjectState readPlanetSaveFileBinary(std::string name, std::string extension=".txt")
{
    std::filesystem::path p = std::filesystem::path(getAppDataPath());
    if (!std::filesystem::exists(p))
        return ObjectState();

    std::ifstream savefile(p / (name + extension), std::ios::in | std::ios::binary);

    ObjectState output;
    
    // read from savefile
    // ------------------
    savefile.read(reinterpret_cast<char*>(&output), sizeof(output));

    // close the savefile
    // ------------------
    savefile.close();

    // return the output
    // -----------------
    return output;
}

Date readWorldStateBinary(std::string name, std::string extension=".txt")
{
    std::filesystem::path p = std::filesystem::path(getAppDataPath());
    if (!std::filesystem::exists(p))
        return Date();

    std::ifstream savefile(p / (name + extension), std::ios::in | std::ios::binary);

    Date output;
    
    // read from savefile
    // ------------------
    savefile.read(reinterpret_cast<char*>(&output), sizeof(output));

    // close the savefile
    // ------------------
    savefile.close();

    // return the output
    // -----------------
    return output;
}
