#define _HAS_STD_BYTE 0
#define WIN32_LEAN_AND_MEAN

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtc/quaternion.hpp>

#include <shader.hpp>
#include <camera.hpp>
#include <model.hpp>
#include <json_parser.hpp>
#include <celestialbody.hpp>
#include <filesystem.hpp>
#include <functionsupport.hpp>
#include <ui.hpp>
#include <threads.hpp>
#include <trajectorysimulator.hpp>
#include <navigator.hpp>
#include <texturesave.hpp>
#include <callbacks.hpp>
#include <rocket.hpp>
#include <glmextension.hpp>
#include <load.hpp>
#include <texturecachemanager.hpp>
#include <soundmanager.hpp>
#include <renderfuncs.hpp>

#include <atomic>
#include <cstdlib>
#include <cctype>
#include <iostream>
#include <random>
#include <functional>
#include <string>
#include <string_view>
#include <format>

//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <unistd.h>
#include <cstring>
#include <thread>
#include <atomic>

#include <ft2build.h>
#include <freetype/freetype.h>

struct SphereCollision;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_click_callback(GLFWwindow* window, int button, int action);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int genTexture(Texture texture, bool gamma_correction=false, bool sixteenFloat=false);
Image loadImage(char const * path, bool gamma_correction);
unsigned int loadCubemap(std::string path, std::string filename_start_text, vector<std::string> faces, bool gamma_correction, bool sixteenFloat=false);
glm::vec3 floatToVec3(float v[3]);
glm::vec2 floatToVec2(float v[2]);
glm::vec4 floatToVec4(float v[4]);
void loadShaderUniforms(Shader shader);
std::string toString(const char* v);
char* stringToChar(std::string v);
void addToArr(const char* arr[], char* v);
void renderQuad();
void renderSphere(bool patches, unsigned int X_SEGMENTS=32, unsigned int Y_SEGMENTS=32);
glm::dvec3 renderSphereCollision(bool patches, unsigned int X_SEGMENTS=32, unsigned int Y_SEGMENTS=32, glm::uvec2 hitSegment=glm::uvec2(0), glm::dvec3 scale=glm::dvec3(0.0), glm::dvec3 rayDirection=glm::dvec3(0.0), glm::dvec3 rayOrigin=glm::dvec3(0.0), Texture heightTexture=Texture(0, 0, 0, 0));
void RenderText(Shader &s, std::string text, float x, float y, float scale, glm::vec3 color, bool centered);
void saveCubemap(unsigned int cubemap, const std::string& folder, const std::string& filename_start_text, unsigned int size);

// settings
unsigned int SCR_WIDTH;
unsigned int SCR_HEIGHT;

// camera
Camera camera(glm::vec3(8.241306702279538e+09, 3.218946189282089e+07, -1.526005896490690e+11)/10000.0f);
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = glfwGetTime();
int timeMultiplier = 1;
int timeMultiplierIndex = 0;

// constants
constexpr float sscale = 1.0f; // scale of the Solar System (1:1)
const std::vector<std::string> faces           = {"right", "left", "top", "bottom", "front", "back"}; // cubemap faces
const std::vector<unsigned int> timewarpValues = {1, 2, 3, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000, 250000, 500000, 1000000}; // timewarp values
const std::vector<std::string> monthNames      = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
constexpr unsigned int numOfPlanets = 11;

// date
struct Date {
    unsigned int year   = 2026;
    unsigned int month  = 6;
    unsigned int day    = 1;
    unsigned int hour   = 0;
    unsigned int minute = 0;
    float        second = 0.0f;
    std::string date = "00:00:00, Jun 1, 2026";

    void increment(float seconds)
    {
        second += seconds;

        if (second >= 60.0f)
        {
            minute += floor(second / 60.0f);
            second = 0.0f;
        }

        if (minute >= 60)
        {
            hour += floor(minute / 60);
            minute = 0;
        }

        if (hour >= 24)
        {
            day += floor(hour / 24);
            hour = 0;
        }

        if (day >= 32)
        {
            month += floor(day / 32);
            day = 1;
        }

        if (month >= 13)
        {
            year += floor(month / 13);
            month = 1;
        }

        date = std::format("{:02}", hour) + ":" + std::format("{:02}", minute) + ":" + std::format("{:02}", floor(second)) + ", " + monthNames[month - 1] + " " + std::to_string(day) + ", " + std::to_string(year);
    }
};

Date currentDate;

bool isServer;
int timestamp;

float pitch, yaw, roll;
glm::vec3 ndv;

float throttle = 0.0f;
bool enginesOn = false;
float fuelConsumption; // liters/s
float fuel = 3000000.0f; // in liters

struct Character {
    unsigned char*     Texture;   // The actual glyph texture
    unsigned int       TextureID; // ID handle of the glyph texture
    glm::ivec2         Size;      // Size of glyph
    glm::ivec2         Bearing;   // Offset from baseline to left/top of glyph
    unsigned int       Advance;   // Offset to advance to next glyph
    std::vector<float> TexCoords; // X texure coordinates of the glyph
};

std::map<char, Character> Characters;

// text VBO and VAO
unsigned int textVBO, textVAO;

// image VBO and VAO
unsigned int imageVBO, imageVAO;

// line VBO and VAO
//unsigned int lineVAO, lineVBO;

// structs
struct MouseInput
{
    double mouseX;
    double mouseY;

    bool lmbPressed = false;
    bool mmbPressed = false;
    bool rmbPressed = false;
};

struct KeyInput
{
    bool keyT_lastFrame = false;

    bool keyGT_lastFrame = false;
    bool keyLT_lastFrame = false;

    bool keyESC_lastFrame = false;

    bool keyF1_lastFrame = false;
};

MouseInput mouseInput;
KeyInput keyInput;

struct PlanetKey 
{
    unsigned int xSegs, ySegs;
    
    bool operator<(const PlanetKey& other) const {
        if (xSegs != other.xSegs) return xSegs < other.xSegs;
        return ySegs < other.ySegs;
    }
};

struct PlanetData {
    unsigned int vao, vbo, ebo;
    unsigned int indexCount;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
};

struct SphereCollision 
{
    bool collisionState = false;
    unsigned int insideCounter;
    glm::dvec3 closestSurface;
};

static std::map<PlanetKey, PlanetData> planetCache;
static std::map<PlanetKey, PlanetData> cPlanetCache;

bool orbitView = false;

unsigned int textAtlas;
int maxBearing = 0, maxHMB = 0;

