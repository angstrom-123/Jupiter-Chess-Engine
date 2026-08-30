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

// template<typename T, std::size_t capacity> class CircularStack {
// public:
//     CircularStack() = default;
//     void Push(T&& value)
//     {
//         if (m_Size == capacity)
//             AdvanceBottom();
//         m_Data[m_Top] = std::move(value);
//         AdvanceTop();
//     }
//     void Push(const T& value)
//     {
//         if (m_Size == capacity)
//             AdvanceBottom();
//         m_Data[m_Top] = value;
//         AdvanceTop();
//     }
//     template<class... Args> void Emplace(Args&&... args)
//     {
//         if (m_Size == capacity)
//             AdvanceBottom();
//         m_Data[m_Top] = T(std::forward<Args>(args)...);
//         AdvanceTop();
//     }
//     void Pop()
//     {
//         if (m_Size == 0) {
//             ERROR("Popping from empty stack");
//             std::raise(SIGILL);
//         }
//
//         m_Top = (m_Top + capacity - 1) % capacity;
//         m_Size--;
//         m_Data[m_Top].~T();
//     }
//     std::size_t Size() const
//     {
//         return m_Size;
//     }
//     T& Top()
//     {
//         return m_Data[(m_Top + capacity - 1) % capacity];     
//     }
//
// private:     
//     void AdvanceTop()
//     {
//         m_Top = (m_Top + 1) % capacity;
//         if (m_Size < capacity)
//             m_Size++;
//     }
//     void AdvanceBottom()
//     {
//         m_Bottom = (m_Bottom + 1) % capacity;
//     }
//
// private:
//     T m_Data[capacity]{};
//     std::size_t m_Top{0};
//     std::size_t m_Bottom{0};
//     std::size_t m_Size{0};
// };
