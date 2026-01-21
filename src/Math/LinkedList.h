#pragma once

#include <array>
#include <cstdint>

template<typename T, uint32_t Size>
class LinkedList
{
public:
    static constexpr uint32_t InvalidIndex = Size;

    struct Entry
    {
        T Value;
        uint32_t NextEntry;
    };

    class Iterator
    {
    public:
        Iterator(const std::array<Entry, Size>& data, uint32_t index) : m_Data(data), m_Index(index) { }

        const T& operator*() const
        {
            assert(m_Index != InvalidIndex && "Trying to dereference an invalid element in the linked list");
            return m_Data[m_Index].Value;
        }

        const T* operator->() const
        {
            assert(m_Index != InvalidIndex && "Trying to dereference an invalid element in the linked list");
            return &m_Data[m_Index].Value;
        }

        Iterator& operator++()
        {
            if (m_Index != InvalidIndex)
            {
                m_Index = m_Data[m_Index].NextEntry;
            }

            return *this;
        }

        bool operator==(const Iterator& other) const
        {
            return m_Index == other.m_Index;
        }

        bool operator!=(const Iterator& other) const
        {
            return m_Index != other.m_Index;
        }

    private:
        const std::array<Entry, Size>& m_Data;
        uint32_t m_Index;
    };

    Iterator begin(uint32_t head) const
    {
        return Iterator(m_Data, head);
    }

    Iterator end() const
    {
        return Iterator(m_Data, InvalidIndex);
    }

    Entry& operator[](uint32_t index)
    {
        return m_Data[index];
    }

    const Entry& operator[](uint32_t index) const
    {
        return m_Data[index];
    }

private:
    std::array<Entry, Size> m_Data { };
};