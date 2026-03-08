#pragma once

#include <array>
#include <vector>
#include <cstring>

#include "../../ECS/ECSSettings.h"
#include "../../ECS/EntityPair.h"
#include "../PhysicsSettings.h"
#include "../../Math/Bitset.h"

// Amount of colors, excluding the last color for remaining pairs
inline constexpr uint32_t s_MaxFillColors = s_MaxColors - 1;

// Data that is returned and used by the solver
template<bool trackUnchanged>
struct EntityIslandData;

template<>
struct EntityIslandData<true>
{
    //Entities of unchanged islands (sorted by Entity ID)
    std::array<Entity, s_MaxEntities + 1> UnchangedEntities;
    uint32_t UnchangedEntityCount;

    //Entity pairs of changed islands, sorted by the color primarily, and Entity ID secondarily
    std::array<EntityPair, s_MaxPairs> ColoredPairs;
    std::array<uint32_t, s_MaxColors + 1> ColorHeads;

    constexpr EntityIslandData() noexcept: UnchangedEntityCount(0) { }
};

template<>
struct EntityIslandData<false>
{
    //Entity pairs of changed islands, sorted by the color primarily, and Entity ID secondarily
    std::array<EntityPair, s_MaxPairs> ColoredPairs;
    std::array<uint32_t, s_MaxColors + 1> ColorHeads;

    constexpr EntityIslandData() noexcept { }
};


struct IslandEntry
{
    Entity Island;              //Island ID (target is the lowest entity in complete island)
    //uint32_t Head;            //Index in the SortedIslands array
    uint32_t EntityCount;       //Entity count of the island
    //bool CanSplit;            //Marks if the island has removed contacts, allowing for potential splitting
    //bool EntityChanges;       //Marks if the island has had any recent entity changes
    //bool Awake;               //Marks if the island is currently awake

    constexpr IslandEntry() noexcept = default;
    constexpr explicit IslandEntry(Entity entity, uint32_t entityCount = 1) noexcept : Island(entity), EntityCount(entityCount) { }
    //constexpr IslandData(Entity island, uint32_t head, uint32_t entityCount = 1, bool canSplit = false, bool entityChanges = false, bool awake = true) noexcept :
    //    Island(island), Head(head), EntityCount(entityCount), CanSplit(canSplit), EntityChanges(entityChanges), Awake(awake) { }
};

// Persistent islands data that is stored for future island updates
struct IslandsData
{
    //All contacts pairs, sorted by their pair ID
    std::array<EntityPair, s_MaxPairs> Pairs;
    uint32_t PairCount;

    //Island map. Maps the entity to the island (entity). The island is root if index = value
    std::array<IslandEntry, s_MaxEntities> Islands;

    //Data for coloring, Index: entity * s_MaxFillColors + color
    Bitset<s_MaxEntities * s_MaxFillColors> ColorEntities;
    std::array<GraphColor, s_MaxPairs> PairColors;
    std::array<uint8_t, s_MaxColors> ColorCounts;

    constexpr IslandsData() noexcept : PairCount(0), ColorEntities(false), ColorCounts()
    {
        for (Entity entity = 0; entity < s_MaxEntities; ++entity)
        {
            Islands[entity] = IslandEntry(entity);
        }
    }
};

struct EntityPairAddition
{
    EntityPair Pair;
    uint32_t Index; //Index in Pair list

    EntityPairAddition(EntityPair pair, uint32_t index) noexcept : Pair(pair), Index(index) { }
};

