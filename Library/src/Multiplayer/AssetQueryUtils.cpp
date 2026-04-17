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
#include "Multiplayer/AssetQueryUtils.h"

#include <algorithm>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <set>
#include <vector>

namespace csp::multiplayer
{
namespace
{

// --- leaf predicate evaluation -------------------------------------------------

std::set<size_t> MatchByAssetType(csp::systems::EAssetType Type, const std::vector<csp::systems::Asset>& Assets)
{
    std::set<size_t> Result;
    for (size_t I = 0; I < Assets.size(); ++I)
    {
        if (Assets[I].Type == Type)
        {
            Result.insert(I);
        }
    }
    return Result;
}

std::set<size_t> MatchByCollectionName(const std::string& Name, const std::vector<csp::systems::Asset>& Assets,
    const std::vector<csp::systems::AssetCollection>& Collections)
{
    std::set<std::string> MatchingCollectionIds;
    for (const auto& Collection : Collections)
    {
        if (Collection.Name.c_str() == Name)
        {
            MatchingCollectionIds.insert(Collection.Id.c_str());
        }
    }

    std::set<size_t> Result;
    for (size_t I = 0; I < Assets.size(); ++I)
    {
        if (MatchingCollectionIds.count(Assets[I].AssetCollectionId.c_str()))
        {
            Result.insert(I);
        }
    }
    return Result;
}

std::set<size_t> BuildAllIndices(const std::vector<csp::systems::Asset>& Assets)
{
    std::set<size_t> All;
    for (size_t I = 0; I < Assets.size(); ++I)
    {
        All.insert(I);
    }
    return All;
}

// Forward declaration
std::set<size_t> EvaluateQueryNode(const rapidjson::Value& Node, const std::vector<csp::systems::Asset>& Assets,
    const std::vector<csp::systems::AssetCollection>& Collections);

std::set<size_t> EvaluateQueryNode(const rapidjson::Value& Node, const std::vector<csp::systems::Asset>& Assets,
    const std::vector<csp::systems::AssetCollection>& Collections)
{
    if (!Node.IsObject() || !Node.HasMember("kind") || !Node["kind"].IsString())
    {
        return {};
    }

    const std::string Kind = Node["kind"].GetString();

    if (Kind == "assetType")
    {
        if (!Node.HasMember("assetType") || !Node["assetType"].IsInt())
        {
            return {};
        }
        const auto Type = static_cast<csp::systems::EAssetType>(Node["assetType"].GetInt());
        return MatchByAssetType(Type, Assets);
    }

    if (Kind == "collectionName")
    {
        if (!Node.HasMember("name") || !Node["name"].IsString())
        {
            return {};
        }
        return MatchByCollectionName(Node["name"].GetString(), Assets, Collections);
    }

    if ((Kind == "and") || (Kind == "or"))
    {
        std::vector<const rapidjson::Value*> OperandValues;
        if (Node.HasMember("operands"))
        {
            const auto& Operands = Node["operands"];
            if (Operands.IsArray())
            {
                for (const auto& Operand : Operands.GetArray())
                {
                    OperandValues.push_back(&Operand);
                }
            }
            else if (Operands.IsObject())
            {
                for (const auto& Member : Operands.GetObject())
                {
                    OperandValues.push_back(&Member.value);
                }
            }
        }

        if (OperandValues.empty())
        {
            return {};
        }

        if (Kind == "and")
        {
            std::set<size_t> Intersection = EvaluateQueryNode(*OperandValues.front(), Assets, Collections);
            for (size_t I = 1; I < OperandValues.size(); ++I)
            {
                const std::set<size_t> Next = EvaluateQueryNode(*OperandValues[I], Assets, Collections);
                std::set<size_t> Merged;
                std::set_intersection(Intersection.begin(), Intersection.end(), Next.begin(), Next.end(),
                    std::inserter(Merged, Merged.begin()));
                Intersection = std::move(Merged);
                if (Intersection.empty())
                {
                    break;
                }
            }
            return Intersection;
        }

        std::set<size_t> Union;
        for (const auto* Operand : OperandValues)
        {
            const auto Next = EvaluateQueryNode(*Operand, Assets, Collections);
            Union.insert(Next.begin(), Next.end());
        }
        return Union;
    }

    if (Kind == "not")
    {
        if (!Node.HasMember("operand"))
        {
            return {};
        }
        const std::set<size_t> All = BuildAllIndices(Assets);
        const std::set<size_t> Excluded = EvaluateQueryNode(Node["operand"], Assets, Collections);
        std::set<size_t> Result;
        std::set_difference(All.begin(), All.end(), Excluded.begin(), Excluded.end(), std::inserter(Result, Result.begin()));
        return Result;
    }

    return {};
}

// --- criteria extraction -------------------------------------------------------

void CollectCriteriaFromNode(const rapidjson::Value& Node, std::vector<csp::systems::EAssetType>& AssetTypes,
    std::vector<std::string>& CollectionNames)
{
    if (!Node.IsObject() || !Node.HasMember("kind") || !Node["kind"].IsString())
    {
        return;
    }

    const std::string Kind = Node["kind"].GetString();

    if (Kind == "assetType")
    {
        if (Node.HasMember("assetType") && Node["assetType"].IsInt())
        {
            AssetTypes.push_back(static_cast<csp::systems::EAssetType>(Node["assetType"].GetInt()));
        }
        return;
    }

    if (Kind == "collectionName")
    {
        if (Node.HasMember("name") && Node["name"].IsString())
        {
            CollectionNames.push_back(Node["name"].GetString());
        }
        return;
    }

    if ((Kind == "and") || (Kind == "or"))
    {
        if (!Node.HasMember("operands"))
        {
            return;
        }
        const auto& Operands = Node["operands"];
        if (Operands.IsArray())
        {
            for (const auto& Operand : Operands.GetArray())
            {
                CollectCriteriaFromNode(Operand, AssetTypes, CollectionNames);
            }
        }
        else if (Operands.IsObject())
        {
            for (const auto& Member : Operands.GetObject())
            {
                CollectCriteriaFromNode(Member.value, AssetTypes, CollectionNames);
            }
        }
        return;
    }

    if (Kind == "not")
    {
        // Do not pass "not" operand to the server filter — we handle it client-side.
        return;
    }
}

} // anonymous namespace

// --- public API ----------------------------------------------------------------

AssetQueryCriteria ExtractAssetQueryCriteria(const std::string& QueryJson)
{
    AssetQueryCriteria Criteria;

    rapidjson::Document Doc;
    if (Doc.Parse(QueryJson.c_str()).HasParseError() || !Doc.IsObject())
    {
        return Criteria;
    }

    std::vector<csp::systems::EAssetType> AssetTypes;
    std::vector<std::string> CollectionNames;
    CollectCriteriaFromNode(Doc, AssetTypes, CollectionNames);

    if (!AssetTypes.empty())
    {
        csp::common::Array<csp::systems::EAssetType> TypeArray(AssetTypes.size());
        for (size_t I = 0; I < AssetTypes.size(); ++I)
        {
            TypeArray[I] = AssetTypes[I];
        }
        Criteria.AssetTypes = TypeArray;
    }

    if (!CollectionNames.empty())
    {
        csp::common::Array<csp::common::String> NameArray(CollectionNames.size());
        for (size_t I = 0; I < CollectionNames.size(); ++I)
        {
            NameArray[I] = CollectionNames[I].c_str();
        }
        Criteria.CollectionNames = NameArray;
    }

    return Criteria;
}

std::vector<csp::systems::Asset> EvaluateAssetQuery(const std::string& QueryJson, const std::vector<csp::systems::Asset>& Assets,
    const std::vector<csp::systems::AssetCollection>& Collections)
{
    rapidjson::Document Doc;
    if (Doc.Parse(QueryJson.c_str()).HasParseError() || !Doc.IsObject())
    {
        return Assets;
    }

    const std::set<size_t> MatchingIndices = EvaluateQueryNode(Doc, Assets, Collections);

    std::vector<csp::systems::Asset> Result;
    Result.reserve(MatchingIndices.size());
    for (const size_t Index : MatchingIndices)
    {
        Result.push_back(Assets[Index]);
    }
    return Result;
}

std::string SerializeAssetsToJson(const std::vector<csp::systems::Asset>& Assets)
{
    rapidjson::StringBuffer Buffer;
    rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);

    Writer.StartArray();
    for (const auto& Asset : Assets)
    {
        Writer.StartObject();
        Writer.Key("id");
        Writer.String(Asset.Id.c_str());
        Writer.Key("assetCollectionId");
        Writer.String(Asset.AssetCollectionId.c_str());
        Writer.Key("name");
        Writer.String(Asset.Name.c_str());
        Writer.Key("url");
        Writer.String(Asset.Uri.c_str());
        Writer.Key("type");
        Writer.Int(static_cast<int>(Asset.Type));
        Writer.EndObject();
    }
    Writer.EndArray();

    return Buffer.GetString();
}

} // namespace csp::multiplayer
