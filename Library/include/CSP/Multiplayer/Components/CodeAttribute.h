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
    Vector2 = 5,
    Vector3 = 6,
    Vector4 = 7,
    Quaternion = 8,
    Color = 9,
    EntityQuery = 10,
    ModelAsset = 11,
    ImageAsset = 12,
    Num
};

class CSP_API CodeAttribute
{
public:
    using EntityQueryValueType = csp::common::Map<csp::common::String, csp::common::ReplicatedValue>;
    using ModelAssetValueType = csp::common::Map<csp::common::String, csp::common::ReplicatedValue>;
    using ImageAssetValueType = csp::common::Map<csp::common::String, csp::common::ReplicatedValue>;

    CodeAttribute();

    static CodeAttribute FromBoolean(bool Value);
    static CodeAttribute FromInteger(int64_t Value);
    static CodeAttribute FromFloat(float Value);
    static CodeAttribute FromString(const csp::common::String& Value);
    static CodeAttribute FromVector2(const csp::common::Vector2& Value);
    static CodeAttribute FromVector3(const csp::common::Vector3& Value);
    static CodeAttribute FromVector4(const csp::common::Vector4& Value);
    static CodeAttribute FromQuaternion(const csp::common::Vector4& Value);
    static CodeAttribute FromColor(const csp::common::Vector3& Value);
    static CodeAttribute FromEntityQuery(const EntityQueryValueType& Value);
    static CodeAttribute FromModelAsset(const ModelAssetValueType& Value);
    static CodeAttribute FromImageAsset(const ImageAssetValueType& Value);

    const csp::common::Vector2& GetVector2Value() const;
    void SetVector2Value(const csp::common::Vector2& Value);

    const csp::common::Vector3& GetVector3Value() const;
    void SetVector3Value(const csp::common::Vector3& Value);

    const csp::common::Vector4& GetVector4Value() const;
    void SetVector4Value(const csp::common::Vector4& Value);

    const csp::common::Vector4& GetQuaternionValue() const;
    void SetQuaternionValue(const csp::common::Vector4& Value);

    const csp::common::Vector3& GetColorValue() const;
    void SetColorValue(const csp::common::Vector3& Value);

    static bool IsValidEntityQueryValue(const EntityQueryValueType& Value);
    const EntityQueryValueType& GetEntityQueryValue() const;
    void SetEntityQueryValue(const EntityQueryValueType& Value);

    static bool IsValidModelAssetValue(const ModelAssetValueType& Value);
    const ModelAssetValueType& GetModelAssetValue() const;
    void SetModelAssetValue(const ModelAssetValueType& Value);

    static bool IsValidImageAssetValue(const ImageAssetValueType& Value);
    const ImageAssetValueType& GetImageAssetValue() const;
    void SetImageAssetValue(const ImageAssetValueType& Value);

    csp::common::ReplicatedValue ToReplicatedValue() const;
    static bool TryFromReplicatedValue(const csp::common::ReplicatedValue& InValue, CodeAttribute& OutAttribute);

    bool operator==(const CodeAttribute& Other) const;
    bool operator!=(const CodeAttribute& Other) const;

    CodePropertyType Type;
    bool BooleanValue;
    int64_t IntegerValue;
    float FloatValue;
    csp::common::String StringValue;
    csp::common::Vector2 Vector2Value;
    csp::common::Vector3 Vector3Value;
    csp::common::Vector4 Vector4Value;
    csp::common::Vector4 QuaternionValue;
    csp::common::Vector3 ColorValue;
    EntityQueryValueType EntityQueryValue;
    ModelAssetValueType ModelAssetValue;
    ImageAssetValueType ImageAssetValue;
};

} // namespace csp::multiplayer

