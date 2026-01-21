#pragma once

#include "ECSSettings.h"

struct EntityPair
{
    EntityTwice Key;

    static constexpr EntityPair Make(Entity entity1, Entity entity2) noexcept
    {
        return EntityPair{ (static_cast<EntityTwice>(entity1) << 32) | entity2 };
    }

    [[nodiscard]] inline constexpr Entity Entity1() const noexcept
    {
        return static_cast<Entity>(Key >> 32);
    }

    [[nodiscard]] inline constexpr Entity Entity2() const noexcept
    {
        return static_cast<Entity>(Key & 0xFFFFFFFFull);
    }

    inline constexpr bool operator<(const EntityPair& other) const noexcept
    {
        return Key < other.Key;
    }

    inline constexpr bool operator==(const EntityPair& other) const noexcept
    {
        return Key == other.Key;
    }
};