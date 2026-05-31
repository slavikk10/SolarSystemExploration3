#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <celestialbody.hpp>
#include <filesystem.hpp>
#include <functionsupport.hpp>

struct JSON {
    std::string contents;
    std::stringstream stream;
};

struct TextureSettings {
    bool flipHorizontally;
    bool skipRM;
};

struct CloudsJSON {
    std::string texturePath;
    std::string textureName;
    double height;

    bool operator==(CloudsJSON compared)
    {
        return (texturePath == compared.texturePath) && (height == compared.height);
    }

    CloudsJSON(std::string tp, double h) 
    {
        texturePath = tp;
        height      = h;

        std::string fileNameWithExtension = texturePath.substr(texturePath.find_last_of('/') + 1);
        textureName = fileNameWithExtension.erase(fileNameWithExtension.find_first_of('.'));
    }

    CloudsJSON(std::string tp, std::string fn, double h) {texturePath = tp; textureName = fn; height = h;}
    CloudsJSON() {texturePath = ""; textureName = ""; height = 0.0;}
};

JSON loadJSON(const char* path)
{
    std::string json;
    std::ifstream jsonFile;
    std::stringstream jsonStream;

    jsonFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    // load JSON file by given path
    try {
        jsonFile.open(getFilePath(path).c_str());
        jsonStream << jsonFile.rdbuf();
        jsonFile.close();
        json = jsonStream.str();
    }
    catch(std::ifstream::failure e) {
        std::cout << "Error reading JSON file at (relative) path: " << path << std::endl;
    }

    JSON result;
    result.contents = json;
    result.stream.str(jsonStream.str());
    return result;
}

std::string getJSONValue(JSON& json, std::string key)
{
    std::stringstream stream(json.contents);

    std::string line;

    while (std::getline(stream, line))
    {
        size_t startPos = line.find('"' + key + '"');
        
        if (startPos != std::string::npos)
        {
            size_t colon = line.find(':');
            size_t start = line.find_first_not_of(" \t", colon + 1);

            std::string extracted;

            if (line[start] == '"')
            {
                size_t firstOpenQuote   = line.find('"');
                size_t firstCloseQuote  = line.find('"', firstOpenQuote + 1);
                
                size_t secondOpenQuote  = line.find('"', firstCloseQuote + 1);
                size_t secondCloseQuote = line.find('"', secondOpenQuote + 1);

                extracted = line.substr(secondOpenQuote + 1, secondCloseQuote - secondOpenQuote - 1);
            }
            else
            {
                std::string result;

                if (line.back() == ',')
                    result = line.substr(colon + 1, line.size() - colon - 2);
                else
                    result = line.substr(colon + 1, line.size() - colon);

                result.erase(0, result.find_first_not_of(" \t"));

                extracted = result;
            }

            return extracted;
        }
    }

    return "";
}

JSON getJSONSub(JSON& json, std::string subName)
{
    std::stringstream stream(json.contents);

    std::string line;
    std::string result = "";
    bool write = false;

    size_t stripOffset;

    JSON resultJson;
    std::stringstream resultStream;

    while (std::getline(json.stream, line))
    {
        size_t startPos = line.find(subName + '"' + ':');
        
        if (startPos != std::string::npos)
        {
            write       = true;
            stripOffset = startPos - 1;
        }

        std::string stripped = line.erase(0, stripOffset);

        if (write)
            result += stripped;

        if (stripped[0] != '}')
        {   
            if (write)
                result += '\n';
        }
        else
        {
            write = false;

            resultStream << result;

            resultJson.contents = result;
            resultJson.stream.str(resultStream.str());

            return resultJson;
        }
    }

    return JSON();
}

CelestialBody parseCelestialJSONData(const char* path)
{
    JSON json = loadJSON(path);

    CelestialBody result;
    result.averageRadius       = std::stod(getJSONValue(json, "averageRadius"));
    result.equatorialRadius    = std::stod(getJSONValue(json, "equatorialRadius"));
    result.polarRadius         = std::stod(getJSONValue(json, "polarRadius"));
    result.gravityAcceleration = std::stod(getJSONValue(json, "gravitationalAcceleration"));

    result.position = glm::dvec3(std::stod(getJSONValue(json, "x")),         std::stod(getJSONValue(json, "y")),         std::stod(getJSONValue(json, "z")))         * 1000.0;
    result.velocity = glm::dvec3(std::stod(getJSONValue(json, "velocityX")), std::stod(getJSONValue(json, "velocityY")), std::stod(getJSONValue(json, "velocityZ"))) * 1000.0;

    result.axialTilt     = std::stod(getJSONValue(json, "axialTilt"));
    result.rotationSpeed = std::stod(getJSONValue(json, "rotationSpeed"));

    result.viewSwitchHeight = std::stod(getJSONValue(json, "viewSwitchHeight"));

    return result;
}

