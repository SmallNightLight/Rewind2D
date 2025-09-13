#pragma once

#include <cstdint>

//Window
static constexpr int32_t s_ScreenWidth = 800;
static constexpr int32_t s_ScreenHeight = 600;

static constexpr short NetworkFPS = 15;
static constexpr uint8_t SimulationFPS = 60;
static constexpr short RenderingFPS = 60;

static constexpr uint64_t MaxFrameTimeNS = 250'000'000;       //Time in ms for the maximum frame time

static constexpr bool RollbackDebugMode = true;     //Always rollback as much as possible

// static const std::vector<uint16_t> playerInputKeys =
// {
//     GLFW_KEY_W, GLFW_KEY_S, GLFW_KEY_A, GLFW_KEY_D, GLFW_KEY_Q, GLFW_KEY_E, GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_RIGHT, GLFW_MOUSE_BUTTON_MIDDLE
// };
//
// static const std::vector<uint16_t> otherInputKeys =
// {
//     GLFW_KEY_ESCAPE, GLFW_KEY_RIGHT, GLFW_KEY_SPACE, GLFW_KEY_R, GLFW_KEY_LEFT_CONTROL, GLFW_KEY_C, GLFW_KEY_V
// };