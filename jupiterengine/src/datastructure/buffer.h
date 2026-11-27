#pragma once

#include <cstring>
#include <iterator>
#include <utility>
#include "util/inlines.h"

template<typename T, std::size_t capacity> class Buffer {
public:
    Buffer() noexcept = default;
    Buffer(std::size_t size)
        : m_Size{size} {}
    consteval Buffer(std::initializer_list<T> init) noexcept
    {
        for (const T& value : init)
            m_Data[m_Size++] = value;
    }
    Buffer(const Buffer<T, capacity>& other) noexcept
        : m_Size{other.m_Size}
    {
        std::memcpy(m_Data, other.m_Data, capacity * sizeof(T));
    }
    Buffer<T, capacity>& operator=(const Buffer<T, capacity>& other) noexcept
    {
        m_Size = other.m_Size;
        std::memcpy(m_Data, other.m_Data, capacity * sizeof(T));
        return *this;
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
    INLINE T *begin() noexcept
    {
        return m_Data;
    }
    INLINE T *end() noexcept
    {
        return m_Data + m_Size;
    }
    INLINE const T *begin() const noexcept
    { 
        return m_Data;
    } 
    INLINE const T *end() const noexcept
    {
        return m_Data + m_Size;
    }
    INLINE const T *cbegin() const noexcept
    {
        return begin();
    }
    INLINE const T *cend() const noexcept
    {
        return end();
    }
    INLINE std::reverse_iterator<T> rbegin() noexcept
    {
        return std::reverse_iterator<T>(end());
    }
    INLINE std::reverse_iterator<T> rend() noexcept
    {
        return std::reverse_iterator<T>(begin());
    }
    INLINE std::reverse_iterator<const T> rbegin() const noexcept
    {
        return std::reverse_iterator<const T>(end());
    }
    INLINE std::reverse_iterator<const T> rend() const noexcept
    {
        return std::reverse_iterator<const T>(begin());
    }
    INLINE std::reverse_iterator<const T> crbegin() const noexcept
    {
        return rbegin();
    }
    INLINE std::reverse_iterator<const T> crend() const noexcept
    {
        return rend();
    }
    INLINE T& operator[](std::size_t i) noexcept
    { 
        return m_Data[i]; 
    }
    INLINE const T& operator[](std::size_t i) const noexcept
    { 
        return m_Data[i]; 
    }

private:
    T m_Data[capacity];
    std::size_t m_Size{0};
};