std::vector<std::string> parseTexturesJSON(const char* path)
{
    JSON json        = loadJSON(path);
    JSON textureJSON = getJSONSub(json, "textures");

    std::vector<std::string> result;
    result.push_back(getJSONValue(textureJSON, "albedo"));
    result.push_back(getJSONValue(textureJSON, "roughness"));
    result.push_back(getJSONValue(textureJSON, "metallic"));
    result.push_back(getJSONValue(textureJSON, "height"));
    result.push_back(getJSONValue(textureJSON, "normal"));

    return result;
}

TextureSettings parseTextureSettingsJSON(const char* path)
{
    JSON json                = loadJSON(path);
    JSON textureSettingsJSON = getJSONSub(json, "textureSettings");

    TextureSettings result;
    result.flipHorizontally = stob(getJSONValue(textureSettingsJSON, "flipHorizontally"));
    result.skipRM           = stob(getJSONValue(textureSettingsJSON, "skipRM"));
    
    return result;
}

CloudsJSON parseCloudsJSON(const char* path)
{
    JSON json      = loadJSON(path);
    bool hasClouds = stob(getJSONValue(json, "hasClouds"));

    CloudsJSON result;
    if (hasClouds)
    {
        JSON cloudSettings = getJSONSub(json, "cloudSettings");
        result = CloudsJSON(getJSONValue(cloudSettings, "cloudTexture"), std::stod(getJSONValue(cloudSettings, "height")));
    }
    else
    {
        result = CloudsJSON();
    }

    return result;
}

// MIGHT NEED THIS FOR LATER

