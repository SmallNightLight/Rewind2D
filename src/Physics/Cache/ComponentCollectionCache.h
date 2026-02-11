#pragma once

template<typename T>
class ComponentCollectionCache
{
public:
    inline ComponentCollectionCache() noexcept = default;

    inline void Initialize()
    {
        filled = false;
    }

    inline void Cache(const ComponentCollection<T>& collection)
    {
        filled = true;
        data.Overwrite(collection);
    }

    inline bool TryGetComponent(Entity entity, T& result)
    {
        if (!filled) return false;

        result = data.GetComponent(entity);
        return true;
    }

    inline void Reset() noexcept
    {
        filled = false;
    }

private:
    bool filled;
    ComponentCollection<T> data;
};

static_assert(std::is_trivially_default_constructible_v<ComponentCollectionCache<int>>, "ComponentCollectionCache needs to be trivial");