class IslandManager
{
public:
    template<bool computeForCache>
    void UpdateIslands(const std::vector<EntityPair>& newPairs, EntityIslandData<computeForCache>& result) noexcept //todo changes should also come from tiny velocity changes
    {
        std::vector<Entity> entityContacts;
        std::vector<uint32_t> entityContactHeads(s_MaxEntities + 1, 0);

        std::vector<EntityPairAddition> addedPairs;
        addedPairs.reserve(newPairs.size());

        Bitset<s_MaxEntities> changedEntities(false);
        std::vector<GraphColor> newColors(newPairs.size());

        //------------------------------------------------------------------------------
        // 1. Find pair differences
        //------------------------------------------------------------------------------
        for (uint32_t i = 0, j = 0; i < m_Data.PairCount || j < newPairs.size();)
        {
            EntityPair oldPair = i < m_Data.PairCount ? m_Data.Pairs[i] : s_InvalidEntityPair;
            EntityPair newPair = j < newPairs.size() ? newPairs[j] : s_InvalidEntityPair;

            if (oldPair == newPair)
            {
                //Pair did not change

                //Keep colors up to date
                newColors[j] = m_Data.PairColors[i];

                ++i;
                ++j;

                //Update contact list
                ++entityContactHeads[newPair.Entity1()];
            }
            else if (newPair < oldPair)
            {
                //Found new pair
                Entity entity1 = newPair.Entity1();
                Entity entity2 = newPair.Entity2();

                if (m_Data.Islands[entity1].Island != m_Data.Islands[entity2].Island)
                {
                    addedPairs.emplace_back(newPair, j);
                }

                ++j;

                //Update contact list
                ++entityContactHeads[entity1];
            }
            else
            {
                //Missing pair
                Entity entity1 = oldPair.Entity1();
                Entity entity2 = oldPair.Entity2();

                GraphColor color = m_Data.PairColors[i];

                if (color < s_MaxFillColors) //todo check if can remove branch
                {
                    m_Data.ColorEntities.SetFalse(entity1 * s_MaxFillColors + color);
                    m_Data.ColorEntities.SetFalse(entity2 * s_MaxFillColors + color);
                }
                --m_Data.ColorCounts[color];

                Entity island = m_Data.Islands[entity1].Island;
                changedEntities.SetTrue(island);

                ++i;
            }
        }

        //Copy new pairs to the persistent data
        std::memcpy(m_Data.Pairs.data(), newPairs.data(), newPairs.size() * sizeof(EntityPair));
        std::memcpy(m_Data.PairColors.data(), newColors.data(), newColors.size() * sizeof(GraphColor));
        m_Data.PairCount = newPairs.size();


        //------------------------------------------------------------------------------
        // 2. Compute entity contact list
        //------------------------------------------------------------------------------
        uint32_t entityContactHead = 0;

        for (uint32_t entity = 0; entity < s_MaxEntities; ++entity) // todo changed entities is technically correct but only works for islands that have been removeds form so not added to
        {
            uint32_t count = entityContactHeads[entity];
            entityContactHeads[entity] = entityContactHead;
            entityContactHead += count;

            //Mark entity as splitting if its island is marked (branchless)
            bool entityChanged = changedEntities.Test(m_Data.Islands[entity].Island);
            changedEntities.Set(entity, entityChanged);

            if constexpr (computeForCache)
            {
                //Add entity to unchanged islands if unmarked (branchless)
                result.UnchangedEntities[result.UnchangedEntityCount] = entity;
                result.UnchangedEntityCount += !entityChanged;
            }
        }

        entityContactHeads[s_MaxEntities] = entityContactHead;
        entityContacts.resize(entityContactHead); //Size should be equal to newPairs.size()
        std::vector<uint32_t> fillState = entityContactHeads;

        for (EntityPair pair : newPairs)
        {
            Entity entity1 = pair.Entity1();
            Entity entity2 = pair.Entity2();

            entityContacts[fillState[entity1]++] = entity2;
        }


        //------------------------------------------------------------------------------
        // 3. Split islands
        //------------------------------------------------------------------------------
        Bitset<s_MaxEntities> visited(false);
        EntityQueue<s_MaxEntities> queue { };

        //Loop over all potentially splitting islands
        for (uint32_t island = changedEntities.NextIndex(); island < s_MaxEntities; island = changedEntities.NextIndex(island + 1))
        {
            if (visited[island]) continue;

            //Per island
            queue.Push(island);
            uint32_t entityCount = 0;

            while (!queue.Empty())
            {
                Entity entity = queue.Dequeue();
                visited.SetTrue(entity);
                ++entityCount;
                m_Data.Islands[entity].Island = island;

                //Loop over entity neighbors
                for (uint32_t n = entityContactHeads[entity]; n < entityContactHeads[entity + 1]; ++n)
                {
                    Entity neighbor = entityContacts[n];

                    if (visited[neighbor]) continue;

                    queue.Push(neighbor);
                }
            }

            m_Data.Islands[island].EntityCount = entityCount;
        }


        //------------------------------------------------------------------------------
        // 4. Merge islands
        //------------------------------------------------------------------------------
        for (EntityPairAddition pairAddition : addedPairs)
        {
            Entity island1 = FindRoot(pairAddition.Pair.Entity1());
            Entity island2 = FindRoot(pairAddition.Pair.Entity2());

            if (island1 != island2)
            {
                // Union
                m_Data.Islands[island2].Island = island1;
                m_Data.Islands[island1].EntityCount += m_Data.Islands[island2].EntityCount;
                m_Data.Islands[island2].EntityCount = 0;
            }
        }

        // Fix islands for all entities
        for (Entity entity = 0; entity < s_MaxEntities; ++entity)
        {
            Island island = m_Data.Islands[entity].Island;
            m_Data.Islands[entity].Island = m_Data.Islands[island].Island;
        }


        //------------------------------------------------------------------------------
        // 5. Color changed islands
        //------------------------------------------------------------------------------
        for (const EntityPairAddition& pairAddition : addedPairs)
        {
            uint32_t entityIndex1 =  pairAddition.Pair.Entity1() * s_MaxFillColors;
            uint32_t entityIndex2 =  pairAddition.Pair.Entity2() * s_MaxFillColors;

            for (GraphColor color = 0; color < s_MaxFillColors; ++color)
            {
                uint32_t index1 = entityIndex1 + color;
                uint32_t index2 = entityIndex2 + color;

                //Check if color is available
                if (m_Data.ColorEntities.Test(index1) || m_Data.ColorEntities.Test(index2)) continue;

                //Color available
                m_Data.ColorEntities.SetTrue(index1);
                m_Data.ColorEntities.SetTrue(index2);
                m_Data.PairColors[pairAddition.Index] = color;
                ++m_Data.ColorCounts[color];

                goto nextPair;
            }

            //No available color left, use last sequential color buffer
            m_Data.PairColors[pairAddition.Index] = s_MaxFillColors;
            ++m_Data.ColorCounts[s_MaxFillColors];

            nextPair:;
        }
        //Color removal was already done in the first section


        //------------------------------------------------------------------------------
        // 5. Collapse entity colors into list
        //------------------------------------------------------------------------------
        if constexpr (computeForCache)
        {
            std::array<uint32_t, s_MaxColors> changedColorCounts { };

            for (uint32_t i = 0; i < m_Data.PairCount; ++i)
            {
                Entity island = m_Data.Islands[m_Data.Pairs[i].Entity1()].Island;
                changedColorCounts[m_Data.PairColors[i]] += changedEntities.Test(island);
            }

            uint32_t colorHead = 0;
            for (uint32_t color = 0; color < s_MaxColors; ++color)
            {
                result.ColorHeads[color] = colorHead;
                colorHead += changedColorCounts[color];
            }
            result.ColorHeads[s_MaxColors] = colorHead;

            std::array<uint32_t, s_MaxColors + 1> filledCounts = result.ColorHeads;

            for (uint32_t i = 0; i < m_Data.PairCount; ++i)
            {
                Entity island = m_Data.Islands[m_Data.Pairs[i].Entity1()].Island;
                GraphColor color = m_Data.PairColors[i];

                result.ColoredPairs[filledCounts[color]] = m_Data.Pairs[i];
                filledCounts[color] += changedEntities.Test(island);
            }
        }
        else
        {
            uint8_t colorHead = 0;

            for (uint8_t color = 0; color < s_MaxColors; ++color)
            {
                result.ColorHeads[color] = colorHead;
                colorHead += m_Data.ColorCounts[color];
            }
            result.ColorHeads[s_MaxColors] = colorHead;

            std::array<uint32_t, s_MaxColors + 1> filledCounts = result.ColorHeads;

            for (uint32_t i = 0; i < m_Data.PairCount; ++i)
            {
                result.ColoredPairs[filledCounts[m_Data.PairColors[i]]++] = m_Data.Pairs[i];
            }
        }
    }

    Entity FindRoot(Entity island)
    {
        Entity root = island;

        while (m_Data.Islands[root].Island != root)
        {
            root = m_Data.Islands[root].Island;
        }

        // Path compression
        while (m_Data.Islands[island].Island != island)
        {
            Entity parent = m_Data.Islands[island].Island;
            m_Data.Islands[island].Island = root;
            island = parent;
        }

        return root;
    }

private:
    IslandsData m_Data { };
};

static_assert(IsCopyTrivial<EntityIslandData<true>>);
static_assert(IsCopyTrivial<IslandEntry>);
static_assert(IsCopyTrivial<IslandsData>);
static_assert(IsCopyTrivial<IslandManager>);