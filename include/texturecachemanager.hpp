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
    std::filesystem::path p = getFilePath("cache/TextureCache");

    // create directory if it doesn't exist yet
    if (!(std::filesystem::exists(p) && std::filesystem::is_directory(p)))
        std::filesystem::create_directories(p);

    // if cache is already saved, return early
    if (std::filesystem::exists(getFilePath("cache/TextureCache/") + cacheFileName + ".tca"))
        return;

    // if path is empty, return early
    if (texturePath == "")
        return;

    int width, height, nrComponents;
    unsigned char* data;

    data = stbi_load(getFilePath(texturePath).c_str(), &width, &height, &nrComponents, 0);

    std::ofstream textureCacheFile(p / (cacheFileName + ".tca"), std::ios::binary);
    textureCacheFile.write(reinterpret_cast<char*>(&width), sizeof(width));               // write texture width
    textureCacheFile.write(reinterpret_cast<char*>(&height), sizeof(height));             // write texture height
    textureCacheFile.write(reinterpret_cast<char*>(&nrComponents), sizeof(nrComponents)); // write number of channels
    textureCacheFile.write(reinterpret_cast<char*>(data), nrComponents * width * height); // write raw pixel data
    textureCacheFile.close();
}

struct Texture
{
    int width, height, nrComponents;
    unsigned char* data;

    Texture(int w, int h, int nc, unsigned char* d): width(w), height(h), nrComponents(nc), data(d) {};

    bool operator==(const Texture& compared) const 
    {
        return width == compared.width && height == compared.height && nrComponents == compared.nrComponents && data == compared.data;
    }
};

Texture loadTextureCache(std::string_view path)
{
    if (!std::filesystem::exists(path))
        return Texture(0, 0, 0, 0);

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

    return Texture(width, height, nrComponents, data);
}