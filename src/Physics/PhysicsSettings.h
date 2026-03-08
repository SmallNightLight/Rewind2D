#pragma once

#include "../ECS/ECSSettings.h"
#include "../Math/FixedTypes.h"

using GraphColor = uint8_t; //Needs to be able to hold the number s_MaxColors

constexpr uint8_t PhysicsIterations = 5;
static constexpr uint32_t MaxCollisionCount = 500;
inline constexpr UInt_E s_MaxPairs = s_MaxEntities * 4;
static constexpr uint8_t s_MaxColors = 9; //Last color is for remaining pairs

constexpr int64_t VelocityEpsilon = (Fixed16_16(1) / Fixed16_16(1000)).GetValueIntermediate();
constexpr Fixed16_16 AngularVelocityEpsilon = Fixed16_16(1) / Fixed16_16(100);

constexpr  Vector2 Gravity = Vector2(Fixed16_16(0), Fixed16_16(-10));

using CollisionHash = std::uint32_t; //TODO

//Debug
constexpr bool PhysicsDebugMode = true;
constexpr bool LogCollisions = false;