int main(int argc, char* argv[]) {
    float start = glfwGetTime();

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw: get primary monitor resolution
    // ------------------------------------
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);

    SCR_WIDTH  = (unsigned int)mode->width;
    SCR_HEIGHT = (unsigned int)mode->height;

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Solar System Exploration 3", glfwGetPrimaryMonitor(), NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, (GLFWmousebuttonfun)mouse_click_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // initialize freetype
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    FT_Face face;
    if (FT_New_Face(ft, getFilePath("resources/fonts/sans-serif.ttf").c_str(), 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;  
        return -1;
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

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glPatchParameteri(GL_PATCH_VERTICES, 3);

    start = glfwGetTime();
    auto sstart = glfwGetTime();

    // build and compile shaders
    // -------------------------
    Shader shader(      getFilePath("shaders/pbr/pbr.vert").c_str(),                   getFilePath("shaders/pbr/pbr.frag").c_str(),                   getFilePath("shaders/pbr/pbr.tesc").c_str(),                   getFilePath("shaders/pbr/pbr.tese").c_str());
    Shader shaderTex(   getFilePath("shaders/pbr_textured/pbr_textured.vert").c_str(), getFilePath("shaders/pbr_textured/pbr_textured.frag").c_str(), getFilePath("shaders/pbr_textured/pbr_textured.tesc").c_str(), getFilePath("shaders/pbr_textured/pbr_textured.tese").c_str());
    Shader lightShader( getFilePath("shaders/light/light.vert").c_str(),               getFilePath("shaders/light/light.frag").c_str(),               getFilePath("shaders/light/light.geom").c_str());
    Shader skyboxShader(getFilePath("shaders/skybox/skybox.vert").c_str(),             getFilePath("shaders/skybox/skybox.frag").c_str(),             getFilePath("shaders/skybox/skybox.geom").c_str());
    Shader hdrShader(   getFilePath("shaders/hdr/hdr.vert").c_str(),                   getFilePath("shaders/hdr/hdr.frag").c_str(),                   getFilePath("shaders/hdr/hdr.geom").c_str());

    // build and compile 2D shaders
    // ----------------------------
    Shader textShader(    getFilePath("shaders/text/text.vert").c_str(),                   getFilePath("shaders/text/text.frag").c_str());
    Shader imageShader(   getFilePath("shaders/image/image.vert").c_str(),                 getFilePath("shaders/image/image.frag").c_str());
    Shader lineShader(    getFilePath("shaders/line/line.vert").c_str(),                   getFilePath("shaders/line/line.frag").c_str());
    Shader optDepthShader(getFilePath("shaders/optical-depth/optical-depth.vert").c_str(), getFilePath("shaders/optical-depth/optical-depth.frag").c_str());

    // load models
    // -----------
    Model rocketModel(getFilePath("resources/models/simple_rocket.obj"));
    Model cylinder(   getFilePath("resources/models/cylinder.obj"));
    Model cone(       getFilePath("resources/models/cone.obj"));

    // setup cube vertices (for skybox)
    // --------------------------------
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    // skybox VAO and VBO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // text VAO and VBO
    glGenBuffers(1, &textVBO);
    glGenVertexArrays(1, &textVAO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // line VAO and VBO
    /*glGenBuffers(1, &lineVBO);
    glGenVertexArrays(1, &lineVAO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float), NULL, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);*/

    //std::cout << "Time elapsed to setup VBO/VAO: " << glfwGetTime() - start << std::endl;
    start = glfwGetTime();
    // generate depth map framebuffer
    /*unsigned int depthCubemapFBO;
    glGenFramebuffers(1, &depthCubemapFBO);

    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

    unsigned int depthCubemap;
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, depthCubemapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);*/

    // initialize buffers
    // ------------------
    unsigned int fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    unsigned int colorBuffer;
    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned int depthBuffer;
    glGenTextures(1, &depthBuffer);
    glBindTexture(GL_TEXTURE_2D, depthBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // attach buffers
    // --------------
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffer, 0);

    //std::cout << "Time elapsed to setup framebuffer: " << glfwGetTime() - start << std::endl;
    start = glfwGetTime();

    // load PBR required textures
    // --------------------------
    unsigned int envCubemap     = loadCubemap(getFilePath("resources/textures/PBR"), "env_", faces, false, false);
    unsigned int irradianceMap  = loadCubemap("resources/textures/PBR", "irradiance_", faces, false, true);
    unsigned int prefilterMap   = loadCubemap("resources/textures/PBR", "prefilter_", faces, false, true);
    unsigned int brdfLUTTexture = loadTexture(getFilePath("resources/textures/PBR/brdf_lut.hdr").c_str(), false, true);

    //std::cout << "Time elapsed to load PBR textures: " << glfwGetTime() - start << std::endl;
    start = glfwGetTime();

    // configure environment cubemap
    // -----------------------------
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // optical depth texture
    // ---------------------
    unsigned int optDepthTex;
    glGenFramebuffers(1, &optDepthTex);
    glBindFramebuffer(GL_FRAMEBUFFER, optDepthTex);

    unsigned int optDepthColor;
    glGenTextures(1, &optDepthColor);
    glBindTexture(GL_TEXTURE_2D, optDepthColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 4096, 4096, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, optDepthTex);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, optDepthColor, 0);

    std::vector<std::string> planetJSONPaths = {
        "resources/planets/sun.json",
        "resources/planets/mercury.json",
        "resources/planets/venus.json",
        "resources/planets/earth.json",
        "resources/planets/moon.json",
        "resources/planets/mars.json",
        "resources/planets/jupiter.json",
        "resources/planets/io.json",
        "resources/planets/europa.json",
        "resources/planets/ganymede.json",
        "resources/planets/callisto.json",
    };

    std::unordered_map<std::string, Shader> shaderNames = {
        {"pbr", shader},
        {"pbr_textured", shaderTex},
        {"light", lightShader}
    };

    std::vector<JSON> planetJSON;
    for (unsigned int i = 0; i < planetJSONPaths.size(); i++)
        planetJSON.push_back(loadJSON(planetJSONPaths[i].c_str()));

    std::vector<std::vector<std::string>> planetTexturePaths;
    for (unsigned int i = 0; i < planetJSONPaths.size(); i++)
        planetTexturePaths.push_back(parseTexturesJSON(planetJSONPaths[i].c_str()));

    std::vector<TextureSettings> planetTextureSettings;
    for (unsigned int i = 0; i < planetJSONPaths.size(); i++)
        planetTextureSettings.push_back(parseTextureSettingsJSON(planetJSONPaths[i].c_str()));

    std::vector<Shader> planetShaders;
    for (unsigned int i = 0; i < planetJSONPaths.size(); i++)
        planetShaders.push_back(shaderNames.at(getJSONValue(planetJSON[i], "shader")));

    unsigned int numOfAtmospheres = 0;
    std::vector<unsigned int> planetsWithAtmospheres;

    std::vector<AtmosphereJSON> atmospheresSettings;
    for (unsigned int i = 0; i < planetJSONPaths.size(); i++)
    {
        atmospheresSettings.push_back(parseAtmosphereJSON(planetJSONPaths[i].c_str()));

        if (atmospheresSettings[i] != AtmosphereJSON())
        {
            numOfAtmospheres++;
            planetsWithAtmospheres.push_back(i);
        }
    }

    std::vector<std::vector<Texture>> textureLoad(planetTexturePaths.size(), std::vector<Texture>(planetTexturePaths[0].size(), Texture(0, 0, 0, 0)));
    std::vector<std::thread> texturesLoadingThreads;
    std::vector<std::vector<unsigned int>> loadedTextures(planetTexturePaths.size(), std::vector<unsigned int>(planetTexturePaths[0].size(), 0));

    // start multiple threads to load textures faster
    // ----------------------------------------------
    for (unsigned int i = 0; i < planetTexturePaths.size(); i++)
    {
        texturesLoadingThreads.emplace_back([i, &planetTexturePaths, &textureLoad]()
        {
            // find file names
            // ---------------
            std::vector<std::string> textureFileNames;
            for (unsigned int j = 0; j < planetTexturePaths[0].size(); j++)
            {
                if (planetTexturePaths[i][j] != "")
                {
                    std::string fileNameWithExtension = planetTexturePaths[i][j].substr(planetTexturePaths[i][j].find_last_of('/') + 1);
                    textureFileNames.push_back(fileNameWithExtension.erase(fileNameWithExtension.find_first_of('.')));
                }
                else
                {
                    textureFileNames.push_back("");
                }
            }

            // save texture cache to use later
            // -------------------------------
            for (unsigned int j = 0; j < planetTexturePaths[0].size(); j++)
                saveTextureCache(textureFileNames[j], planetTexturePaths[i][j]);

            // load cached textures
            // --------------------
            for (unsigned int j = 0; j < planetTexturePaths[0].size(); j++)
                textureLoad[i][j] = loadTextureCache(getFilePath("cache/TextureCache/") + textureFileNames[j] + ".tca");
        });
    }

    for (auto &t: texturesLoadingThreads)
        t.join();

    for (unsigned int i = 0; i < textureLoad.size(); i++)
        for (unsigned int j = 0; j < textureLoad[0].size(); j++)
            loadedTextures[i][j] = genTexture(textureLoad[i][j]);

    // load cloud textures
    // -------------------
    std::vector<CloudsJSON> cloudsJSON;
    for (unsigned int i = 0; i < planetJSONPaths.size(); i++)
        cloudsJSON.push_back(parseCloudsJSON(planetJSONPaths[i].c_str()));

    std::vector<Texture> cloudsTextureLoad;
    std::vector<unsigned int> loadedCloudTextures;

    for (unsigned int i = 0; i < cloudsJSON.size(); i++)
    {
        saveTextureCache(cloudsJSON[i].textureName, cloudsJSON[i].texturePath);

        cloudsTextureLoad.push_back(loadTextureCache(getFilePath("cache/TextureCache/") + cloudsJSON[i].textureName + ".tca"));
        loadedCloudTextures.push_back(genTexture(cloudsTextureLoad[i]));
    }

    // Load UI textures
    // ----------------
    Image objectNavImg   = loadImage(getFilePath("resources/textures/UI/objectnav.png").c_str(),                    false);
    Image apoap_periapsi = loadImage(getFilePath("resources/textures/UI/apo-per.png").c_str(),                      false);
    Image mmtl           = loadImage(getFilePath("resources/textures/UI/main_menu/mmtl_sse3.png").c_str(),          false);
    Image start_button   = loadImage(getFilePath("resources/textures/UI/main_menu/start_button.png").c_str(),       false);
    Image options_button = loadImage(getFilePath("resources/textures/UI/main_menu/options_button.png").c_str(),     false);
    Image quit_button    = loadImage(getFilePath("resources/textures/UI/main_menu/quit_button.png").c_str(),        false);
    Image start_hover    = loadImage(getFilePath("resources/textures/UI/main_menu/start_hover.png").c_str(),        false);
    Image options_hover  = loadImage(getFilePath("resources/textures/UI/main_menu/options_hover.png").c_str(),      false);
    Image quit_hover     = loadImage(getFilePath("resources/textures/UI/main_menu/quit_hover.png").c_str(),         false);
    Image options_panel  = loadImage(getFilePath("resources/textures/UI/main_menu/options_panel.png").c_str(),      false);
    Image options_close  = loadImage(getFilePath("resources/textures/UI/main_menu/options_close.png").c_str(),      false);
    Image black_overlap  = loadImage(getFilePath("resources/textures/UI/black_overlap.png").c_str(),                false);
    Image rtg_button     = loadImage(getFilePath("resources/textures/UI/esc_menu/return_to_game.png").c_str(),      false);
    Image rtmm_button    = loadImage(getFilePath("resources/textures/UI/esc_menu/return_to_main_menu.png").c_str(), false);

    // configure irradiance map
    // ------------------------
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // shader configuration
    // --------------------
    hdrShader.use();
    hdrShader.setInt("colorTex", 0);
    hdrShader.setInt("depthTex", 1);
    hdrShader.setInt("opticalDepthTex", 2);

    shader.use();
    shader.setInt("heightMap", 2);
    shader.setInt("irradianceMap", 3);
    shader.setInt("prefilterMap", 4);
    shader.setInt("brdfLUT", 5);

    shaderTex.use();
    shaderTex.setInt("albedoMap", 0);
    shaderTex.setInt("metallicMap", 1);
    shaderTex.setInt("roughnessMap", 2);
    shaderTex.setInt("irradianceMap", 3);
    shaderTex.setInt("prefilterMap", 4);
    shaderTex.setInt("brdfLUT", 5);
    shaderTex.setInt("heightMap", 6);
    shaderTex.setInt("normalMap", 7);

    glm::vec3 lightPos   = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 lightColor = glm::vec3(400000000000000000000000.0f, 400000000000000000000000.0f, 400000000000000000000000.0f);

    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    
    // configure prefilter map
    // -----------------------
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // configure brdf-lut texture
    // --------------------------
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glm::dvec3 spawnPos = parseCelestialJSONData("resources/planets/earth.json").position / glm::dvec3(1000.0f);
    glm::dvec3 spawnVel = parseCelestialJSONData("resources/planets/earth.json").velocity / glm::dvec3(1000.0f);

    std::vector<CelestialBody> bodies;
    for (unsigned int i = 0; i < planetJSONPaths.size(); i++)
        bodies.push_back(parseCelestialJSONData(planetJSONPaths[i].c_str()));
    
    bodies.push_back(CelestialBody(glm::dvec3(0.0), glm::dvec3(0.0), 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0)); // placeholder for player celestial data

    Rocket rocket(camera, 1000.0f, glm::dvec3(spawnPos.x, spawnPos.y, spawnPos.z + bodies[3].equatorialRadius / 1000.0 + 5000.0f), spawnVel, 0.00000000266972, glm::vec3(5.0f));
    bodies[numOfPlanets] = rocket;

    std::vector<std::string> bodyNames;
    for (unsigned int i = 0; i < planetJSON.size(); i++)
        bodyNames.push_back(getJSONValue(planetJSON[i], "name"));

    bodyNames.push_back("Rocket");

    std::vector<bool> patchesOptions;
    for (unsigned int i = 0; i < planetJSON.size(); i++)
        patchesOptions.push_back(stob(getJSONValue(planetJSON[i], "patchedRendering")));

    std::vector<bool> collidable;
    for (unsigned int i = 0; i < planetJSON.size(); i++)
        collidable.push_back(stob(getJSONValue(planetJSON[i], "isCollidable")));

    std::vector<double> cloudHeights;
    for (unsigned int i = 0; i < cloudsJSON.size(); i++)
        cloudHeights.push_back(cloudsJSON[i].height);

    // initialize buttons
    // ------------------
    HoverButton startButton(startCallback,                       glm::vec2(300.0f, 500.0f), 0.2f, imageShader, start_button,   start_hover);
    HoverButton optionsButton(optionsCallback,                   glm::vec2(300.0f, 420.0f), 0.2f, imageShader, options_button, options_hover);
    HoverButton quitButton([window]() { quitCallback(window); }, glm::vec2(300.0f, 340.0f), 0.2f, imageShader, quit_button,    quit_hover);
    Button optionsClose(optionsCloseCallback,                    glm::vec2(SCR_WIDTH - SCR_WIDTH / 20.0f, SCR_HEIGHT - SCR_HEIGHT / 10.0f), 0.2f, imageShader, options_close);

    Button rtgESCButton(rtgCallback,                             glm::vec2(SCR_WIDTH / 2, SCR_HEIGHT / 2 + 50.0f), 0.3f, imageShader, rtg_button);
    Button rtmmESCButton(rtmmCallback,                           glm::vec2(SCR_WIDTH / 2, SCR_HEIGHT / 2 - 50.0f), 0.3f, imageShader, rtmm_button);

    ThreadPool planetRenderThreads(numOfPlanets);
    std::atomic<int> threadsLeft = 0;

    double majorRadius = 0.0;
    double minorRadius = 99999999999999999999999999999.0;

    glm::dvec3 semiMajorAxis, semiMinorAxis;

    // render to optical depth texture
    // -------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, optDepthTex);
    glViewport(0, 0, 4096, 4096);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        optDepthShader.use();
        renderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    saveTexture(optDepthColor, getFilePath("resources/textures/HDR"), "optical_depth", 4096, 4096, GL_RED, GL_FLOAT, EXT_HDR);

    glm::dmat4 prevRocketModel = glm::dmat4(1.0);

    double lastSemiMajorAxis = 0.0;
    double lastEccentricity  = 0.0;
    double lastInclination   = 0.0;
    double lastTime          = 0.0;
    double lastDeltaVelocity = 0.0;

    glm::dvec3 lastVelocityVector = glm::dvec3(0.0);
    glm::dvec3 lastRocketVelocity = glm::dvec3(0.0);

    bool lastDeltaVelocityShow = false;

    Transfer transfer;
    glm::dvec3 transferWindow;

    OrbitalObject rocketOrbital;
    OrbitalObject moonOrbital;
    std::mutex orbitalElementsMutex;

    std::thread navigatorThread([&]() {
        while (!glfwWindowShouldClose(window))
        {
            if (orbitView)
            {
                {
                    std::lock_guard<std::mutex> lock_rom(orbitalElementsMutex);
                    rocketOrbital = findOrbitalElements(bodies[numOfPlanets].position - bodies[3].position, bodies[numOfPlanets].velocity - bodies[3].velocity, bodies[3].mu);
                    moonOrbital   = findOrbitalElements(bodies[4].position - bodies[3].position, bodies[4].velocity - bodies[3].velocity, bodies[3].mu);
                }

                if ((((abs(rocketOrbital.orbit.semiMajorAxis - lastSemiMajorAxis) > 10000) || (abs(rocketOrbital.orbit.eccentricity - lastEccentricity) > 0.01) || (abs(rocketOrbital.orbit.inclination - lastInclination) > 0.1)) || (glfwGetTime() - lastTime) * timeMultiplier >= 10.0) && (rocketOrbital.orbit.semiMajorAxis * (1 - rocketOrbital.orbit.eccentricity) > bodies[3].averageRadius) && (pow(glm::length(rocketOrbital.state.v), 2) / 2.0 - bodies[3].mu / glm::length(rocketOrbital.state.r) < 0.0))
                {
                    Transfer moonTransfer = findTransferWindow(0.0, 150.0, 70.0, 120.0, 1.0, 1.0, rocketOrbital, moonOrbital, bodies[3].mu);

                    if (moonTransfer.departureTime <= 150.0 && std::abs(moonTransfer.deltaVelocity - lastDeltaVelocity) >= 1000.0 && !(enginesOn && throttle > 0.0))
                    {
                        transfer           = moonTransfer;
                        transferWindow     = findMeanPosition(transfer.departureTime, rocketOrbital, bodies[3].mu);

                        lastDeltaVelocity  = transfer.deltaVelocity;
                        lastVelocityVector = transfer.velocityVector;
                        lastRocketVelocity = bodies[numOfPlanets].velocity;
                    }

                    lastSemiMajorAxis = rocketOrbital.orbit.semiMajorAxis;
                    lastEccentricity  = rocketOrbital.orbit.eccentricity;
                    lastInclination   = rocketOrbital.orbit.inclination;
                    
                    lastTime = glfwGetTime();
                }
                else if ((glfwGetTime() - lastTime) * timeMultiplier >= 10.0)
                {
                    lastTime = glfwGetTime();
                }
            }
        }
    });

    navigatorThread.detach();

    glm::dquat orbitalCameraQuaternion, localQuaternion;

    glm::dvec3 lastD = glm::dvec3(0.0);

    bool firstD = true;
    bool lastViewSwitch = false;

    double lastYaw, lastPitch = 0.0;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        timestamp += 1;

        // input
        // -----
        processInput(window);
        rocket.processControls(window, camera);

        // get cursor position
        // -------------------
        glfwGetCursorPos(window, &mouseInput.mouseX, &mouseInput.mouseY);

        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        if (timestamp == 1 && deltaTime > 0.1f)
            deltaTime = 0.1f;            
        lastFrame = currentFrame;

        float fps = 1.0f / deltaTime;

        timeMultiplier = timewarpValues[timeMultiplierIndex];

        rocket.update(deltaTime);

        for (unsigned int i = 0; i < 100; i++)
        {
            unsigned int j = 0;
            for (auto &body : bodies)
            {
                if (!menuState.inMenu && !menuState.escMenu)
                {
                    body.updateObject(bodies, deltaTime * timeMultiplier / 100);
                    
                    if (j == numOfPlanets)
                    {
                        rocket.position          = bodies[numOfPlanets].position;
                        rocket.velocity          = bodies[numOfPlanets].velocity;
                        rocket.totalAcceleration = bodies[numOfPlanets].totalAcceleration;
                    }
                }

                j++;
            }
        }

        float yaw   = camera.Yaw   / 57.296f;
        float pitch = camera.Pitch / 57.296f;

        float deltaYaw   = yaw   - lastYaw;
        float deltaPitch = pitch - lastPitch;

        // calculate thrust acceleration of the rocket and calculate total acceleration
        glm::dvec3 thrustAccel = glm::normalize(rocket.rotationQuaternion * glm::dvec3(0.0, 1.0, 0.0)) * 70.0;
        glm::dvec3 totalAccel = bodies[numOfPlanets].totalAcceleration + thrustAccel;

        bodies[numOfPlanets].velocity += (enginesOn ? (double)(throttle/100) : 0) * (totalAccel * ((double)deltaTime * timeMultiplier));

        // check whatever body we are close to
        bool isViewSwitchHeight = false;
        unsigned int viewSwitchBody;
        double distance = INFINITY;
        for (unsigned int i = 0; i < numOfPlanets; i++)
        {
            if (glm::length(bodies[numOfPlanets].position - bodies[i].position) - bodies[i].averageRadius <= bodies[i].viewSwitchHeight)
            {
                isViewSwitchHeight = true;

                if (!lastViewSwitch)
                    firstD = true;

                lastViewSwitch = true;
            }
            else if (!lastViewSwitch)
            {
                lastViewSwitch = false;
            }

            if (glm::length(bodies[numOfPlanets].position - bodies[i].position) < distance)
            {
                distance       = glm::length(bodies[numOfPlanets].position - bodies[i].position);
                viewSwitchBody = i;
            }
        }

        glm::dvec3 d = glm::normalize(bodies[numOfPlanets].position - bodies[viewSwitchBody].position);

        if (firstD)
        {
            orbitalCameraQuaternion = glm::dquat(glm::dvec3(0.0, 1.0, 0.0), d);
            localQuaternion         = glm::dquat(1.0, 0.0, 0.0, 0.0);

            lastD = d;
            firstD = false;

        }
        else
        {
            localQuaternion = glm::angleAxis((double)-deltaYaw, glm::dvec3(0.0, 1.0, 0.0)) * localQuaternion * glm::angleAxis((double)-deltaPitch, glm::dvec3(1.0, 0.0, 0.0));
            orbitalCameraQuaternion = glm::normalize(glm::dquat(glm::dot(lastD, d) + 1.0, glm::cross(lastD, d)) * orbitalCameraQuaternion);

            lastD = d;
        }

        lastYaw   = yaw;
        lastPitch = pitch;

        glm::dvec3 orbitalCameraPosition = (isViewSwitchHeight ? (orbitalCameraQuaternion * localQuaternion) : localQuaternion) * glm::dvec3(0.0, 0.0, static_cast<double>(camera.Zoom));
        camera.OrbitalCameraPosition = orbitalCameraPosition / glm::dvec3(static_cast<double>(camera.Zoom));

        if (menuState.inMenu)
            camera.Position = glm::dvec3(bodies[3].position.x/sscale + 1000000.0f, bodies[3].position.y/sscale + 1000000.0f, bodies[3].position.z/sscale + 50000000.0f);
        else
            camera.Position = glm::dvec3(bodies[numOfPlanets].position.x/sscale, bodies[numOfPlanets].position.y/sscale, bodies[numOfPlanets].position.z/sscale) - orbitalCameraPosition;

        TrajectorySimulator rocketTrajectory;
        if (glm::length(bodies[numOfPlanets].position - bodies[4].position) > 66100000.0)
            rocketTrajectory = TrajectorySimulator(bodies[numOfPlanets], bodies[3], 3);
        else
            rocketTrajectory = TrajectorySimulator(bodies[numOfPlanets], bodies[4], 4);

        TrajectorySimulator moonTrajectory(bodies[4], bodies[3], 3);
        TrajectorySimulator earthTrajectory(bodies[3], bodies[0], 0);

        if (orbitView && !menuState.inMenu)
        {
            {
                std::lock_guard<std::mutex> lock_rom(orbitalElementsMutex);
                rocketTrajectory.simulateTrajectory();

                /*if (rocketTrajectory.moonSoi)
                {
                    rocketTrajectoryMoon.moonSoi = true;
                    rocketTrajectoryMoon.simulateTrajectory(moonOrbital, true);
                }*/
            }

            moonTrajectory.simulateTrajectory();
            //earthTrajectory.simulateTrajectory();
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
            glEnable(GL_DEPTH_TEST);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100000000000000000.0f); // 100 quintillion meters far plane (1e17, or 100 trillion (1e14) km)
            glm::mat4 orthoProjection = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);
            glm::mat4 view = camera.GetViewMatrix(bodies[numOfPlanets].position, (isViewSwitchHeight ? (orbitalCameraQuaternion * localQuaternion) : localQuaternion) * glm::dvec3(0.0, 1.0, 0.0));

            // setup shaders
            // -------------
            shader.use();
            shader.setMat4("projection", projection);
            shader.setMat4("view", view);

            lightShader.use();
            lightShader.setMat4("projection", projection);
            lightShader.setMat4("view", view);

            shaderTex.use();
            shaderTex.setMat4("projection", projection);
            shaderTex.setMat4("view", view);

            hdrShader.use();
            hdrShader.setMat4("projection", projection);
            hdrShader.setMat4("view", view);
            hdrShader.setMat4("invViewProj", glm::inverse(projection * view));

            shader.use();

            shader.setVec3("albedo", glm::vec3(1.0f, 0.0f, 0.0f));
            shader.setFloat("ao", 1.0f);
            shader.setVec3("camPos", glm::vec3(0.0f, 0.0f, 0.0f));
            for (unsigned int i = 0; i < 1; i++) {
                shader.setVec3("lightPositions[" + std::to_string(i) + "]", -camera.Position);
                shader.setVec3("lightColors[" + std::to_string(i) + "]", glm::vec3(lightColor[0], lightColor[1], lightColor[2]));
            }

            hdrShader.use();
            hdrShader.setVec3("camPos", glm::vec3(0.0f, 0.0f, 0.0f));
            hdrShader.setVec3("lightPos", -camera.Position);
            
            shaderTex.use();

            shaderTex.setVec3("camPos", glm::vec3(0.0f, 0.0f, 0.0f));
            for (unsigned int i = 0; i < 1; i++) {
                shaderTex.setVec3("lightPositions[" + std::to_string(i) + "]", -camera.Position);
                shaderTex.setVec3("lightColors[" + std::to_string(i) + "]", glm::vec3(lightColor[0], lightColor[1], lightColor[2]));
            }

            // bind PBR textures
            bindPBRTextures(irradianceMap, prefilterMap, brdfLUTTexture);

            glm::dmat4 model = glm::dmat4(1.0);

            std::vector<bool> renderState(numOfPlanets);
            std::mutex renderStateMutex;
            std::vector<glm::mat4> modelMatrices(numOfPlanets);
            std::mutex modelMatricesMutex;
            std::vector<glm::mat4> rotationMatrices(numOfPlanets);
            std::mutex rotationMatricesMutex;
            std::vector<unsigned int> numOfSegments(numOfPlanets);
            std::mutex numOfSegmentsMutex;
            std::vector<glm::dvec3> planetScales(numOfPlanets);
            std::mutex planetScalesMutex;
            std::vector<bool> collisionTestState(numOfPlanets);
            std::mutex collisionTestStateMutex;

            // looped planet initialization
            // ----------------------------
            for (unsigned int i = 0; i < numOfPlanets; i++)
            {
                if (i == numOfPlanets)
                    continue;

                threadsLeft++;
                planetRenderThreads.enqueue([&, i] {
                    float distanceToPlanet = glm::length(static_cast<glm::vec3>(bodies[i].position - camera.Position));
                    float apparentSize = bodies[i].averageRadius / distanceToPlanet;

                    if (apparentSize > 0.001)
                    {
                        {
                            std::lock_guard<std::mutex> lock_rsm(renderStateMutex);
                            renderState[i] = true;
                        }

                        glm::dmat4 planet_model(1.0);

                        glm::dvec3 planetScale = glm::dvec3(bodies[i].equatorialRadius, bodies[i].polarRadius, bodies[i].equatorialRadius);
                        float rotationAroundAxis = glm::radians(bodies[i].rotationSpeed * timeMultiplier) * static_cast<float>(glfwGetTime());
                        setupPlanetModel(planet_model, bodies[i].position, planetScale, camera.Position, bodies[i].axialTilt, rotationAroundAxis);

                        {
                            std::lock_guard<std::mutex> lock_mmm(modelMatricesMutex);
                            modelMatrices[i] = planet_model;
                        }

                        glm::mat3 rotationMatrixY = glm::mat3(glm::vec3(glm::cos(rotationAroundAxis), 0.0f, glm::sin(rotationAroundAxis)), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-glm::sin(rotationAroundAxis), 0.0f, glm::cos(rotationAroundAxis)));                                                                                                                          // rotation matrix for Y axis
                        glm::mat3 rotationMatrixZ = glm::mat3(glm::vec3(glm::cos(glm::radians(bodies[i].axialTilt)), -glm::sin(glm::radians(bodies[i].axialTilt)), 0.0f), glm::vec3(glm::sin(glm::radians(bodies[i].axialTilt)), glm::cos(glm::radians(bodies[i].axialTilt)), 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));  // rotation matrix for Z axis
                        glm::mat3 rotationMatrix = rotationMatrixY * rotationMatrixZ;

                        {
                            std::lock_guard<std::mutex> lock_rmm(rotationMatricesMutex);
                            rotationMatrices[i] = rotationMatrix;
                        }

                        unsigned int minSeg = 8;
                        unsigned int maxSeg = 256;

                        unsigned int k = 256;
                        unsigned int segments = (unsigned int)(k * apparentSize);

                        segments = std::clamp(segments, minSeg, maxSeg);

                        {
                            std::lock_guard<std::mutex> lock_nsm(numOfSegmentsMutex);
                            numOfSegments[i] = segments;
                        }

                        if (apparentSize > 0.1)
                        {
                            {
                                std::lock_guard<std::mutex> lock_ctm(collisionTestStateMutex);
                                collisionTestState[i] = true;
                            }

                            glm::dvec3 planetScale = glm::dvec3(bodies[i].equatorialRadius, bodies[i].polarRadius, bodies[i].equatorialRadius);

                            {
                                std::lock_guard<std::mutex> lock_psm(planetScalesMutex);
                                planetScales[i] = planetScale;
                            }
                        }
                        else
                        {
                            std::lock_guard<std::mutex> lock_ctm(collisionTestStateMutex);
                            collisionTestState[i] = false;
                        }
                    }
                    else
                    {
                        std::lock_guard<std::mutex> lock_rsm(renderStateMutex);
                        renderState[i] = false;
                    }

                    threadsLeft--;
                });
            }

            // wait for all threads to finish
            while (threadsLeft > 0)
                std::this_thread::yield();

            glm::dvec3 collisionPoint;
            unsigned int collisionBody;

            // looped planet rendering
            // -----------------------
            for (unsigned int i = 0; i < numOfPlanets; i++)
            {
                if (!renderState[i])
                    continue;

                planetShaders[i].use();

                if (loadedTextures[i][0] != 0) bindDiffuseTexture(  loadedTextures[i][0]);
                if (loadedTextures[i][1] != 0) bindMetallicTexture( loadedTextures[i][1]);
                if (loadedTextures[i][2] != 0) bindRoughnessTexture(loadedTextures[i][2]);
                if (loadedTextures[i][3] != 0) bindHeightTexture(   loadedTextures[i][3]);
                if (loadedTextures[i][4] != 0) bindNormalTexture(   loadedTextures[i][4]);

                planetShaders[i].setBool("flipHor", planetTextureSettings[i].flipHorizontally);
                planetShaders[i].setBool("skipRM",  planetTextureSettings[i].skipRM);
                planetShaders[i].setMat4("model", static_cast<glm::mat4>(modelMatrices[i]));

                planetShaders[i].setMat3("rotationMatrix", rotationMatrices[i]);

                if (collidable[i] && collisionTestState[i])
                {
                    glm::uvec2 hitSegment = checkForCollision(bodies[numOfPlanets].position, bodies[3].position, bodies[3].equatorialRadius + 10000.0, numOfSegments[i]);
                    
                    glm::dvec3 collisionPosition;
                    collisionPosition = renderSphereCollision(patchesOptions[i], numOfSegments[i], numOfSegments[i], hitSegment, planetScales[i], glm::normalize(bodies[i].position - bodies[numOfPlanets].position), bodies[numOfPlanets].position - bodies[i].position, textureLoad[3][3]);

                    collisionPoint = collisionPosition;
                    collisionBody  = i;
                }
                else
                {
                    renderSphere(patchesOptions[i], numOfSegments[i], numOfSegments[i]);
                }
            }

            /*if (glm::length(bodies[numOfPlanets].position - bodies[3].position) <= glm::length(collisionPoint))
            {
                bodies[numOfPlanets].velocity = bodies[collisionBody].velocity;

                if (glm::length(bodies[numOfPlanets].position - bodies[3].position) < glm::length(collisionPoint))
                    bodies[numOfPlanets].position += (collisionPoint - (bodies[numOfPlanets].position - bodies[3].position));
            }*/

            // render skybox
            // set depth function to GL_LEQUAL so fragment passes if it is less or equal to previous value
            glDepthFunc(GL_LEQUAL);
            // setup matrices and other values
            skyboxShader.use();
            skyboxShader.setInt("environmentMap", 0);
            skyboxShader.setMat4("view", view);
            skyboxShader.setMat4("projection", projection);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
                
            glBindVertexArray(skyboxVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // set depth func to GL_LESS back
            glDepthFunc(GL_LESS);
            
            shader.use();

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, 0);

            model = glm::dmat4(1.0);
            model = glm::translate(model, bodies[numOfPlanets].position - camera.Position);
            model = glm::scale(model, glm::dvec3(1.0));
            glm::dmat4 casted_quat = glm::mat4_cast(rocket.rotationQuaternion);

            if (!glm::approximately_equal_to(casted_quat, glm::mat4(0.0f)))
                model *= glm::mat4_cast(rocket.rotationQuaternion);

            shader.setMat4("model", model);
            shader.setVec3("albedo", glm::vec3(1.0f));
            rocketModel.DrawPatched(shader);

            // render trajectories
            // -------------------
            lineShader.use();
            lineShader.setMat4("projection", orthoProjection);

            if (orbitView && !menuState.inMenu)
            {
                rocketTrajectory.renderTrajectory(camera, view, projection, SCR_WIDTH, SCR_HEIGHT);
                moonTrajectory.renderTrajectory(  camera, view, projection, SCR_WIDTH, SCR_HEIGHT);

                //earthTrajectory.renderTrajectory( camera, view, projection, SCR_WIDTH, SCR_HEIGHT);
            }

            glm::dvec3 relativeVelocity = bodies[numOfPlanets].velocity - bodies[3].velocity;
        
            double velocityScale = glm::length(relativeVelocity) / 200.0;

            glm::dvec3 towardsPlanet   = glm::normalize(bodies[viewSwitchBody].position - bodies[numOfPlanets].position);
            glm::dvec3 rightFromPlanet = glm::cross(glm::dvec3(0.0, 1.0, 0.0), -towardsPlanet);
            glm::dvec3 forward         = glm::cross(-towardsPlanet, rightFromPlanet);

            glm::dvec3 awayFromPlanet  = -towardsPlanet;
            glm::dvec3 leftFromPlanet  = -rightFromPlanet;
            glm::dvec3 backward        = -forward;

            double velocityTowardsPlanet = round(glm::dot(relativeVelocity, towardsPlanet)   * 10.0) / 10.0;
            double velocityRight         = round(glm::dot(relativeVelocity, rightFromPlanet) * 10.0) / 10.0;
            double velocityForward       = round(glm::dot(relativeVelocity, forward)         * 10.0) / 10.0;

            double velocityAwayFromPlanet = round(glm::dot(relativeVelocity, awayFromPlanet) * 10.0) / 10.0;
            double velocityLeftFromPlanet = round(glm::dot(relativeVelocity, leftFromPlanet) * 10.0) / 10.0;
            double velocityBackward       = round(glm::dot(relativeVelocity, backward)       * 10.0) / 10.0;

            if (!menuState.inMenu && orbitView)
            {
                if (!isViewSwitchHeight)
                    renderArrow(bodies[numOfPlanets].position, (float)velocityScale, (glm::vec3)glm::normalize(-relativeVelocity), 5.0f, cylinder, cone, lightShader, camera);

                if (isViewSwitchHeight)
                {
                    if (velocityTowardsPlanet > 0.0)
                        renderArrow(bodies[numOfPlanets].position, (float)velocityTowardsPlanet  / 100.0f, (glm::vec3)-towardsPlanet,   5.0f, cylinder, cone, lightShader, camera);
                    else if (velocityAwayFromPlanet > 0.0)
                        renderArrow(bodies[numOfPlanets].position, (float)velocityAwayFromPlanet / 100.0f, (glm::vec3)-awayFromPlanet,  5.0f, cylinder, cone, lightShader, camera);
                    
                    if (velocityRight > 0.0)
                        renderArrow(bodies[numOfPlanets].position, (float)velocityRight          / 100.0f, (glm::vec3)-rightFromPlanet, 5.0f, cylinder, cone, lightShader, camera);
                    else if (velocityLeftFromPlanet > 0.0)
                        renderArrow(bodies[numOfPlanets].position, (float)velocityLeftFromPlanet / 100.0f, (glm::vec3)-awayFromPlanet,  5.0f, cylinder, cone, lightShader, camera);

                    if (velocityForward > 0.0)
                        renderArrow(bodies[numOfPlanets].position, (float)velocityForward  / 100.0f, (glm::vec3)-forward,  5.0f, cylinder, cone, lightShader, camera);
                    else if (velocityBackward > 0.0)
                        renderArrow(bodies[numOfPlanets].position, (float)velocityBackward / 100.0f, (glm::vec3)-backward, 5.0f, cylinder, cone, lightShader, camera);
                }
            }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        hdrShader.use();

        hdrShader.setInt("numOfPlanets", numOfAtmospheres);

        for (unsigned int i = 0; i < numOfAtmospheres; i++)
        {
            hdrShader.setVec3("planetWorldPos[" + std::to_string(i) + "]", bodies[planetsWithAtmospheres[i]].position - camera.Position);
            hdrShader.setVec3("wavelengths["    + std::to_string(i) + "]", atmospheresSettings[planetsWithAtmospheres[i]].wavelengths);

            hdrShader.setFloat("planetRadius["     + std::to_string(i) + "]", bodies[planetsWithAtmospheres[i]].polarRadius/sscale);
            hdrShader.setFloat("atmosphereHeight[" + std::to_string(i) + "]", (float)atmospheresSettings[planetsWithAtmospheres[i]].height);
        }

        hdrShader.setFloat("densityFalloff", 12.43f);
        hdrShader.setFloat("scatteringStrength", 250000.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthBuffer);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, optDepthColor);

        renderQuad();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        // copy depth from fbo to default framebuffer
        // ------------------------------------------
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        // enable blending
        // ---------------
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // render Earth clouds
        // -------------------
        model = glm::dmat4(1.0);
        shaderTex.use();

        bindDiffuseTexture(loadedCloudTextures[3]);
        bindMetallicTexture(0);
        bindRoughnessTexture(0);
        bindHeightTexture(0);
        bindNormalTexture(loadedTextures[3][4]);

        double rotationAroundAxis = glm::radians(10 * bodies[3].rotationSpeed * timeMultiplier) * static_cast<float>(glfwGetTime());
        setupPlanetModel(model, bodies[3].position, glm::dvec3(bodies[3].averageRadius + cloudHeights[3]), camera.Position, bodies[3].axialTilt, rotationAroundAxis);

        glm::mat3 rotationMatrixY = glm::mat3(glm::vec3(glm::cos(rotationAroundAxis), 0.0f, glm::sin(rotationAroundAxis)), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-glm::sin(rotationAroundAxis), 0.0f, glm::cos(rotationAroundAxis)));                                                                                                                          // rotation matrix for Y axis
        glm::mat3 rotationMatrixZ = glm::mat3(glm::vec3(glm::cos(glm::radians(bodies[3].axialTilt)), -glm::sin(glm::radians(bodies[3].axialTilt)), 0.0f), glm::vec3(glm::sin(glm::radians(bodies[3].axialTilt)), glm::cos(glm::radians(bodies[3].axialTilt)), 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));  // rotation matrix for Z axis
        glm::mat3 rotationMatrix  = rotationMatrixY * rotationMatrixZ;

        shaderTex.setMat4("model", static_cast<glm::mat4>(model));
        shaderTex.setMat3("rotationMatrix", rotationMatrix);
                
        renderSphere(true, 128, 128);

        // calculate fuel consumption and update fuel left
        // -----------------------------------------------
        fuelConsumption = throttle/100 * 12890.0f;
        if (enginesOn)
        {
            fuel -= fuelConsumption * deltaTime;
            if (fuel <= 0.0f)
            {
                fuel = 0.0f;
                enginesOn = false; 
                throttle = 0.0f;
            }
        }

        // render pointers to bodies
        // -------------------------
        if (!menuState.inMenu && orbitView)
        {
            for (unsigned int i = 0; i < bodyNames.size(); i++)
            {
                if (glm::dot(glm::normalize(bodies[i].position - (glm::dvec3)camera.Position), (glm::dvec3)camera.Right) > 0.0) // check if the target is in front of the camera
                {
                    glm::vec2 bodyScreenPos = convert3Dto2D(bodies[i].position - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT);

                    RenderCenteredImage(imageShader, objectNavImg, bodyScreenPos, 0.15f);
                    RenderText(textShader, bodyNames[i], bodyScreenPos.x, bodyScreenPos.y + 35, 0.4f, glm::vec3(1.0f), true);
                }
            }
        }

        // render text
        // -----------
        std::string fps_s      = "FPS: " + std::to_string(fps);
        std::string throttle_s = "Throttle: " + std::format("{:.1f}", throttle) + "%";
        std::string fuel_s     = "Fuel: " + std::format("{:.1f}", fuel / 3000000.0f * 100.0f) + "%"; // divide the left fuel by original fuel and multiply by 100 to get in percents
        std::string tm_s       = "Time multiplier: " + std::to_string(timeMultiplier);

        textShader.use();
        textShader.setMat4("projection", orthoProjection); // switch to orthographic projection for rending text
        imageShader.use();
        imageShader.setMat4("projection", orthoProjection);
        imageShader.setFloat("transparency", 1.0f);
        if (!menuState.inMenu)
        {
            RenderText(textShader, fps_s, 5.0f, SCR_HEIGHT - 15.0f, 0.3f, glm::vec3(1.0f), false); // render at top-left corner with 0.3 size with white color
            RenderText(textShader, throttle_s, SCR_WIDTH/4, 10.0f, 0.5f, glm::vec3(1.0f), true);
            RenderText(textShader, enginesOn ? "Engines: on" : "Engines: off", SCR_WIDTH/2, 10.0f, 0.5f, enginesOn ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f), true);
            RenderText(textShader, fuel_s, 3*SCR_WIDTH/4, 10.0f, 0.5f, glm::vec3(1.0f), true);
            RenderText(textShader, tm_s, SCR_WIDTH / 2, SCR_HEIGHT - 15.0f, 0.3f, glm::vec3(1.0f), true);
            RenderText(textShader, currentDate.date, 3*SCR_WIDTH/4, SCR_HEIGHT - 15.0f, 0.3f, glm::vec3(1.0f), true);
        }

        if (menuState.options)
            RenderCenteredImage(imageShader, options_panel, SCR_WIDTH / 2, SCR_HEIGHT / 2 - 50.0f, 0.85f);

        // render apoapsis and periapsis and also transfer window
        // ------------------------------------------------------
        if (!menuState.inMenu && orbitView)
        {
            if (glm::length(rocketTrajectory.apoapsis) > 0.0)
            {
                glm::vec2 a = glm::vec2(convert3Dto2D(rocketTrajectory.apoapsis - camera.Position,  view, projection, SCR_WIDTH, SCR_HEIGHT).x, convert3Dto2D(rocketTrajectory.apoapsis - camera.Position,  view, projection, SCR_WIDTH, SCR_HEIGHT).y);
                std::string apoapsiss  = std::to_string(std::round((rocketTrajectory.apoapsisd  - bodies[3].averageRadius)  / 100.0)  / 10.0).substr(0, std::to_string(std::round(rocketTrajectory.apoapsisd   / 100.0)  / 10.0).find(".") + 2);

                RenderCenteredImage(imageShader, apoap_periapsi, a.x, a.y, 0.05f);
                RenderText(textShader, apoapsiss  + " km", a.x, a.y + 50.0f, 0.4f, glm::vec3(1.0f), true);
            }

            glm::vec2 p = glm::vec2(convert3Dto2D(rocketTrajectory.periapsis - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT).x, convert3Dto2D(rocketTrajectory.periapsis - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT).y);
            std::string periapsiss = std::to_string(std::round((rocketTrajectory.periapsisd - bodies[3].averageRadius)  / 100.0)  / 10.0).substr(0, std::to_string(std::round(rocketTrajectory.periapsisd  / 100.0)  / 10.0).find(".") + 2);
            
            RenderCenteredImage(imageShader, apoap_periapsi, p.x, p.y, 0.05f);
            RenderText(textShader, periapsiss + " km", p.x, p.y + 50.0f, 0.4f, glm::vec3(1.0f), true);

            glm::vec2 bp = glm::vec2(convert3Dto2D((bodies[numOfPlanets].position + transfer.burnDirection * 5.0) - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT));
            RenderCenteredImage(imageShader, apoap_periapsi, bp.x, bp.y, 0.05f); // burn direсtion point

            if (glm::length(bodies[numOfPlanets].position - (transferWindow + bodies[3].position)) > 200000.0)
            {
                glm::vec2 tw = glm::vec2(convert3Dto2D((transferWindow + bodies[3].position) - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT));
                RenderCenteredImage(imageShader, apoap_periapsi, tw.x, tw.y, 0.05f);
                RenderText(textShader, "Transfer window", tw.x, tw.y + 75.0f, 0.4f, glm::vec3(1.0f), true);
            } 
            else
            {
                if (enginesOn && throttle > 0.0)
                    transferWindow = bodies[numOfPlanets].position - bodies[3].position;

                if (!lastDeltaVelocityShow)
                {
                    lastRocketVelocity    = bodies[numOfPlanets].velocity;
                    lastDeltaVelocityShow = true;
                }

                glm::vec2 tw = glm::vec2(convert3Dto2D(bodies[numOfPlanets].position - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT));
                RenderText(textShader, std::to_string(glm::length(lastRocketVelocity + lastVelocityVector - bodies[numOfPlanets].velocity) / 1000.0) + " km/s", tw.x, tw.y + 75.0f, 0.4f, glm::vec3(1.0f), true);
            }

            // render velocities relative to planet
            // ----------------------------------
            if (isViewSwitchHeight)
            {
                glm::dvec3 relativeVelocity = bodies[numOfPlanets].velocity - bodies[viewSwitchBody].velocity;

                glm::vec2 vtpp = convert3Dto2D((bodies[numOfPlanets].position + towardsPlanet   * (5.0 + velocityTowardsPlanet / 50.0)) - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT);
                glm::vec2 rfpp = convert3Dto2D((bodies[numOfPlanets].position + rightFromPlanet * (5.0 + velocityRight         / 50.0)) - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT);
                glm::vec2 ffpp = convert3Dto2D((bodies[numOfPlanets].position + forward         * (5.0 + velocityForward       / 50.0)) - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT);

                glm::vec2 vapp = convert3Dto2D((bodies[numOfPlanets].position + awayFromPlanet * (5.0 + velocityAwayFromPlanet / 50.0)) - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT);
                glm::vec2 lfpp = convert3Dto2D((bodies[numOfPlanets].position + leftFromPlanet * (5.0 + velocityLeftFromPlanet / 50.0)) - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT);
                glm::vec2 bfpp = convert3Dto2D((bodies[numOfPlanets].position + backward       * (5.0 + velocityBackward       / 50.0)) - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT);

                std::string svtp = std::format("{:.1f}", velocityTowardsPlanet) + " m/s";
                std::string srfp = std::format("{:.1f}", velocityRight)         + " m/s";
                std::string sffp = std::format("{:.1f}", velocityForward)       + " m/s";

                std::string svap = std::format("{:.1f}", velocityAwayFromPlanet) + " m/s";
                std::string slfp = std::format("{:.1f}", velocityLeftFromPlanet) + " m/s";
                std::string sbfp = std::format("{:.1f}", velocityBackward)       + " m/s";

                if (velocityTowardsPlanet > 0.0)
                    RenderText(textShader, svtp, vtpp.x, vtpp.y, 0.4f, glm::vec3(1.0f), true);
                else if (velocityAwayFromPlanet > 0.0)
                    RenderText(textShader, svap, vapp.x, vapp.y, 0.4f, glm::vec3(1.0f), true);

                if (velocityRight > 0.0)
                    RenderText(textShader, srfp, rfpp.x, rfpp.y, 0.4f, glm::vec3(1.0f), true);
                else if (velocityLeftFromPlanet > 0.0)
                    RenderText(textShader, slfp, lfpp.x, lfpp.y, 0.4f, glm::vec3(1.0f), true);

                if (velocityForward > 0.0)
                    RenderText(textShader, sffp, ffpp.x, ffpp.y, 0.4f, glm::vec3(1.0f), true);
                else if (velocityBackward > 0.0)
                    RenderText(textShader, sbfp, bfpp.x, bfpp.y, 0.4f, glm::vec3(1.0f), true);
            }

            // render velocity text
            // --------------------
            if (!isViewSwitchHeight)
            {
                glm::vec2 vtp = convert3Dto2D((glm::normalize(bodies[numOfPlanets].velocity - bodies[3].velocity) * (2 * velocityScale + 7.5) + bodies[numOfPlanets].position) - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT);
                RenderText(textShader, std::format("{:.4f}", glm::length(bodies[numOfPlanets].velocity - bodies[3].velocity) / 1000.0) + " km/s", vtp.x, vtp.y, 0.4f, glm::vec3(1.0f), true);
            }
        }

        //glm::vec2 ecp = glm::vec2(convert3Dto2D((bodies[3].position + collisionPoint) - camera.Position, view, projection, SCR_WIDTH, SCR_HEIGHT));
        //RenderCenteredImage(imageShader, apoap_periapsi, ecp.x, ecp.y, 0.05f);

        // main menu
        // ---------
        if (menuState.inMenu && !menuState.options)
        {
            RenderCenteredImage(imageShader, mmtl, SCR_WIDTH / 4.5, SCR_HEIGHT - 250, 1.0f);
            startButton.Render(  glm::vec2(mouseInput.mouseX - SCR_WIDTH / 2, -(mouseInput.mouseY - SCR_HEIGHT / 2)), mouseInput.lmbPressed, SCR_WIDTH, SCR_HEIGHT);
            optionsButton.Render(glm::vec2(mouseInput.mouseX - SCR_WIDTH / 2, -(mouseInput.mouseY - SCR_HEIGHT / 2)), mouseInput.lmbPressed, SCR_WIDTH, SCR_HEIGHT);
            quitButton.Render(   glm::vec2(mouseInput.mouseX - SCR_WIDTH / 2, -(mouseInput.mouseY - SCR_HEIGHT / 2)), mouseInput.lmbPressed, SCR_WIDTH, SCR_HEIGHT);
        }

        glDepthMask(GL_FALSE);

        // escape menu
        // -----------
        if (menuState.escMenu)
        {
            RenderCenteredImage(imageShader, black_overlap, SCR_WIDTH / 2, SCR_HEIGHT / 2, 1.0f);
            rtgESCButton.Render( glm::vec2(mouseInput.mouseX - SCR_WIDTH / 2, -(mouseInput.mouseY - SCR_HEIGHT / 2)), mouseInput.lmbPressed, SCR_WIDTH, SCR_HEIGHT);
            rtmmESCButton.Render(glm::vec2(mouseInput.mouseX - SCR_WIDTH / 2, -(mouseInput.mouseY - SCR_HEIGHT / 2)), mouseInput.lmbPressed, SCR_WIDTH, SCR_HEIGHT);
        }

        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();

        currentDate.increment(deltaTime * timeMultiplier);
    }

    // de-allocate all resources once they've outlived their purpose
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);

    glDeleteFramebuffers(1, &fbo);
    glDeleteFramebuffers(1, &optDepthTex);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    // close window if escape key is pressed, or open escape menu if in-game
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        if (menuState.inMenu)
        {
            glfwSetWindowShouldClose(window, true);
        }
        else
        {
            if (!keyInput.keyESC_lastFrame)
                menuState.escMenu = !menuState.escMenu;
            keyInput.keyESC_lastFrame = true;
        }
    }
    else
    {
        keyInput.keyESC_lastFrame = false;
    }

    // rocket controls
    // ---------------
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
    {
        if (!keyInput.keyT_lastFrame)
            enginesOn = !enginesOn;
        keyInput.keyT_lastFrame = true;
    }
    else
    {
        keyInput.keyT_lastFrame = false;
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && throttle != 100.0f)
    {
        throttle += 50.0f * deltaTime;
        if (throttle > 100.0f)
            throttle = 100.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && throttle != 0.0f)
    {
        throttle -= 50.0f * deltaTime;
        if (throttle < 0.0f)
            throttle = 0.0f;
    }

    // time control (speed up and slow down)
    if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS)
    {
        if (!keyInput.keyGT_lastFrame && timeMultiplier != timewarpValues[timewarpValues.size() - 1])
            timeMultiplierIndex += 1;
        keyInput.keyGT_lastFrame = true;
    }
    else
    {
        keyInput.keyGT_lastFrame = false;
    }

    if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS)
    {
        if (!keyInput.keyLT_lastFrame && timeMultiplierIndex != 0)
            timeMultiplierIndex -= 1;
        keyInput.keyLT_lastFrame = true;
    }
    else
    {
        keyInput.keyLT_lastFrame = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
    {
        if (!keyInput.keyF1_lastFrame)
            orbitView = !orbitView;
        keyInput.keyF1_lastFrame = true;
    }
    else
    {
        keyInput.keyF1_lastFrame = false;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (!menuState.inMenu && !menuState.escMenu)
    {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);
        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_TRUE)
            camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

// glfw: whenever the mouse click is detected, this callback is called
// -------------------------------------------------------------------
void mouse_click_callback(GLFWwindow* window, int button, int action)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        mouseInput.lmbPressed = true;
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        mouseInput.lmbPressed = false;
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

unsigned int genTexture(Texture texture, bool gamma_correction, bool sixteenFloat)
{
    if (texture == Texture(0, 0, 0, 0))
        return 0;

    unsigned int textureID;
    glGenTextures(1, &textureID);
    if (texture.data)
    {
        GLenum format;
        if (gamma_correction) {
            if (texture.nrComponents == 3)
                format = GL_SRGB;
            else if (texture.nrComponents == 4)
                format = GL_SRGB_ALPHA;
        } else {
            if (texture.nrComponents == 1)
                format = GL_RED;
            else if (texture.nrComponents == 3)
                format = GL_RGB;
            else if (texture.nrComponents == 4)
                format = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        if (gamma_correction) {
            if (format == GL_SRGB_ALPHA)
                glTexImage2D(GL_TEXTURE_2D, 0, format, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture.data);
            else
                glTexImage2D(GL_TEXTURE_2D, 0, format, texture.width, texture.height, 0, sixteenFloat ? GL_RGB16F : GL_RGB, sixteenFloat ? GL_FLOAT : GL_UNSIGNED_BYTE, texture.data);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, format, texture.width, texture.height, 0, format, GL_UNSIGNED_BYTE, texture.data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(texture.data);
    }
    else
    {
        stbi_image_free(texture.data);
    }

    return textureID;
}

Image loadImage(char const * path, bool gamma_correction)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (gamma_correction) {
            if (nrComponents == 3)
                format = GL_SRGB;
            else if (nrComponents == 4)
                format = GL_SRGB_ALPHA;
        } else {
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        if (gamma_correction) {
            if (format == GL_SRGB_ALPHA)
                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            else
                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    Image image;
    image.ImageID = textureID;
    image.Size.x = width;
    image.Size.y = height;

    return image;
}

// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front) 
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(std::string path, std::string filename_start_text, vector<std::string> faces, bool gamma_correction, bool sixteenFloat)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(getFilePath(path + "/" + filename_start_text + faces[i] + ".hdr").c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            if (gamma_correction) {
                if (nrChannels == 4) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB_ALPHA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                } else if (nrChannels == 3) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB, width, height, 0, sixteenFloat ? GL_RGB16F : GL_RGB, sixteenFloat ? GL_FLOAT : GL_UNSIGNED_BYTE, data);
                }
            } else {
                if (nrChannels == 4) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                } else if (nrChannels == 3) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, sixteenFloat ? GL_RGB16F : GL_RGB, sixteenFloat ? GL_FLOAT : GL_UNSIGNED_BYTE, data);
                }
            }
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

