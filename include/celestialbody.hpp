#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ostream>

#include <constants.hpp>
#include <glmextension.hpp>

#pragma once

struct AccelOut
{
    glm::dvec3 accel;
    std::vector<glm::dvec3> accels;

    AccelOut() {accel = glm::dvec3(0.0); accels = {};}
};

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
    double mass;
    double mu;
    double soi;

    glm::dvec3 velocity = glm::dvec3(0.0);
    glm::dvec3 sqrDstVec = glm::dvec3(0.0);
    double sqrDst = 0.0;
    glm::dvec3 forceDir = glm::dvec3(0.0);
    glm::dvec3 acceleration = glm::dvec3(0.0);
    glm::dvec3 totalAcceleration = glm::dvec3(0.0);

    std::vector<glm::dvec3> accelerations;

    CelestialBody() : position(0.0), velocity(0.0), averageRadius(0), equatorialRadius(0), polarRadius(0), gravityAcceleration(0), axialTilt(0.0), rotationSpeed(0.0), viewSwitchHeight(0.0) {}

    CelestialBody(glm::dvec3 position, glm::dvec3 velocity, double averageRadius, double equatorialRadius, double polarRadius, double gravityAcceleration, double axialTilt, double rotationSpeed, double viewSwitchHeight) 
        : position(position*1000.0), velocity(velocity*1000.0), averageRadius(averageRadius), equatorialRadius(equatorialRadius), polarRadius(polarRadius), gravityAcceleration(gravityAcceleration), axialTilt(axialTilt), rotationSpeed(rotationSpeed), viewSwitchHeight(viewSwitchHeight)
    {
        this->mass = calculateMass();
        this->mu   = calculateGravitationalParameter();
    }

    AccelOut calculateAcceleration(std::vector<CelestialBody> &bodies) 
    {
        AccelOut accelOut = AccelOut();
        for (const CelestialBody &body : bodies) 
        {
            if (!(&body == this) && ((body.mass / glm::length(this->position - body.position)) >= 1e12))
            {
                glm::dvec3 accelerationTowardsBody(0.0);;

                this->sqrDstVec = (body.position - this->position) * (body.position - this->position);
                this->sqrDst    = this->sqrDstVec.x + this->sqrDstVec.y + this->sqrDstVec.z;

                if (this->sqrDst == 0.0) continue;

                this->forceDir = (body.position - this->position) / std::sqrt(this->sqrDst);

                accelerationTowardsBody = this->forceDir * ((G * body.mass) / this->sqrDst);
                accelOut.accels.push_back(accelerationTowardsBody);
                accelOut.accel     +=     accelerationTowardsBody;
            }
            else
            {
                accelOut.accels.push_back(glm::dvec3(0.0));
            }
            
        }

        return accelOut;
    }

    void updateObject(std::vector<CelestialBody> &bodies, double deltaTime) 
    {
        this->position += this->velocity * deltaTime + (this->acceleration * pow(deltaTime, 2.0)) / 2.0;

        AccelOut newAccelData = calculateAcceleration(bodies);

        this->velocity += (deltaTime / 2.0) * (this->acceleration + newAccelData.accel);

        this->accelerations = newAccelData.accels;
        this->acceleration  = newAccelData.accel;
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