#pragma once

#include <cstdint>

//Window
static constexpr int32_t s_ScreenWidth = 800;
static constexpr int32_t s_ScreenHeight = 600;

//Simulation rates
static constexpr short s_NetworkFPS = 15;
static constexpr uint8_t s_SimulationFPS = 60;
static constexpr short s_RenderingFPS = 60;

static constexpr uint64_t s_MaxFrameTimeNS = 250'000'000;     //Time in ms for the maximum frame time

static constexpr uint8_t s_MaxRollBackFrames = 15;            //Amount of save states
static constexpr bool s_RollbackDebugMode = true;             //Always rollback as much as possible