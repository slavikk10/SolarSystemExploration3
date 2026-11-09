#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <functional>
#include <string>
#include <string_view>

#include <ft2build.h>
#include <freetype/freetype.h>

#include <shader.hpp>
#include <texturesave.hpp>
#include <glmextension.hpp>

#pragma once

struct Character 
{
    unsigned char*     Texture;   // The actual glyph texture
    unsigned int       TextureID; // ID handle of the glyph texture
    glm::ivec2         Size;      // Size of glyph
    glm::ivec2         Bearing;   // Offset from baseline to left/top of glyph
    unsigned int       Advance;   // Offset to advance to next glyph
    std::vector<float> TexCoords; // X texure coordinates of the glyph
};

std::map<char, Character> Characters;

unsigned int textAtlas;
int maxBearing = 0, maxHMB = 0;

void genTextAtlas()
{
    // initialize freetype
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        //return -1;
    }

    FT_Face face;
    if (FT_New_Face(ft, getFilePath("Resources/resources/fonts/sans-serif.ttf").c_str(), 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;  
        //return -1;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
  
    // load characters
    // ---------------
    unsigned int atlasWidth = 0, atlasHeight = 0;
    for (unsigned char c = 0; c < 128; c++)
    {
        // load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYPE: Failed to load Glyph" << std::endl;
            continue;
        }

        unsigned int bufferSize = face->glyph->bitmap.width * face->glyph->bitmap.rows;
        unsigned char* bufferCopy = new unsigned char[bufferSize];
        memcpy(bufferCopy, face->glyph->bitmap.buffer, bufferSize);

        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            bufferCopy
        );

        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
            bufferCopy,
            texture, 
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));

        atlasWidth += face->glyph->bitmap.width + (face->glyph->advance.x >> 6);
        atlasHeight = max(atlasHeight, face->glyph->bitmap.rows);
    }

    // find max character values
    // -------------------------
    for (unsigned char c = 0; c < 128; c++)
    {
        maxBearing = max(maxBearing, Characters[c].Bearing.y);
        maxHMB     = max(maxHMB, Characters[c].Size.y - Characters[c].Bearing.y);
    }

    // configure text atlas height
    atlasHeight = maxBearing + maxHMB;

    // generate text atlas
    // -------------------
    glGenTextures(1, &textAtlas);
    glBindTexture(GL_TEXTURE_2D, textAtlas);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // fill the text atlas texture
    // ---------------------------
    unsigned int x_offset = 0;
    for (unsigned char c = 0; c < 128; c++)
    {
        Character ch = Characters[c];

        glTexSubImage2D(GL_TEXTURE_2D, 0, x_offset, atlasHeight - ch.Size.y, ch.Size.x, ch.Size.y, GL_RED, GL_UNSIGNED_BYTE, ch.Texture);

        ch.TexCoords.push_back(x_offset / (float)atlasWidth);
        ch.TexCoords.push_back((x_offset + ch.Size.x) / (float)atlasWidth);

        Characters[c] = ch;

        x_offset += ch.Size.x + (ch.Advance >> 6);
    }

    // save text atlas to game resources
    // ---------------------------------
    saveTexture(textAtlas, getFilePath("resources/textures/HDR"), "text_atlas", atlasWidth, atlasHeight, GL_RED, GL_UNSIGNED_BYTE, EXT_PNG);

    // free up resources
    // -----------------
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

struct Image {
    unsigned int ImageID;
    glm::ivec2 Size;
};

void RenderCenteredImage(Shader& s, Image image, float x, float y, float scale, glm::uvec2 screenRes)
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

    // convert coordinates
    // -------------------
    x += (double)screenRes.x / 2.0;
    y += (double)screenRes.y / 2.0;
    y = -y;

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

