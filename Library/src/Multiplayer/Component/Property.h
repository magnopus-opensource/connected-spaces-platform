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

#include "CSP/Common/ReplicatedValue.h"

#include "Multiplayer/MCS/MCSTypes.h"

namespace csp::multiplayer::component
{

/// @brief Represents an individual data field, or "property", within a component schema,
/// consisting of a stable ID/key, a type kind and default value (via ReplicatedValue),
/// and other metadata.
///
/// @tparam KeyType: The type of the stable ID value used for this property's unique key.
template <typename KeyType> struct Property final
{
    static_assert(mcs::IsPackableIdV<KeyType, mcs::PropertyKeyType>);

    KeyType Key;
    csp::common::ReplicatedValue DefaultValue;
};

} // namespace csp::multiplayer::component
