#include <functional>
#include <GLFW/glfw3.h>

#include <celestialbody.hpp>
#include <savefilesmanager.hpp>

#pragma once

struct MenuState
{
    bool inMenu = true;
    float transparency = 1.0f;
    bool options = false;

    bool escMenu = false;
};

struct GameState
{
    bool inTutorial = false;
};

MenuState menuState;
GameState gameState;

std::function<void(CelestialBody& player, PlayerSaveData& save)> startCallback = [](CelestialBody& player, PlayerSaveData& save)
{
    menuState.inMenu = false;

    if (!save.isEmpty)
    {
        player.position = save.state.r;
        player.velocity = save.state.v;
    }
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
    menuState.inMenu     = true;
    menuState.escMenu    = false;
    gameState.inTutorial = false;
};
