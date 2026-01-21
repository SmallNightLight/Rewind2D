#pragma once

#include <array>
#include <cassert>

#include "ECSSettings.h"

template<uint32_t Capacity>
class EntityQueue
{
public:
    inline EntityQueue() noexcept = default; //todo: make queue use optimization & (capacity - 1) with capacity power of 2

    inline void Push(Entity value)
    {
        assert(m_Size < Capacity && "EntityQueue overflow");
        m_Data[m_Tail] = value;
        m_Tail = (m_Tail + 1) % Capacity;
        ++m_Size;
    }

    [[nodiscard]] inline Entity Front() const
    {
        assert(m_Size > 0 && "EntityQueue is empty");
        return m_Data[m_Head];
    }

    inline void Pop()
    {
        assert(m_Size > 0 && "EntityQueue is empty");
        m_Head = (m_Head + 1) % Capacity;
        --m_Size;
    }

    inline Entity Dequeue()
    {
        assert(m_Size > 0 && "EntityQueue is empty");

        Entity value = m_Data[m_Head];
        m_Head = (m_Head + 1) % Capacity;
        --m_Size;
        return value;
    }

    [[nodiscard]] inline bool Empty() const
    {
        return m_Size == 0;
    }

    [[nodiscard]] inline std::size_t Count() const
    {
        return m_Size;
    }

private:
    std::array<Entity, Capacity> m_Data { };
    uint32_t m_Head = 0;
    uint32_t m_Tail = 0;
    uint32_t m_Size = 0;
};