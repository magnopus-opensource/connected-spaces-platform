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
#include "CSP/Multiplayer/Script/EntityScript.h"

#include "CSP/Common/StringFormat.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
#include "CSP/Multiplayer/Script/EntityScriptMessages.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"

namespace csp::multiplayer
{

constexpr const char* SCRIPT_ERROR_NO_COMPONENT = "No script component";
constexpr const char* SCRIPT_ERROR_EMPTY_SCRIPT = "Script is empty";

EntityScript::EntityScript(SpaceEntity* InEntity, SpaceEntitySystem* InSpaceEntitySystem)
    : ScriptSystem(csp::systems::SystemsManager::Get().GetScriptSystem())
    , Entity(InEntity)
    , EntityScriptComponent(nullptr)
    , HasLastError(false)
    , HasBinding(false)
    , SpaceEntitySystemPtr(InSpaceEntitySystem)
{
}

EntityScript::~EntityScript() { Shutdown(); }

bool EntityScript::Invoke()
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "EntityScript::Invoke called for %s", Entity->GetName().c_str());

    CheckBinding();

    HasLastError = false;
    LastError = "Unknown Error";

    if (EntityScriptComponent == nullptr)
    {
        HasLastError = true;
        LastError = SCRIPT_ERROR_NO_COMPONENT;
    }
    else
    {
        const csp::common::String& ScriptSource = EntityScriptComponent->GetScriptSource();

        if (!ScriptSource.IsEmpty())
        {
            HasLastError = !ScriptSystem->RunScript(Entity->GetId(), ScriptSource);
        }
        else
        {
            HasLastError = true;
            LastError = SCRIPT_ERROR_EMPTY_SCRIPT;
        }
    }

    if (HasLastError)
    {
        CSP_LOG_ERROR_FORMAT("Script Error: %s", LastError.c_str());
    }

    return !HasLastError;
}

void EntityScript::RunScript(const csp::common::String& ScriptSource)
{
    bool RunScriptLocally = true;

    if (SpaceEntitySystemPtr)
    {
        RunScriptLocally = SpaceEntitySystemPtr->CheckIfWeShouldRunScriptsLocally();
    }

    if (RunScriptLocally)
    {
        ScriptSystem->RunScript(Entity->GetId(), ScriptSource);
    }
    else
    {

        SpaceEntitySystemPtr->RunScriptRemotely(Entity->GetId(), ScriptSource);
    }
}

void EntityScript::SetScriptSource(const csp::common::String& InScriptSource)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "EntityScript::SetScriptSource called for %s\nSource:", Entity->GetName().c_str(),
        InScriptSource.c_str());
    CSP_LOG_MSG(csp::systems::LogLevel::VeryVerbose, "--EndScriptSource--");

    if (EntityScriptComponent == nullptr)
    {
        EntityScriptComponent = (ScriptSpaceComponent*)(Entity->AddComponent(ComponentType::ScriptData));
    }

    EntityScriptComponent->SetScriptSource(InScriptSource);

    Entity->MarkForUpdate();
}

bool EntityScript::HasError() { return HasLastError; }

csp::common::String EntityScript::GetErrorText() { return LastError; }

void EntityScript::SetScriptSpaceComponent(ScriptSpaceComponent* InEnityScriptComponent)
{
    EntityScriptComponent = InEnityScriptComponent;
    ScriptSystem->CreateContext(Entity->GetId());
}

csp::common::String EntityScript::GetScriptSource()
{
    if (EntityScriptComponent == nullptr)
    {
        return csp::common::String();
    }

    return EntityScriptComponent->GetScriptSource();
}

void EntityScript::RegisterSourceAsModule()
{
    if (EntityScriptComponent != nullptr)
    {
        ScriptSystem->SetModuleSource(Entity->GetName(), GetScriptSource());
    }
}

void EntityScript::SetOwnerId(uint64_t ClientId)
{
    if (EntityScriptComponent != nullptr)
    {
        if (GetOwnerId() != ClientId)
        {
            EntityScriptComponent->SetOwnerId(ClientId);
            Entity->MarkForUpdate();
        }
    }
}

uint64_t EntityScript::GetOwnerId() const
{
    if (EntityScriptComponent != nullptr)
    {
        return EntityScriptComponent->GetOwnerId();
    }

    return 0;
}

void EntityScript::Shutdown()
{
    ScriptSystem->ClearModuleSource(Entity->GetName());
    ScriptSystem->DestroyContext(Entity->GetId());
}

void EntityScript::OnSourceChanged(const csp::common::String& InScriptSource)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "OnSourceChanged: %s\n", InScriptSource.c_str());

    if (EntityScriptComponent != nullptr)
    {
        MessageMap.clear();
        PropertyMap.clear();

        ScriptSystem->ResetContext(Entity->GetId());
        HasBinding = false; // we've reset the context which means this script is no longer bound

        ScriptSystem->SetModuleSource(Entity->GetName(), InScriptSource);

        Bind();
    }
}

// Called when an entity has been created
void EntityScript::Bind()
{
    if (EntityScriptComponent != nullptr)
    {
        ScriptSystem->BindContext(Entity->GetId());
        HasBinding = true;
    }
}

void EntityScript::CheckBinding()
{
    if (!HasBinding)
    {
        Bind();
    }
}

void EntityScript::SubscribeToPropertyChange(int32_t ComponentId, int32_t PropertyKey, csp::common::String Message)
{
    PropertyChangeKey Key = std::make_pair(ComponentId, PropertyKey);
    PropertyChangeMap::iterator It = PropertyMap.find(Key);

    if (It == PropertyMap.end())
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "SubscribeToPropertyChange: (%d, %d) %s\n", ComponentId, PropertyKey, Message.c_str());

        PropertyMap.insert(PropertyChangeMap::value_type(Key, Message));
    }
}

void EntityScript::OnPropertyChanged(int32_t ComponentId, int32_t PropertyKey)
{
    PropertyChangeKey Key = std::make_pair(ComponentId, PropertyKey);
    PropertyChangeMap::iterator It = PropertyMap.find(Key);

    if (It != PropertyMap.end())
    {
        const csp::common::String& Message = It->second;

        // Generate a call to the callback with the correct parameters
        csp::common::String ParamJson = csp::common::StringFormat("{\"id\": %d, \"key\": %d}", ComponentId, PropertyKey);

        PostMessageToScript(Message, ParamJson);
    }
}

void EntityScript::SubscribeToMessage(const csp::common::String Message, const csp::common::String OnMessageCallback)
{
    SubscribedMessageMap::iterator It = MessageMap.find(Message);

    if (It == MessageMap.end())
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "SubscribeToMessage: %s -> %s\n", Message.c_str(), OnMessageCallback.c_str());

        MessageMap.insert(SubscribedMessageMap::value_type(Message, OnMessageCallback));
    }
}

void EntityScript::PostMessageToScript(const csp::common::String Message, const csp::common::String MessageParamsJson)
{
    SubscribedMessageMap::iterator It = MessageMap.find(Message);

    if (It != MessageMap.end())
    {
        const csp::common::String& OnMessageCallback = It->second;

        // Generate a call to the callback with the correct parameters
        csp::common::String ScriptText
            = csp::common::StringFormat("%s('%s','%s')", OnMessageCallback.c_str(), Message.c_str(), MessageParamsJson.c_str());

        if (Message != SCRIPT_MSG_ENTITY_TICK)
        {
            CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "PostMessageToScript: %s\n", ScriptText.c_str());
        }

        RunScript(ScriptText.c_str());
    }
}

} // namespace csp::multiplayer
