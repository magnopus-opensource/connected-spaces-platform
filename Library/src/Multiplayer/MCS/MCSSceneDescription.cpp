/*
 * Copyright 2025 Magnopus LLC

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

#include "MCSSceneDescription.h"
#include "Services/ApiBase/ApiBase.h"
#include "Json/JsonSerializer.h"

#include <map>
#include <string>
#include <vector>

namespace csp::multiplayer::mcs
{
static constexpr const char* BooleanType = "ComponentData[[Boolean]]";
static constexpr const char* Int64Type = "ComponentData[[Int64]]";
static constexpr const char* UInt64Type = "ComponentData[[UInt64]]";
static constexpr const char* SinglePrecisionType = "ComponentData[[Single]]";
static constexpr const char* SinglePrecisionArrayType = "ComponentData[[Single[]]]";
static constexpr const char* DoubleType = "ComponentData[[Double]]";
static constexpr const char* StringType = "ComponentData[[String]]";
static constexpr const char* UInt16DictionaryType = "ComponentData[[Dictionary[[UInt16],[ComponentData]]]]";
static constexpr const char* StringDictionaryType = "ComponentData[[Dictionary[[String],[ComponentData]]]]";

namespace
{
    std::string GetComponentString(bool) { return BooleanType; }
    std::string GetComponentString(int64_t) { return Int64Type; }
    std::string GetComponentString(uint64_t) { return UInt64Type; }
    std::string GetComponentString(float) { return SinglePrecisionType; }
    std::string GetComponentString(const std::vector<float>&) { return SinglePrecisionArrayType; }
    std::string GetComponentString(double) { return DoubleType; }
    std::string GetComponentString(const std::string&) { return StringType; }
    std::string GetComponentString(const std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData>&) { return UInt16DictionaryType; }
    std::string GetComponentString(const std::map<std::string, csp::multiplayer::mcs::ItemComponentData>&) { return StringDictionaryType; }

    void SerializeComponentData(csp::json::JsonSerializer& serializer, bool value) { serializer.SerializeMember("item", value); }
    void SerializeComponentData(csp::json::JsonSerializer& serializer, int64_t value) { serializer.SerializeMember("item", value); }
    void SerializeComponentData(csp::json::JsonSerializer& serializer, uint64_t value) { serializer.SerializeMember("item", value); }
    void SerializeComponentData(csp::json::JsonSerializer& serializer, float value) { serializer.SerializeMember("item", value); }
    void SerializeComponentData(csp::json::JsonSerializer& serializer, const std::vector<float>& value) { serializer.SerializeMember("item", value); }
    void SerializeComponentData(csp::json::JsonSerializer& serializer, double value) { serializer.SerializeMember("item", value); }
    void SerializeComponentData(csp::json::JsonSerializer& serializer, const std::string& value)
    {
        serializer.SerializeMember("item", csp::common::String { value.c_str() });
    }

    void SerializeComponentData(csp::json::JsonSerializer& serializer, const std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData>& value)
    {
        std::map<std::string, csp::multiplayer::mcs::ItemComponentData> stringMap;

        for (const auto& pair : value)
        {
            stringMap[std::to_string(pair.first)] = pair.second;
        }

        serializer.SerializeMember("item", stringMap);
    }

    void SerializeComponentData(csp::json::JsonSerializer& serializer, const std::map<std::string, csp::multiplayer::mcs::ItemComponentData>& value)
    {
        serializer.SerializeMember("item", value);
    }

    void SerializeComponents(csp::json::JsonSerializer& serializer, const std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData>& value)
    {
        std::map<std::string, csp::multiplayer::mcs::ItemComponentData> stringMap;

        for (const auto& pair : value)
        {
            stringMap[std::to_string(pair.first)] = pair.second;
        }

        serializer.SerializeMember("components", stringMap);
    }

    template <class T>
    void DeserializeComponentDataFromTypeStringInternal(
        const csp::json::JsonDeserializer& deserializer, csp::multiplayer::mcs::ItemComponentDataVariant& outVal)
    {
        T val;
        deserializer.SafeDeserializeMember("item", val);
        outVal = val;
    }

    void DeserializeComponentDataFromTypeString(
        const csp::json::JsonDeserializer& deserializer, const std::string& type, csp::multiplayer::mcs::ItemComponentDataVariant& outVal)
    {
        if (type == BooleanType)
        {
            DeserializeComponentDataFromTypeStringInternal<bool>(deserializer, outVal);
        }
        else if (type == Int64Type)
        {
            DeserializeComponentDataFromTypeStringInternal<int64_t>(deserializer, outVal);
        }
        else if (type == UInt64Type)
        {
            DeserializeComponentDataFromTypeStringInternal<uint64_t>(deserializer, outVal);
        }
        else if (type == SinglePrecisionType)
        {
            DeserializeComponentDataFromTypeStringInternal<float>(deserializer, outVal);
        }
        else if (type == SinglePrecisionArrayType)
        {
            DeserializeComponentDataFromTypeStringInternal<std::vector<float>>(deserializer, outVal);
        }
        else if (type == DoubleType)
        {
            DeserializeComponentDataFromTypeStringInternal<double>(deserializer, outVal);
        }
        else if (type == StringType)
        {
            DeserializeComponentDataFromTypeStringInternal<std::string>(deserializer, outVal);
        }
        else if (type == UInt16DictionaryType)
        {
            DeserializeComponentDataFromTypeStringInternal<std::map<std::string, csp::multiplayer::mcs::ItemComponentData>>(deserializer, outVal);

            // We can only deserialize string maps, so we need to convert to int afterwards.
            const auto& stringMap = std::get<std::map<std::string, csp::multiplayer::mcs::ItemComponentData>>(outVal);
            std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData> uIntMap;

            for (const auto& val : stringMap)
            {
                uIntMap[static_cast<uint16_t>(std::stoi(val.first))] = val.second;
            }

            outVal = uIntMap;
        }
        else if (type == StringDictionaryType)
        {
            DeserializeComponentDataFromTypeStringInternal<std::map<std::string, csp::multiplayer::mcs::ItemComponentData>>(deserializer, outVal);
        }
        else
        {
            throw std::runtime_error("Invalid component type");
        }
    }
}

void DeserializeComponents(const csp::json::JsonDeserializer& deserializer, std::optional<std::map<uint16_t, ItemComponentData>>& outComponents)
{
    std::map<std::string, ItemComponentData> components;
    deserializer.SafeDeserializeMember("components", components);

    if (components.size() > 0)
    {
        std::map<uint16_t, ItemComponentData> uIntComponents;

        for (const auto& componentPair : components)
        {
            uIntComponents[static_cast<uint16_t>(stoi(componentPair.first))] = componentPair.second;
        }

        outComponents = uIntComponents;
    }
}

}

void FromJson(const csp::json::JsonDeserializer& deserializer, csp::multiplayer::mcs::SceneDescription& obj)
{
    deserializer.EnterMember("data");
    deserializer.SafeDeserializeMember("objectMessages", obj.Objects);
    deserializer.ExitMember();
}

void ToJson(csp::json::JsonSerializer& serializer, const csp::multiplayer::mcs::ItemComponentData& obj)
{
    std::visit(
        [&serializer](const auto& valueType)
        {
            std::string typeString = csp::multiplayer::mcs::GetComponentString(valueType);
            serializer.SerializeMember("$type", typeString.c_str());

            csp::multiplayer::mcs::SerializeComponentData(serializer, valueType);
        },
        obj.GetValue());
}

void FromJson(const csp::json::JsonDeserializer& deserializer, csp::multiplayer::mcs::ItemComponentData& obj)
{
    csp::common::String typeString;
    deserializer.SafeDeserializeMember("$type", typeString);

    std::string typeStdString = typeString.c_str();

    csp::multiplayer::mcs::ItemComponentDataVariant variant;
    csp::multiplayer::mcs::DeserializeComponentDataFromTypeString(deserializer, typeStdString, variant);
    obj = csp::multiplayer::mcs::ItemComponentData { variant };
}

void ToJson(csp::json::JsonSerializer& serializer, const csp::multiplayer::mcs::ObjectMessage& obj)
{
    serializer.SerializeMember("id", obj.GetId());
    serializer.SerializeMember("prefabId", obj.GetType());
    serializer.SerializeMember("isTransferable", obj.GetIsTransferable());
    serializer.SerializeMember("isPersistent", obj.GetIsPersistent());
    serializer.SerializeMember("ownerUserId", obj.GetOwnerId());

    if (obj.GetParentId())
    {
        serializer.SerializeMember("parentId", *obj.GetParentId());
    }

    if (obj.GetComponents())
    {
        csp::multiplayer::mcs::SerializeComponents(serializer, *obj.GetComponents());
    }
}

void FromJson(const csp::json::JsonDeserializer& deserializer, csp::multiplayer::mcs::ObjectMessage& obj)
{
    uint64_t id = 0;
    uint64_t type = 0;
    bool isTransferable = false;
    bool isPersistent = false;
    uint64_t ownerId = 0;
    std::optional<uint64_t> parentId;
    std::optional<std::map<csp::multiplayer::mcs::PropertyKeyType, csp::multiplayer::mcs::ItemComponentData>> components;

    deserializer.SafeDeserializeMember("id", id);
    deserializer.SafeDeserializeMember("prefabId", type);
    deserializer.SafeDeserializeMember("isTransferable", isTransferable);
    deserializer.SafeDeserializeMember("isPersistent", isPersistent);
    // Deserializer.SafeDeserializeMember("ownerUserId", Obj.OwnerId);

    uint64_t deserializedParentId = 0;
    deserializer.SafeDeserializeMember("parentId", deserializedParentId);

    if (deserializedParentId != 0)
    {
        parentId = deserializedParentId;
    }

    csp::multiplayer::mcs::DeserializeComponents(deserializer, components);

    obj = csp::multiplayer::mcs::ObjectMessage { id, type, isTransferable, isPersistent, ownerId, parentId, components };
}
