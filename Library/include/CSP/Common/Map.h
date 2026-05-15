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
public:
    using MapType = std::map<TKey, TValue>;

    /// @brief Constructs a map with 0 elements.
    Map() { m_container = new MapType(); }

    /// @brief Copy constructor.
    /// @param Other const Map<TKey, TValue>&
    Map(const Map<TKey, TValue>& other) { m_container = new MapType(*other.m_container); }

    /// @brief Move constructor.
    /// @param Other Map<TKey, TValue>&&
    CSP_NO_EXPORT Map(Map<TKey, TValue>&& other) { m_container = new MapType(std::move(*other.m_container)); }

    /// @brief Constructs a map from a `std::initializer_list`.
    /// @param Values const std::initializer_list<std::pair<const TKey, const TValue>> : Elements to construct the map from
    CSP_NO_EXPORT Map(const std::initializer_list<std::pair<const TKey, const TValue>> values)
    {
        m_container = new MapType();

        for (const auto& pair : values)
        {
            m_container->emplace(pair.first, pair.second);
        }
    }

    /// @brief Destructor.
    /// Frees map memory.
    ~Map() { delete m_container; }

    /// @brief Returns a reference to the element with the given key in this map.
    ///        Will create a new element if the given key is not present.
    /// @param Key const TKey& : Key of element in this map
    /// @return TValue& : Map element
    TValue& operator[](const TKey& key) { return m_container->operator[](key); }

    /// @brief Returns a const reference to the element with the given key in this map.
    ///        Throws if the given key is not present.
    /// @param Key const TKey& : Key of element in this map
    /// @return const TValue& : Map element
    const TValue& operator[](const TKey& key) const
    {
        if (m_container->count(key) == 0)
        {
            throw std::runtime_error("Key not present in Map. Please ensure an element with the given key exists before attempting to access it.");
        }

        return m_container->operator[](key);
    }

    /// @brief Copy assignment.
    /// @param Other const Map<TKey, TValue>&
    /// @return Map<TKey, TValue>&
    CSP_NO_EXPORT Map<TKey, TValue>& operator=(const Map<TKey, TValue>& other)
    {
        if (this == &other)
            return *this;

        delete m_container;
        m_container = new MapType(*other.m_container); // Copy construct
        return *this;
    }

    /// @brief Move assignment.
    /// @param Other Map<TKey, TValue>&&
    /// @return Map<TKey, TValue>&
    CSP_NO_EXPORT Map<TKey, TValue>& operator=(Map<TKey, TValue>&& other)
    {
        if (this == &other)
            return *this;

        delete m_container;
        m_container = new MapType(std::move(*other.m_container)); // Move construct
        return *this;
    }

    /// @brief Whether two maps are equal to each other. Defers to std::maps equality operator
    /// @param OtherValue const Map<TKey, TValue>& : Map to compare
    bool operator==(const Map<TKey, TValue>& otherValue) const { return *m_container == *otherValue.m_container; }

    /// @brief Whether two maps are inequal to each other. Defers to std::maps equality operator
    /// @param OtherValue const Map<TKey, TValue>& : Map to compare
    bool operator!=(const Map<TKey, TValue>& otherValue) const { return *m_container != *otherValue.m_container; }

    /// @brief Returns the number of elements in this map.
    /// @return size_t
    size_t Size() const { return m_container->size(); }

    /// @brief Returns `true` if this map contains an element with the given key.
    /// @param Key const TKey& : key to check if the map contains
    /// @return bool
    bool HasKey(const TKey& key) const { return m_container->count(key) > 0; }

    // Iterators
    CSP_NO_EXPORT typename MapType::iterator begin() { return m_container->begin(); }
    CSP_NO_EXPORT typename MapType::const_iterator begin() const { return m_container->begin(); }
    CSP_NO_EXPORT typename MapType::const_iterator cbegin() const { return m_container->cbegin(); }

    CSP_NO_EXPORT typename MapType::iterator end() { return m_container->end(); }
    CSP_NO_EXPORT typename MapType::const_iterator end() const { return m_container->end(); }
    CSP_NO_EXPORT typename MapType::const_iterator cend() const { return m_container->cend(); }

    // These finds are more efficient than using std::find over the iterators (non-linear container)
    CSP_NO_EXPORT typename MapType::iterator Find(const TKey& key) { return m_container->find(key); }
    CSP_NO_EXPORT typename MapType::const_iterator Find(const TKey& key) const { return m_container->find(key); }

    /// @brief Returns a copy of all keys in this map.
    ///        This copy should be disposed by the caller once it is no longer needed.
    /// @return const csp::common::Array<TKey>* : Array of keys
    const Array<TKey>* Keys() const
    {
        auto keys = new Array<TKey>(m_container->size());
        int i = 0;

        for (const auto& pair : *m_container)
        {
            keys->operator[](i++) = pair.first;
        }

        return keys;
    }

    /// @brief Returns all values in this map.
    /// @return const csp::common::Array<TValue>* : Array of values
    const Array<TValue>* Values() const
    {
        auto values = new Array<TValue>(m_container->size());
        int i = 0;

        for (const auto& pair : *m_container)
        {
            values->operator[](i++) = pair.second;
        }

        return values;
    }

    /// @brief Removes the element with the given key from this map.
    /// @param Key const TKey& : Key to remove from the map
    void Remove(const TKey& key)
    {
        if (HasKey(key))
        {
            m_container->erase(key);
        }
    }

    /// @brief Removes all elements in this map.
    void Clear() { m_container->clear(); }

    // The fact that we can do this makes having this type a bit silly ... no actual ABI protection ... may as well just be a std::map.
    CSP_NO_EXPORT const MapType& GetUnderlying() const { return *m_container; }
    CSP_NO_EXPORT MapType& GetUnderlying() { return *m_container; }

private:
    MapType* m_container;
};

} // namespace csp::common
