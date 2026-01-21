#pragma once

#include <array>
#include <cassert>
#include <cstdint>

class SparseSet
{
public:
    inline SparseSet() noexcept = default;

    void Initialize()
    {
        sparse.fill(ENTITYNULL);
        dense.fill(ENTITYNULL);
        count = 0;
    }

    // Adds entity to the set, returns its dense index
    uint32_t Add(Entity entity)
    {
        assert(entity < MAXENTITIES);
        assert(!Contains(entity) && "Entity already in SparseSet");

        uint32_t index = count++;
        sparse[entity] = index;
        dense[index] = entity;

        return index;
    }

    // Removes entity from the set
    void Remove(Entity entity)
    {
        assert(Contains(entity) && "Removing non-existent entity");

        uint32_t index = sparse[entity];
        uint32_t last = count - 1;
        Entity lastEntity = dense[last];

        dense[index] = lastEntity;
        sparse[lastEntity] = index;

        sparse[entity] = ENTITYNULL;
        dense[last] = ENTITYNULL;

        count--;
    }

    bool Contains(Entity entity) const
    {
        return entity < MAXENTITIES && sparse[entity] != ENTITYNULL;
    }

    uint32_t GetIndex(Entity entity) const
    {
        assert(Contains(entity));
        return sparse[entity];
    }

    Entity GetEntity(uint32_t index) const
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
    std::array<Entity, MAXENTITIES> dense{};
    std::array<uint32_t, MAXENTITIES> sparse{};
    uint32_t count{};
};
