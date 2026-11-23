#pragma once 

#include <csignal>
#include <cstddef>
#include <cstring>
#include <utility>

// TODO: Add debug only checks to all my datastructures
#ifdef DEBUG 
    #include "util/log.h"
#endif

template<typename T, std::size_t capacity> class CircularQueue {
public:
    CircularQueue() noexcept = default;
    CircularQueue(const CircularQueue<T, capacity>& other) noexcept
        : m_Top{other.m_Top}, m_Bottom{other.m_Bottom}, m_Size{other.m_Size}
    {
        std::memcpy(m_Data, other.m_Data, capacity * sizeof(T));
    }
    void operator=(const CircularQueue<T, capacity>& other) noexcept
    {
        m_Top = other.m_Top;
        m_Bottom = other.m_Bottom;
        m_Size = other.m_Size;
        std::memcpy(m_Data, other.m_Data, capacity * sizeof(T));
    }
    void Push(const T& value)
    {
        if (m_Size < capacity)
            m_Size++;
        #ifdef DEBUG 
        else
            ERROR("Ouroboros situation in the circular queue.");
        #endif

        m_Data[m_Top] = value;
        m_Top = (m_Top + 1) % capacity;
    }
    void Push(T&& value)
    {
        if (m_Size < capacity)
            m_Size++;
        #ifdef DEBUG 
        else
            ERROR("Ouroboros situation in the circular queue.");
        #endif

        m_Data[m_Top] = std::forward<T>(value);
        m_Top = (m_Top + 1) % capacity;
    }
    template<class... Args> void Emplace(Args&&... args) 
    {
        if (m_Size < capacity)
            m_Size++;
        #ifdef DEBUG 
        else
            ERROR("Ouroboros situation in the circular queue.");
        #endif

        new (&m_Data[m_Top]) T(std::forward<Args>(args)...);
        m_Top = (m_Top + 1) % capacity;
    }
    void Pop()
    {
        #ifdef DEBUG 
        if (m_Size == 0)
            ERROR("Underflowing circular queue.");
        #endif

        m_Top = (m_Top + capacity - 1) % capacity;
        m_Data[m_Top].~T();
        m_Size--;
    }
    const T& Peek() const
    {
        #ifdef DEBUG 
        if (m_Size == 0)
            ERROR("Nothing to peek in circular queue.");
        #endif
        return m_Data[(m_Top + capacity - 1) % capacity];
    }
    std::size_t Size() const 
    {
        return m_Size;
    }

private:
    std::size_t m_Top{0};
    std::size_t m_Bottom{0};
    std::size_t m_Size{0};
    T m_Data[capacity];
};
