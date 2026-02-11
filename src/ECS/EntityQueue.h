#pragma once

#include <array>
#include <cassert>
#include <bit>

#include "ECSSettings.h"

template<uint32_t N_Capacity>
    requires (N_Capacity > 0 && std::has_single_bit(N_Capacity))
class EntityQueue
{
public:
    constexpr EntityQueue() noexcept = default;

    constexpr void Push(Entity value) noexcept
    {
        assert(m_Size < N_Capacity && "EntityQueue overflow");
        m_Data[(m_Head + m_Size) & s_Mask] = value;
        ++m_Size;
    }

    [[nodiscard]] constexpr Entity Front() const noexcept
    {
        assert(m_Size > 0 && "EntityQueue is empty");
        return m_Data[m_Head];
    }

    constexpr void Pop() noexcept
    {
        assert(m_Size > 0 && "EntityQueue is empty");
        m_Head = (m_Head + 1) & s_Mask;
        --m_Size;
    }

    [[nodiscard]] constexpr Entity Dequeue() noexcept
    {
        assert(m_Size > 0 && "EntityQueue is empty");
        Entity value = m_Data[m_Head];
        m_Head = (m_Head + 1) & s_Mask;
        --m_Size;
        return value;
    }

    constexpr void Clear() noexcept
    {
        m_Head = 0;
        m_Size = 0;
    }

    [[nodiscard]] constexpr bool Empty() const noexcept { return m_Size == 0; }
    [[nodiscard]] constexpr uint32_t Count() const noexcept { return m_Size; }
    [[nodiscard]] constexpr bool Full() const noexcept { return m_Size == N_Capacity; }
    static constexpr uint32_t Capacity = N_Capacity;

private:
    static constexpr uint32_t s_Mask = N_Capacity - 1;

    std::array<Entity, N_Capacity> m_Data { };
    uint32_t m_Head = 0;
    uint32_t m_Size = 0;
};