#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace glm
{
    bool approximately_equal_to(const glm::highp_mat4 &m1, const glm::highp_mat4 &m2)
    {
        unsigned int c = 0;
        for (unsigned int i = 0; i < 4; i++)
            for (unsigned int j = 0; j < 4; j++)
                if ((m1[i][j] > (m2[i][j] - 0.0001)) && (m1[i][j] < (m2[i][j] + 0.0001)))
                    c++;

        if (c == 16)
            return true;
        else
            return false;
    }
}
