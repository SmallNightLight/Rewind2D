#pragma once

#include <array>

#include "../Collision/ContactPair.h"
#include "SortedDoubleMap.h"
#include "../../ECS/ComponentCollection.h"

#include <array>
#include <vector>

#include "../../Math/Bitset.h"
#include "../../Math/LinkedList.h"
#include "../../ECS/EntityPair.h"
#include "../../ECS/EntityQueue.h"

struct IslandData
{
    uint32_t Head;
    uint32_t Tail;
    uint32_t EntityCount;
    bool CanSplit;
    bool Awake;
};

//Island is based on the sorting of the contact Pairs. But the final entity groups are the same. Only use the island for internal use.
template<uint32_t EntityCount, uint32_t MaxContactPairs>
class ContactCache2
{
    using EntityPairList = LinkedList<EntityPair, MaxContactPairs>;
    using EntityPairIterator = typename LinkedList<EntityPair, MaxContactPairs>::Iterator;

public:
    inline ContactCache2() noexcept = default;

    inline ContactCache2(const ContactCache2& other)
    {
        m_Islands = other.m_Islands;
    }

    void Initialize()
    {
        m_ContactPairs.Initialize();

        for (Entity entity = 0; entity < s_MaxEntities; ++entity)
        {
            m_AvailableIslands.Push(entity);
        }
    }

    void StartPairUpdate()
    {
        m_ContactPairs.Flip();
    }

    ///Updates all islands and contact pairs. Requires the contact pairs to be sorted by the entity pair keys
    void UpdateContactPair(ContactPair& newContactPair)
    {
        while (m_ContactPairs.CurrentIndex() < m_ContactPairs.ReadCurrentCount())
        {
            ContactPair oldContactPair;
            m_ContactPairs.ReadCurrent(oldContactPair);

            if (newContactPair.EntityKey == oldContactPair.EntityKey)
            {
                //Update contact
                m_ContactPairs.Cache(newContactPair.EntityKey, newContactPair);
                m_ContactPairs.Advance();
                return;
            }

            if (newContactPair.EntityKey < oldContactPair.EntityKey)
            {
                //New pair appeared
                Island island1 = EntityIslands[newContactPair.EntityKey.Entity1()];
                Island island2 = EntityIslands[newContactPair.EntityKey.Entity2()];

                //Check if pair is assigned to a different island
                if (island1 != island2)
                {
                    MergeIslands(island1, island2);
                }

                m_ContactPairs.Cache(newContactPair.EntityKey, newContactPair);
                m_ContactPairs.Advance();
                return;
            }

            //Missing pairs
            Island island = EntityIslands[oldContactPair.EntityKey.Entity1()];
            m_Islands[island].CanSplit = true;
            m_ContactPairs.Advance();
        }

        //Reached end of list
        Island island1 = EntityIslands[newContactPair.EntityKey.Entity1()];
        Island island2 = EntityIslands[newContactPair.EntityKey.Entity2()];

        if (island1 != island2)
        {
            MergeIslands(island1, island2);
        }

        m_ContactPairs.Cache(newContactPair.EntityKey, newContactPair);
        m_ContactPairs.Advance();

        //m_Islands[island1].Awake = true;
        //m_Islands[island2].Awake = true;
    }

    void EndPairUpdate()
    {
        //Handle splits at end of pair list
        while (m_ContactPairs.CurrentIndex() < m_ContactPairs.ReadCurrentCount())
        {
            ContactPair& oldContactPair = m_ContactPairs.ReadCurrent();
            Island island = EntityIslands[oldContactPair.EntityKey.Entity1()];
            m_Islands[island].CanSplit = true;
            m_ContactPairs.Advance();
        }

        //Split marked island if needed

    }

    [[nodiscard]] Island GetIslandCount() const
    {
        return s_MaxEntities - m_AvailableIslands.Count();
    }


    class EntityPairRange
    {
    public:
        EntityPairRange(const EntityPairList& data, uint32_t head) : m_Begin(data.begin(head)), m_End(data.end()) { }

        EntityPairIterator begin() const { return m_Begin; }
        EntityPairIterator end() const { return m_End; }

    private:
        EntityPairIterator m_Begin;
        EntityPairIterator m_End;
    };

    EntityPairRange GetEntityPairs(Island island) const
    {
        assert(island < s_MaxEntities && "Invalid Island");
        assert(m_IslandStates.Test(island) && "Island does not exist");
        return EntityPairRange(m_LinkedIslands, m_Islands[island].Head);
    }

    class EntityIterator
    {
    public:
        EntityIterator() : m_Entity(EntityCount) { } // End iterator

