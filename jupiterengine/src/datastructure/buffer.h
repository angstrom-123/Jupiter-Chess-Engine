#pragma once

#include <iterator>
#include <utility>
#include "inlines.h"

template<typename T, std::size_t capacity> class Buffer {
public:
    Buffer() = default;
    Buffer(std::size_t size)
        : m_Size{size} {}
    consteval Buffer(std::initializer_list<T> init)
    {
        for (const T& value : init)
            m_Data[m_Size++] = value;
    }

    INLINE void PushBack(T &&value)
    {
        m_Data[m_Size++] = std::forward<T>(value);
    }
    INLINE void PushBack(const T& value)
    {
        m_Data[m_Size++] = value;
    }
    template<class... Args> INLINE void EmplaceBack(Args&&... args)
    {
        new (&m_Data[m_Size++]) T(std::forward<Args>(args)...);
    } 
    INLINE void PopBack() 
    {
        m_Data[m_Size--].~T();
    }
    void Clear() 
    {
        for (std::size_t i = 0; i < m_Size; i++)
            m_Data[i].~T();
        m_Size = 0;
    }
    INLINE void Resize(std::size_t size) 
    {
        m_Size = size;
    }
    INLINE T *Data() 
    {
        return m_Data;
    }
    INLINE std::size_t Size() const 
    {
        return m_Size;
    }
    INLINE std::size_t Capacity() const 
    {
        return capacity;
    }
    bool Contains(const T& item) const 
    {
        for (std::size_t i = 0; i < m_Size; i++) {
            if (m_Data[i] == item)
                return true;
        }
        return false;
    }
    INLINE T *begin() 
    {
        return m_Data;
    }
    INLINE T *end() 
    {
        return m_Data + m_Size;
    }
    INLINE const T *begin() const 
    { 
        return m_Data;
    } 
    INLINE const T *end() const 
    {
        return m_Data + m_Size;
    }
    INLINE const T *cbegin() const 
    {
        return begin();
    }
    INLINE const T *cend() const 
    {
        return end();
    }
    INLINE std::reverse_iterator<T> rbegin() 
    {
        return std::reverse_iterator<T>(end());
    }
    INLINE std::reverse_iterator<T> rend() 
    {
        return std::reverse_iterator<T>(begin());
    }
    INLINE std::reverse_iterator<const T> rbegin() const
    {
        return std::reverse_iterator<const T>(end());
    }
    INLINE std::reverse_iterator<const T> rend() const
    {
        return std::reverse_iterator<const T>(begin());
    }
    INLINE std::reverse_iterator<const T> crbegin() const
    {
        return rbegin();
    }
    INLINE std::reverse_iterator<const T> crend() const
    {
        return rend();
    }
    INLINE T& operator[](std::size_t i) 
    { 
        return m_Data[i]; 
    }
    INLINE const T& operator[](std::size_t i) const 
    { 
        return m_Data[i]; 
    }

private:
    T m_Data[capacity];
    std::size_t m_Size{0};
};
