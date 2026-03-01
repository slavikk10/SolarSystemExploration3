#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>

#include <stb_image.h>

#include <load.hpp>
#include <filesystem.hpp>

void saveTextureCache(std::string cacheFileName, std::string texturePath) {
    std::filesystem::path p = std::getenv("HOME");
    p /= "SSE3_OpenGL/cache/TextureCache";

    if (!(std::filesystem::exists(p) && std::filesystem::is_directory(p)))
        std::filesystem::create_directories(p);

    if (std::filesystem::exists(getFilePath("cache/TextureCache/") + cacheFileName + ".tca"))
        return;

    int width, height, nrComponents;
    unsigned char* data;

    data = stbi_load(texturePath.c_str(), &width, &height, &nrComponents, 0);

    std::ofstream textureCacheFile(p / (cacheFileName + ".tca"), std::ios::binary);
    textureCacheFile.write(reinterpret_cast<char*>(&width), sizeof(width));               // write texture width
    textureCacheFile.write(reinterpret_cast<char*>(&height), sizeof(height));             // write texture height
    textureCacheFile.write(reinterpret_cast<char*>(&nrComponents), sizeof(nrComponents)); // write number of channels
    textureCacheFile.write(reinterpret_cast<char*>(data), nrComponents * width * height); // write raw pixel data
    textureCacheFile.close();
}

struct TextureCache
{
    int width, height, nrComponents;
    unsigned char* data;

    TextureCache(int w, int h, int nc, unsigned char* d): width(w), height(h), nrComponents(nc), data(d) {};
};

TextureCache loadTextureCache(std::string_view path)
{
    int width, height, nrComponents;
    unsigned char* data;

    std::filesystem::path p = path;

    std::string line;

    std::ifstream textureCacheFile(p, std::ios::binary);
    textureCacheFile.read(reinterpret_cast<char*>(&width),        sizeof(width));
    textureCacheFile.read(reinterpret_cast<char*>(&height),       sizeof(height));
    textureCacheFile.read(reinterpret_cast<char*>(&nrComponents), sizeof(nrComponents));

    data = new unsigned char [nrComponents * width * height];
    textureCacheFile.read(reinterpret_cast<char*>(data),          nrComponents * width * height);

    textureCacheFile.close();

    return TextureCache(width, height, nrComponents, data);
}