void RenderCenteredImage(Shader& s, Image image, glm::vec2 pos, float scale, glm::uvec2 screenRes)
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

    // convert coordinates
    // -------------------
    pos += (glm::dvec2)screenRes / 2.0;

    float vertices[6][4] = {
        {pos.x - width / 2, pos.y + height / 2, 0.0f, 0.0f},
        {pos.x - width / 2, pos.y - height / 2, 0.0f, 1.0f},
        {pos.x + width / 2, pos.y - height / 2, 1.0f, 1.0f},

        {pos.x - width / 2, pos.y + height / 2, 0.0f, 0.0f},
        {pos.x + width / 2, pos.y - height / 2, 1.0f, 1.0f},
        {pos.x + width / 2, pos.y + height / 2, 1.0f, 0.0f}
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

    Button() {}
    Button(std::function<void()> func, glm::vec2 pos, float sc, Shader shader, Image img)
    {
        action   = func;
        position = pos;
        scale    = sc;
        s        = shader;
        image    = img;
    }

    void Render(glm::vec2 mouse, bool lmbPressed, bool lmbPressedLastFrame, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
    {
        RenderCenteredImage(s, image, position, scale, glm::uvec2(SCR_WIDTH, SCR_HEIGHT));
        if (mouse.x > position.x - (image.Size.x * scale / 2) && mouse.x < position.x + (image.Size.x * scale / 2)) // check if cursor is inside the button hitbox
        {
            if (mouse.y > position.y - (image.Size.y * scale / 2) && mouse.y < position.y + (image.Size.y * scale / 2))
            {
                if (lmbPressed && !lmbPressedLastFrame)
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

    bool mousePressed;
    bool mousePressedLastFrame;

    HoverButton() {}
    HoverButton(std::function<void()> func, glm::vec2 pos, float sc, Shader shader, Image img, Image himg)
    {
        action     = func;
        position   = pos;
        scale      = sc;
        s          = shader;
        image      = img;
        hoverImage = himg;
    }

    void Render(glm::vec2 mouse, bool lmbPressed, bool lmbPressedLastFrame, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
    {
        if (mouse.x > position.x - (image.Size.x * scale / 2) && mouse.x < position.x + (image.Size.x * scale / 2)) // check if cursor is inside the button hitbox
        {
            if (mouse.y > position.y - (image.Size.y * scale / 2) && mouse.y < position.y + (image.Size.y * scale / 2))
            {
                RenderCenteredImage(s, hoverImage, position, scale, glm::uvec2(SCR_WIDTH, SCR_HEIGHT));
                if (lmbPressed && !lmbPressedLastFrame)
                    action();
            }
            else
            {
                RenderCenteredImage(s, image, position, scale, glm::uvec2(SCR_WIDTH, SCR_HEIGHT));
            }
        }
        else
        {
            RenderCenteredImage(s, image, position, scale, glm::uvec2(SCR_WIDTH, SCR_HEIGHT));
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

static unsigned int lineVAO = 0, lineVBO = 0;
void RenderLine(glm::dvec3 pos1, glm::dvec3 pos2, const glm::mat4& view, const glm::mat4& projection, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT, glm::vec3 color, Shader shader)
{
    if (lineVAO == 0)
    {
        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);

        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

        glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float), NULL, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);

    /*std::vector<glm::vec2> data;
    data.push_back(convert3Dto2D(pos1, view, projection, SCR_WIDTH, SCR_HEIGHT));
    data.push_back(convert3Dto2D(pos2, view, projection, SCR_WIDTH, SCR_HEIGHT));*/

    glm::vec2 convertedPos1 = convert3Dto2D(pos1, view, projection, SCR_WIDTH, SCR_HEIGHT);
    glm::vec2 convertedPos2 = convert3Dto2D(pos2, view, projection, SCR_WIDTH, SCR_HEIGHT);

    // render the line, if it's fully outside the screen then don't render it
    // ----------------------------------------------------------------------
    if ((convertedPos1.x > 0.0 && convertedPos1.x < SCR_WIDTH && convertedPos1.y > 0.0 && convertedPos1.y < SCR_HEIGHT) || (convertedPos2.x > 0.0 && convertedPos2.x < SCR_WIDTH && convertedPos2.y > 0.0 && convertedPos2.y < SCR_HEIGHT))
    {
        float data[]
        {
            convertedPos1.x, convertedPos1.y,
            convertedPos2.x, convertedPos2.y
        };

        shader.use();
        shader.setVec3("WorldPos", (pos1 + pos2) / 2.0);

        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

        glBindVertexArray(lineVAO);
        glDrawArrays(GL_LINES, 0, 2);
    }
}

void RenderText(Shader &s, std::string text, glm::vec2 pos, float scale, glm::vec3 color, bool centered=true, bool convert=false, glm::uvec2 screenRes=glm::uvec2(0))
{
    // convert coordinates
    // -------------------
    if (convert)
        pos += (glm::dvec2)screenRes / 2.0;

    // text VAO and VBO
    // ----------------
    unsigned int textVBO, textVAO;

    glGenBuffers(1, &textVBO);
    glGenVertexArrays(1, &textVAO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    s.use();
    s.setVec3("textColor", color);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    glBindTexture(GL_TEXTURE_2D, textAtlas);

    float totalWidth = 0;

    std::string::const_iterator c2;
    for (c2 = text.begin(); c2 != text.end(); c2++)
        totalWidth += Characters[*c2].Size.x * scale;

    if (centered)
        pos.x -= totalWidth / 2;

    std::vector<float> totalVertices;

    std::string::const_iterator c;
    unsigned int i = 0;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        float xpos = pos.x + ch.Bearing.x * scale;
        float ypos = pos.y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale + (maxHMB - (ch.Size.y - ch.Bearing.y)) * scale;

        float s1 = ch.TexCoords[0];
        float s2 = ch.TexCoords[1];

        float vertices[6][4] = {
            { xpos,     ypos + h,   s1, 0.0f },
            { xpos,     ypos,       s1, 1.0f },
            { xpos + w, ypos,       s2, 1.0f },

            { xpos,     ypos + h,   s1, 0.0f },
            { xpos + w, ypos,       s2, 1.0f },
            { xpos + w, ypos + h,   s2, 0.0f }
        };

        for (unsigned int j = 0; j < 6; j++)
            for (unsigned int k = 0; k < 4; k++)
                totalVertices.push_back(vertices[j][k]);

        pos.x += (ch.Advance >> 6) * scale;
        i++;
    }

    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, totalVertices.size() * sizeof(float), totalVertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6 * text.size());
}

struct Text
{
    std::string text;
    glm::vec2 pos;
    float scale;
    glm::vec3 color;

    Shader shader;

    bool isCentered;
    bool convert;

    glm::uvec2 screenResolution;

    Text() {}
    Text(std::string t, glm::vec2 p, float s, glm::vec3 c, const Shader& sh, bool ic=true, bool co=false, glm::uvec2 sr=glm::uvec2(0))
    {
        text   = t;
        pos    = p;
        scale  = s;
        color  = c;
        shader = sh;

        isCentered = ic;
        convert    = co;

        screenResolution = sr;
    }

    void render()
    {
        RenderText(shader, text, pos, scale, color, isCentered, true, screenResolution);
    }
};

/*void renderLabeledButton(Image &bg, std::string text, float bg_scale, float text_scale, Shader &image_shader, Shader &text_shader, std::function<void()> action, glm::vec2 position, glm::vec3 text_color)
{
    RenderCenteredImage(image_shader, bg, position, bg_scale);
    RenderText(text_shader, text, position, text_scale, text_color, true);
}*/

class Dropdown
{
public:
    bool activated        = false;
    bool activated_last_f = false;
    unsigned int selected = 0;

    std::vector<std::string> button_texts;
    std::vector<Button>      buttons;
    Button dropdown_button;

    glm::vec2 position;
    Shader image_shader;
    Shader text_shader;

    Image dropdown_image;
    Image button_image;

    Image triangle_down;
    Image triangle_up;

    float button_scale;
    float text_scale;

    Dropdown(std::vector<std::string> bt, Shader is, Shader ts, Image di, Image bi, Image td, Image tu, glm::vec2 pos, float bsc, float tsc)
    {
        button_texts = bt;

        image_shader = is;
        text_shader  = ts;

        dropdown_image = di;
        button_image   = bi;

        position = pos;

        button_scale = bsc;
        text_scale   = tsc;

        for (unsigned int i = 0; i < button_texts.size(); i++)
            buttons.push_back(Button([this, i]() {button_clicked(i);}, glm::vec2(position.x, position.y - button_image.Size.y * button_scale * (i + 1)), button_scale, image_shader, button_image));

        dropdown_button = Button([this]() {dropdown_button_clicked();}, position, button_scale, image_shader, dropdown_image);
    }

    void render(glm::vec2 mouse, bool lmbPressed, bool lmbPressedLastFrame, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
    {
        updateButtons();

        dropdown_button.Render(mouse, lmbPressed, lmbPressedLastFrame, SCR_WIDTH, SCR_HEIGHT);
        RenderText(text_shader, button_texts[selected], glm::vec2(position.x, position.y - (70.0f * button_scale)), text_scale, glm::vec3(1.0f));

        if (activated)
        {
            unsigned int i = 0; 
            for (auto &button : buttons)
            {
                button.Render(mouse, lmbPressed, lmbPressedLastFrame, SCR_WIDTH, SCR_HEIGHT);
                RenderText(text_shader, button_texts[i], glm::vec2(button.position.x, button.position.y - (70.0f * button_scale)), text_scale, glm::vec3(1.0f));

                i++;
            }
        }
    }

private:
    std::function<void(unsigned int)> button_clicked = [this](unsigned int i)
    {
        selected  = i;
        activated = false;
    };

    std::function<void()> dropdown_button_clicked = [this]()
    {
        activated = !activated;
    };

    void updateButtons()
    {
        buttons.clear();

        for (unsigned int i = 0; i < button_texts.size(); i++)
            buttons.push_back(Button([this, i]() {button_clicked(i);}, glm::vec2(position.x, position.y - button_image.Size.y * button_scale * (i + 1)), button_scale, image_shader, button_image));

        dropdown_button = Button([this]() {dropdown_button_clicked();}, position, button_scale, image_shader, dropdown_image);
    }
};

/*class Popup
{
public:
    bool isOpen = false;

    std::string text;
    Shader textShader;
    Shader imageShader;

    Image panel;

    glm::vec2 pos;

    Popup() {}
    Popup(std::string_view t, const Shader& ts, const Shader& is, const Image& p, unsigned int SCR_WIDTH, unsigned int SCR_HEIGHT)
    {
        text        = t;
        textShader  = ts;
        imageShader = is;

        panel = p;

        pos = glm::vec2(SCR_WIDTH / 2.0, SCR_HEIGHT / 2.0);
    }

    void open()
    {
        isOpen = true;
    }

    void close()
    {
        isOpen = false;
    }

    void update()
    {
        if (isOpen)
            render();
    }

private:
    void render()
    {
        RenderCenteredImage(imageShader, panel, pos, 0.2f);
        RenderText(textShader, text, pos + glm::vec2(0.0f, 250.0f), 0.1f, glm::vec3(1.0f), true);
    }
};*/

struct PImage
{
    Image image;
    glm::vec2 position;
    float scale;

    PImage() {}
    PImage(Image i, glm::vec2 pos, float s) {image = i; position = pos; scale = s;}
};

// UI scene struct
// ---------------
struct UIScene
{
    std::vector<std::vector<Button>> buttons{{}, {}, {}, {}, {}, {}, {}, {}, {}, {}}; // 10 max layers
    std::vector<std::vector<HoverButton>> hoverButtons{{}, {}, {}, {}, {}, {}, {}, {}, {}, {}};
    std::vector<std::vector<PImage>> images{{}, {}, {}, {}, {}, {}, {}, {}, {}, {}};
    std::vector<std::vector<Text>> texts{{}, {}, {}, {}, {}, {}, {}, {}, {}, {}};

    void addButton(const Button& button, unsigned int layer)
    {
        buttons[layer].push_back(button);
    }

    void addHoverButton(const HoverButton& button, unsigned int layer)
    {
        hoverButtons[layer].push_back(button);
    }

    void addImage(const Image& image, glm::vec2 position, unsigned int layer, float scale=1.0f)
    {
        images[layer].push_back(PImage(image, position, scale));
    }

    void addText(const Text& text, unsigned int layer)
    {
        texts[layer].push_back(text);
    }

    void render(glm::uvec2 screenRes, glm::vec2 mousePos, bool lmb, bool lmbLF, Shader& imageShader, const Shader& textShader)
    {
        for (unsigned int layer = 0; layer < 10; layer++) // per layer
        {
            // render buttons
            for (Button button : buttons[layer])
                button.Render(mousePos, lmb, lmbLF, screenRes.x, screenRes.y);

            // render hover buttons
            for (HoverButton button : hoverButtons[layer])
                button.Render(mousePos, lmb, lmbLF, screenRes.x, screenRes.y);

            // render images
            for (PImage image : images[layer])
                RenderCenteredImage(imageShader, image.image, image.position, image.scale, screenRes);

            // render texts
            for (Text text : texts[layer])
                text.render();
        }
    }
};

// UI engine class
// ---------------
class UIEngine
{
public:
    // public variables
    // ----------------
    glm::uvec2 screenResolution = glm::uvec2(0);

    // mouse settings
    // --------------
    glm::dvec2 mousePosition = glm::dvec2(0.0);
    std::vector<bool> mbcf   = {false, false, false};
    std::vector<bool> mblf   = {false, false, false};

    // the vector that holds all the UI scenes
    // ---------------------------------------
    std::vector<UIScene> scenes;
    std::vector<bool> showScenes;

    // the vector that holds the shaders (index 0: image shader, index 1: text shader)
    // -------------------------------------------------------------------------------
    std::vector<Shader> shaders;

    GLFWwindow* window;

    // constructors
    // ------------
    UIEngine() {}
    UIEngine(glm::uvec2 screenRes, GLFWwindow*& w, const std::vector<Shader>& shs) {screenResolution = screenRes; window = w; shaders = shs;}

    // function to add a scene to the game's UI
    // ----------------------------------------
    void addScene(UIScene scene, bool showCondition)
    {
        scenes.push_back(scene);
    }

    // update the UI
    // -------------
    void update(const std::vector<bool>& ssc)
    {
        // get mouse position
        // ------------------
        glfwGetCursorPos(window, &mousePosition.x, &mousePosition.y);
        mousePosition -= (glm::dvec2)screenResolution / 2.0;
        mousePosition.y = -mousePosition.y;

        // get mouse button presses
        // ------------------------
        for (unsigned int i = 0; i < 3; i++)
            mbcf[i] = (glfwGetMouseButton(window, i) == GLFW_PRESS ? true : false);

        // replace local vector with provided
        // ----------------------------------
        showScenes = ssc;
        
        // render the UI contents
        // ----------------------
        for (unsigned int i = 0; i < scenes.size(); i++)
            if (showScenes[i])
                scenes[i].render(screenResolution, mousePosition, mbcf[0], mblf[0], shaders[0], shaders[1]);

        mblf = mbcf;
    }

    // deletes everything from the scenes vector
    // -----------------------------------------
    void clear()
    {
        scenes.clear();
    }

private:
    /* private stuff here */
};