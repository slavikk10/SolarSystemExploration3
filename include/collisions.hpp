#include <glm/glm.hpp>
#include <celestialbody.hpp>

glm::dvec3 avgSegment(std::vector<glm::dvec3> vertices)
{
    return (vertices[0] + vertices[1] + vertices[2] + vertices[3]) / (double)vertices.size();
}

glm::dvec3 pushPlayerToSurface(CelestialBody player, CelestialBody collisionBody, std::vector<glm::vec3> vertices, unsigned int X_SEGMENTS, unsigned int Y_SEGMENTS, double scale)
{
    glm::dvec3 relative = player.position - collisionBody.position;

    // find the closest segment to the player
    // ----------------------------------
    double minDistance = INFINITY;

    std::vector<unsigned int> closestSegment       = {0, 0};
    std::vector<glm::dvec3> closestSegmentVertices = {glm::dvec3(0.0), glm::dvec3(0.0), glm::dvec3(0.0), glm::dvec3(0.0)};

    for (unsigned int y = 0; y < Y_SEGMENTS; y++)
    {
        for (unsigned int x = 0; x < X_SEGMENTS; x++)
        {
            glm::dvec3 v0 = (glm::dvec3)vertices[x * (X_SEGMENTS + 1) + y]                          * scale;
            glm::dvec3 v1 = (glm::dvec3)vertices[x * (X_SEGMENTS + 1) + y + 1]                      * scale;
            glm::dvec3 v2 = (glm::dvec3)vertices[((x + 1) % X_SEGMENTS) * (X_SEGMENTS + 1) + y]     * scale;
            glm::dvec3 v3 = (glm::dvec3)vertices[((x + 1) % X_SEGMENTS) * (X_SEGMENTS + 1) + y + 1] * scale;

            double distance = glm::length(relative - avgSegment({v0, v1, v2, v3}));
            if (distance < minDistance)
            {
                minDistance    = distance;
                closestSegment = {x, y};

                closestSegmentVertices = {v0, v1, v2, v3};
            }
        }
    }

    // find the distance for which the player is going to be pushed back to the surface
    // --------------------------------------------------------------------------------
    glm::dvec3 direction = glm::normalize(relative);
    glm::dvec3 normal    = glm::normalize(glm::cross((closestSegmentVertices[1] - closestSegmentVertices[0]), (closestSegmentVertices[2] - closestSegmentVertices[0])));

    double t = glm::dot(normal, (closestSegmentVertices[0] - relative)) / glm::dot(normal, direction);

    // push the player back to the surface
    // -----------------------------------
    if (t >= 0.0)
        return t * direction;
    else
        return glm::dvec3(0.0);
}