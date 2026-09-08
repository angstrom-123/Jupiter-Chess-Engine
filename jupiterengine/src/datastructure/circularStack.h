#pragma once 

#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <utility>

template<typename T, std::size_t capacity> class CircularStack {
public:
    CircularStack() = default;
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
        if (m_Size == 0)
            return;

        m_Top = (m_Top + capacity - 1) % capacity;
        m_Data[m_Top].~T();
        m_Size--;
    }
    T& Top()
    {
        return m_Data[(m_Top + capacity - 1) % capacity];
    }
    const T& Peek(std::size_t depth) const 
    {
        return m_Data[(m_Top + capacity - 1 - depth) % capacity];
    }
    std::size_t Size()
    {
        return m_Size;
    }

private:
    T m_Data[capacity]{};
    std::size_t m_Size{0};
    std::size_t m_Top{0};
    std::size_t m_Bottom{0};
};