static unsigned int quadVAO = 0;
void renderQuad() {
    static float quadVertices[] = {
      // positions           tex coords
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f
    };

    if (quadVAO == 0)
    {
        glGenVertexArrays(1, &quadVAO);

        unsigned int quadVBO;
        glGenBuffers(1, &quadVBO);

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);

        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void renderSphere(bool patches, unsigned int X_SEGMENTS, unsigned int Y_SEGMENTS)
{
    PlanetKey key = {X_SEGMENTS, Y_SEGMENTS};
    if (planetCache.find(key) == planetCache.end())
    {
        PlanetData sphere;
        glGenVertexArrays(1, &sphere.vao);
        glGenBuffers(1, &sphere.vbo);
        glGenBuffers(1, &sphere.ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                glm::vec3 pos = glm::vec3(xPos, yPos, zPos);
                positions.push_back(pos);
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(pos);
            }
        }

        indices.clear();
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x < X_SEGMENTS; ++x)
            {
                indices.push_back(y       * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back(y       * (X_SEGMENTS + 1) + x + 1);

                indices.push_back(y       * (X_SEGMENTS + 1) + x + 1);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
            }
        }
        sphere.indexCount = (unsigned int)indices.size();

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);           
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }

        positions.clear();
        normals.clear();
        uv.clear();

        glBindVertexArray(sphere.vao);
        glBindBuffer(GL_ARRAY_BUFFER, sphere.vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);        
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

        planetCache[key] = sphere;
        data.clear();
    }

    PlanetData& sphere = planetCache[key];
    glBindVertexArray(sphere.vao);
    if (!patches)
        glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);
    else
        glDrawElements(GL_PATCHES, sphere.indexCount, GL_UNSIGNED_INT, 0);
}

