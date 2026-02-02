#include <celestialbody.hpp>
#include <ui.hpp>
#include <camera.hpp>

class TrajectorySimulator
{
public:
    std::vector<glm::dvec3> positions;
    std::vector<CelestialBody> system;
    double r, v, E, a, T;

    TrajectorySimulator(CelestialBody body, CelestialBody relativeBody) 
    {
        system = { body, relativeBody };

        system[0].velocity -= system[1].velocity;
        system[1].velocity = glm::dvec3(0.0);

        r = glm::length(system[0].position - system[1].position);
        v = glm::length(system[0].velocity);

        E = (v * v) / 2.0 - relativeBody.mu / r;
    }

    void simulateTrajectory()
    {
        glm::dvec3 startPos = system[0].position;
        glm::dvec3 startVel = system[0].velocity;

        double r = glm::length(startPos - system[1].position);
        double v = glm::length(startVel);
        
        double a = -relativeBody.mu / (2.0 * E);

        double T = 2.0 * M_PI * sqrt(a * a * a / relativeBody.mu);
        double simulatedTime = 0.0;

        if (E < 0)
        {
            while(simulatedTime < T)
            {
                system[0].updateObject(system, 10.0f);

                positions.push_back(system[0].position);
                simulatedTime += 10.0;
            }
        }
        else
        {
            while (glm::length(system[0].position - system[1].position) < 1500000000.0)
            {
                system[0].updateObject(system, 10.0f);

                positions.push_back(system[0].position);
                simulatedTime += 10.0;
            }
        }
    }

    void renderTrajectory(Camera camera, glm::mat4 view, glm::mat4 projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
    {
        RenderLine(camera.Position - system[0].position, camera.Position - positions[0], view, projection, SCR_WIDTH, SCR_HEIGHT);

        for (unsigned int i = 0; i < positions.size() - 1; i++)
            RenderLine(camera.Position - positions[i], camera.Position - positions[i + 1], view, projection, SCR_WIDTH, SCR_HEIGHT);

        if (E < 0)
            RenderLine(camera.Position - positions[positions.size() - 1], camera.Position - system[0].position, view, projection, SCR_WIDTH, SCR_HEIGHT);
    }
};