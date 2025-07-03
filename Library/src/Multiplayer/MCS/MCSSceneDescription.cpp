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
static constexpr const char* BooleanType = "Magnopus.Service.Multiplayer.Messages.Components.ItemComponentData`1[[System.Boolean, "
                                           "System.Private.CoreLib]], Magnopus.Service.Multiplayer.Contracts";
static constexpr const char* Int64Type = "Magnopus.Service.Multiplayer.Messages.Components.ItemComponentData`1[[System.Int64, "
                                         "System.Private.CoreLib]], Magnopus.Service.Multiplayer.Contracts";
static constexpr const char* UInt64Type = "Magnopus.Service.Multiplayer.Messages.Components.ItemComponentData`1[[System.UInt64, "
                                          "System.Private.CoreLib]], Magnopus.Service.Multiplayer.Contracts";
static constexpr const char* SingleType = "Magnopus.Service.Multiplayer.Messages.Components.ItemComponentData`1[[System.Single, "
                                          "System.Private.CoreLib]], Magnopus.Service.Multiplayer.Contracts";
static constexpr const char* SingleArrayType = "Magnopus.Service.Multiplayer.Messages.Components.ItemComponentData`1[[System.Single[], "
                                               "System.Private.CoreLib]], Magnopus.Service.Multiplayer.Contracts";
static constexpr const char* DoubleType = "Magnopus.Service.Multiplayer.Messages.Components.ItemComponentData`1[[System.Double, "
                                          "System.Private.CoreLib]], Magnopus.Service.Multiplayer.Contracts";
static constexpr const char* StringType = "Magnopus.Service.Multiplayer.Messages.Components.ItemComponentData`1[[System.String, "
                                          "System.Private.CoreLib]], Magnopus.Service.Multiplayer.Contracts";
static constexpr const char* UInt16DictionaryType
    = "Magnopus.Service.Multiplayer.Messages.Components.ItemComponentData`1[[System.Collections.Generic.IDictionary`2[[System.UInt16, "
      "System.Private.CoreLib],[Magnopus.Service.Multiplayer.Messages.Components.IComponentData, Magnopus.Service.Multiplayer.Contracts]], "
      "System.Private.CoreLib]], Magnopus.Service.Multiplayer.Contracts";
static constexpr const char* StringDictionaryType
    = "Magnopus.Service.Multiplayer.Messages.Components.ItemComponentData`1[[System.Collections.Generic.IDictionary`2[[System.String, "
      "System.Private.CoreLib],[Magnopus.Service.Multiplayer.Messages.Components.IComponentData, Magnopus.Service.Multiplayer.Contracts]], "
      "System.Private.CoreLib]], Magnopus.Service.Multiplayer.Contracts";

namespace
{
    std::string GetComponentString(bool) { return BooleanType; }
    std::string GetComponentString(int64_t) { return Int64Type; }
    std::string GetComponentString(uint64_t) { return UInt64Type; }
    std::string GetComponentString(float) { return SingleType; }
    std::string GetComponentString(const std::vector<float>&) { return SingleArrayType; }
    std::string GetComponentString(double) { return DoubleType; }
    std::string GetComponentString(const std::string&) { return StringType; }
    std::string GetComponentString(const std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData>&) { return UInt16DictionaryType; }
    std::string GetComponentString(const std::map<std::string, csp::multiplayer::mcs::ItemComponentData>&) { return StringDictionaryType; }

    void SerializeComponentData(csp::json::JsonSerializer& Serializer, bool Value) { Serializer.SerializeMember("item", Value); }
    void SerializeComponentData(csp::json::JsonSerializer& Serializer, int64_t Value) { Serializer.SerializeMember("item", Value); }
    void SerializeComponentData(csp::json::JsonSerializer& Serializer, uint64_t Value) { Serializer.SerializeMember("item", Value); }
    void SerializeComponentData(csp::json::JsonSerializer& Serializer, float Value) { Serializer.SerializeMember("item", Value); }
    void SerializeComponentData(csp::json::JsonSerializer& Serializer, const std::vector<float>& Value) { Serializer.SerializeMember("item", Value); }
    void SerializeComponentData(csp::json::JsonSerializer& Serializer, double Value) { Serializer.SerializeMember("item", Value); }
    void SerializeComponentData(csp::json::JsonSerializer& Serializer, const std::string& Value)
    {
        Serializer.SerializeMember("item", csp::common::String { Value.c_str() });
    }

    void SerializeComponentData(csp::json::JsonSerializer& Serializer, const std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData>& Value)
    {
        std::map<std::string, csp::multiplayer::mcs::ItemComponentData> StringMap;

        for (const auto& Pair : Value)
        {
            StringMap[std::to_string(Pair.first)] = Pair.second;
        }

        Serializer.SerializeMember("item", StringMap);
    }

    void SerializeComponentData(csp::json::JsonSerializer& Serializer, const std::map<std::string, csp::multiplayer::mcs::ItemComponentData>& Value)
    {
        Serializer.SerializeMember("item", Value);
    }

    void SerializeComponents(csp::json::JsonSerializer& Serializer, const std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData>& Value)
    {
        std::map<std::string, csp::multiplayer::mcs::ItemComponentData> StringMap;

        for (const auto& Pair : Value)
        {
            StringMap[std::to_string(Pair.first)] = Pair.second;
        }

        Serializer.SerializeMember("components", StringMap);
    }

    template <class T>
    void DeserializeComponentDataFromTypeStringInternal(
        const csp::json::JsonDeserializer& Deserializer, csp::multiplayer::mcs::ItemComponentDataVariant& OutVal)
    {
        T Val;
        Deserializer.SafeDeserializeMember("item", Val);
        OutVal = Val;
    }

