#pragma once

#include "ECSSettings.h"

struct EntityPair
{
    EntityTwice Key;

    static constexpr EntityPair Make(Entity entity1, Entity entity2) noexcept
    {
        return EntityPair{ (static_cast<EntityTwice>(entity1) << 32) | entity2 };
    }

    [[nodiscard]] constexpr Entity Entity1() const noexcept
    {
        return static_cast<Entity>(Key >> 32);
    }

    [[nodiscard]] constexpr Entity Entity2() const noexcept
    {
        return static_cast<Entity>(Key); //Implicit truncation, avoids: Key & 0xFFFFFFFFull
    }

    constexpr bool operator<(const EntityPair& other) const noexcept
    {
        return Key < other.Key;
    }

    constexpr bool operator==(const EntityPair& other) const noexcept
    {
        return Key == other.Key;
    }
};

static_assert(IsTrivial<EntityPair>, "EntityPair needs to be trivial");