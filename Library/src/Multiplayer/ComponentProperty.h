/*
 * Copyright 2026 Magnopus LLC

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

#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace csp::multiplayer
{

/// @brief An unconstrained property value, carrying only its default.
template <typename T> struct PlainValue final
{
    /// @brief The default value of this property, used to initialise it on a newly created component.
    T Default;

    bool operator==(const PlainValue& Other) const;
    bool operator!=(const PlainValue& Other) const;
};

template <typename T> bool PlainValue<T>::operator==(const PlainValue& Other) const { return Default == Other.Default; }

template <typename T> bool PlainValue<T>::operator!=(const PlainValue& Other) const { return !(*this == Other); }

/// @brief An inclusive range of values, constraining what a bounded property may hold.
template <typename T> struct Range final
{
    T Min;
    T Max;

    bool operator==(const Range& Other) const;
    bool operator!=(const Range& Other) const;
};

template <typename T> bool Range<T>::operator==(const Range& Other) const { return Min == Other.Min && Max == Other.Max; }

template <typename T> bool Range<T>::operator!=(const Range& Other) const { return !(*this == Other); }

/// @brief A property value constrained to an inclusive range.
template <typename T> struct BoundedValue final
{
    /// @brief The default value of this property, used to initialise it on a newly created component.
    T Default;

    /// @brief The range this property's value is constrained to.
    csp::multiplayer::Range<T> Range;

    bool operator==(const BoundedValue& Other) const;
    bool operator!=(const BoundedValue& Other) const;
};

template <typename T> bool BoundedValue<T>::operator==(const BoundedValue& Other) const { return Default == Other.Default && Range == Other.Range; }

template <typename T> bool BoundedValue<T>::operator!=(const BoundedValue& Other) const { return !(*this == Other); }

/// @brief A named value within the set an enumerated property is constrained to.
template <typename T> struct SchemaOption final
{
    /// @brief A human-readable name for this option, for clients presenting a choice of values.
    /// Carries no meaning at runtime; the value is what is replicated.
    csp::common::String Name;

    T Value;

    bool operator==(const SchemaOption& Other) const;
    bool operator!=(const SchemaOption& Other) const;
};

template <typename T> bool SchemaOption<T>::operator==(const SchemaOption& Other) const { return Name == Other.Name && Value == Other.Value; }

template <typename T> bool SchemaOption<T>::operator!=(const SchemaOption& Other) const { return !(*this == Other); }

/// @brief A property value constrained to a fixed set of named options.
template <typename T> struct EnumeratedValue final
{
    /// @brief The default value of this property, used to initialise it on a newly created component.
    T Default;

    /// @brief The set of values this property is constrained to.
    std::vector<SchemaOption<T>> Options;

    bool operator==(const EnumeratedValue& Other) const;
    bool operator!=(const EnumeratedValue& Other) const;
};

template <typename T> bool EnumeratedValue<T>::operator==(const EnumeratedValue& Other) const
{
    return Default == Other.Default && Options == Other.Options;
}

template <typename T> bool EnumeratedValue<T>::operator!=(const EnumeratedValue& Other) const { return !(*this == Other); }

/// @brief A property value as a schema declares it. This gives the type of the value, the default it
/// is initialised with, and the constraint it is subject to, which may be nothing, an inclusive range,
/// or a set of named options. The set of declarations is closed, so only valid combinations of type,
/// default and constraint can be expressed.
using PropertyValue
    = std::variant<PlainValue<bool>, PlainValue<int64_t>, PlainValue<float>, PlainValue<std::string>, PlainValue<csp::common::Vector2>,
        PlainValue<csp::common::Vector3>, PlainValue<csp::common::Vector4>, PlainValue<std::unordered_map<std::string, std::string>>,
        BoundedValue<int64_t>, BoundedValue<float>, EnumeratedValue<int64_t>, EnumeratedValue<float>, EnumeratedValue<std::string>>;

/// @brief Represents an individual data field, or "property", within a component schema,
/// consisting of a stable ID/key, a name, a typed value, and other metadata.
struct ComponentProperty final
{
    using KeyType = uint16_t;

    /// @brief An Id/Key for identifying this property within a component. Need only be unique
    /// within the scope of the component (which is historically the case), but could also be
    /// globally unique. Will ultimately be serialized and used in messages sent over the
    /// multiplayer connection.
    KeyType Key;

    /// @brief A human-readable name describing this propery (in `camelCase`).
    /// Must be unique within the component (two properties should not have the same name).
    /// This name will be used for generating script bindings i.e. a property with this name will
    /// be exposed on the component in scripts.
    csp::common::String Name;

    /// @brief The value of this property. This also jointly declares the type of the property,
    /// which is considered static i.e. if the value is a `float`, it can't be later changed to hold
    /// a `std::string`.
    PropertyValue Value;

    bool operator==(const ComponentProperty& Other) const;
    bool operator!=(const ComponentProperty& Other) const;
};

} // namespace csp::multiplayer
