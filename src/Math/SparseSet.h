#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <limits>

template<typename T, uint32_t Capacity>
class SparseSet
{
    static_assert(std::is_unsigned_v<T>, "T must be an unsigned integral type");
    static_assert(Capacity < std::numeric_limits<uint32_t>::max(), "Capacity can not be max uint32 since that is reserved for invalid elements");

public:
    inline SparseSet() noexcept = default;

    void Initialize()
    {
        sparse.fill(s_InvalidElement);
        dense.fill(s_InvalidElement);
        count = 0;
    }

    // Adds element to the set, returns its dense index
    uint32_t Add(T element)
    {
        assert(element < Capacity);
        assert(!Contains(element) && "Element already in SparseSet");

        uint32_t index = count++;
        sparse[element] = index;
        dense[index] = element;

        return index;
    }

    // Removes element from the set
    void Remove(T element)
    {
        assert(Contains(element) && "Removing non-existent element");

        uint32_t index = sparse[element];
        uint32_t last = count - 1;
        T lastElement = dense[last];

        dense[index] = lastElement;
        sparse[lastElement] = index;

        sparse[element] = s_InvalidElement;
        dense[last] = s_InvalidElement;

        count--;
    }

    bool Contains(T element) const
    {
        return element < Capacity && sparse[element] != s_InvalidElement;
    }

    [[nodiscard]] uint32_t GetIndex(T element) const
    {
        assert(Contains(element));
        return sparse[element];
    }

    T GetElement(uint32_t index) const
    {
        assert(index < count);
        return dense[index];
    }

    [[nodiscard]] uint32_t Size() const { return count; }

    void Overwrite(const SparseSet& other)
    {
        sparse = other.sparse;
        dense = other.dense;
        count = other.count;
    }

private:
    static constexpr T s_InvalidElement = Capacity + 1;
    static constexpr uint32_t s_InvalidIndex = std::numeric_limits<uint32_t>::max();

    std::array<T, Capacity> dense { };
    std::array<uint32_t, Capacity> sparse { };
    uint32_t count { };
};