/*CelestialBody parsePlanetJSON(const char* path)
{
    std::string json;
    std::ifstream jsonFile;
    std::stringstream jsonStream;

    jsonFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    // load JSON file by given path
    try {
        jsonFile.open(getFilePath(path).c_str());
        jsonStream << jsonFile.rdbuf();
        jsonFile.close();
        json = jsonStream.str();
    }
    catch(std::ifstream::failure e) {
        std::cout << "Error reading JSON file at (relative) path: " << path << std::endl;
    }

    // parse the loaded JSON file line-by-line
    std::string line;
    std::string name;
    std::string s_g_accel;
    std::string s_avg_radius;
    std::string s_eq_radius;
    std::string s_polar_radius;
    glm::dvec3 position;
    glm::dvec3 velocity;
    std::string x;
    std::string y;
    std::string z;
    std::string vel_x;
    std::string vel_y;
    std::string vel_z;
    std::string s_axial_tilt;
    std::string s_rotation_speed;
    std::string s_view_switch_height;

    double g_accel;
    double avg_radius;
    double eq_radius;
    double polar_radius;
    double axial_tilt;
    double rotation_speed;
    double view_switch_height;

    unsigned int err = 14;
    while (std::getline(jsonStream, line)) {
        size_t name_pos = line.find("name");
        if (name_pos != std::string::npos) {
            err -= 1;
            // check if there is space between : and key value or not
            size_t start;
            if (line[name_pos+6] == '\"')
                start = name_pos+7;
            else
                start = name_pos+8;

            size_t lastQuote = line.find('\"', start + 1);
            name = line.substr(start, lastQuote - start);
        }

        size_t s_g_accel_pos = line.find("gravitationalAcceleration");
        if (s_g_accel_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[s_g_accel_pos+27] == ' ')
                start = s_g_accel_pos+28;
            else
                start = s_g_accel_pos+27;

            size_t lastComma = line.find(',', start + 1);
            s_g_accel = line.substr(start, lastComma - start);
        }

        size_t s_avg_radius_pos = line.find("averageRadius");
        if (s_avg_radius_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[s_avg_radius_pos+15] == ' ')
                start = s_avg_radius_pos+16;
            else
                start = s_avg_radius_pos+15;

            size_t lastComma = line.find(',', start + 1);
            s_avg_radius = line.substr(start, lastComma - start);
        }

        // equatorial radius (float, used for rendering)
        size_t s_eq_radius_pos = line.find("equatorialRadius");
        if (s_eq_radius_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[s_eq_radius_pos+18] == ' ')
                start = s_eq_radius_pos+19;
            else
                start = s_eq_radius_pos+18;

            size_t lastComma = line.find(',', start + 1);
            s_eq_radius = line.substr(start, lastComma - start);
        }

        // polar radius (float, used for rendering)
        size_t s_polar_radius_pos = line.find("polarRadius");
        if (s_polar_radius_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[s_polar_radius_pos+13] == ' ')
                start = s_polar_radius_pos+14;
            else
                start = s_polar_radius_pos+13;

            size_t lastComma = line.find(',', start + 1);
            s_polar_radius = line.substr(start, lastComma - start);
        }

        // position (vec3)
        size_t x_pos = line.find("\"x");
        size_t y_pos = line.find("\"y");
        size_t z_pos = line.find("\"z");
        if (x_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[x_pos+4] == ' ')
                start = x_pos+5;
            else
                start = x_pos+4;

            size_t lastComma = line.find(',', start);
            x = line.substr(start, lastComma - start);
            position.x = std::stod(x);
        }
        if (y_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[y_pos+4] == ' ')
                start = y_pos+5;
            else
                start = y_pos+4;

            size_t lastComma = line.find(',', start + 1);
            y = line.substr(start, lastComma - start);
            position.y = std::stod(y);
        }
        if (z_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[z_pos+4] == ' ')
                start = z_pos+5;
            else
                start = z_pos+4;

            size_t lastComma = line.find(',', start + 1);
            z = line.substr(start, lastComma - start);
            position.z = std::stod(z);
        }

        // velocity (vec3)
        size_t vel_x_pos = line.find("velocityX");
        size_t vel_y_pos = line.find("velocityY");
        size_t vel_z_pos = line.find("velocityZ");
        if (vel_x_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[vel_x_pos+11] == ' ')
                start = vel_x_pos+12;
            else
                start = vel_x_pos+11;

            size_t lastComma = line.find(',', start);
            vel_x = line.substr(start, lastComma - start);
            velocity.x = std::stod(vel_x);
        }
        if (vel_y_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[vel_y_pos+11] == ' ')
                start = vel_y_pos+12;
            else
                start = vel_y_pos+11;

            size_t lastComma = line.find(',', start + 1);
            vel_y = line.substr(start, lastComma - start);
            velocity.y = std::stod(vel_y);
        }
        if (vel_z_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[vel_z_pos+11] == ' ')
                start = vel_z_pos+12;
            else
                start = vel_z_pos+11;

            size_t lastComma = line.find(',', start + 1);
            vel_z = line.substr(start, lastComma - start);
            velocity.z = std::stod(vel_z);
        }

        // axial tilt (float, used for rotating the planet)
        size_t s_axial_tilt_pos = line.find("axialTilt");
        if (s_axial_tilt_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[s_axial_tilt_pos+11] == ' ')
                start = s_axial_tilt_pos+12;
            else
                start = s_axial_tilt_pos+11;

            size_t lastComma = line.find(',', start + 1);
            s_axial_tilt = line.substr(start, lastComma - start);
        }

        // rotation speed (float, used to rotate the planet around it's axis)
        size_t s_rotation_speed_pos = line.find("rotationSpeed");
        if (s_rotation_speed_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[s_rotation_speed_pos+15] == ' ')
                start = s_rotation_speed_pos+16;
            else
                start = s_rotation_speed_pos+15;

            size_t lastComma = line.find(',', start + 1);
            s_rotation_speed = line.substr(start, lastComma - start);
        }

        // view switch height (altitude above planet's surface at which camera view becomes relative to planet)
        size_t s_view_switch_height_pos = line.find("viewSwitchHeight");
        if (s_view_switch_height_pos != std::string::npos) {
            err -= 1;

            size_t start;
            if (line[s_view_switch_height_pos+18] == ' ')
                start = s_view_switch_height_pos+19;
            else
                start = s_view_switch_height_pos+18;

            size_t lastComma = line.find(',', start + 1);
            s_view_switch_height = line.substr(start, lastComma - start);
        }

    }

    if (err != 0) {
        std::cout << "Error: JSON file error at path: " << getFilePath(path) << std::endl;
    }

    g_accel            = std::stod(s_g_accel);
    avg_radius         = std::stod(s_avg_radius);
    eq_radius          = std::stod(s_eq_radius);
    polar_radius       = std::stod(s_polar_radius);
    axial_tilt         = std::stod(s_axial_tilt);
    rotation_speed     = std::stod(s_rotation_speed);
    view_switch_height = std::stod(s_view_switch_height);

    CelestialBody body(position, velocity, avg_radius, eq_radius, polar_radius, g_accel, axial_tilt, rotation_speed, view_switch_height);

    return body;
}*/