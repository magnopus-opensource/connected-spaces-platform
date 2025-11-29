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
#include <initializer_list>
#include <vector>

namespace csp::common
{

CSP_START_IGNORE
template <typename T> class Array;
CSP_END_IGNORE

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
    List() { }

    /// @brief Constructs a list with the given number of elements.
    /// @param Size size_t : Number of elements in the array
    List(size_t MinimumSize) { Storage.reserve(MinimumSize); }

    /// @brief Copy constructor.
    /// @param Other const List<T>&
    List(const List<T>& Other)
        : Storage(Other.Storage)
    {
    }

    CSP_NO_EXPORT List(List<T>&& Other)
        : Storage(std::move(Other.Storage))
    {
        Other.Storage.clear();
    }

    /// @brief Constructs a list from an initializer_list.
    /// @param List std::initializer_list : Elements to construct the list from
    CSP_NO_EXPORT List(std::initializer_list<T> List)
        : Storage(std::move(List))
    {
    }

    /// @brief Returns a pointer to the start of the list.
    /// @return T*
    CSP_NO_EXPORT T* Data() { return Storage.size() > 0 ? Storage.data() : nullptr; }

    /// @brief Returns a const pointer to the start of the list.
    /// @return const T*
    CSP_NO_EXPORT const T* Data() const { return Storage.size() > 0 ? Storage.data() : nullptr; }

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
        Storage = Other.Storage;
        return *this;
    }

    /// @brief Returns an element at the given index of the list.
    /// @param Index size_t : Element index to access
    /// @return T& : List element
    T& operator[](size_t Index)
    {
        assert(Index < Storage.size());

        return Storage[Index];
    }

    /// @brief Returns a const element at the given index of the list.
    /// @param Index size_t : Element index to access
    /// @return const T& : List element
    const T& operator[](size_t Index) const
    {
        assert(Index < Storage.size());

        return Storage[Index];
    }

    /// @brief Appends an element to the end of the list.
    /// @param Item const T&
    void Append(const T& Item) { Storage.push_back(Item); }

    /// @brief Appends an element to the end of the list.
    /// @param Item T&&
    CSP_NO_EXPORT void Append(T&& Item) { Storage.push_back(std::forward<T>(Item)); }

    /// @brief Appends an element at the given index of the list.
    /// @param Index size_t
    /// @param Item const T&
    void Insert(size_t Index, const T& Item) { Storage.insert(Storage.begin() + Index, Item); }

    /// @brief Removes an element to a specific index of the list.
    /// @param Index size_t
    void Remove(size_t Index) { Storage.erase(Storage.begin() + Index); }

    /// @brief Removes the given element from the list.
    /// @param Item const T& : Element to remove from the list
    void RemoveItem(const T& Item)
    {
        // Shift all the elements to remove to the back of the list
        auto NewEnd = std::remove(Storage.begin(), Storage.end(), Item);

        // Erase them
        Storage.erase(NewEnd, Storage.end());
    }

    /// @brief Returns the number of elements in the array.
    /// @return const size_t
    const size_t Size() const { return Storage.size(); }

    /// @brief Removes all elements in the list.
    void Clear() { Storage.clear(); }

    /// @brief Checks if the list contains the given element.
    /// @param Item const T& : Element to check if the list contains
    /// @return bool
    bool Contains(const T& Item) const { return std::find(Storage.cbegin(), Storage.cend(), Item) != Storage.cend(); }

    /// @brief Returns a copy of this List as an Array
    /// @return Array<T>
    CSP_NO_EXPORT Array<T> ToArray() const
    {
        Array<T> Result(Storage.size());

        for (size_t i = 0; i < Storage.size(); ++i)
        {
            Result[i] = Storage[i];
        }

        return std::move(Result);
    }

private:
    std::vector<T> Storage;
};

} // namespace csp::common
