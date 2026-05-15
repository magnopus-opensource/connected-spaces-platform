/*
 * Copyright 2023 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "CSP/CSPCommon.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <utility>

namespace csp::common
{

CSP_START_IGNORE
template <typename T> class Array;
CSP_END_IGNORE

const auto LIST_DEFAULT_SIZE = 4;

inline size_t next_pow2(size_t val)
{
    --val;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    ++val;

    return val;
}

/// @brief Simple DLL-safe resizable collection of objects.
///
/// Simple list type used to pass a collection of objects across the DLL boundary.
/// This class is implemented using an array and, as such, removing items is not cheap as it requires
/// us to move all items after it down one space.
///
/// @tparam T : Object type to store in the list
template <typename T> class CSP_API List
{
public:
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    /// @brief Constructs a list with 0 elements.
    List()
        : m_currentSize(0)
        , m_maximumSize(0)
        , m_objectArray(nullptr)
    {
        AllocList(LIST_DEFAULT_SIZE);
    }

    /// @brief Constructs a list with the given number of elements.
    /// @param Size size_t : Number of elements in the array
    List(size_t minimumSize)
        : m_currentSize(0)
        , m_maximumSize(0)
        , m_objectArray(nullptr)
    {
        auto size = next_pow2(minimumSize);
        AllocList(size);
    }

    /// @brief Copy constructor.
    /// @param Other const List<T>&
    List(const List<T>& other)
        : m_currentSize(0)
        , m_maximumSize(0)
        , m_objectArray(nullptr)
    {
        if (other.m_currentSize == 0)
        {
            AllocList(LIST_DEFAULT_SIZE);

            return;
        }

        AllocList(other.m_maximumSize);
        m_currentSize = other.m_currentSize;

        for (size_t i = 0; i < m_currentSize; ++i)
        {
            T* objectPtr = &m_objectArray[i];
            new (objectPtr) T;
            m_objectArray[i] = other.m_objectArray[i];
        }
    }

    CSP_NO_EXPORT List(List<T>&& other)
        : m_currentSize(0)
        , m_maximumSize(0)
        , m_objectArray(nullptr)
    {
        if (other.m_currentSize == 0)
        {
            AllocList(LIST_DEFAULT_SIZE);

            return;
        }

        m_currentSize = other.m_currentSize;
        m_maximumSize = other.m_maximumSize;
        m_objectArray = other.m_objectArray;

        other.m_objectArray = nullptr;
    }

    /// @brief Constructs a list from an initializer_list.
    /// @param List std::initializer_list : Elements to construct the list from
    CSP_NO_EXPORT List(std::initializer_list<T> list)
        : m_currentSize(0)
        , m_maximumSize(0)
        , m_objectArray(nullptr)
    {
        if (list.size() == 0)
        {
            AllocList(LIST_DEFAULT_SIZE);

            return;
        }

        auto size = next_pow2(list.size());
        AllocList(size);
        m_currentSize = list.size();

        for (size_t i = 0; i < m_currentSize; ++i)
        {
            T* objectPtr = &m_objectArray[i];
            new (objectPtr) T;
            m_objectArray[i] = *(list.begin() + i);
        }
    }

    /// @brief Destructor.
    /// Frees list memory.
    ~List() { FreeList(); }

    /// @brief Returns a pointer to the start of the list.
    /// @return T*
    CSP_NO_EXPORT T* Data() { return m_currentSize > 0 ? &m_objectArray[0] : nullptr; }

    /// @brief Returns a const pointer to the start of the list.
    /// @return const T*
    CSP_NO_EXPORT const T* Data() const { return m_currentSize > 0 ? &m_objectArray[0] : nullptr; }

    // Iterators
    CSP_NO_EXPORT T* begin() { return Data(); }
    CSP_NO_EXPORT const T* begin() const { return Data(); }
    CSP_NO_EXPORT const T* cbegin() const { return Data(); }

    CSP_NO_EXPORT T* end() { return Data() + Size(); }
    CSP_NO_EXPORT const T* end() const { return Data() + Size(); }
    CSP_NO_EXPORT const T* cend() const { return Data() + Size(); }

    CSP_NO_EXPORT reverse_iterator rbegin() { return reverse_iterator(end()); }
    CSP_NO_EXPORT const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    CSP_NO_EXPORT const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }

    CSP_NO_EXPORT reverse_iterator rend() { return reverse_iterator(begin()); }
    CSP_NO_EXPORT const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    CSP_NO_EXPORT const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }

    /// @brief Copy assignment.
    /// @param Other const List<T>&
    /// @return List<T>&
    List<T>& operator=(const List<T>& other)
    {
        if (this == &other)
        {
            return *this;
        }

        m_currentSize = other.m_currentSize;
        m_maximumSize = 0;
        m_objectArray = nullptr;

        if (m_currentSize == 0)
        {
            AllocList(LIST_DEFAULT_SIZE);

            return *this;
        }

        AllocList(other.m_maximumSize);

        for (size_t i = 0; i < m_currentSize; i++)
        {
            m_objectArray[i] = other.m_objectArray[i];
        }

        return *this;
    }

    /// @brief Returns an element at the given index of the list.
    /// @param Index size_t : Element index to access
    /// @return T& : List element
    T& operator[](size_t index)
    {
        assert(index < m_currentSize);

        return m_objectArray[index];
    }

    /// @brief Returns a const element at the given index of the list.
    /// @param Index size_t : Element index to access
    /// @return const T& : List element
    const T& operator[](size_t index) const
    {
        assert(index < m_currentSize);

        return m_objectArray[index];
    }

    /// @brief Appends an element to the end of the list.
    /// @param Item const T&
    void Append(const T& item)
    {
        if (m_currentSize == m_maximumSize)
        {
            auto size = next_pow2(m_maximumSize + 1);
            ReallocList(size);
        }

        // Instantiate element first to allow copy assignment
        T* objectPtr = &m_objectArray[m_currentSize];
        new (objectPtr) T;
        m_objectArray[m_currentSize++] = item;
    }

    /// @brief Appends an element to the end of the list.
    /// @param Item T&&
    CSP_NO_EXPORT void Append(T&& item)
    {
        if (m_currentSize == m_maximumSize)
        {
            auto size = next_pow2(m_maximumSize + 1);
            ReallocList(size);
        }

        // Instantiate element first to allow move assignment
        T* objectPtr = &m_objectArray[m_currentSize];
        new (objectPtr) T;
        m_objectArray[m_currentSize++] = std::move(item);
    }

    /// @brief Appends an element at the given index of the list.
    /// @param Index size_t
    /// @param Item const T&
    void Insert(size_t index, const T& item)
    {
        if (m_currentSize == m_maximumSize)
        {
            auto size = next_pow2(m_maximumSize + 1);
            ReallocList(size);
        }

        auto after = m_currentSize - index;
// This is a real problem, don't ignore this, it needs fixed. (Or this entire type needs deleted)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wnontrivial-memcall"
#endif
        std::memmove(m_objectArray + (index + 1), m_objectArray + index, sizeof(T) * after);
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
        ++m_currentSize;

        T* objectPtr = &m_objectArray[index];
        new (objectPtr) T;
        m_objectArray[index] = item;
    }

    /// @brief Removes an element to a specific index of the list.
    /// @param Index size_t
    void Remove(size_t index)
    {
        assert(index < m_currentSize);

        if constexpr (std::is_pointer<T>::value)
        {
            // I dont think the list owns any pointers that are in it. That seems weird, because it _will_ destruct value types
            // To maintain exactly the same behaviour, do the destruct behaviour for pointers. Use regular RAII cleanup for value types below during
            // the shift.

            // delete ObjectArray[Index]; <-- What we should be doing ... (although actually, we should just have this type backed by std::list)
            T* objectPtr = &m_objectArray[index];
            objectPtr->~T();
        }

        // Shift everything left
        for (size_t i = index; i < m_currentSize - 1; ++i)
        {
            m_objectArray[i] = m_objectArray[i + 1];
        }
        --m_currentSize;
    }

    /// @brief Removes the given element from the list.
    /// @param Item const T& : Element to remove from the list
    void RemoveItem(const T& item)
    {
        for (size_t i = 0; i < m_currentSize; ++i)
        {
            if (m_objectArray[i] == item)
            {
                Remove(i);
                return;
            }
        }
    }

    /// @brief Returns the number of elements in the array.
    /// @return const size_t
    const size_t Size() const { return m_currentSize; }

    /// @brief Removes all elements in the list.
    void Clear()
    {
        FreeList();
        AllocList(LIST_DEFAULT_SIZE);
    }

    /// @brief Checks if the list contains the given element.
    /// @param Item const T& : Element to check if the list contains
    /// @return bool
    bool Contains(const T& item) const
    {
        for (size_t i = 0; i < m_currentSize; ++i)
        {
            if (m_objectArray[i] == item)
            {
                return true;
            }
        }

        return false;
    }

    /// @brief Returns a copy of this List as an Array
    /// @return Array<T>
    CSP_NO_EXPORT Array<T> ToArray() const
    {
        Array<T> result(m_currentSize);

        for (size_t i = 0; i < m_currentSize; ++i)
        {
            result[i] = m_objectArray[i];
        }

        return std::move(result);
    }

private:
    /// @brief Allocates memory for the list.
    /// @param Size size_t : Number of elements in the list
    void AllocList(const size_t size)
    {
        m_objectArray = new T[size];
        m_maximumSize = size;
    }

    void ReallocList(const size_t size)
    {
        T* newArray = new T[size];
        if (m_objectArray)
        {
            std::copy(m_objectArray, m_objectArray + std::min(m_maximumSize, size), newArray);
            delete[] m_objectArray;
        }
        m_objectArray = newArray;
        m_maximumSize = size;
    }

    /// @brief Frees memory for the list.
    void FreeList()
    {
        delete[] m_objectArray;
        m_objectArray = nullptr;
        m_currentSize = 0;
        m_maximumSize = 0;
    }

    size_t m_currentSize;
    size_t m_maximumSize;
    T* m_objectArray;
};

} // namespace csp::common
