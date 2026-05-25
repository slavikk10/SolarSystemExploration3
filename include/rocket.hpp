#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include <camera.hpp>
#include <celestialbody.hpp>
#include <functionsupport.hpp>
#include <constants.hpp>

#include <vector>

class Rocket: public CelestialBody
{
public:
    glm::vec3 totalTorque            = glm::vec3(0.0f);
    glm::vec3 rotationalAcceleration = glm::vec3(0.0f);
    glm::vec3 rotationalVelocity     = glm::vec3(0.0f);
    glm::dquat rotationQuaternion;

    Rocket(const Camera& camera, float mass, glm::dvec3 position, glm::dvec3 velocity, float gravity, glm::vec3 size)
    {
        this->mass                = static_cast<double>(mass);
        this->position            = position * 1000.0;
        this->velocity            = velocity * 1000.0;
        this->gravityAcceleration = static_cast<double>(gravity);
        this->polarRadius         = static_cast<double>(size.y);
        this->equatorialRadius    = static_cast<double>(size.x);
        this->averageRadius       = static_cast<double>((size.x + size.y) / 2.0f);
    }

    void update(float deltaTime)
    {
        this->updateRotation(deltaTime);
    }

    void processControls(GLFWwindow* window, Camera camera)
    {
        this->totalTorque = glm::vec3(0.0f);

        glm::vec3 cam = camera.OrbitalCameraPosition;
        //cam.x = cam.x + cam.x * cam.y;
        //cam.z = cam.z + cam.z * cam.y;

        glm::vec3 forward = glm::quat(this->rotationQuaternion) * glm::vec3(0.0f, 1.0f, 0.0f);

        // rocket rotation controls
        // ------------------------
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            this->totalTorque = 1000.0f * -glm::vec3(-cam.z, 0.0f, cam.x);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            this->totalTorque = 1000.0f *  glm::vec3(-cam.z, 0.0f, cam.x);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            this->totalTorque = 1000.0f * -glm::vec3( cam.x, 0.0f, cam.z);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            this->totalTorque = 1000.0f *  glm::vec3( cam.x, 0.0f, cam.z);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            this->totalTorque = 1000.0f * -forward;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            this->totalTorque = 1000.0f *  forward;
    }

private:
    void updateRotation(float deltaTime)
    {
        glm::vec3 frictionTorque = -500.0f * this->rotationalVelocity;                 // calculate friction due to rotation
        this->rotationalAcceleration = (this->totalTorque + frictionTorque) / 1000.0f; // apply total torque to get rotational acceleration
        this->rotationalVelocity += this->rotationalAcceleration * deltaTime;

        glm::vec3 axis = glm::normalize(this->rotationalVelocity);
        float angle = glm::length(this->rotationalVelocity) * deltaTime;
        if (angle > 0.0001f) {
            glm::dquat delta = glm::angleAxis(angle, axis);
            this->rotationQuaternion = glm::normalize(delta * this->rotationQuaternion);
        }

        //this->rotation += this->rotationalVelocity * deltaTime;
        //this->rotation = resetVec(this->rotation);
        //this->rotationQuaternion = glm::angleAxis(glm::length(this->rotation), glm::normalize(this->rotation));
    }

    glm::vec3 resetVec(glm::vec3 vec)
    {
        glm::vec3 result = glm::vec3(0.0f);
        for (unsigned int i = 0; i < 3; i++)
        {
            if (vec[i] > (2.0 * PI))
                result[i] = vec[i] - int(vec[i] / static_cast<float>(2.0 * PI)) * static_cast<float>(2.0 * PI);
            else
                result[i] = vec[i];
        }

        return result;
    }
};
