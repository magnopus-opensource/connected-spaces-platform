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

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/ReplicatedValue.h"

namespace csp::multiplayer
{

/// @brief Represents an individual data field, or "property", within a component schema,
/// consisting of a stable ID/key, a type kind and default value (via ReplicatedValue),
/// and other metadata.
class CSP_API ComponentProperty
{
public:
    using KeyType = uint16_t;

    KeyType Key;
    csp::common::ReplicatedValue DefaultValue;
};

} // namespace csp::multiplayer
