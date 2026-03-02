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
#include "CSP/Common/Map.h"
#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Common/String.h"

namespace csp::multiplayer
{

enum class CodePropertyType
{
    Invalid = 0,
    Boolean = 1,
    Integer = 2,
    Float = 3,
    String = 4,
    EntityQuery = 5,
    Num
};

class CSP_API CodeAttribute
{
public:
    using EntityQueryValueType = csp::common::Map<csp::common::String, csp::common::ReplicatedValue>;

    CodeAttribute();

    static CodeAttribute FromBoolean(bool Value);
    static CodeAttribute FromInteger(int64_t Value);
    static CodeAttribute FromFloat(float Value);
    static CodeAttribute FromString(const csp::common::String& Value);
    static CodeAttribute FromEntityQuery(const EntityQueryValueType& Value);

    static bool IsValidEntityQueryValue(const EntityQueryValueType& Value);
    const EntityQueryValueType& GetEntityQueryValue() const;
    void SetEntityQueryValue(const EntityQueryValueType& Value);

    csp::common::ReplicatedValue ToReplicatedValue() const;
    static bool TryFromReplicatedValue(const csp::common::ReplicatedValue& InValue, CodeAttribute& OutAttribute);

    bool operator==(const CodeAttribute& Other) const;
    bool operator!=(const CodeAttribute& Other) const;

    CodePropertyType Type;
    bool BooleanValue;
    int64_t IntegerValue;
    float FloatValue;
    csp::common::String StringValue;
    EntityQueryValueType EntityQueryValue;
};

} // namespace csp::multiplayer