    void DeserializeComponentDataFromTypeString(
        const csp::json::JsonDeserializer& Deserializer, const std::string& Type, csp::multiplayer::mcs::ItemComponentDataVariant& OutVal)
    {
        if (Type == BooleanType)
        {
            DeserializeComponentDataFromTypeStringInternal<bool>(Deserializer, OutVal);
        }
        else if (Type == Int64Type)
        {
            DeserializeComponentDataFromTypeStringInternal<int64_t>(Deserializer, OutVal);
        }
        else if (Type == UInt64Type)
        {
            DeserializeComponentDataFromTypeStringInternal<uint64_t>(Deserializer, OutVal);
        }
        else if (Type == SingleType)
        {
            DeserializeComponentDataFromTypeStringInternal<float>(Deserializer, OutVal);
        }
        else if (Type == SingleArrayType)
        {
            DeserializeComponentDataFromTypeStringInternal<std::vector<float>>(Deserializer, OutVal);
        }
        else if (Type == DoubleType)
        {
            DeserializeComponentDataFromTypeStringInternal<double>(Deserializer, OutVal);
        }
        else if (Type == StringType)
        {
            DeserializeComponentDataFromTypeStringInternal<std::string>(Deserializer, OutVal);
        }
        else if (Type == UInt16DictionaryType)
        {
            DeserializeComponentDataFromTypeStringInternal<std::map<std::string, csp::multiplayer::mcs::ItemComponentData>>(Deserializer, OutVal);

            // We can only deserialize string maps, so we need to convert to int afterwards.
            const auto& StringMap = std::get<std::map<std::string, csp::multiplayer::mcs::ItemComponentData>>(OutVal);
            std::map<uint16_t, csp::multiplayer::mcs::ItemComponentData> UIntMap;

            for (const auto& Val : StringMap)
            {
                UIntMap[static_cast<uint16_t>(std::stoi(Val.first))] = Val.second;
            }

            OutVal = UIntMap;
        }
        else if (Type == StringDictionaryType)
        {
            DeserializeComponentDataFromTypeStringInternal<std::map<std::string, csp::multiplayer::mcs::ItemComponentData>>(Deserializer, OutVal);
        }
        else
        {
            throw std::runtime_error("Invalid component type");
        }
    }
}

void DeserializeComponents(const csp::json::JsonDeserializer& Deserializer, std::optional<std::map<uint16_t, ItemComponentData>>& OutComponents)
{
    std::map<std::string, ItemComponentData> Components;
    Deserializer.SafeDeserializeMember("components", Components);

    if (Components.size() > 0)
    {
        std::map<uint16_t, ItemComponentData> UIntComponents;

        for (const auto& ComponentPair : Components)
        {
            UIntComponents[static_cast<uint16_t>(stoi(ComponentPair.first))] = ComponentPair.second;
        }

        OutComponents = UIntComponents;
    }
}

}

void ToJson(csp::json::JsonSerializer& /* Deserializer*/, const csp::multiplayer::mcs::SceneDescription& /* Obj*/) { }

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::multiplayer::mcs::SceneDescription& Obj)
{
    Deserializer.EnterMember("Data");
    Deserializer.SafeDeserializeMember("ObjectMessages", Obj.Objects);
    Deserializer.ExitMember();
}

void ToJson(csp::json::JsonSerializer& Serializer, const csp::multiplayer::mcs::ItemComponentData& Obj)
{
    std::visit(
        [&Serializer](const auto& ValueType)
        {
            std::string TypeString = csp::multiplayer::mcs::GetComponentString(ValueType);
            Serializer.SerializeMember("$type", TypeString.c_str());

            csp::multiplayer::mcs::SerializeComponentData(Serializer, ValueType);
        },
        Obj.GetValue());
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::multiplayer::mcs::ItemComponentData& Obj)
{
    csp::common::String TypeString;
    Deserializer.SafeDeserializeMember("$type", TypeString);

    std::string TypeStdString = TypeString.c_str();

    csp::multiplayer::mcs::ItemComponentDataVariant Variant;
    csp::multiplayer::mcs::DeserializeComponentDataFromTypeString(Deserializer, TypeStdString, Variant);
    Obj.Value = Variant;
}

void ToJson(csp::json::JsonSerializer& Serializer, const csp::multiplayer::mcs::ObjectMessage& Obj)
{
    Serializer.SerializeMember("id", Obj.GetId());
    Serializer.SerializeMember("prefabId", Obj.GetType());
    Serializer.SerializeMember("isTransferable", Obj.GetIsTransferable());
    Serializer.SerializeMember("isPersistent", Obj.GetIsPersistent());
    Serializer.SerializeMember("ownerUserId", Obj.GetOwnerId());

    if (Obj.GetParentId())
    {
        Serializer.SerializeMember("parentId", *Obj.GetParentId());
    }

    if (Obj.Components)
    {
        csp::multiplayer::mcs::SerializeComponents(Serializer, *Obj.GetComponents());
    }
}

void FromJson(const csp::json::JsonDeserializer& Deserializer, csp::multiplayer::mcs::ObjectMessage& Obj)
{
    Deserializer.SafeDeserializeMember("id", Obj.Id);
    Deserializer.SafeDeserializeMember("prefabId", Obj.Type);
    Deserializer.SafeDeserializeMember("isTransferable", Obj.IsTransferable);
    Deserializer.SafeDeserializeMember("isPersistent", Obj.IsPersistent);
    // Deserializer.SafeDeserializeMember("ownerUserId", Obj.OwnerId);

    uint64_t ParentId = 0;
    Deserializer.SafeDeserializeMember("parentId", ParentId);

    if (ParentId != 0)
    {
        Obj.ParentId = ParentId;
    }

    csp::multiplayer::mcs::DeserializeComponents(Deserializer, Obj.Components);
}
