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

#include <cassert>
#include <cstring>
#include <initializer_list>
#include <stdexcept>
#include <utility>

#ifndef CSP_DISABLE_OVERFLOW_CHECKING
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <cstdint>
#endif
#endif

namespace csp::common
{

CSP_START_IGNORE
template <typename T> class List;
CSP_END_IGNORE

/// @brief Simple DLL-safe array of objects.
///
/// Simple array type used to pass arrays of objects across the DLL boundary.
///
/// @tparam T : Object type to store in the array
template <typename T> class CSP_API Array
{
public:
    /// @brief Constructs an array with 0 elements
    Array()
        : m_arraySize(0)
        , m_objectArray(nullptr)
    {
    }

    /// @brief Constructs an array with the given number of elements.
    /// Each element in the array will have it's default constuctor called.
    /// @param Size const size_t : Number of elements in the array
    explicit Array(const size_t size)
        : m_arraySize(0)
        , m_objectArray(nullptr)
    {
        if (size > 0)
        {
            AllocArray(size);
        }
    }

    /// @brief Copy constructor.
    /// @param Other const Array<T>& Other
    CSP_NO_EXPORT Array(const Array<T>& other)
        : m_arraySize(0)
        , m_objectArray(nullptr)
    {
        m_arraySize = other.m_arraySize;

        if (m_arraySize > 0)
        {
            AllocArray(m_arraySize);

            for (size_t i = 0; i < m_arraySize; i++)
            {
                m_objectArray[i] = other.m_objectArray[i];
            }
        }
    }

    /// @brief Constructs an array from an initializer_list.
    /// @param List std::initializer_list : Elements to construct the array from
    CSP_NO_EXPORT Array(std::initializer_list<T> list)
        : m_arraySize(0)
        , m_objectArray(nullptr)
    {
        if (list.size() > 0)
        {
            AllocArray(list.size());

            for (size_t i = 0; i < list.size(); ++i)
            {
                m_objectArray[i] = *(list.begin() + i);
            }
        }
    }

    /// @brief Destructor.
    /// Frees array memory.
    ~Array() { FreeArray(); }

    /// @brief Returns a pointer to the start of the array.
    /// @return T*
    CSP_NO_EXPORT T* Data() { return m_objectArray; }

    /// @brief Returns a const pointer to the start of the array.
    /// @return const T*
    CSP_NO_EXPORT const T* Data() const { return m_objectArray; }

    // Iterators
    CSP_NO_EXPORT T* begin() { return Data(); }
    CSP_NO_EXPORT const T* begin() const { return Data(); }
    CSP_NO_EXPORT const T* cbegin() const { return Data(); }

    CSP_NO_EXPORT T* end() { return Data() + Size(); }
    CSP_NO_EXPORT const T* end() const { return Data() + Size(); }
    CSP_NO_EXPORT const T* cend() const { return Data() + Size(); }

    /// @brief Copy assignment.
    /// @param Other const Array<T>&
    /// @return Array<T>&
    Array<T>& operator=(const Array<T>& other)
    {
        if (this == &other)
        {
            return *this;
        }

        m_arraySize = other.m_arraySize;
        m_objectArray = nullptr;

        if (m_arraySize > 0)
        {
            AllocArray(m_arraySize);

            for (size_t i = 0; i < m_arraySize; i++)
            {
                m_objectArray[i] = other.m_objectArray[i];
            }
        }

        return *this;
    }

    /// @brief Returns an element at the given index of the array.
    /// @param Index const size_t : Element index to access
    /// @return T& : Array element
    T& operator[](const size_t index)
    {
#ifndef CSP_DISABLE_BOUNDS_CHECKING
        if (index >= m_arraySize)
        {
            throw std::out_of_range("Index");
        }
#endif

        return m_objectArray[index];
    }

    /// @brief Returns a const element at the given index of the array.
    /// @param Index const size_t : Element index to access
    /// @return const T& : Array element
    const T& operator[](const size_t index) const
    {
#ifndef CSP_DISABLE_BOUNDS_CHECKING
        if (index >= m_arraySize)
        {
            throw std::out_of_range("Index");
        }
#endif

        return m_objectArray[index];
    }

    /// @brief Compares two arrays for equality.
    /// @param Other const Array<T>& : Array to compare against
    /// @return bool
    bool operator==(const Array<T>& other) const
    {
        if (m_arraySize != other.m_arraySize)
        {
            return false;
        }

        for (size_t i = 0; i < m_arraySize; ++i)
        {
            if (m_objectArray[i] != other.m_objectArray[i])
            {
                return false;
            }
        }

        return true;
    }

    bool operator!=(const Array<T>& other) const { return !(*this == other); }

    /// @brief Returns the number of elements in the array.
    /// @return const size_t
    const size_t Size() const { return m_arraySize; }

    /// @brief Checks if the array has any elements.
    /// @return bool
    bool IsEmpty() const { return (m_arraySize == 0); }

    /// @brief Returns a copy of this Array as a List
    /// @return List<T>
    CSP_NO_EXPORT List<T> ToList() const
    {
        List<T> result(m_arraySize);

        for (size_t i = 0; i < m_arraySize; ++i)
        {
            result.Append(m_objectArray[i]);
        }

        return std::move(result);
    }

private:
    /// @brief Allocates memory for the array.
    /// @param Size const size_t : Number of elements in the array
    void AllocArray(const size_t size)
    {
        if (m_objectArray == nullptr)
        {
            m_objectArray = new T[size];
            m_arraySize = size;
        }
    }

    /// @brief Frees memory for the array.
    void FreeArray()
    {
        delete[] m_objectArray;
        m_objectArray = nullptr;
    }

    size_t m_arraySize;
    T* m_objectArray;
};

} // namespace csp::common
