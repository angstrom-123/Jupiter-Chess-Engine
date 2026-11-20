#pragma once 

#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <utility>
#include "inlines.h"

template<typename T, std::size_t capacity> class CircularStack {
public:
    CircularStack() = default;
    CircularStack(const CircularStack<T, capacity>& other)
        : m_Size{other.Size()}, m_Top{other.TopIndex()}, m_Bottom{other.BottomIndex()}
    {
        std::memcpy(m_Data, other.Data(), capacity * sizeof(T));
    }
    void Push(T&& value)
    {
        if (m_Size == capacity)
            m_Bottom = (m_Bottom + 1) % capacity;

        m_Data[m_Top] = std::move(value);
        m_Top = (m_Top + 1) % capacity;
        if (m_Size < capacity)
            m_Size++;
    }
    void Push(const T& value)
    {
        if (m_Size == capacity)
            m_Bottom = (m_Bottom + 1) % capacity;

        m_Data[m_Top] = std::move(value);
        m_Top = (m_Top + 1) % capacity;
        if (m_Size < capacity)
            m_Size++;
    }
    void Pop()
    {
        m_Top = (m_Top + capacity - 1) % capacity;
        m_Data[m_Top].~T();
        m_Size--;
    }
    INLINE T& Top()
    {
        return m_Data[(m_Top + capacity - 1) % capacity];
    }
    INLINE const T& Top() const
    {
        return m_Data[(m_Top + capacity - 1) % capacity];
    }
    INLINE const T& Peek(std::size_t depth) const 
    {
        return m_Data[(m_Top + capacity - 1 - depth) % capacity];
    }
    bool Contains(const T& item) const 
    {
        for (std::size_t i = 0; i < m_Size; i++) {
            if (Peek(i) == item)
                return true;
        }
        return false;
    }
    void Clear() 
    {
        for (std::size_t i = 0; i < m_Size; i++) {
            m_Top = (m_Top + capacity - 1) % capacity;
            m_Data[m_Top].~T();
        }
        m_Size = 0;
    }
    INLINE std::size_t Size() const
    {
        return m_Size;
    }
    INLINE std::size_t Capacity() const
    {
        return capacity;
    }
    INLINE T* Data()
    {
        return m_Data;
    }
    INLINE const T * const Data() const
    {
        return m_Data;
    }
    INLINE std::size_t TopIndex() const
    {
        return m_Top;
    }
    INLINE std::size_t BottomIndex() const
    {
        return m_Bottom;
    }

private:
    T m_Data[capacity]{};
    std::size_t m_Size{0};
    std::size_t m_Top{0};
    std::size_t m_Bottom{0};
};
