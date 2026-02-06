#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stb_image.h>
#include <stb_image_write.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

typedef unsigned int EXTenum;

EXTenum EXT_HDR = 0;
EXTenum EXT_PNG = 1;
EXTenum EXT_JPG = 2;
EXTenum EXT_TGA = 3;

void writeTexture(EXTenum extension, const char* path, unsigned int size_x, unsigned int size_y, unsigned int nr_channels, const float* pixels)
{
    if (extension == EXT_HDR)
        stbi_write_hdr(path, size_x, size_y, nr_channels, pixels);
    else if (extension == EXT_PNG)
        stbi_write_png(path, size_x, size_y, nr_channels, pixels, size_x * nr_channels + 2);
    else if (extension == EXT_JPG)
        stbi_write_jpg(path, size_x, size_y, nr_channels, pixels, 0);
    else if (extension == EXT_TGA)
        stbi_write_tga(path, size_x, size_y, nr_channels, pixels);
    else
        std::cerr << "Error while writing texture: extension undefined\n";
}

void saveTexture(unsigned int texture, const std::string& folder, const std::string& filename, unsigned int size_x, unsigned int size_y, GLenum format, GLenum type, EXTenum extension)
{
    int data_size_coeff = (format == GL_RGB) ? 3 : (format == GL_RGBA) ? 4 : (format == GL_RED || format == GL_GREEN || format == GL_BLUE) ? 1 : 0;
    std::vector<float> data(size_x * size_y * data_size_coeff);

    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, format, type, data.data());
        
    std::string file_path = folder + "/" + filename;
    if (extension == EXT_HDR)
        file_path += ".hdr";
    else if (extension == EXT_PNG)
        file_path += ".png";
    else if (extension == EXT_JPG)
        file_path += ".jpg";
    else if (extension == EXT_TGA)
        file_path += ".tga";

    if (format == GL_RGB)
        writeTexture(extension, file_path.c_str(), size_x, size_y, 3, data.data());
    else if (format == GL_RGBA)
        writeTexture(extension, file_path.c_str(), size_x, size_y, 4, data.data());
    else if (format == GL_RED || format == GL_GREEN || format == GL_BLUE)
        writeTexture(extension, file_path.c_str(), size_x, size_y, 1, data.data());

    data.clear();
}
