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
#include "CSP/Common/List.h"
#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Common/String.h"

namespace csp::multiplayer
{

/// @brief The data type of a script attribute as declared in the script's schema.
enum class ScriptAttributeType
{
    Invalid = 0,
    Boolean,
    Integer,
    Float,
    String,
    EntityQuery,
    Num
};

/// @brief High-level description of a single CodeComponent attribute, combining schema
/// metadata (type constraints, ranges) with the current replicated value.
///
/// Instances are produced by CodeSpaceComponent::GetScriptAttributes(), which merges the
/// raw $schema JSON with the live attribute values.
class CSP_API NgxScriptAttribute
{
public:
    NgxScriptAttribute();

    /// The attribute key name from the schema.
    csp::common::String Name;

    /// The declared type of this attribute.
    ScriptAttributeType Type;

    /// Optional numeric constraints. When not specified by the schema these
    /// default to 0 and HasMin / HasMax / HasStep will be false.
    float Min;
    float Max;
    float Step;

    bool HasMin;
    bool HasMax;
    bool HasStep;

    /// Whether the schema marks this attribute as required.
    bool Required;

    /// The current replicated value for this attribute.
    csp::common::ReplicatedValue Value;

    bool operator==(const NgxScriptAttribute& Other) const;
    bool operator!=(const NgxScriptAttribute& Other) const;
};

} // namespace csp::multiplayer
