#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <functional>

#include <freetype2/ft2build.h>
#include <freetype2/freetype/freetype.h>

#include <shader.hpp>

#pragma once

struct Image {
    unsigned int ImageID;
    glm::ivec2 Size;
};

void RenderCenteredImage(Shader &s, Image image, float x, float y, float scale)
{
    unsigned int imageVBO, imageVAO;
    glGenBuffers(1, &imageVBO);
    glGenVertexArrays(1, &imageVAO);
    glBindVertexArray(imageVAO);
    glBindBuffer(GL_ARRAY_BUFFER, imageVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), NULL, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    s.use();
    glBindVertexArray(imageVAO);

    float width  = image.Size.x * scale;
    float height = image.Size.y * scale;

    float vertices[6][4] = {
        {x - width / 2, y + height / 2, 0.0f, 0.0f},
        {x - width / 2, y - height / 2, 0.0f, 1.0f},
        {x + width / 2, y - height / 2, 1.0f, 1.0f},

        {x - width / 2, y + height / 2, 0.0f, 0.0f},
        {x + width / 2, y - height / 2, 1.0f, 1.0f},
        {x + width / 2, y + height / 2, 1.0f, 0.0f}
    };

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, image.ImageID);
    glBindBuffer(GL_ARRAY_BUFFER, imageVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDeleteBuffers(1, &imageVBO);
    glDeleteVertexArrays(1, &imageVAO);
}

struct Button 
{
    std::function<void()> action;
    glm::vec2 position;
    float scale;

    Shader s;
    Image image;

    inline Button(std::function<void()> func, glm::vec2 pos, float sc, Shader shader, Image img)
        : action(func), position(pos), scale(sc), s(shader), image(img) {}

    void Render(glm::vec2 mouse, bool lmbPressed, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
    {
        RenderCenteredImage(s, image, position.x, position.y, scale);
        if (mouse.x > (position.x - SCR_WIDTH / 2) - (image.Size.x * scale / 2) && mouse.x < (position.x - SCR_WIDTH / 2) + (image.Size.x * scale / 2)) // check if cursor is inside the button hitbox
        {
            if (mouse.y > (position.y - SCR_HEIGHT / 2) - (image.Size.y * scale / 2) && mouse.y < (position.y - SCR_HEIGHT / 2) + (image.Size.y * scale / 2))
            {
                if (lmbPressed)
                    action();
            }
        }
    }
};

struct HoverButton
{
    std::function<void()> action;
    glm::vec2 position;
    float scale;
    
    Shader s;
    Image image;
    Image hoverImage;

    inline HoverButton(std::function<void()> func, glm::vec2 pos, float sc, Shader shader, Image img, Image himg)
        : action(func), position(pos), scale(sc), s(shader), image(img), hoverImage(himg) {}

    void Render(glm::vec2 mouse, bool lmbPressed, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
    {
        if (mouse.x > (position.x - SCR_WIDTH / 2) - (image.Size.x * scale / 2) && mouse.x < (position.x - SCR_WIDTH / 2) + (image.Size.x * scale / 2)) // check if cursor is inside the button hitbox
        {
            if (mouse.y > (position.y - SCR_HEIGHT / 2) - (image.Size.y * scale / 2) && mouse.y < (position.y - SCR_HEIGHT / 2) + (image.Size.y * scale / 2))
            {
                RenderCenteredImage(s, hoverImage, position.x, position.y, scale);
                if (lmbPressed)
                    action();
            }
            else
            {
                RenderCenteredImage(s, image, position.x, position.y, scale);
            }
        }
        else
        {
            RenderCenteredImage(s, image, position.x, position.y, scale);
        }
    }
};

glm::vec2 convert3Dto2D(glm::vec3 position, const glm::mat4 &view, const glm::mat4 &projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
{
    glm::vec4 projected = projection * view * glm::vec4(position, 1.0f);

    projected.w = std::max(projected.w, 0.001f);

    glm::vec3 newPosition = glm::vec3(projected.x / projected.w, projected.y / projected.w, projected.z / projected.w);

    newPosition.x = newPosition.x / 2.0f + 0.5f;
    newPosition.y = newPosition.y / 2.0f + 0.5f;

    return glm::vec2(newPosition.x * SCR_WIDTH, newPosition.y * SCR_HEIGHT);
}

void RenderLine(glm::dvec3 pos1, glm::dvec3 pos2, const glm::mat4 &view, const glm::mat4 &projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
{
    unsigned int lineVAO, lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);

    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float), NULL, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);

    glm::dvec3 pos1c = pos1;
    glm::dvec3 pos2c = pos2;

    std::vector<glm::vec2> data;
    data.push_back(convert3Dto2D(pos1c, view, projection, SCR_WIDTH, SCR_HEIGHT));
    data.push_back(convert3Dto2D(pos2c, view, projection, SCR_WIDTH, SCR_HEIGHT));

    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float), &data[0], GL_STATIC_DRAW);

    glBindVertexArray(lineVAO);
    glDrawArrays(GL_LINES, 0, 2);
}
