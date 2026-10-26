#include <GLFW/glfw3.h>
#include <iostream>
#include <unordered_map>

const std::vector<int> keys = {GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9, GLFW_KEY_0, 
                               GLFW_KEY_Q, GLFW_KEY_W, GLFW_KEY_E, GLFW_KEY_R, GLFW_KEY_T, GLFW_KEY_Y, GLFW_KEY_U, GLFW_KEY_I, GLFW_KEY_O, GLFW_KEY_P, GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET, 
                               GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_J, GLFW_KEY_K, GLFW_KEY_L, GLFW_KEY_SEMICOLON, 
                               GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_Z, GLFW_KEY_X, GLFW_KEY_C, GLFW_KEY_V, GLFW_KEY_B, GLFW_KEY_N, GLFW_KEY_M, GLFW_KEY_COMMA, GLFW_KEY_PERIOD, 
                               GLFW_KEY_EQUAL, GLFW_KEY_MINUS,
                               GLFW_KEY_F1, GLFW_KEY_F2, GLFW_KEY_F3, GLFW_KEY_F4, GLFW_KEY_F5, GLFW_KEY_F6, GLFW_KEY_F7, GLFW_KEY_F8, GLFW_KEY_F9, GLFW_KEY_F10, GLFW_KEY_F11, GLFW_KEY_F12,
                               GLFW_KEY_F13, GLFW_KEY_F14, GLFW_KEY_F15, GLFW_KEY_F16, GLFW_KEY_F17, GLFW_KEY_F18, GLFW_KEY_F19, GLFW_KEY_F20, GLFW_KEY_F21, GLFW_KEY_F22, GLFW_KEY_F23, GLFW_KEY_F24, GLFW_KEY_F25,
                               GLFW_KEY_SPACE, GLFW_KEY_BACKSPACE, GLFW_KEY_DELETE, GLFW_KEY_LEFT_SHIFT, GLFW_KEY_RIGHT_SHIFT, GLFW_KEY_LEFT_CONTROL, GLFW_KEY_RIGHT_CONTROL, GLFW_KEY_TAB, GLFW_KEY_ENTER, GLFW_KEY_ESCAPE};

std::vector<bool> keysPressedLastFrame;

void defaultKPLFfill()
{
    for (unsigned int i = 0; i < keys.size(); i++)
        keysPressedLastFrame.push_back(false);
}

void updateKeysPressedLastFrame(GLFWwindow* window)
{
    keysPressedLastFrame.clear();
    for (unsigned int i = 0; i < keys.size(); i++)
    {
        if (glfwGetKey(window, keys[i]) == GLFW_PRESS)
            keysPressedLastFrame.push_back(true);
        else
            keysPressedLastFrame.push_back(false);
    }
}

unsigned int findIndexOfItem(std::vector<int> input, int item)
{
    for (unsigned int i = 0; i < input.size(); i++)
        if (input[i] == item)
            return i;
}