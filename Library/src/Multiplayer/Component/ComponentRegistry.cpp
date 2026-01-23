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

#include "CSP/Multiplayer/Component/ComponentRegistry.h"
#include "CSP/Common/Map.h"
#include "Json/JsonSerializer.h"

namespace csp::multiplayer
{

static constexpr const char* BooleanType = "bool";
static constexpr const char* Int64Type = "int";
static constexpr const char* FloatType = "float";
static constexpr const char* StringType = "string";
static constexpr const char* Vec2Type = "vec2";
static constexpr const char* Vec3Type = "vec3";
static constexpr const char* Vec4Type = "vec4";
static constexpr const char* StringMapType = "stringMap";

namespace
{
    template <class T> T DeserializeTypeFromTypeString(const csp::json::JsonDeserializer& Deserializer)
    {
        T Val;
        Deserializer.SafeDeserializeMember("value", Val);
        return Val;
    }

    csp::common::ReplicatedValue DeserializeReplicatedValueFromTypeString(
        const csp::json::JsonDeserializer& Deserializer, const csp::common::String& Type)
    {
        if (Type == BooleanType)
        {
            return DeserializeTypeFromTypeString<bool>(Deserializer);
        }
        else if (Type == Int64Type)
        {
            return DeserializeTypeFromTypeString<int64_t>(Deserializer);
        }
        else if (Type == FloatType)
        {
            return DeserializeTypeFromTypeString<float>(Deserializer);
        }
        else if (Type == StringType)
        {
            return DeserializeTypeFromTypeString<csp::common::String>(Deserializer);
        }
        else if (Type == Vec2Type)
        {
            const auto Array = DeserializeTypeFromTypeString<std::vector<float>>(Deserializer);

            if (Array.size() != 2)
            {
                // todo throw
                return csp::common::ReplicatedValue {};
            }

            return csp::common::ReplicatedValue(csp::common::Vector2 { Array[0], Array[1] });
        }
        else if (Type == Vec3Type)
        {
            const auto Array = DeserializeTypeFromTypeString<std::vector<float>>(Deserializer);

            if (Array.size() != 3)
            {
                // todo throw
                return csp::common::ReplicatedValue {};
            }

            return csp::common::ReplicatedValue(csp::common::Vector3 { Array[0], Array[1], Array[2] });
        }
        else if (Type == Vec4Type)
        {
            const auto Array = DeserializeTypeFromTypeString<std::vector<float>>(Deserializer);

            if (Array.size() != 4)
            {
                // todo throw
                return csp::common::ReplicatedValue {};
            }

            return csp::common::ReplicatedValue(csp::common::Vector4 { Array[0], Array[1], Array[2], Array[3] });
        }
        else if (Type == StringMapType)
        {
            auto ComponentVals = DeserializeTypeFromTypeString<csp::common::Map<csp::common::String, ComponentProperty>>(Deserializer);
            // TODO: delete
            auto Keys = ComponentVals.Keys();

            csp::common::Map<csp::common::String, csp::common::ReplicatedValue> Vals;

            for (const auto& Key : *Keys)
            {
                Vals[Key] = ComponentVals[Key].Value;
            }

            return Vals;
        }
        else
        {
            return csp::common::ReplicatedValue {};
        }
    }
}

bool csp::multiplayer::ComponentRegistry::RegisterComponents(const csp::common::String& ComponentsJson)
{
    try
    {
        return csp::json::JsonDeserializer::Deserialize(ComponentsJson.c_str(), *this);
    }
    catch (const std::exception&)
    {
        // TODO: log
        return false;
    }
}
const csp::common::Array<ComponentTemplate>& ComponentRegistry::GetTemplates() const { return Templates; }

}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::multiplayer::ComponentRegistry& Registry)
{
    Deserializer.SafeDeserializeMember("components", Registry.Templates);
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::multiplayer::ComponentProperty& Property)
{
    if (!Deserializer.SafeDeserializeMember("name", Property.Name))
    {
        // TODO: log
        return;
    }

    csp::common::String Type;
    if (!Deserializer.SafeDeserializeMember("type", Type))
    {
        // TODO: log
        return;
    }

    Property.Value = csp::multiplayer::DeserializeReplicatedValueFromTypeString(Deserializer, Type);
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::multiplayer::ComponentTemplate& Template)
{
    if (!Deserializer.SafeDeserializeMember("type", Template.Type))
    {
        // TODO: log
        return;
    }

    if (!Deserializer.SafeDeserializeMember("name", Template.Name))
    {
        // TODO: log
        return;
    }

    Deserializer.SafeDeserializeMember("id", Template.Id);
    Deserializer.SafeDeserializeMember("category", Template.Category);
    Deserializer.SafeDeserializeMember("description", Template.Description);
    Deserializer.SafeDeserializeMember("properties", Template.Properties);
}
