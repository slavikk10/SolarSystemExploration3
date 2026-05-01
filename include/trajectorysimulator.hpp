#include <celestialbody.hpp>
#include <ui.hpp>
#include <camera.hpp>
#include <functionsupport.hpp>
#include <navigator.hpp>

constexpr double PI = 3.1415926535;

class TrajectorySimulator
{
public:
    std::vector<glm::dvec3> positions;
    std::vector<CelestialBody> system;
    double r, v, E, a, T;

    glm::dvec3 periapsis,  apoapsis;
    double     periapsisd, apoapsisd;
    
    bool orbit;

    TrajectorySimulator(const CelestialBody& body, const CelestialBody& relativeBody)
    {
        system = { body, relativeBody };

        system[0].velocity -= system[1].velocity;
        system[1].velocity = glm::dvec3(0.0);

        r = glm::length(system[0].position - system[1].position);
        v = glm::length(system[0].velocity);

        E = pow(v, 2) / 2.0 - system[1].mu / r;
    }

    void simulateTrajectory()
    {
        glm::dvec3 startPos  = system[0].position;
        glm::dvec3 startVel  = system[0].velocity;

        double r             = glm::length(startPos - system[1].position);
        double v             = glm::length(startVel);
        
        double a             = -system[1].mu / (2.0 * E);
        double T             = 2.0 * PI * sqrt(pow(a, 3) / system[1].mu);
        double simulatedTime = 0.0;

        glm::dvec3 h         = glm::cross(startPos - system[1].position, startVel);

        glm::dvec3 eVec      = glm::cross(system[0].velocity, h) / system[1].mu - glm::normalize(startPos - system[1].position);
        double eccentricity  = glm::length(eVec);
        double b             = a * sqrt(1 - pow(eccentricity, 2));

        periapsisd    = a * (1 - eccentricity);
        apoapsisd     = a * (1 + eccentricity);

        if (E < 0.0 && periapsisd > system[1].averageRadius)
        {
            orbit = true;
            for (unsigned int i = 0; i < 1000; ++i)
            {
                double M = 2.0 * PI * (i / 1000.0);

                double E_anom = M;
                for (unsigned int i = 0; i < 5; i++)
                    E_anom += ((M - E_anom + eccentricity * sin(E_anom)) / (1 - eccentricity * cos(E_anom)));

                glm::dvec3 eNorm        = glm::normalize(eVec);
                glm::dvec3 orbitalPlane = glm::normalize(glm::cross(h, eNorm));

                periapsis =  eNorm * periapsisd + system[1].position;
                apoapsis  = -eNorm * apoapsisd  + system[1].position;

                positions.push_back(((a * cos(E_anom) - a * eccentricity) * eNorm + b * sin(E_anom) * orbitalPlane) + system[1].position);
            }
        }
        else
        {
            orbit = false;
            while ((glm::length(system[0].position - system[1].position) < 1500000000.0) && (glm::length(system[0].position - system[1].position) > system[1].averageRadius))
            {
                system[0].updateObject(system, T / 1000.0f);

                positions.push_back(system[0].position);
                simulatedTime += T / 1000.0f;
            }
        }
    }

    void renderTrajectory(const Camera& camera, const glm::mat4& view, const glm::mat4& projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
    {
        for (unsigned int i = 0; i < positions.size() - 1; i++)
            RenderLine(camera.Position - positions[i], camera.Position - positions[i + 1], view, projection, SCR_WIDTH, SCR_HEIGHT);

        if (orbit)
            RenderLine(camera.Position - positions[positions.size() - 1], camera.Position - positions[0], view, projection, SCR_WIDTH, SCR_HEIGHT);
    }
};