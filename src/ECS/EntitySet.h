#pragma once

#include <cstdint>
#include <cassert>
#include <array>
#include <algorithm>

#include "ECSSettings.h"

class EntitySet
{
public:
    constexpr EntitySet() noexcept = default;
    EntitySet(const EntitySet&) noexcept = default;
    EntitySet& operator=(const EntitySet&) noexcept = default;

    constexpr void Initialize() noexcept
    {
        std::fill_n(m_Data.begin(), s_MaxEntities, s_Empty);
        m_Data[s_EntityTail] = s_EntityTail;
        m_Data[s_Empty] = s_EntityTail;
        m_Head = s_Empty;
        m_Tail = s_Empty;
        m_EntityCount = 0;
    }

    constexpr bool Insert(Entity entity)
    {
        assert(entity < s_MaxEntities && "Entity out of range");

        if (Contains(entity)) return false;

        ++m_EntityCount;

        if (m_Head == s_Empty)
        {
            //No entities in set
            m_Head = entity;
            m_Tail = entity;
            m_Data[entity] = s_EntityTail;
        }
        else if (entity < m_Head)
        {
            m_Data[entity] = m_Head;
            m_Head = entity;
        }
        else if (entity > m_Tail)
        {
            m_Data[m_Tail] = entity;
            m_Data[entity] = s_EntityTail;
            m_Tail = entity;
        }
        else
        {
            //Search backwards to find predecessor
            for (Entity i = entity - 1; i > m_Head; --i)
            {
                if (Contains(i))
                {
                    m_Data[entity] = m_Data[i];
                    m_Data[i] = entity;
                    return true;
                }
            }

            assert(Contains(m_Head) && "Could not find predecessor in EntitySet");

            m_Data[entity] = m_Data[m_Head];
            m_Data[m_Head] = entity;
        }

        return true;
    }

    //Removes the entity from the set
    constexpr bool Erase(Entity entity)
    {
        assert(entity < s_MaxEntities && "Entity out of range");

        if (m_EntityCount == 0 || !Contains(entity)) return false;

        --m_EntityCount;

        if (m_EntityCount == 0)
        {
            m_Data[entity] = s_Empty;
            m_Head = s_Empty;
            m_Tail = s_Empty;
        }
        else if (entity == m_Head)
        {
            m_Head = m_Data[entity];
            m_Data[entity] = s_Empty;
        }
        else
        {
            //Search backwards to find predecessor
            for (Entity i = entity - 1; i > m_Head; --i)
            {
                if (Contains(i))
                {
                    m_Data[i] = m_Data[entity];
                    m_Data[entity] = s_Empty;

                    if (entity == m_Tail)
                    {
                        m_Tail = i;
                    }

                    return true;
                }
            }

            assert(Contains(m_Head) && "Could not find predecessor in EntitySet");

            m_Data[m_Head] = m_Data[entity];
            m_Data[entity] = s_Empty;

            if (entity == m_Tail)
            {
                m_Tail = m_Head;
            }

            return true;
        }

        return true;
    }

    [[nodiscard]] constexpr bool Contains(Entity entity) const
    {
        assert(entity < s_MaxEntities && "Entity out of range");
        return m_Data[entity] != s_Empty;
    }

    constexpr void Clear() noexcept
    {
        std::fill_n(m_Data.begin(), s_MaxEntities, s_Empty);
        m_Data[s_EntityTail] = s_EntityTail;
        m_Data[s_Empty] = s_EntityTail;
        m_Head = s_Empty;
        m_Tail = s_Empty;
        m_EntityCount = 0;
    }

    void Overwrite(const EntitySet& other) noexcept
    {
        if (this == &other) return;

        m_Data = other.m_Data;
        m_Head = other.m_Head;
        m_Tail = other.m_Tail;
        m_EntityCount = other.m_EntityCount;
    }

    class Iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = Entity;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const Entity*;
        using reference         = const Entity&;

        Iterator(const std::array<Entity, s_MaxEntities + 2>& data, Entity current) : m_Data(data.data()), m_Entity(current) { }

        Entity operator*() const
        {
            return m_Entity;
        }

        Iterator& operator++()
        {
            m_Entity = m_Data[m_Entity];
            return *this;
        }

        bool operator==(const Iterator& other) const
        {
            return m_Entity == other.m_Entity;
        }

        bool operator!=(const Iterator& other) const
        {
            return m_Entity != other.m_Entity;
        }

    private:
        const Entity* m_Data;
        Entity m_Entity;
    };

    [[nodiscard]] constexpr uint32_t Size() const noexcept { return m_EntityCount; }
    [[nodiscard]] constexpr bool Empty() const noexcept { return m_EntityCount == 0; }

    [[nodiscard]] constexpr Iterator begin() const noexcept { return Iterator(m_Data, m_Head); }
    [[nodiscard]] constexpr Iterator end() const noexcept { return Iterator(m_Data, s_EntityTail); }

private:
    static constexpr Entity s_EntityTail = s_MaxEntities;
    static constexpr Entity s_Empty = s_MaxEntities + 1;

    std::array<Entity, s_MaxEntities + 2> m_Data;
    Entity m_Head;
    Entity m_Tail;
    UInt_E m_EntityCount;
};

static_assert(IsTrivial<EntitySet>, "EntitySet needs to be trivial");