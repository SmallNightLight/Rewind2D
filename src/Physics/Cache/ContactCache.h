#pragma once

#include "../../ECS/ECSSettings.h"
#include "../../Math/Span.h"
#include "../Collision/ContactPair.h"

#include <array>
#include <vector>

//Island is based on the sorting of the contact Pairs. But the final entity groups are the same. Only use the island for internal use.
template<uint32_t EntityCount>
class ContactCache
{
public:
    inline ContactCache() noexcept = default;

    void CreateGroup(std::vector<ContactPair>& contactPairs)
    {
        m_EntityIslands.fill(InvalidIsland);
        m_IslandEntityCounts.fill(0);
        m_IslandOffsets.fill(0);
        m_IslandCount = 0;
        m_ActiveEntityCount = 0;

        //Assign and merge islands
        for (const ContactPair& pair : contactPairs)
        {
            Entity entity1 = pair.EntityKey.Entity1();
            Entity entity2 = pair.EntityKey.Entity2();

            Island island1 = m_EntityIslands[entity1];
            Island island2 = m_EntityIslands[entity2];

            if (m_EntityIslands[entity1] == InvalidIsland)
            {
                m_ActiveEntities[m_ActiveEntityCount++] = entity1;
            }
            if (m_EntityIslands[entity2] == InvalidIsland)
            {
                m_ActiveEntities[m_ActiveEntityCount++] = entity2;
            }

            assert((!pair.EntityStatic1 || !pair.EntityStatic2) && "Two static entities should not form a contact pair");

            if (pair.EntityStatic1)
            {
                //Entity 1 is static: Only add island to entity 2
                Island newIsland = m_IslandCount++;
                m_EntityIslands[entity1] = StaticIsland;
                m_EntityIslands[entity2] = newIsland;

                ++m_IslandEntityCounts[newIsland];
            }
            else if (pair.EntityStatic2)
            {
                //Entity 2 is static: Only add island to entity 1
                Island newIsland = m_IslandCount++;
                m_EntityIslands[entity2] = StaticIsland;
                m_EntityIslands[entity1] = newIsland;

                ++m_IslandEntityCounts[newIsland];
            }
            else if (island1 == InvalidIsland && island2 == InvalidIsland)
            {
                //No island yet assigned for either entities: Create new island
                Island newIsland = m_IslandCount++;
                m_EntityIslands[entity1] = newIsland;
                m_EntityIslands[entity2] = newIsland;

                m_IslandEntityCounts[newIsland] += 2;
            }
            else if (island1 == InvalidIsland)
            {
                //Only entity 1 has an island assigned: Assign island 2 to entity 1
                m_EntityIslands[entity1] = island2;

                ++m_IslandEntityCounts[island2];
            }
            else if (island2 == InvalidIsland)
            {
                //Only entity 2 has an island assigned: Assign island 1 to entity 2
                m_EntityIslands[entity2] = island1;

                ++m_IslandEntityCounts[island1];
            }
            else if (island1 != island2)
            {
                //Entities have different island: Merge the two islands
                Island newIsland, oldIsland;
                if (m_IslandEntityCounts[island1] <= m_IslandEntityCounts[island2])
                {
                    newIsland = island1;
                    oldIsland = island2;
                }
                else
                {
                    newIsland = island2;
                    oldIsland = island1;
                }

                for (uint32_t i = 0; i < m_ActiveEntityCount; ++i)
                {
                    Entity entity = m_ActiveEntities[i];
                    if (m_EntityIslands[entity] == oldIsland)
                    {
                        m_EntityIslands[entity] = newIsland;
                        ++m_IslandEntityCounts[newIsland];
                        --m_IslandEntityCounts[oldIsland];
                    }
                }
            }
        }

        //Build offset table
        Entity currentIndex = 0;
        for (Island island = 0; island < m_IslandCount; ++island)
        {
            m_IslandOffsets[island] = currentIndex;
            currentIndex += m_IslandEntityCounts[island];
        }

        //Fill entity list
        std::array<Entity, EntityCount> islandWriteCursor = m_IslandOffsets;
        for (uint32_t i = 0; i < m_ActiveEntityCount; ++i)
        {
            Entity entity = m_ActiveEntities[i];
            Island island = m_EntityIslands[entity];
            if (island != InvalidIsland && island != StaticIsland)
            {
                m_Entities[islandWriteCursor[island]++] = entity;
            }
        }
    }

    [[nodiscard]] inline Island GetTotalIslandCount() const noexcept
    {
        return m_IslandCount;
    }

    [[nodiscard]] inline Island GetIslandEntityCount(Island island) const noexcept
    {
        assert(island < m_IslandCount && "Invalid island");
        return m_IslandEntityCounts[island];
    }

    [[nodiscard]] inline Island GetIsland(Entity entity) const noexcept
    {
        return m_EntityIslands[entity];
    }

    [[nodiscard]] inline Span<Entity> GetEntities(Island island) const noexcept
    {
        assert(island < m_IslandCount && "Invalid island");
        return Span(&m_Entities[m_IslandOffsets[island]], m_IslandEntityCounts[island]);
    }

private:
    static constexpr Island InvalidIsland = EntityCount;
    static constexpr Island StaticIsland = EntityCount + 1;

    Island m_IslandCount = 0;
    Entity m_ActiveEntityCount = 0;

    std::array<Island, EntityCount> m_EntityIslands { };        //Maps an entity to an island
    std::array<Entity, EntityCount> m_IslandEntityCounts { };   //Maps as island to its entity count
    std::array<Entity, EntityCount> m_IslandOffsets { };        //Maps an island to an index in m_Entities
    std::array<Entity, EntityCount> m_Entities { };             //List of entities, ordered by islands
    std::array<Entity, EntityCount> m_ActiveEntities { };       //Tracks all entities that have a contact
};