        EntityIterator(const EntityPairList& data, uint32_t head)
        {
            uint32_t currentIndex = head;

            while (currentIndex != EntityPairList::InvalidIndex)
            {
                const auto& entry = data[currentIndex];

                Entity entity1 = entry.Value.Entity1();
                Entity entity2 = entry.Value.Entity2();

                m_Visited.SetTrue(entity1);
                m_Visited.SetTrue(entity2);

                assert (currentIndex != entry.NextEntry && "Infinite loop detected while processing a linked list");

                currentIndex = entry.NextEntry;
            }

            m_Entity = m_Visited.NextIndex();
        }

        Entity operator*() const
        {
            return m_Entity;
        }

        EntityIterator& operator++()
        {
            m_Entity = m_Visited.NextIndex(m_Entity + 1);
            return *this;
        }

        bool operator==(const EntityIterator& other) const
        {
            return m_Entity == other.m_Entity;
        }

        bool operator!=(const EntityIterator& other) const
        {
            return m_Entity != other.m_Entity;
        }

    private:
        Entity m_Entity;
        Bitset<EntityCount> m_Visited;
    };

    class EntityRange
    {
    public:
        EntityRange(const EntityPairList& data, uint32_t head) : m_Begin(data, head), m_End() { }

        EntityIterator begin() const { return m_Begin; }
        EntityIterator end() const { return m_End; }

    private:
        EntityIterator m_Begin;
        EntityIterator m_End;
    };

    EntityRange GetEntities(Island island) const
    {
        assert(island < s_MaxEntities && "Invalid Island");
        assert(m_IslandStates.Test(island) && "Island does not exist");
        return EntityRange(m_LinkedIslands, m_Islands[island].Head);
    }

private:
    Island CreateIsland()
    {
        Island newIsland = m_AvailableIslands.Dequeue();
        m_IslandStates.SetTrue(newIsland);
        return newIsland;
    }

    void RemoveIsland(Island island)
    {
        m_IslandStates.SetFalse(island);
        m_AvailableIslands.Push(island);
    }

    /// Merges two island together, invalidates one of the islands
    void MergeIslands(Island island1, Island island2)
    {
        if (island1 == island2) return;

        IslandData& islandData1 = m_Islands[island1].Data;
        IslandData& islandData2 = m_Islands[island2].Data;

        bool mergeDirection = islandData1.EntityCount >= islandData2.EntityCount;

        Island islandKeep = mergeDirection ? island1 : island2;
        Island islandMerge = mergeDirection ? island2 : island1;

        IslandData& islandDataKeep = m_Islands[islandKeep];
        IslandData& islandDataMerge =  m_Islands[islandMerge];

        //Make tail point to other island head
        m_LinkedIslands[islandDataKeep.Tail].NextEntry = islandDataMerge.Head;
        islandDataKeep.Tail = islandDataMerge.Tail;
        islandDataKeep.EntityCount += islandDataMerge.EntityCount;
        islandDataKeep.Awake = false;
        islandDataKeep.CanSplit |= islandDataMerge.CanSplit;

        //Set entity islands
        uint32_t currentIndex = islandDataMerge.Head;
        for (uint32_t i = 0; i < islandDataMerge.EntityCount; ++i)
        {
            EntityIslands[currentIndex] = islandDataKeep;
            currentIndex = m_LinkedIslands[currentIndex].NextIndex;
        }

        RemoveIsland(islandMerge);
    }

    void ValidateIsland(Island island)
    {
        if (!m_Islands[island].CanSplit) return;

        SplitIsland(island);
    }

    void SplitIsland(Island island)
    {
        Bitset<EntityCount> visited;
        EntityQueue<EntityCount> queue;

        std::array<uint32_t, EntityCount> components;
        uint32_t componentCount = 0;

        EntityRange entities = GetEntities(island);

        for (Entity entity : entities)
        {
            if (visited.Test(entity)) continue;

            uint32_t islandHead = EntityPairList::InvalidIndex;
        }
    }

private:
    SortedDoubleMap<EntityPair, ContactPair, MaxContactPairs> m_ContactPairs;       // Stores contact pairs
    std::array<Island, EntityCount> EntityIslands { };                              // Stores the island, the entity belongs to
    std::array<IslandData, EntityCount> m_Islands { };                              // Stores island data
    EntityPairList m_LinkedIslands;                                                 // Stores the connecting entities as a linkedlist //todo make it be sorted per linked list per entitykey
    EntityQueue<EntityCount> m_AvailableIslands;                                    // Store available islands          todo: make queue also for other types
    Bitset<EntityCount> m_IslandStates;                                             // Stores if an island ID is used

    std::array<ContactPair, MaxContactPairs> m_NewContacts { };
};

//clear contact pair array
//build array of contact pairs when contact pair is first added in the collision detection
//contact pair data changed, but most entity pairs stayed the same, island stay but rebuild
//function that needs to write the new contact pair somewhere
//it als need to access: EntityKey

//Required
//Get contact pairs per island
//Get active islands (store inactive?)
//Get also entities per island


//todo
//linked list should store the entity pairs per island
//store whether the island that it tries to access does even exist