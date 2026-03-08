#pragma once

#include <array>
#include <cassert>
#include <type_traits>

template<typename KeyType, typename ValueType, uint32_t Size>
class SortedDoubleMap
{
    static_assert(std::is_copy_assignable_v<ValueType>, "Value type must be copy assignable");
    static_assert(std::is_copy_constructible_v<ValueType>, "Value type must be copy constructible");
    static_assert(std::is_convertible_v<decltype(std::declval<const ValueType&>() < std::declval<const ValueType&>()), bool>, "ValueType must support operator< with ValueType");
    static_assert(std::is_convertible_v<decltype(std::declval<const ValueType&>() == std::declval<const KeyType&>()), bool>, "ValueType must support operator== with KeyType");
    static_assert(std::is_convertible_v<decltype(std::declval<const KeyType&>() < std::declval<const ValueType&>()), bool>, "KeyType must support operator< with ValueType");

public:
    inline SortedDoubleMap() noexcept = default;

    ///Clears the cache completely
    inline constexpr void Initialize() noexcept
    {
        Reset();
    }

    ///Cache the key. Function needs to be called with sorted keys (with keyA1 < keyB1 || (keyA1 == keyB1 && keyA2 < keyB2)).
    ///From key with lsb to msb
    inline constexpr void Cache(const ValueType& value)
    {
        assert(count[flip] < Size && "Trying to cache key, but buffer is full");
        assert((count[flip] == 0 || GetFlip(count[flip] - 1) < value) && "Keys must be cached in sorted order");
        GetFlip(count[flip]++) = value;
    }

    ///Advances the cache to the key, expecting keys in order. Mirrors the keys to the secondary buffer if found.
    ///Returns if the key exists in the primary buffer and write the value into the result
    inline constexpr bool AdvanceCache(KeyType key, ValueType& result) noexcept
    {
        while (currentIndex < count[flop])
        {
            ValueType& currentValue = GetFlop(currentIndex);

            if (key < currentValue)
            {
                //key does not exist in cache
                return false;
            }

            if (currentValue == key)
            {
                //Mirror key into other buffer
                Cache(currentValue);
                ++currentIndex;
                result = currentValue;
                return true;
            }

            //Key in list is not anymore present
            ++currentIndex;
        }

        return false;
    }

    ///Reads the value at the key, without ever writing to the secondary buffer
    ///Returns if the key was found in the buffer
    inline constexpr bool ReadCurrent(ValueType& valueResult) noexcept //todo trygetcurrent
    {
        if (currentIndex >= count[flop]) return false;

        valueResult = GetFlop(currentIndex);
        return true;
    }

    //Advances the current index. Use this with the ReadCurrent function
    inline constexpr void Advance() noexcept
    {
        ++currentIndex;
    }

    [[nodiscard]] inline constexpr uint32_t CurrentIndex() const noexcept
    {
        return currentIndex;
    }

    ///Returns the count of the reading buffer
    [[nodiscard]] inline constexpr uint32_t ReadCurrentCount() const noexcept
    {
        return count[flop];
    }

    ///Returns the count of the writing buffer
    [[nodiscard]] inline constexpr uint32_t ReadOtherCount() const noexcept //todo no read
    {
        return count[flop];
    }

    ///Clears the cache completely
    inline constexpr void Reset() noexcept
    {
        currentIndex = 0;
        count = {0, 0};
        flip = 0;
        flop = 1;
    }

    ///Flips the buffer, to access to the mirrored buffer
    inline constexpr void Flip() noexcept
    {
        currentIndex = 0;
        count[flop] = 0;
        flip = flop;
        flop = flip ^ 1;
    }

private:
    uint32_t currentIndex;
    std::array<uint32_t, 2> count;
    uint8_t flip;
    uint8_t flop;
    std::array<ValueType, 2 * Size> data;

    inline constexpr ValueType& GetFlip(uint32_t index) noexcept
    {
        return data[(index << 1) | flip];
    }

    inline constexpr ValueType& GetFlop(uint32_t index) noexcept
    {
        return data[(index << 1) | flop];
    }

    static_assert(std::is_trivially_default_constructible_v<KeyType>, "Type for sorted cache needs to be trivial");
};

static_assert(std::is_trivially_default_constructible_v<SortedDoubleMap<int, bool, 1>>, "SortedDoubleMap needs to be trivial");