#pragma once

#include <array>
#include <cstdint>
#include <cassert>

template <uint32_t N_Capacity>
class Bitset
{
public:
    explicit Bitset(bool value = false) noexcept
    {
        uint64_t fill = value ? ~0ull : 0ull;
        m_Data.fill(fill);

        if (value && LastBits > 0)
        {
            m_Data[WordCount - 1] &= (1ull << LastBits) - 1;
        }
    }

    constexpr void SetTrue(uint32_t index) noexcept
    {
        assert(index < Capacity && "Index in bitset out of range");
        m_Data[index >> 6] |= 1ull << (index & 63);
    }

    constexpr void SetFalse(uint32_t index) noexcept
    {
        assert(index < Capacity && "Index in bitset out of range");
        m_Data[index >> 6] &= ~(1ull << (index & 63));
    }

    constexpr void Set(uint32_t index, bool value) noexcept
    {
        assert(index < Capacity && "Index in bitset out of range");
        uint64_t mask = 1ull << (index & 63);
        uint64_t& word = m_Data[index >> 6];
        word = (word & ~mask) | (-static_cast<uint64_t>(value) & mask);
    }

    [[nodiscard]] constexpr bool Test(uint32_t index) const noexcept
    {
        assert(index < Capacity && "Index in bitset out of range");
        return (m_Data[index >> 6] >> (index & 63)) & 1ull;
    }

    constexpr void Clear() noexcept
    {
        m_Data.fill(0);
    }

    constexpr void SetAll() noexcept
    {
        m_Data.fill(~0ull);
    }

    [[nodiscard]] constexpr bool operator[](uint32_t index) const noexcept
    {
        assert(index < Capacity && "Index in bitset out of range");
        return (m_Data[index >> 6] >> (index & 63)) & 1ull;
    }

    [[nodiscard]] constexpr uint32_t NextIndex(uint32_t start = 0) const noexcept
    {
        if (start >= Capacity) return Capacity;

        uint32_t index = start >> 6;
        uint32_t bitOffset = start & 63;
        uint64_t bits = m_Data[index] & (~0ull << bitOffset);

        while (index < WordCount)
        {
            if (bits != 0)
            {
                uint32_t result = (index << 6) + std::countr_zero(bits);
                return result < Capacity ? result : Capacity;
            }

            bits = m_Data[++index];
        }

        return Capacity;
    }

    static constexpr uint32_t Capacity = N_Capacity;

private:
    static constexpr uint32_t BitsPerWord = 64;
    static constexpr uint32_t LastBits = Capacity % BitsPerWord;
    static constexpr uint32_t WordCount = (Capacity + BitsPerWord - 1) / BitsPerWord;

    std::array<uint64_t, WordCount> m_Data { };
};