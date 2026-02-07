#include <functional>
#include <glfw/glfw3.h>

#pragma once

struct MenuState
{
    bool inMenu = true;
    float transparency = 1.0f;
    bool options = false;

    bool escMenu = false;
};

MenuState menuState;

std::function<void()> startCallback = []()
{
    menuState.inMenu = false;
};

std::function<void()> optionsCallback = []()
{
    menuState.options = true;
};

std::function<void()> optionsCloseCallback = []()
{
    menuState.options = false;
};

std::function<void(GLFWwindow* window)> quitCallback = [](GLFWwindow* window)
{
    glfwSetWindowShouldClose(window, true);
};

std::function<void()> rtgCallback = []()
{
    menuState.escMenu = false;
};

std::function<void()> rtmmCallback = []()
{
    menuState.inMenu  = true;
    menuState.escMenu = false;
};
