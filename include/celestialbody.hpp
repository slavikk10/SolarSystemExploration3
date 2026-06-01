#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ostream>

#include <constants.hpp>

#pragma once

class CelestialBody 
{
public:
    glm::dvec3 position;
    double gravityAcceleration;
    double averageRadius;
    double equatorialRadius;
    double polarRadius;
    double axialTilt;
    double rotationSpeed;
    double viewSwitchHeight;
    std::string albedoTexPath;
    std::string metallicTexPath;
    std::string roughnessTexPath;
    std::string heightTexPath;
    std::string normalTexPath;
    double mass;
    double mu;
    double soi;

    glm::dvec3 velocity = glm::dvec3(0.0);
    glm::dvec3 sqrDstVec = glm::dvec3(0.0);
    double sqrDst = 0.0;
    glm::dvec3 forceDir = glm::dvec3(0.0);
    glm::dvec3 acceleration = glm::dvec3(0.0);
    glm::dvec3 totalAcceleration = glm::dvec3(0.0);

    CelestialBody() : position(0.0), velocity(0.0), averageRadius(0), equatorialRadius(0), polarRadius(0), gravityAcceleration(0), axialTilt(0.0), rotationSpeed(0.0), viewSwitchHeight(0.0) {}

    CelestialBody(glm::dvec3 position, glm::dvec3 velocity, double averageRadius, double equatorialRadius, double polarRadius, double gravityAcceleration, double axialTilt, double rotationSpeed, double viewSwitchHeight) 
        : position(position*1000.0), velocity(velocity*1000.0), averageRadius(averageRadius), equatorialRadius(equatorialRadius), polarRadius(polarRadius), gravityAcceleration(gravityAcceleration), axialTilt(axialTilt), rotationSpeed(rotationSpeed), viewSwitchHeight(viewSwitchHeight)
    {
        this->mass = calculateMass();
        this->mu   = calculateGravitationalParameter();
    }

    glm::dvec3 calculateAcceleration(std::vector<CelestialBody> &bodies) 
    {
        glm::dvec3 totalAccel = glm::dvec3(0.0);
        for (CelestialBody &body : bodies) 
        {
            if (&body == this) continue;

            this->sqrDstVec.x = (body.position.x - this->position.x) * (body.position.x - this->position.x);
            this->sqrDstVec.y = (body.position.y - this->position.y) * (body.position.y - this->position.y);
            this->sqrDstVec.z = (body.position.z - this->position.z) * (body.position.z - this->position.z);
            this->sqrDst = this->sqrDstVec.x + this->sqrDstVec.y + this->sqrDstVec.z;

            if (this->sqrDst == 0.0) continue;

            this->forceDir.x = (body.position.x - this->position.x) / std::sqrt(this->sqrDst);
            this->forceDir.y = (body.position.y - this->position.y) / std::sqrt(this->sqrDst);
            this->forceDir.z = (body.position.z - this->position.z) / std::sqrt(this->sqrDst);

            this->acceleration.x = this->forceDir.x * ((G * body.mass) / this->sqrDst);
            this->acceleration.y = this->forceDir.y * ((G * body.mass) / this->sqrDst);
            this->acceleration.z = this->forceDir.z * ((G * body.mass) / this->sqrDst);

            totalAccel += this->acceleration;
        }

        return totalAccel;
    }

    void updateObject(std::vector<CelestialBody> &bodies, double deltaTime) 
    {
        this->position += this->velocity * glm::dvec3(deltaTime) + this->totalAcceleration * glm::dvec3(deltaTime * deltaTime * 0.5);
        glm::dvec3 newAcceleration = calculateAcceleration(bodies);
        this->velocity += (this->totalAcceleration + newAcceleration) * (deltaTime * 0.5);
        this->totalAcceleration = newAcceleration;
    }

    void fillData()
    {
        this->mass = calculateMass();
        this->mu   = calculateGravitationalParameter();
    }

private:
    double calculateMass() 
    {
        return this->gravityAcceleration * pow(this->averageRadius, 2) / G;
    }

    double calculateGravitationalParameter()
    {
        return G * this->mass;
    }
};

std::ostream& operator<<(std::ostream& arg1, const CelestialBody& arg2)
{
    arg1 << "Gravity: " << arg2.gravityAcceleration << std::endl;
    arg1 << "Average radius: " << arg2.averageRadius << std::endl;
    arg1 << "Mass: " << arg2.mass;

    return arg1;
}