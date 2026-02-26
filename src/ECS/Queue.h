#pragma once

#include <array>
#include <cassert>
#include <bit>

template<typename T, uint32_t N_Capacity>
    requires (N_Capacity > 0 && std::has_single_bit(N_Capacity))
class Queue
{
public:
    constexpr Queue() noexcept = default;

    constexpr void Push(T value) noexcept
    {
        assert(m_Size < N_Capacity && "Queue overflow");
        m_Data[(m_Head + m_Size) & s_Mask] = value;
        ++m_Size;
    }

    [[nodiscard]] constexpr T Front() const noexcept
    {
        assert(m_Size > 0 && "Queue is empty");
        return m_Data[m_Head];
    }

    constexpr void Pop() noexcept
    {
        assert(m_Size > 0 && "Queue is empty");
        m_Head = (m_Head + 1) & s_Mask;
        --m_Size;
    }

    [[nodiscard]] constexpr T Dequeue() noexcept
    {
        assert(m_Size > 0 && "Queue is empty");
        T value = m_Data[m_Head];
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

    std::array<T, N_Capacity> m_Data { };
    uint32_t m_Head = 0;
    uint32_t m_Size = 0;
};