#include <celestialbody.hpp>
#include <ui.hpp>
#include <camera.hpp>
#include <functionsupport.hpp>
#include <navigator.hpp>
#include <constants.hpp>

#pragma once

std::vector<double> soiRadiuses = {
    4570000000000000,
    117000000,
    616000000,
    1470000000,
    66100000,
    578000000,
    48000000000,
    10200000,
    9700000,
    32200000,
    52200000,
};

class TrajectorySimulator
{
public:
    std::vector<glm::dvec3> positions;
    std::vector<CelestialBody> system;
    double r, v, E, a, T;

    glm::dvec3 periapsis,  apoapsis;
    double     periapsisd = INFINITY;
    double     apoapsisd  = 0.0;
    
    bool orbit;

    unsigned int rbi;

    TrajectorySimulator(const CelestialBody& body, const CelestialBody& relativeBody, unsigned int relativeBodyIndex)
    {
        system = { body, relativeBody };

        system[0].velocity -= system[1].velocity;
        system[1].velocity = glm::dvec3(0.0);

        rbi = relativeBodyIndex;

        r = glm::length(system[0].position - system[1].position);
        v = glm::length(system[0].velocity);

        E = pow(v, 2) / 2.0 - system[1].mu / r;
    }

    TrajectorySimulator() {}

    void simulateTrajectory(unsigned int iterations=1000, const OrbitalObject& moonOrbital=OrbitalObject(), bool isRocket=false)
    {
        glm::dvec3 startPos  = system[0].position;
        glm::dvec3 startVel  = system[0].velocity;

        double r             = glm::length(startPos - system[1].position);
        double v             = glm::length(startVel);
        
        double a             = -system[1].mu / (2.0 * E);

        glm::dvec3 h         = glm::cross(startPos - system[1].position, startVel);

        glm::dvec3 eVec      = glm::cross(system[0].velocity, h) / system[1].mu - glm::normalize(startPos - system[1].position);
        double eccentricity  = glm::length(eVec);
        double b             = a * sqrt(1 - pow(eccentricity, 2));

        periapsisd    = a * (1 - eccentricity);
        apoapsisd     = a * (1 + eccentricity);

        double vc = (glm::dot(startPos - system[1].position, startVel) / abs(glm::dot(startPos - system[1].position, startVel))) * acos((1.0 / eccentricity) * ((-(a * (pow(eccentricity, 2) - 1)) / r) - 1));
        double vl = acos((1.0 / eccentricity) * ((-(a * (pow(eccentricity, 2) - 1)) / soiRadiuses[rbi]) - 1));

        double Fc = (vc / abs(vc)) * acosh((eccentricity + cos(vc)) / (1.0 + eccentricity * cos(vc)));
        double Fl = (vl / abs(vl)) * acosh((eccentricity + cos(vl)) / (1.0 + eccentricity * cos(vl)));

        double T;
        if (a >= 0.0)
            T = 2.0 * PI * sqrt(pow(a, 3) / system[1].mu);
        else
            T = sqrt(-pow(a, 3) / system[1].mu) * ((eccentricity * sinh(Fl) - Fl) - (eccentricity * sinh(Fc) - Fc));

        double simulatedTime = 0.0;

        if (E < 0.0 && periapsisd > system[1].averageRadius && apoapsisd < soiRadiuses[rbi])
        {
            orbit   = true;
            for (unsigned int i = 0; i < iterations; ++i)
                positions.push_back(findMeanPosition((i / (double)iterations) * T, system[0].position, system[0].velocity, system[1].mu));
        }
        else
        {
            orbit   = false;
            unsigned int i = 0;
            while ((glm::length(system[0].position - system[1].position) > system[1].averageRadius) && (glm::length(system[0].position - system[1].position) <= soiRadiuses[rbi]) && i < 1000)
            {
                system[0].updateObject(system, T / 1000.0f);

                positions.push_back(system[0].position);
                simulatedTime += T / 1000.0f;

                if (glm::length(system[0].position - system[1].position) < periapsisd)
                {
                    periapsisd = glm::length(system[0].position - system[1].position);
                    periapsis  = system[0].position;
                }

                if (glm::length(system[0].position - system[1].position) > apoapsisd)
                {
                    apoapsisd = glm::length(system[0].position - system[1].position);
                    apoapsis  = system[0].position;
                }

                i++;
            }
        }
    }

    void renderTrajectory(const Camera& camera, const glm::mat4& view, const glm::mat4& projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
    {
        for (unsigned int i = 0; i < positions.size() - 1; i++)
            RenderLine(positions[i] - camera.Position, positions[i + 1] - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT);

        if (orbit)
            RenderLine(positions[positions.size() - 1] - camera.Position, positions[0] - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT);
    }
};