#pragma once

#include <utility>

template<typename T, std::size_t capacity> class Buffer {
public:
    Buffer() = default;
    Buffer(std::size_t size)
        : m_Size{size} {}

    void PushBack(T &&value)
    {
        m_Data[m_Size++] = std::forward<T>(value);
    }
    void PushBack(const T& value)
    {
        m_Data[m_Size++] = value;
    }
    template<class... Args> void EmplaceBack(Args&&... args)
    {
        new (&m_Data[m_Size++]) T(std::forward<Args>(args)...);
    } 
    void PopBack() 
    {
        m_Data[m_Size--].~T();
    }
    void Clear() 
    {
        for (std::size_t i = 0; i < m_Size; i++)
            m_Data[i].~T();
        m_Size = 0;
    }
    void Resize(std::size_t size) 
    {
        m_Size = size;
    }
    [[nodiscard]] T *Data() 
    {
        return m_Data;
    }
    [[nodiscard]] std::size_t Size() const 
    {
        return m_Size;
    }
    [[nodiscard]] std::size_t Capacity() const 
    {
        return capacity;
    }
    [[nodiscard]] bool Contains(const T& item) const 
    {
        for (std::size_t i = 0; i < m_Size; i++) {
            if (m_Data[i] == item)
                return true;
        }
        return false;
    }
    [[nodiscard]] T *begin() 
    {
        return m_Data;
    }
    [[nodiscard]] T *end() 
    {
        return m_Data + m_Size;
    }
    [[nodiscard]] const T *begin() const 
    { 
        return m_Data;
    } 
    [[nodiscard]] const T *end() const 
    {
        return m_Data + m_Size;
    }
    [[nodiscard]] const T *cbegin() const 
    {
        return m_Data;
    }
    [[nodiscard]] const T *cend() const 
    {
        return m_Data + m_Size;
    }
    [[nodiscard]] T& operator[](std::size_t i) 
    { 
        return m_Data[i]; 
    }
    [[nodiscard]] const T& operator[](std::size_t i) const 
    { 
        return m_Data[i]; 
    }

private:
    T m_Data[capacity];
    std::size_t m_Size{0};
};