glm::dvec3 renderSphereCollision(bool patches, unsigned int X_SEGMENTS, unsigned int Y_SEGMENTS, glm::uvec2 hitSegment, glm::dvec3 scale, glm::dvec3 rayDirection, glm::dvec3 rayOrigin, Texture heightTexture)
{
    PlanetKey key = {X_SEGMENTS, Y_SEGMENTS};
    bool collisionState = false;
    unsigned int insideCounter = 0;

    if (cPlanetCache.find(key) == cPlanetCache.end())
    {
        PlanetData sphere;
        glGenVertexArrays(1, &sphere.vao);
        glGenBuffers(1, &sphere.vbo);
        glGenBuffers(1, &sphere.ebo);

        std::vector<glm::vec2> uv;
        std::vector<unsigned int> indices;

        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                glm::vec3 pos = glm::vec3(xPos, yPos, zPos);
                sphere.positions.push_back(pos);
                uv.push_back(glm::vec2(xSegment, ySegment));
                sphere.texCoords.push_back(glm::vec2(xSegment, ySegment));
                sphere.normals.push_back(pos);
            }
        }

        indices.clear();
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x < X_SEGMENTS; ++x)
            {
                indices.push_back(y       * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back(y       * (X_SEGMENTS + 1) + x + 1);

                indices.push_back(y       * (X_SEGMENTS + 1) + x + 1);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
            }
        }
        sphere.indexCount = (unsigned int)indices.size();

        std::vector<float> data;
        for (unsigned int i = 0; i < sphere.positions.size(); ++i)
        {
            data.push_back(sphere.positions[i].x);
            data.push_back(sphere.positions[i].y);
            data.push_back(sphere.positions[i].z);           
            if (sphere.normals.size() > 0)
            {
                data.push_back(sphere.normals[i].x);
                data.push_back(sphere.normals[i].y);
                data.push_back(sphere.normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }

        glBindVertexArray(sphere.vao);
        glBindBuffer(GL_ARRAY_BUFFER, sphere.vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);        
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

        cPlanetCache[key] = sphere;
        data.clear();
    }

    PlanetData& sphere = cPlanetCache[key];
    glBindVertexArray(sphere.vao);
    if (!patches)
        glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);
    else
        glDrawElements(GL_PATCHES, sphere.indexCount, GL_UNSIGNED_INT, 0);

    glm::dvec3 v0 = (glm::dvec3)sphere.positions[hitSegment.x * (X_SEGMENTS + 1) + hitSegment.y]                          * scale;
    glm::dvec3 v1 = (glm::dvec3)sphere.positions[hitSegment.x * (X_SEGMENTS + 1) + hitSegment.y + 1]                      * scale;
    glm::dvec3 v2 = (glm::dvec3)sphere.positions[((hitSegment.x + 1) % X_SEGMENTS) * (X_SEGMENTS + 1) + hitSegment.y]     * scale;
    glm::dvec3 v3 = (glm::dvec3)sphere.positions[((hitSegment.x + 1) % X_SEGMENTS) * (X_SEGMENTS + 1) + hitSegment.y + 1] * scale;

    v0.x = -v0.x;
    v0.z = -v0.z;

    v1.x = -v1.x;
    v1.z = -v1.z;

    v2.x = -v2.x;
    v2.z = -v2.z;

    v3.x = -v3.x;
    v3.z = -v3.z;

    std::vector<glm::dvec3> ve0 = {v0, v1, v2};
    std::vector<glm::dvec3> ve1 = {v1, v3, v2};

    glm::dvec3 triangle1Hit = rayTriangle(rayOrigin, rayDirection, ve0);
    glm::dvec3 triangle2Hit = rayTriangle(rayOrigin, rayDirection, ve1);

    //double height = getRgbPixel(heightTexture.data, glm::vec2(fmod(2.5f - sphere.texCoords[hitSegment.x * (X_SEGMENTS + 1) + hitSegment.y].x, 1.0f), 1.0f - sphere.texCoords[hitSegment.x * (X_SEGMENTS + 1) + hitSegment.y].y), glm::vec2(heightTexture.width, heightTexture.height)).x / 255.0f * 0.001387f * scale.z;

    //triangle1Hit += glm::normalize(triangle1Hit) * height;
    //triangle2Hit += glm::normalize(triangle2Hit) * height;

    /*glm::dvec3 hitPosition = glm::dvec3(INFINITY);
    double lastt           = INFINITY;
    for (unsigned int x = 0; x < X_SEGMENTS; x++)
    {
        for (unsigned int y = 0; y < Y_SEGMENTS; y++) 
        {
            
            
            glm::dvec3 u = i1 - i0;
            glm::dvec3 v = i2 - i0;

            glm::dvec3 n = glm::normalize(glm::cross(u, v));

            double t = glm::dot(n, (i0 - rayOrigin)) / glm::dot(n, rayDirection);
            glm::dvec3 rayHit = rayOrigin + t * rayDirection;

            if (glm::length(rayHit) < glm::length(hitPosition) && t >= 0.0 && t < lastt)
            {
                hitPosition = rayHit;
                lastt       = t;
            }
        }
    }*/

    //return rayOrigin + raySphere(rayOrigin, glm::dvec3(0.0), scale.x + 15000.0, rayDirection).x * rayDirection;

    //return v0;

    if (glm::length(triangle1Hit) < 1e-5)
        return triangle2Hit;
    else
        return triangle1Hit;
}

void RenderText(Shader &s, std::string text, float x, float y, float scale, glm::vec3 color, bool centered)
{
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
        x -= totalWidth / 2;

    std::vector<float> totalVertices;

    std::string::const_iterator c;
    unsigned int i = 0;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

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

        x += (ch.Advance >> 6) * scale;
        i++;
    }

    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, totalVertices.size() * sizeof(float), totalVertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6 * text.size());
}
