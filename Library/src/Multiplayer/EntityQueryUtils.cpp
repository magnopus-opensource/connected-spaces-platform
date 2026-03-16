#include "Multiplayer/EntityQueryUtils.h"

#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"

#include <algorithm>
#include <optional>
#include <rapidjson/document.h>
#include <set>

namespace csp::multiplayer
{
namespace
{
std::optional<uint64_t> TryParseEntityId(const std::string& EntityIdText)
{
    if (EntityIdText.empty())
    {
        return std::nullopt;
    }

    try
    {
        return std::stoull(EntityIdText);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

SpaceEntity* FindEntityById(const std::vector<SpaceEntity*>& Entities, uint64_t EntityId)
{
    for (auto* Entity : Entities)
    {
        if ((Entity != nullptr) && (Entity->GetId() == EntityId))
        {
            return Entity;
        }
    }

    return nullptr;
}

std::set<uint64_t> BuildAllEntityIdSet(const std::vector<SpaceEntity*>& Entities)
{
    std::set<uint64_t> EntityIds;
    for (const auto* Entity : Entities)
    {
        if (Entity != nullptr)
        {
            EntityIds.insert(Entity->GetId());
        }
    }

    return EntityIds;
}

std::set<uint64_t> EvaluateQueryToEntityIds(const rapidjson::Value& Query, const std::vector<SpaceEntity*>& Entities)
{
    if (!Query.IsObject())
    {
        return {};
    }

    if (!Query.HasMember("kind") || !Query["kind"].IsString())
    {
        return {};
    }

    const std::string Kind = Query["kind"].GetString();
    if (Kind == "id")
    {
        if (!Query.HasMember("id"))
        {
            return {};
        }

        uint64_t RequestedId = 0;
        if (Query["id"].IsUint64())
        {
            RequestedId = Query["id"].GetUint64();
        }
        else if (Query["id"].IsInt64() && (Query["id"].GetInt64() > 0))
        {
            RequestedId = static_cast<uint64_t>(Query["id"].GetInt64());
        }
        else if (Query["id"].IsString())
        {
            const auto ParsedId = TryParseEntityId(Query["id"].GetString());
            if (!ParsedId.has_value())
            {
                return {};
            }

            RequestedId = *ParsedId;
        }
        else
        {
            return {};
        }

        return (FindEntityById(Entities, RequestedId) != nullptr) ? std::set<uint64_t> { RequestedId } : std::set<uint64_t> {};
    }

    if (Kind == "name")
    {
        if (!Query.HasMember("name") || !Query["name"].IsString())
        {
            return {};
        }

        const std::string RequestedName = Query["name"].GetString();
        std::set<uint64_t> MatchingIds;
        for (const auto* Entity : Entities)
        {
            if ((Entity != nullptr) && (Entity->GetName().c_str() == RequestedName))
            {
                MatchingIds.insert(Entity->GetId());
            }
        }
        return MatchingIds;
    }

    if (Kind == "tag")
    {
        if (!Query.HasMember("tag") || !Query["tag"].IsString())
        {
            return {};
        }

        const csp::common::String RequestedTag = csp::common::String(Query["tag"].GetString()).Trim().ToLower();
        if (RequestedTag.IsEmpty())
        {
            return {};
        }

        std::set<uint64_t> MatchingIds;
        for (const auto* Entity : Entities)
        {
            if ((Entity != nullptr) && Entity->HasTag(RequestedTag))
            {
                MatchingIds.insert(Entity->GetId());
            }
        }
        return MatchingIds;
    }

    if (Kind == "componentType")
    {
        if (!Query.HasMember("componentType") || !Query["componentType"].IsInt())
        {
            return {};
        }

        const int32_t RawComponentType = Query["componentType"].GetInt();
        if (RawComponentType <= 0)
        {
            return {};
        }

        const auto ComponentType = static_cast<csp::multiplayer::ComponentType>(RawComponentType);
        std::set<uint64_t> MatchingIds;
        for (auto* Entity : Entities)
        {
            if ((Entity != nullptr) && (Entity->FindFirstComponentOfType(ComponentType) != nullptr))
            {
                MatchingIds.insert(Entity->GetId());
            }
        }
        return MatchingIds;
    }

    if ((Kind == "and") || (Kind == "or"))
    {
        std::vector<const rapidjson::Value*> OperandValues;
        if (Query.HasMember("operands"))
        {
            const auto& Operands = Query["operands"];
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
            std::set<uint64_t> Intersection = EvaluateQueryToEntityIds(*OperandValues.front(), Entities);
            for (size_t OperandIndex = 1; OperandIndex < OperandValues.size(); ++OperandIndex)
            {
                const std::set<uint64_t> Next = EvaluateQueryToEntityIds(*OperandValues[OperandIndex], Entities);
                std::set<uint64_t> Result;
                std::set_intersection(Intersection.begin(), Intersection.end(), Next.begin(), Next.end(), std::inserter(Result, Result.begin()));
                Intersection = std::move(Result);
                if (Intersection.empty())
                {
                    break;
                }
            }

            return Intersection;
        }

        std::set<uint64_t> Union;
        for (const auto* Operand : OperandValues)
        {
            const auto Next = EvaluateQueryToEntityIds(*Operand, Entities);
            Union.insert(Next.begin(), Next.end());
        }
        return Union;
    }

    if (Kind == "not")
    {
        if (!Query.HasMember("operand"))
        {
            return {};
        }

        const std::set<uint64_t> AllEntityIds = BuildAllEntityIdSet(Entities);
        const std::set<uint64_t> ExcludedIds = EvaluateQueryToEntityIds(Query["operand"], Entities);
        std::set<uint64_t> Result;
        std::set_difference(AllEntityIds.begin(), AllEntityIds.end(), ExcludedIds.begin(), ExcludedIds.end(), std::inserter(Result, Result.begin()));
        return Result;
    }

    return {};
}
}

std::set<uint64_t> ResolveEntityIdsFromQueryJson(const std::string& QueryJson, const std::vector<SpaceEntity*>& Entities)
{
    rapidjson::Document QueryDocument;
    if (QueryDocument.Parse(QueryJson.c_str()).HasParseError() || !QueryDocument.IsObject())
    {
        return {};
    }

    return EvaluateQueryToEntityIds(QueryDocument, Entities);
}

std::optional<uint64_t> ResolveEntityIdFromQueryJson(const std::string& QueryJson, const std::vector<SpaceEntity*>& Entities)
{
    const std::set<uint64_t> MatchingIds = ResolveEntityIdsFromQueryJson(QueryJson, Entities);
    if (MatchingIds.empty())
    {
        return std::nullopt;
    }

    return *MatchingIds.begin();
}

} // namespace csp::multiplayer
