#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader.hpp>
#include <filesystem.hpp>
#include <constants.hpp>

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
    return originalVec - camPos;
}

void setupPlanetModel(glm::dmat4 &model, glm::dvec3 pos, glm::dvec3 scale, glm::dvec3 camPos, float axialTilt=0.0f, float planetRotation=0.0f)
{
    model = glm::dmat4(1.0);
    model = glm::translate(model, originRebase(pos, camPos));
    model = glm::scale(model, scale);
    if (axialTilt != 0.0f)
        model = glm::rotate(model, static_cast<double>(glm::radians(axialTilt)), glm::dvec3(0.0, 0.0, 1.0));
    if (planetRotation != 0.0f)
        model = glm::rotate(model, static_cast<double>(planetRotation), glm::dvec3(0.0, 1.0, 0.0));
}

glm::dvec2 raySphere(glm::dvec3 rayOrigin, glm::dvec3 sphereCenter, double sphereRadius, glm::dvec3 rayDirection)
{
    glm::dvec3 relative = rayOrigin - sphereCenter;

    double a = 1;
    double b = 2 * glm::dot(relative, rayDirection);
    double c = glm::dot(relative, relative) - pow(sphereRadius, 2);
    double d = pow(b, 2) - 4 * a * c;

    if (d > 0.0)
    {
        double s = sqrt(d);
        double dstToSphereNear = max(0.0, (-b - s) / (2 * a));
        double dstToSphereFar = (-b + s) / (2 * a);

        if (dstToSphereFar >= 0) {
            return glm::dvec2(dstToSphereNear, dstToSphereFar);
        }
    }
}

glm::dvec3 rayTriangle(glm::dvec3 rayOrigin, glm::dvec3 rayDirection, std::vector<glm::dvec3> triangleVertices)
{
    glm::dvec3 e0 = triangleVertices[1] - triangleVertices[0];
    glm::dvec3 e1 = triangleVertices[2] - triangleVertices[0];

    glm::dvec3 N = glm::cross(e0, e1);

    // check if ray is parallel to triangle
    // ------------------------------------
    if (abs(glm::dot(e0, glm::cross(rayDirection, e1))) < 1e-5)
        return glm::dvec3(0.0);

    double inv_e0dprdce1p = 1.0 / glm::dot(e0, glm::cross(rayDirection, e1));
    glm::dvec3 s          = rayOrigin - triangleVertices[0];
    double u              = inv_e0dprdce1p * glm::dot(s, glm::cross(rayDirection, e1));

    // ray is outside second edge bounds
    // ---------------------------------
    if (u < -1e-5 || u - 1.0 > 1e-5)
        return glm::dvec3(0.0);

    glm::dvec3 sce0 = glm::cross(s, e0);
    double v        = inv_e0dprdce1p * glm::dot(rayDirection, sce0);

    // ray is outside first edge bounds
    // --------------------------------
    if (v < -1e-5 || u + v - 1.0 > 1e-5)
        return glm::dvec3(0.0);

    double t = inv_e0dprdce1p * glm::dot(e1, sce0);

    if (t > 1e-5)
        return rayOrigin + t * rayDirection;
    else
        return glm::dvec3(0.0); // line intersection but not ray intersection

    return glm::dvec3(0.0);
}

glm::uvec2 checkForCollision(glm::dvec3 point, glm::dvec3 planetPosition, double radius, unsigned int numOfSegments)
{
    // first stage collision test (perfect sphere intersection)
    // --------------------------------------------------------
    double sphereCollisionPoint = raySphere(point, planetPosition, radius, glm::normalize(planetPosition - point)).x;

    glm::dvec3 hitPoint = (point - planetPosition) + sphereCollisionPoint * glm::normalize(planetPosition - point);

    // find approximate hit segment
    // ----------------------------
    double longitude = (atan2(glm::normalize(hitPoint).z, glm::normalize(hitPoint).x) + PI) / (2 * PI);
    unsigned int a = floor(longitude * numOfSegments);

    double latitude  = acos(glm::normalize(hitPoint).y) / PI;
    unsigned int r = floor(latitude  * numOfSegments);

    // return approximate hit segment position
    // ---------------------------------------
    return glm::uvec2(a, r);
}

std::string vec3ToString(glm::dvec3 vec)
{
    std::string x = std::to_string(vec.x);
    std::string y = std::to_string(vec.y);
    std::string z = std::to_string(vec.z);
    
    return x + ", " + y + ", " + z;
}

glm::vec3 getRgbPixel(unsigned char* texture, glm::vec2 texCoords, glm::vec2 size, unsigned int nrComponents)
{
    unsigned int index = ((1.0f - texCoords.y) * size.y) * size.x + (texCoords.x * size.x);
    index *= nrComponents;

    return glm::vec3(texture[index], texture[index + 1], texture[index + 2]);
}

unsigned int getGrayscalePixel(unsigned char* texture, glm::vec2 texCoords, glm::vec2 size)
{
    std::cout << "started ggp\n";
    unsigned int index = ((1.0f - texCoords.y) * size.y) * size.x + (texCoords.x * size.x);
    std::cout << "index: " << index << ", width: " << size.x << ", height: " << size.y << ", tex coord x: " << texCoords.x << ", tex coord y: " << texCoords.y << std::endl;
    std::cout << "address: " << static_cast<const void*>(texture) << std::endl;
    std::cout << "data: " << (unsigned int)(*texture) << std::endl;
    std::cout << "success read? " << (unsigned int)texture[index] << std::endl;
    return (unsigned int)texture[index];
}

bool stob(std::string s)
{
    return (s == "true");
}
