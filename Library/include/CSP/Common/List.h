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
    /// @brief Constructs a list with 0 elements.
    List()
        : CurrentSize(0)
        , MaximumSize(0)
        , ObjectArray(nullptr)
    {
        AllocList(LIST_DEFAULT_SIZE);
    }

    /// @brief Constructs a list with the given number of elements.
    /// @param Size size_t : Number of elements in the array
    List(size_t MinimumSize)
        : CurrentSize(0)
        , MaximumSize(0)
        , ObjectArray(nullptr)
    {
        auto Size = next_pow2(MinimumSize);
        AllocList(Size);
    }

    /// @brief Copy constructor.
    /// @param Other const List<T>&
    List(const List<T>& Other)
        : CurrentSize(0)
        , MaximumSize(0)
        , ObjectArray(nullptr)
    {
        if (Other.CurrentSize == 0)
        {
            AllocList(LIST_DEFAULT_SIZE);

            return;
        }

        AllocList(Other.MaximumSize);
        CurrentSize = Other.CurrentSize;

        for (size_t i = 0; i < CurrentSize; ++i)
        {
            T* ObjectPtr = &ObjectArray[i];
            new (ObjectPtr) T;
            ObjectArray[i] = Other.ObjectArray[i];
        }
    }

    CSP_NO_EXPORT List(List<T>&& Other)
        : CurrentSize(0)
        , MaximumSize(0)
        , ObjectArray(nullptr)
    {
        if (Other.CurrentSize == 0)
        {
            AllocList(LIST_DEFAULT_SIZE);

            return;
        }

        CurrentSize = Other.CurrentSize;
        MaximumSize = Other.MaximumSize;
        ObjectArray = Other.ObjectArray;

        Other.ObjectArray = nullptr;
    }

    /// @brief Constructs a list from an initializer_list.
    /// @param List std::initializer_list : Elements to construct the list from
    CSP_NO_EXPORT List(std::initializer_list<T> List)
        : CurrentSize(0)
        , MaximumSize(0)
        , ObjectArray(nullptr)
    {
        if (List.size() == 0)
        {
            AllocList(LIST_DEFAULT_SIZE);

            return;
        }

        auto Size = next_pow2(List.size());
        AllocList(Size);
        CurrentSize = List.size();

        for (size_t i = 0; i < CurrentSize; ++i)
        {
            T* ObjectPtr = &ObjectArray[i];
            new (ObjectPtr) T;
            ObjectArray[i] = *(List.begin() + i);
        }
    }

    /// @brief Destructor.
    /// Frees list memory.
    ~List() { FreeList(); }

    /// @brief Returns a pointer to the start of the list.
    /// @return T*
    CSP_NO_EXPORT T* Data() { return CurrentSize > 0 ? &ObjectArray[0] : nullptr; }

    /// @brief Returns a const pointer to the start of the list.
    /// @return const T*
    CSP_NO_EXPORT const T* Data() const { return CurrentSize > 0 ? &ObjectArray[0] : nullptr; }

    // Iterators
    CSP_NO_EXPORT T* begin() { return Data(); }
    CSP_NO_EXPORT const T* begin() const { return Data(); }
    CSP_NO_EXPORT const T* cbegin() const { return Data(); }

    CSP_NO_EXPORT T* end() { return Data() + Size(); }
    CSP_NO_EXPORT const T* end() const { return Data() + Size(); }
    CSP_NO_EXPORT const T* cend() const { return Data() + Size(); }

    /// @brief Copy assignment.
    /// @param Other const List<T>&
    /// @return List<T>&
    List<T>& operator=(const List<T>& Other)
    {
        if (this == &Other)
        {
            return *this;
        }

        CurrentSize = Other.CurrentSize;
        MaximumSize = 0;
        ObjectArray = nullptr;

        if (CurrentSize == 0)
        {
            AllocList(LIST_DEFAULT_SIZE);

            return *this;
        }

        AllocList(Other.MaximumSize);

        for (size_t i = 0; i < CurrentSize; i++)
        {
            ObjectArray[i] = Other.ObjectArray[i];
        }

        return *this;
    }

    /// @brief Returns an element at the given index of the list.
    /// @param Index size_t : Element index to access
    /// @return T& : List element
    T& operator[](size_t Index)
    {
        assert(Index < CurrentSize);

        return ObjectArray[Index];
    }

    /// @brief Returns a const element at the given index of the list.
    /// @param Index size_t : Element index to access
    /// @return const T& : List element
    const T& operator[](size_t Index) const
    {
        assert(Index < CurrentSize);

        return ObjectArray[Index];
    }

    /// @brief Appends an element to the end of the list.
    /// @param Item const T&
    void Append(const T& Item)
    {
        if (CurrentSize == MaximumSize)
        {
            auto Size = next_pow2(MaximumSize + 1);
            ReallocList(Size);
        }

        // Instantiate element first to allow copy assignment
        T* ObjectPtr = &ObjectArray[CurrentSize];
        new (ObjectPtr) T;
        ObjectArray[CurrentSize++] = Item;
    }

    /// @brief Appends an element to the end of the list.
    /// @param Item T&&
    CSP_NO_EXPORT void Append(T&& Item)
    {
        if (CurrentSize == MaximumSize)
        {
            auto Size = next_pow2(MaximumSize + 1);
            ReallocList(Size);
        }

        // Instantiate element first to allow move assignment
        T* ObjectPtr = &ObjectArray[CurrentSize];
        new (ObjectPtr) T;
        ObjectArray[CurrentSize++] = std::move(Item);
    }

    /// @brief Appends an element at the given index of the list.
    /// @param Index size_t
    /// @param Item const T&
    void Insert(size_t Index, const T& Item)
    {
        if (CurrentSize == MaximumSize)
        {
            auto Size = next_pow2(MaximumSize + 1);
            ReallocList(Size);
        }

        auto After = CurrentSize - Index;
        std::memmove(ObjectArray + (Index + 1), ObjectArray + Index, sizeof(T) * After);
        ++CurrentSize;

        T* ObjectPtr = &ObjectArray[0];
        new (ObjectPtr) T;
        ObjectArray[Index] = Item;
    }

    /// @brief Removes an element to a specific index of the list.
    /// @param Index size_t
    void Remove(size_t Index)
    {
        assert(Index < CurrentSize);

        if constexpr (std::is_pointer<T>::value)
        {
            // I dont think the list owns any pointers that are in it. That seems weird, because it _will_ destruct value types
            // To maintain exactly the same behaviour, do the destruct behaviour for pointers. Use regular RAII cleanup for value types below during
            // the shift.

            // delete ObjectArray[Index]; <-- What we should be doing ... (although actually, we should just have this type backed by std::list)
            T* ObjectPtr = &ObjectArray[Index];
            ObjectPtr->~T();
        }

        // Shift everything left
        for (size_t i = Index; i < CurrentSize - 1; ++i)
        {
            ObjectArray[i] = ObjectArray[i + 1];
        }
        --CurrentSize;
    }

    /// @brief Removes the given element from the list.
    /// @param Item const T& : Element to remove from the list
    void RemoveItem(const T& Item)
    {
        for (size_t i = 0; i < CurrentSize; ++i)
        {
            if (ObjectArray[i] == Item)
            {
                Remove(i);
                return;
            }
        }
    }

    /// @brief Returns the number of elements in the array.
    /// @return const size_t
    const size_t Size() const { return CurrentSize; }

    /// @brief Removes all elements in the list.
    void Clear()
    {
        FreeList();
        AllocList(LIST_DEFAULT_SIZE);
    }

    /// @brief Checks if the list contains the given element.
    /// @param Item const T& : Element to check if the list contains
    /// @return bool
    bool Contains(const T& Item) const
    {
        for (size_t i = 0; i < CurrentSize; ++i)
        {
            if (ObjectArray[i] == Item)
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
        Array<T> Result(CurrentSize);

        for (size_t i = 0; i < CurrentSize; ++i)
        {
            Result[i] = ObjectArray[i];
        }

        return std::move(Result);
    }

private:
    /// @brief Allocates memory for the list.
    /// @param Size size_t : Number of elements in the list
    void AllocList(const size_t Size)
    {
        ObjectArray = new T[Size];
        MaximumSize = Size;
    }

    void ReallocList(const size_t Size)
    {
        T* NewArray = new T[Size];
        if (ObjectArray)
        {
            std::copy(ObjectArray, ObjectArray + std::min(MaximumSize, Size), NewArray);
            delete[] ObjectArray;
        }
        ObjectArray = NewArray;
        MaximumSize = Size;
    }

    /// @brief Frees memory for the list.
    void FreeList()
    {
        delete[] ObjectArray;
        ObjectArray = nullptr;
        CurrentSize = 0;
        MaximumSize = 0;
    }

    size_t CurrentSize;
    size_t MaximumSize;
    T* ObjectArray;
};

} // namespace csp::common
