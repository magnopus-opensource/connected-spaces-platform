/*
 * Copyright 2023 Magnopus LLC

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
#include "Multiplayer/Script/EntityScriptInterface.h"

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/Script/EntityScript.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/SystemsManager.h"
#include "Multiplayer/EntityQueryUtils.h"
#include "Multiplayer/Script/ComponentScriptInterface.h"
#include <fmt/format.h>

using namespace csp::systems;

namespace csp::multiplayer
{

EntityScriptInterface::EntityScriptInterface(SpaceEntity* InEntity, bool IsLocal)
    : Entity(InEntity)
    , LocalScope(IsLocal)
{
}

EntityScriptInterface::Vector3 EntityScriptInterface::GetPosition() const
{
    EntityScriptInterface::Vector3 Pos = { 0, 0, 0 };

    if (Entity)
    {
        const csp::common::Vector3& Position = Entity->GetPosition();

        Pos[0] = Position.X;
        Pos[1] = Position.Y;
        Pos[2] = Position.Z;
    }

    return Pos;
}

void EntityScriptInterface::CommitEntityUpdate()
{
    if (LocalScope)
    {
        Entity->ApplyLocalPropertyPatch();
    }
    else
    {
        Entity->QueueUpdate();
    }
}

void EntityScriptInterface::SetPosition(EntityScriptInterface::Vector3 Pos)
{
    // CSP_LOG_FORMAT(LogLevel::VeryVerbose, "EntityScriptWrapper::SetPosition { %.2f, %.2f, %.2f }\n", Pos[0], Pos[1], Pos[2]);

    const csp::common::Vector3 NewPosition(Pos[0], Pos[1], Pos[2]);
    const csp::common::Vector3& CurrentPosition = Entity->GetPosition();

    if (CurrentPosition != NewPosition)
    {
        Entity->SetPosition(NewPosition);
        CommitEntityUpdate();
    }
}

EntityScriptInterface::Vector3 EntityScriptInterface::GetGlobalPosition() const
{
    EntityScriptInterface::Vector3 GlobalPos = { 0, 0, 0 };

    if (Entity)
    {
        const csp::common::Vector3& Position = Entity->GetGlobalPosition();

        GlobalPos[0] = Position.X;
        GlobalPos[1] = Position.Y;
        GlobalPos[2] = Position.Z;
    }

    return GlobalPos;
}

EntityScriptInterface::Vector4 EntityScriptInterface::GetRotation() const
{
    EntityScriptInterface::Vector4 Rot = { 0, 0, 0, 0 };

    if (Entity)
    {
        csp::common::Vector4 Rotation = Entity->GetRotation();

        Rot[0] = Rotation.X;
        Rot[1] = Rotation.Y;
        Rot[2] = Rotation.Z;
        Rot[3] = Rotation.W;
    }

    return Rot;
}

void EntityScriptInterface::SetRotation(EntityScriptInterface::Vector4 Rot)
{

    const csp::common::Vector4 NewRotation(Rot[0], Rot[1], Rot[2], Rot[3]);
    const csp::common::Vector4& CurrentRotation = Entity->GetRotation();

    if (CurrentRotation != NewRotation)
    {
        Entity->SetRotation(NewRotation);
        CommitEntityUpdate();
    }
}

EntityScriptInterface::Vector4 EntityScriptInterface::GetGlobalRotation() const
{
    EntityScriptInterface::Vector4 GlobalRot = { 0, 0, 0, 0 };

    if (Entity)
    {
        csp::common::Vector4 Rotation = Entity->GetGlobalRotation();

        GlobalRot[0] = Rotation.X;
        GlobalRot[1] = Rotation.Y;
        GlobalRot[2] = Rotation.Z;
        GlobalRot[3] = Rotation.W;
    }

    return GlobalRot;
}

int64_t EntityScriptInterface::GetParentId()
{
    if (const auto Parent = Entity->GetParentEntity())
    {
        return Parent->GetId();
    }

    return 0;
}

void EntityScriptInterface::SetParentId(int64_t ParentId)
{
    if (Entity)
    {
        Entity->SetParentId(ParentId);
    }
}

void EntityScriptInterface::RemoveParentEntity()
{
    if (Entity)
    {
        Entity->RemoveParentEntity();
    }
}

EntityScriptInterface* EntityScriptInterface::GetParentEntity() const
{
    if (Entity)
    {
        const auto Parent = Entity->GetParentEntity();
        if (Parent)
        {
            auto* Iface = Parent->GetScriptInterface();
            if (Iface != nullptr)
            {
                Iface->SetLocalScope(LocalScope);
            }
            return Iface;
        }
    }

    return nullptr;
}

static void CollectDescendants(SpaceEntity* Root, std::vector<SpaceEntity*>& Out)
{
    const auto* Children = Root->GetChildEntities();
    if (!Children)
    {
        return;
    }

    for (size_t i = 0; i < Children->Size(); ++i)
    {
        if (auto* Child = (*Children)[i])
        {
            Out.push_back(Child);
            CollectDescendants(Child, Out);
        }
    }
}

std::vector<EntityScriptInterface*> EntityScriptInterface::GetChildEntitiesByQuery(qjs::Value Query, qjs::rest<bool> Recursive)
{
    std::vector<EntityScriptInterface*> Result;

    if (!Entity || !Query.ctx)
    {
        return Result;
    }

    const std::string QueryJson = Query.toJSON();

    const auto* Children = Entity->GetChildEntities();
    if (!Children || Children->Size() == 0)
    {
        return Result;
    }

    std::vector<SpaceEntity*> Candidates;
    if (!Recursive.empty() && Recursive[0])
    {
        CollectDescendants(Entity, Candidates);
    }
    else
    {
        Candidates.reserve(Children->Size());
        for (size_t i = 0; i < Children->Size(); ++i)
        {
            if (auto* Child = (*Children)[i])
            {
                Candidates.push_back(Child);
            }
        }
    }

    const auto MatchingIds = ResolveEntityIdsFromQueryJson(QueryJson, Candidates);
    for (auto* Candidate : Candidates)
    {
        if ((Candidate != nullptr) && (MatchingIds.find(Candidate->GetId()) != MatchingIds.end()))
        {
            auto* Iface = Candidate->GetScriptInterface();
            if (Iface != nullptr)
            {
                Iface->SetLocalScope(LocalScope);
                Result.push_back(Iface);
            }
        }
    }

    return Result;
}

EntityScriptInterface::Vector3 EntityScriptInterface::GetScale() const
{
    EntityScriptInterface::Vector3 Scale = { 0, 0, 0 };

    if (Entity)
    {
        csp::common::Vector3 EntityScale = Entity->GetScale();

        Scale[0] = EntityScale.X;
        Scale[1] = EntityScale.Y;
        Scale[2] = EntityScale.Z;
    }

    return Scale;
}

void EntityScriptInterface::SetScale(EntityScriptInterface::Vector3 Scale)
{
    const csp::common::Vector3 NewScale(Scale[0], Scale[1], Scale[2]);
    const csp::common::Vector3& CurrentScale = Entity->GetScale();

    if (CurrentScale != NewScale)
    {
        Entity->SetScale(NewScale);
        CommitEntityUpdate();
    }
}

EntityScriptInterface::Vector3 EntityScriptInterface::GetGlobalScale() const
{
    EntityScriptInterface::Vector3 GlobalScale = { 0, 0, 0 };

    if (Entity)
    {
        csp::common::Vector3 EntityScale = Entity->GetGlobalScale();

        GlobalScale[0] = EntityScale.X;
        GlobalScale[1] = EntityScale.Y;
        GlobalScale[2] = EntityScale.Z;
    }

    return GlobalScale;
}

const std::string EntityScriptInterface::GetName() const { return Entity->GetName().c_str(); }

int64_t EntityScriptInterface::GetId() const { return Entity->GetId(); }

std::vector<std::string> EntityScriptInterface::GetTags() const
{
    std::vector<std::string> Tags;
    if (!Entity)
    {
        return Tags;
    }

    const auto EntityTags = Entity->GetTags();
    Tags.reserve(EntityTags.Size());
    for (size_t Index = 0; Index < EntityTags.Size(); ++Index)
    {
        Tags.emplace_back(EntityTags[Index].c_str());
    }
    return Tags;
}

void EntityScriptInterface::SetTags(const std::vector<std::string>& Tags)
{
    if (!Entity)
    {
        return;
    }

    csp::common::Array<csp::common::String> NewTags(Tags.size());
    for (size_t Index = 0; Index < Tags.size(); ++Index)
    {
        NewTags[Index] = Tags[Index].c_str();
    }

    if (Entity->SetTags(NewTags))
    {
        CommitEntityUpdate();
    }
}

bool EntityScriptInterface::HasTag(const std::string& Tag) const
{
    return Entity ? Entity->HasTag(Tag.c_str()) : false;
}

bool EntityScriptInterface::AddTag(const std::string& Tag)
{
    if (!Entity)
    {
        return false;
    }

    const bool bUpdated = Entity->AddTag(Tag.c_str());
    if (bUpdated)
    {
        CommitEntityUpdate();
    }
    return bUpdated;
}

bool EntityScriptInterface::RemoveTag(const std::string& Tag)
{
    if (!Entity)
    {
        return false;
    }

    const bool bUpdated = Entity->RemoveTag(Tag.c_str());
    if (bUpdated)
    {
        CommitEntityUpdate();
    }
    return bUpdated;
}

void EntityScriptInterface::SubscribeToPropertyChange(int32_t ComponentId, int32_t PropertyKey, std::string Message)
{
    Entity->GetScript().SubscribeToPropertyChange(ComponentId, PropertyKey, Message.c_str());
}

void EntityScriptInterface::SubscribeToMessage(std::string Message, std::string MessageParamsJson)
{
    Entity->GetScript().SubscribeToMessage(Message.c_str(), MessageParamsJson.c_str());
}

void EntityScriptInterface::PostMessageToScript(std::string Message, std::string MessageParamsJson)
{
    Entity->GetScript().PostMessageToScript(Message.c_str(), MessageParamsJson.c_str());
}

void EntityScriptInterface::ClaimScriptOwnership() { Entity->ClaimScriptOwnership(); }

void EntityScriptInterface::On(const std::string& EventName, qjs::Value Callback)
{
    // If this is a local wrapper (created by LocalEntitySystemScriptInterface),
    // delegate to the entity's canonical ScriptInterface so that Fire() — which
    // is called via Entity->GetScriptInterface() — finds the registered listeners.
    if (Entity && Entity->GetScriptInterface() != this)
    {
        Entity->GetScriptInterface()->On(EventName, std::move(Callback));
        return;
    }

    if (!Callback.ctx || !JS_IsFunction(Callback.ctx, Callback.v))
    {
        auto* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
        LogSystem->LogMsg(csp::common::LogLevel::Error,
            fmt::format("NgxScript: entity.on('{}') called with a non-function callback (entityId={}).",
                EventName, Entity ? std::to_string(Entity->GetId()) : "?").c_str());
        return;
    }
    EventListeners[EventName].push_back(std::move(Callback));
}

void EntityScriptInterface::Off(const std::string& EventName, qjs::Value Callback)
{
    if (Entity && Entity->GetScriptInterface() != this)
    {
        Entity->GetScriptInterface()->Off(EventName, std::move(Callback));
        return;
    }

    auto it = EventListeners.find(EventName);
    if (it == EventListeners.end())
    {
        return;
    }
    auto& Listeners = it->second;
    for (auto i = Listeners.begin(); i != Listeners.end();)
    {
        if (JS_VALUE_GET_PTR(i->v) == JS_VALUE_GET_PTR(Callback.v))
            i = Listeners.erase(i);
        else
            ++i;
    }
}

void EntityScriptInterface::Fire(const std::string& EventName, qjs::Value EventData)
{
    auto* LogSystem = csp::systems::SystemsManager::Get().GetLogSystem();
    const std::string EntityIdStr = Entity ? std::to_string(Entity->GetId()) : "?";

    if (!EventData.ctx)
    {
        LogSystem->LogMsg(csp::common::LogLevel::Error,
            fmt::format("NgxScript: Fire('{}') on entity {} called with null JS context; ignoring.",
                EventName, EntityIdStr).c_str());
        return;
    }

    auto it = EventListeners.find(EventName);
    if (it == EventListeners.end() || it->second.empty())
    {
        return;
    }

    // Copy the listener list so that callbacks may safely call on()/off() without invalidating iterators.
    auto Listeners = it->second;
    for (auto& Listener : Listeners)
    {
        JSValueConst Args[] = { EventData.v };
        JSValue Result      = JS_Call(EventData.ctx, Listener.v, JS_UNDEFINED, 1, Args);
        if (JS_IsException(Result))
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error,
                fmt::format("NgxScript: Fire('{}') on entity {} — listener threw an exception.",
                    EventName, EntityIdStr).c_str());
        }
        JS_FreeValue(EventData.ctx, Result);
    }
}

void EntityScriptInterface::ClearEventListeners()
{
    EventListeners.clear();
}

std::vector<ComponentScriptInterface*> EntityScriptInterface::GetComponents()
{
    std::vector<ComponentScriptInterface*> Components;

    if (Entity)
    {
        const csp::common::Map<uint16_t, ComponentBase*>& ComponentMap = *Entity->GetComponents();
        const auto ComponentKeys = ComponentMap.Keys();

        for (size_t i = 0; i < ComponentKeys->Size(); ++i)
        {
            ComponentBase* Component = ComponentMap[ComponentKeys->operator[](i)];

            if (Component->GetScriptInterface() != nullptr)
            {
                auto* Iface = Component->GetScriptInterface();
                Iface->SetLocalScope(LocalScope);
                Components.push_back(Iface);
            }
        }

        delete (ComponentKeys);
    }

    return Components;
}

} // namespace csp::multiplayer
