#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <cassert>

template <uint32_t N>
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

    inline void Clear() noexcept
    {
        std::memset(m_Data.data(), 0, sizeof(m_Data));
    }

    inline void SetTrue(uint32_t index) noexcept
    {
        assert(index < N && "Index in bitset out of range");
        m_Data[index >> 6] |= 1ull << (index & 63);
    }

    inline void SetFalse(uint32_t index) noexcept
    {
        assert(index < N && "Index in bitset out of range");
        m_Data[index >> 6] &= ~(1ull << (index & 63));
    }

    inline void Set(uint32_t index, bool value) noexcept
    {
        if (value) SetTrue(index);
        else SetFalse(index);
    }

    [[nodiscard]] inline bool Test(uint32_t index) const noexcept
    {
        assert(index < N && "Index in bitset out of range");
        return (m_Data[index >> 6] >> (index & 63)) & 1ull;
    }

    [[nodiscard]] inline bool operator[](uint32_t index) const noexcept
    {
        assert(index < N && "Index in bitset out of range");
        return (m_Data[index >> 6] >> (index & 63)) & 1ull;
    }

    [[nodiscard]] static constexpr inline uint32_t Size() noexcept
    {
        return N;
    }

    [[nodiscard]] inline uint32_t NextIndex(uint32_t start = 0) const noexcept
    {
        if (start >= N) return N;

        uint32_t index = start >> 6;
        uint32_t bitOffset = start & 63;
        uint64_t bits = m_Data[index] & (~0ull << bitOffset);

        while (true)
        {
            if (bits != 0)
            {
                return (index << 6) + std::countr_zero(bits);
            }

            if (++index >= WordCount) return N;

            bits = m_Data[index];
        }
    }

private:
    static constexpr uint32_t BitsPerWord = 64;
    static constexpr uint32_t LastBits = N % BitsPerWord;
    static constexpr uint32_t WordCount = (N + BitsPerWord - 1) / BitsPerWord;

    std::array<uint64_t, WordCount> m_Data { };
};