#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtc/quaternion.hpp>

#include <model.hpp>
#include <mesh.hpp>
#include <shader.hpp>

void renderArrow(glm::dvec3 origin, float length, glm::vec3 rotation, float arrowStart, Model cylinder, Model cone, Shader shader, Camera camera)
{
    // find arrow rotation matrix
    // -----------------------------------
    rotation.y = -rotation.y;

    glm::quat arrowQuat = glm::quat(rotation, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::dmat4 arrowRotationMat = (glm::dmat4)glm::mat4_cast(arrowQuat);

    // render arrow
    // ---------------------
    shader.use();

    // cylinder
    glm::dmat4 model = glm::dmat4(1.0);
    model = glm::translate(model, origin - camera.Position);
    model *= arrowRotationMat;
    model = glm::translate(model, glm::dvec3(0.0, arrowStart + length, 0.0));
    model = glm::scale(model, glm::dvec3(0.05, length, 0.05));

    shader.setMat4("model", model);
    cylinder.Draw(shader);

    // cone
    model = glm::dmat4(1.0);
    model = glm::translate(model, origin - camera.Position);
    model *= arrowRotationMat;
    model = glm::translate(model, glm::dvec3(0.0, arrowStart + length * 2.0, 0.0));
    model = glm::scale(model, glm::dvec3(0.25));

    shader.setMat4("model", model);
    cone.Draw(shader);
}