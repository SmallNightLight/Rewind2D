#pragma once

#include <cstdint>
#include <cassert>
#include <array>
#include <cstring>

#include "ECSSettings.h"

template<uint32_t N_Capacity>
class EntitySet
{
public:
    constexpr EntitySet() noexcept = default;
    EntitySet(const EntitySet&) noexcept = default;
    EntitySet& operator=(const EntitySet&) noexcept = default;

    constexpr void Initialize() noexcept
    {
        m_EntityToIndex.fill(s_InvalidEntity);
        m_EntityCount = 0;
    }

    constexpr bool Insert(Entity entity)
    {
        assert(entity < s_MaxEntities && "Entity out of range");

        if (m_EntityToIndex[entity] != s_InvalidEntity) return false;

        assert(m_EntityCount < Capacity && "EntitySet capacity exceeded");

        uint32_t index = m_EntityCount++;
        m_Entities[index] = entity;
        m_EntityToIndex[entity] = index;

        return true;
    }

    //Removes the entity from the set
    constexpr void Erase(Entity entity)
    {
        assert(entity < s_MaxEntities && "Entity out of range");

        uint32_t index = m_EntityToIndex[entity];

        if (m_EntityCount == 0 || index == s_InvalidEntity) return;

        uint32_t lastIndex = m_EntityCount - 1;
        Entity lastEntity = m_Entities[lastIndex];

        //Swap the last entity into the removed spot
        m_Entities[index] = lastEntity;
        m_EntityToIndex[lastEntity] = index;

        //Invalidate the removed entity
        m_EntityToIndex[entity] = s_InvalidEntity;
        --m_EntityCount;
    }

    [[nodiscard]] constexpr bool Contains(Entity entity) const
    {
        assert(entity < s_MaxEntities && "Entity out of range");
        return m_EntityToIndex[entity] != s_InvalidEntity;
    }

    constexpr void Clear() noexcept
    {
        m_EntityToIndex.fill(s_InvalidEntity);
        m_EntityCount = 0;
    }

    // more optimized then the assignment operator
    void Overwrite(const EntitySet& other) noexcept
    {
        if (this == &other) return;

        std::memcpy(m_Entities.data(), other.m_Entities.data(), other.m_EntityCount * sizeof(Entity));
        std::memcpy(m_EntityToIndex.data(), other.m_EntityToIndex.data(), m_EntityToIndex.size() * sizeof(uint32_t));
        m_EntityCount = other.m_EntityCount;
    }

    [[nodiscard]] constexpr uint32_t Size() const noexcept { return m_EntityCount; }
    [[nodiscard]] constexpr bool Empty() const noexcept { return m_EntityCount == 0; }
    static constexpr uint32_t Capacity = N_Capacity;

    [[nodiscard]] constexpr Entity* begin() noexcept { return m_Entities.data(); }
    [[nodiscard]] constexpr Entity* end() noexcept { return m_Entities.data() + m_EntityCount; }
    [[nodiscard]] constexpr const Entity* begin() const noexcept { return m_Entities.data(); }
    [[nodiscard]] constexpr const Entity* end() const noexcept { return m_Entities.data() + m_EntityCount; }

private:
    std::array<Entity, Capacity> m_Entities;
    std::array<uint32_t, s_MaxEntities> m_EntityToIndex;
    uint32_t m_EntityCount;
};

static_assert(IsTrivial<EntitySet<10>>, "EntitySet needs to be trivial");