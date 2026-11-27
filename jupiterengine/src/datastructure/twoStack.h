#pragma once

#include "util/inlines.h"
#include <cstdint>
#include <utility>

template<typename T> class TwoStack {
public:
    TwoStack() noexcept = default;
    TwoStack(const TwoStack<T>& other) noexcept
        : m_Size{other.m_Size}, m_Index{other.m_Index} 
    {
        m_Data[0] = other.m_Data[0];
        m_Data[1] = other.m_Data[1];
    }
    TwoStack& operator=(const TwoStack<T>& other) noexcept
    {
        m_Size = other.m_Size;
        m_Index = other.m_Index;
        m_Data[0] = other.m_Data[0];
        m_Data[1] = other.m_Data[1];
        return *this;
    }
    INLINE void Push(T&& value)
    {
        m_Data[m_Index] = std::forward<T>(value);
        m_Index ^= 1;
        if (m_Size < 2)
            m_Size++;
    }
    INLINE void Push(const T& value)
    {
        m_Data[m_Index] = value;
        m_Index ^= 1;
        if (m_Size < 2)
            m_Size++;
    }
    INLINE const T& Peek(uint8_t depth) const
    {
        return m_Data[(m_Index + 1 - depth) & 1];
    }
    void Clear() 
    {
        for (std::size_t i = 0; i < m_Size; i++) {
            m_Index = (m_Index + 1) & 1;
            m_Data[m_Index].~T();
        }
        m_Size = 0;
    }
    bool Contains(const T& item) const 
    {
        if (m_Size == 2)
            return m_Data[0] == item || m_Data[1] == item;
        else if (m_Size == 1)
            return m_Data[0] == item;
        return false;
    }
    uint8_t Size() const 
    {
        return m_Size;
    }

private:
    T m_Data[2];
    uint8_t m_Size{0};
    uint8_t m_Index{0};
};
