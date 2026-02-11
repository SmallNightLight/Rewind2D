#pragma once

#include "ECSSettings.h"

#include <array>
#include <cassert>

//Stores the components of type T in an array
//Sparse set-based ECS
//Issues: When removing components are removed the array reorders the entity indexes to make the array dense, resulting in a non-optimal order
template<typename T>
class alignas(64) ComponentCollection
{
	static_assert(IsTrivial<T>, "ComponentCollection requires T to be trivial");

public:
    constexpr ComponentCollection() noexcept = default;

    //Initializes the sparse set with null entities, to indicate that all entities have no components
    constexpr void Initialize() noexcept
    {
        m_EntityToIndex.fill(s_InvalidEntity);
        m_EntityCount = 0;
    }

    //Adds the component of type T to the given entity
    T* AddComponent(Entity entity, const T& component)
    {
        assert(entity < s_MaxEntities && "Entity out of range");
        assert(m_EntityToIndex[entity] == s_InvalidEntity&& "Component already added to the entity");

        //New index is the next available index in the component list
        std::int32_t entityIndex = m_EntityCount++;

        //Update the maps
        m_EntityToIndex[entity] = entityIndex;
        m_IndexToEntity[entityIndex] = entity;

        //Store the component
        m_Components[entityIndex] = component;
    	return &m_Components[entityIndex];
    }

    //Emplace component of type T to the given entity
    template<typename... Args>
    T* EmplaceComponent(Entity entity, Args&&... args)
    {
        assert(entity < s_MaxEntities && "Entity out of range");
        assert(m_EntityToIndex[entity] == s_InvalidEntity&& "Component already added to the entity");

        //New index is the next available index in the component list
        std::int32_t entityIndex = m_EntityCount++;

        //Render the maps and assign the component
        m_EntityToIndex[entity] = entityIndex;
        m_IndexToEntity[entityIndex] = entity;

        //Construct the component at the target location
        m_Components[entityIndex] = T(std::forward<Args>(args)...);

        return &m_Components[entityIndex];
    }

    //Removes the component from the given entity
    void RemoveComponent(Entity entity)
    {
        assert(entity < s_MaxEntities && "Entity out of range");
        assert(m_EntityToIndex[entity] != s_InvalidEntity && "Removing a component that does not exist");
        assert(m_EntityCount > 0 && "No components exist that could be removed");

        uint32_t indexOfRemovedEntity = m_EntityToIndex[entity];
        uint32_t lastEntityIndex = m_EntityCount - 1;

        //Move the last component to the index of the removed entity
        m_Components[indexOfRemovedEntity] = m_Components[lastEntityIndex];
        Entity entityOfLastIndex = m_IndexToEntity[lastEntityIndex];

        //Update the sparse set
        m_EntityToIndex[entityOfLastIndex] = indexOfRemovedEntity;
        m_IndexToEntity[indexOfRemovedEntity] = entityOfLastIndex;

        //Set the now invalid index to NULL
        m_EntityToIndex[entity] = s_InvalidEntity;
        m_EntityCount--;
    }

    //Gets a reference to the component for the given entity
    [[nodiscard]] constexpr T& GetComponent(Entity entity)
    {
        assert(entity < s_MaxEntities);
        assert(m_EntityToIndex[entity] != s_InvalidEntity && "Trying to get a component that does not exist");
        return m_Components[m_EntityToIndex[entity]];
    }

    [[nodiscard]] constexpr const T& GetComponent(Entity entity) const
    {
        assert(entity < s_MaxEntities);
        assert(m_EntityToIndex[entity] != s_InvalidEntity && "Trying to get a component that does not exist");
        return m_Components[m_EntityToIndex[entity]];
    }

    [[nodiscard]] constexpr T* TryGetComponent(Entity entity)
    {
        assert(entity < s_MaxEntities);
        uint32_t index = m_EntityToIndex[entity];
        return index != s_InvalidEntity ? &m_Components[index] : nullptr;
    }

    [[nodiscard]] constexpr const T* TryGetComponent(Entity entity) const
    {
        assert(entity < s_MaxEntities);
        uint32_t index = m_EntityToIndex[entity];
        return index != s_InvalidEntity ? &m_Components[index] : nullptr;
    }

    //Checks whether the given entity has the component by checking the sparse set for entity null
    [[nodiscard]] constexpr bool HasComponent(Entity entity) const noexcept
    {
        return entity < s_MaxEntities && m_EntityToIndex[entity] != s_InvalidEntity;
    }

    //Returns the entity count (all entities that have this component type attached)
    [[nodiscard]] constexpr uint32_t GetEntityCount() const noexcept { return m_EntityCount; }
    [[nodiscard]] constexpr bool Empty() const noexcept { return m_EntityCount == 0; }

    //Removes the component from the entity if possible
    void DestroyEntity(Entity entity)
    {
        assert(entity < s_MaxEntities && "Entity out of range");

        if (m_EntityToIndex[entity] != s_InvalidEntity)
        {
            RemoveComponent(entity);
        }
    }

    void Overwrite(const ComponentCollection& other) noexcept
    {
        std::memcpy(m_Components.data(), other.m_Components.data(), other.m_EntityCount * sizeof(T));
        std::memcpy(m_IndexToEntity.data(), other.m_IndexToEntity.data(), other.m_EntityCount * sizeof(Entity));
        m_EntityToIndex = other.m_EntityToIndex;
        m_EntityCount = other.m_EntityCount;
    }

    [[nodiscard]] constexpr T* begin() noexcept { return m_Components.data(); }
    [[nodiscard]] constexpr T* end() noexcept { return m_Components.data() + m_EntityCount; }
    [[nodiscard]] constexpr const T* begin() const noexcept { return m_Components.data(); }
    [[nodiscard]] constexpr const T* end() const noexcept { return m_Components.data() + m_EntityCount; }

private:
    std::array<T, s_MaxEntities> m_Components;
    std::array<Entity, s_MaxEntities> m_IndexToEntity;
    std::array<std::uint32_t, s_MaxEntities> m_EntityToIndex;
    std::uint32_t m_EntityCount;
};

static_assert(std::is_trivially_default_constructible_v<ComponentCollection<bool>>, "Component Collection needs to be trivial");