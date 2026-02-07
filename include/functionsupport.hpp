#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader.hpp>
#include <filesystem.hpp>

#include <iostream>
#include <math.h>

#pragma once

struct Collision 
{
    bool collided;
    glm::vec3 normal;
};

void bindDiffuseTexture(unsigned int tex)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
}

void bindMetallicTexture(unsigned int tex)
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex);
}

void bindRoughnessTexture(unsigned int tex)
{
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tex);
}

void bindHeightTexture(unsigned int tex)
{
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, tex);
}

void bindNormalTexture(unsigned int tex)
{
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, tex);
}

void bindPBRTextures(unsigned int irMap, unsigned int preMap, unsigned int LUTTex)
{
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irMap);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, preMap);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, LUTTex);
}

glm::dvec3 originRebase(glm::dvec3 originalVec, glm::dvec3 camPos)
{
    return camPos - originalVec;
}

void setupPlanetModel(glm::dmat4 &model, glm::dvec3 pos, glm::dvec3 scale, glm::dvec3 camPos, glm::dvec3 orbitalCamPos, float axialTilt=0.0f, float planetRotation=0.0f)
{
    model = glm::dmat4(1.0);
    model = glm::translate(model, originRebase(pos, camPos));
    model = glm::scale(model, scale);
    if (axialTilt != 0.0f)
        model = glm::rotate(model, static_cast<double>(glm::radians(-axialTilt)), glm::dvec3(0.0, 0.0, 1.0));
    if (planetRotation != 0.0f)
        model = glm::rotate(model, static_cast<double>(planetRotation), glm::dvec3(0.0, 1.0, 0.0));
}

Collision checkForCollision(glm::dvec3 point, glm::dvec3 planetCenter, glm::mat3 rotationMatrix, float equatorialRadius, float polarRadius)
{
    glm::dvec3 relativePos = glm::inverse(static_cast<glm::dmat3>(rotationMatrix)) * (point - planetCenter); // we multiply the relative to planet position by the inverse of the rotation matrix of the planet to get local position relative to the planet's axial tilt
    double collision =  std::pow(relativePos.x, 2) / std::pow(equatorialRadius, 2) + std::pow(relativePos.y, 2) / std::pow(polarRadius, 2) + std::pow(relativePos.z, 2) / std::pow(equatorialRadius, 2);

    Collision result;
    if (collision <= 1.0)
    {
        result.collided = true;
        result.normal = glm::normalize(glm::vec3(glm::length(static_cast<glm::vec3>(relativePos))));
    } else {
        result.collided = false;
    }

    return result;
}
