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
#include "CSP/Common/Array.h"
#include "CSP/Memory/DllAllocator.h"

#include <map>

namespace csp::common
{

/// @brief Simple DLL-safe map of key object pairs.
///
/// Simple map type used to pass maps of key object pairs across the DLL boundary.
///
/// @tparam TKey : Type to use as the key
/// @tparam TValue : Type to use as the value
template <typename TKey, typename TValue> class CSP_API Map
{
    using MapType = std::map<TKey, TValue>;

public:
    /// @brief Constructs a map with 0 elements.
    Map()
    {
        Container = (MapType*)csp::memory::DllAlloc(sizeof(MapType));
        new (Container) MapType;
    }

    /// @brief Copy constructor.
    /// @param Other const Map<TKey, TValue>&
    Map(const Map<TKey, TValue>& Other)
    {
        Container = (MapType*)csp::memory::DllAlloc(sizeof(MapType));
        new (Container) MapType(*Other.Container);
    }

    /// @brief Move constructor.
    /// @param Other Map<TKey, TValue>&&
    CSP_NO_EXPORT Map(Map<TKey, TValue>&& Other)
    {
        Container = (MapType*)csp::memory::DllAlloc(sizeof(MapType));
        new (Container) MapType(*Other.Container);
    }

    /// @brief Constructs a map from a `std::initializer_list`.
    /// @param Values const std::initializer_list<std::pair<const TKey, const TValue>> : Elements to construct the map from
    CSP_NO_EXPORT Map(const std::initializer_list<std::pair<const TKey, const TValue>> Values)
    {
        Container = (MapType*)csp::memory::DllAlloc(sizeof(MapType));
        new (Container) MapType;

        for (const auto& Pair : Values)
        {
            Container->emplace(Pair.first, Pair.second);
        }
    }

    /// @brief Destructor.
    /// Frees map memory.
    ~Map()
    {
        Container->~MapType();
        csp::memory::DllFree(Container);
    }

    /// @brief Returns a reference to the element with the given key in this map.
    ///        Will create a new element if the given key is not present.
    /// @param Key const TKey& : Key of element in this map
    /// @return TValue& : Map element
    TValue& operator[](const TKey& Key) { return Container->operator[](Key); }

    /// @brief Returns a const reference to the element with the given key in this map.
    ///        Throws if the given key is not present.
    /// @param Key const TKey& : Key of element in this map
    /// @return const TValue& : Map element
    const TValue& operator[](const TKey& Key) const
    {
        if (Container->count(Key) == 0)
        {
            throw std::runtime_error("Key not present in Map. Please ensure an element with the given key exists before attempting to access it.");
        }

        return Container->operator[](Key);
    }

    /// @brief Copy assignment.
    /// @param Other const Map<TKey, TValue>&
    /// @return Map<TKey, TValue>&
    CSP_NO_EXPORT Map<TKey, TValue>& operator=(const Map<TKey, TValue>& Other)
    {
        if (this == &Other)
        {
            return *this;
        }

        if (Container)
        {
            Container->~MapType();
        }
        else
        {
            Container = (MapType*)csp::memory::DllAlloc(sizeof(MapType));
        }

        new (Container) MapType(*Other.Container);

        return *this;
    }

    /// @brief Move assignment.
    /// @param Other Map<TKey, TValue>&&
    /// @return Map<TKey, TValue>&
    CSP_NO_EXPORT Map<TKey, TValue>& operator=(Map<TKey, TValue>&& Other)
    {
        if (this == &Other)
        {
            return *this;
        }

        if (Container)
        {
            Container->~MapType();
        }
        else
        {
            Container = (MapType*)csp::memory::DllAlloc(sizeof(MapType));
        }

        new (Container) MapType(*Other.Container);

        return *this;
    }

    CSP_NO_EXPORT bool operator==(const Map<TKey, TValue>& OtherValue) const { return *Container == *OtherValue.Container; }

    CSP_NO_EXPORT bool operator!=(const Map<TKey, TValue>& OtherValue) const { return *Container != *OtherValue.Container; }

    /// @brief Returns the number of elements in this map.
    /// @return size_t
    size_t Size() const { return Container->size(); }

    /// @brief Returns `true` if this map contains an element with the given key.
    /// @param Key const TKey& : key to check if the map contains
    /// @return bool
    bool HasKey(const TKey& Key) const { return Container->count(Key) > 0; }

    /// @brief Returns a copy of all keys in this map.
    ///        This copy should be disposed by the caller once it is no longer needed.
    /// @return const csp::common::Array<TKey>* : Array of keys
    const Array<TKey>* Keys() const
    {
        auto Keys = (Array<TKey>*)csp::memory::DllAlloc(sizeof(Array<TKey>));
        new (Keys) Array<TKey>(Container->size());
        int i = 0;

        for (const auto& Pair : *Container)
        {
            Keys->operator[](i++) = Pair.first;
        }

        return Keys;
    }

    /// @brief Returns all values in this map.
    /// @return const csp::common::Array<TValue>* : Array of values
    const Array<TValue>* Values() const
    {
        auto Values = (Array<TValue>*)csp::memory::DllAlloc(sizeof(Array<TValue>));
        new (Values) Array<TValue>(Container->size());
        int i = 0;

        for (const auto& Pair : *Container)
        {
            Values->operator[](i++) = Pair.second;
        }

        return Values;
    }

    /// @brief Removes the element with the given key from this map.
    /// @param Key const TKey& : Key to remove from the map
    void Remove(const TKey& Key)
    {
        if (HasKey(Key))
        {
            Container->erase(Key);
        }
    }

    /// @brief Removes all elements in this map.
    void Clear() { Container->clear(); }

private:
    MapType* Container;
};

} // namespace csp::common
