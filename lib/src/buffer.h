#pragma once

#include "stackTrace.h"
#include <utility>

template<typename T, std::size_t capacity> class Buffer {
public:
    Buffer() = default;
    Buffer(std::size_t size)
        : m_Size{size} 
    {
        JUPITER_TRACE();
    }

    void PushBack(T &&value)
    {
        JUPITER_TRACE();
        m_Data[m_Size++] = std::forward<T>(value);
    }
    void PushBack(const T& value)
    {
        JUPITER_TRACE();
        m_Data[m_Size++] = value;
    }
    template<class... Args> void EmplaceBack(Args&&... args)
    {
        JUPITER_TRACE();
        new (&m_Data[m_Size++]) T(std::forward<Args>(args)...);
    }
    void PopBack()
    {
        JUPITER_TRACE();
        m_Data[m_Size--].~T();
    }
    void Clear()
    {
        JUPITER_TRACE();
        for (std::size_t i = 0; i < m_Size; i++)
            m_Data[i].~T();
        m_Size = 0;
    }
    void Resize(std::size_t size)
    {
        JUPITER_TRACE();
        m_Size = size;
    }
    T *Data()
    {
        JUPITER_TRACE();
        return m_Data;
    }
    std::size_t Size() const
    {
        JUPITER_TRACE();
        return m_Size;
    }
    std::size_t Capacity() const
    {
        JUPITER_TRACE();
        return capacity;
    }
    T *begin()
    {
        JUPITER_TRACE();
        return m_Data;
    }
    T *end()
    {
        JUPITER_TRACE();
        return m_Data + m_Size;
    }
    const T *begin() const
    { 
        JUPITER_TRACE();
        return m_Data;
    }
    const T *end() const
    {
        JUPITER_TRACE();
        return m_Data + m_Size;
    }
    const T *cbegin() const
    {
        JUPITER_TRACE();
        return m_Data;
    }
    const T *cend() const
    {
        JUPITER_TRACE();
        return m_Data + m_Size;
    }
    T& operator[](std::size_t i) 
    { 
        JUPITER_TRACE();
        return m_Data[i]; 
    }
    const T& operator[](std::size_t i) const
    { 
        JUPITER_TRACE();
        return m_Data[i]; 
    }

private:
    T m_Data[capacity];
    std::size_t m_Size{0};
};
