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
#include "CSP/Memory/DllAllocator.h"

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
        : ArraySize(0)
        , ObjectArray(nullptr)
    {
    }

    /// @brief Constructs an array with the given number of elements.
    /// Each element in the array will have it's default constuctor called.
    /// @param Size const size_t : Number of elements in the array
    explicit Array(const size_t Size)
        : ArraySize(0)
        , ObjectArray(nullptr)
    {
        if (Size > 0)
        {
            AllocArray(Size);
        }
    }

    /// @brief Constructs an array from a buffer.
    /// @param Buffer const T* : Pointer to the beginning of the buffer
    /// @param Size size_t : Number of elements in the buffer
    CSP_NO_EXPORT Array(const T* Buffer, size_t Size)
        : ArraySize(0)
        , ObjectArray(nullptr)
    {
        if (Buffer != nullptr && Size > 0)
        {
            AllocArray(Size);
            memcpy(ObjectArray, Buffer, Size * sizeof(T));
        }
    }

    /// @brief Copy constructor.
    /// @param Other const Array<T>& Other
    CSP_NO_EXPORT Array(const Array<T>& Other)
        : ArraySize(0)
        , ObjectArray(nullptr)
    {
        ArraySize = Other.ArraySize;

        if (ArraySize > 0)
        {
            AllocArray(ArraySize);

            for (size_t i = 0; i < ArraySize; i++)
            {
                ObjectArray[i] = Other.ObjectArray[i];
            }
        }
    }

    /// @brief Constructs an array from an initializer_list.
    /// @param List std::initializer_list : Elements to construct the array from
    CSP_NO_EXPORT Array(std::initializer_list<T> List)
        : ArraySize(0)
        , ObjectArray(nullptr)
    {
        if (List.size() > 0)
        {
            AllocArray(List.size());

            for (size_t i = 0; i < List.size(); ++i)
            {
                ObjectArray[i] = *(List.begin() + i);
            }
        }
    }

    /// @brief Destructor.
    /// Frees array memory.
    ~Array() { FreeArray(); }

    /// @brief Returns a pointer to the start of the array.
    /// @return T*
    CSP_NO_EXPORT T* Data() { return ObjectArray; }

    /// @brief Returns a const pointer to the start of the array.
    /// @return const T*
    CSP_NO_EXPORT const T* Data() const { return ObjectArray; }

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
    Array<T>& operator=(const Array<T>& Other)
    {
        if (this == &Other)
        {
            return *this;
        }

        ArraySize = Other.ArraySize;
        ObjectArray = nullptr;

        if (ArraySize > 0)
        {
            AllocArray(ArraySize);

            for (size_t i = 0; i < ArraySize; i++)
            {
                ObjectArray[i] = Other.ObjectArray[i];
            }
        }

        return *this;
    }

    /// @brief Returns an element at the given index of the array.
    /// @param Index const size_t : Element index to access
    /// @return T& : Array element
    T& operator[](const size_t Index)
    {
#ifndef CSP_DISABLE_BOUNDS_CHECKING
        if (Index >= ArraySize)
        {
            throw std::out_of_range("Index");
        }
#endif

        return ObjectArray[Index];
    }

    /// @brief Returns a const element at the given index of the array.
    /// @param Index const size_t : Element index to access
    /// @return const T& : Array element
    const T& operator[](const size_t Index) const
    {
#ifndef CSP_DISABLE_BOUNDS_CHECKING
        if (Index >= ArraySize)
        {
            throw std::out_of_range("Index");
        }
#endif

        return ObjectArray[Index];
    }

    /// @brief Returns the number of elements in the array.
    /// @return const size_t
    const size_t Size() const { return ArraySize; }

    /// @brief Checks if the array has any elements.
    /// @return bool
    bool IsEmpty() const { return (ArraySize == 0); }

    /// @brief Returns a copy of this Array as a List
    /// @return List<T>
    CSP_NO_EXPORT List<T> ToList() const
    {
        List<T> Result(ArraySize);

        for (size_t i = 0; i < ArraySize; ++i)
        {
            Result.Append(ObjectArray[i]);
        }

        return std::move(Result);
    }

private:
    /// @brief Allocates memory for the array.
    /// @param Size const size_t : Number of elements in the array
    void AllocArray(const size_t Size)
    {
        if (ObjectArray == nullptr)
        {
#ifndef CSP_DISABLE_OVERFLOW_CHECKING
#ifdef _MSC_VER // MSVC
            auto HighBits = __umulh(sizeof(T), Size);
#else // GCC or Clang
            auto MultiplyResult = static_cast<__uint128_t>(sizeof(T)) * static_cast<__uint128_t>(Size);
            auto HighBits = static_cast<size_t>(MultiplyResult >> static_cast<__uint128_t>(64));
#endif

            if (HighBits > 0)
            {
                throw std::overflow_error("Size");
            }
#endif

            auto BufferSize = sizeof(T) * Size;
            ObjectArray = (T*)csp::memory::DllAlloc(BufferSize);

            for (size_t i = 0; i < Size; ++i)
            {
                T* ObjectPtr = &ObjectArray[i];
                new (ObjectPtr) T;
            }

            ArraySize = Size;
        }
    }

    /// @brief Frees memory for the array.
    void FreeArray()
    {
        if (ObjectArray != nullptr)
        {
            for (size_t i = 0; i < ArraySize; ++i)
            {
                T* ObjectPtr = &ObjectArray[i];
                ObjectPtr->~T();
            }

            csp::memory::DllFree(ObjectArray);
            ObjectArray = nullptr;
        }
    }

    size_t ArraySize;
    T* ObjectArray;
};

} // namespace csp::common
