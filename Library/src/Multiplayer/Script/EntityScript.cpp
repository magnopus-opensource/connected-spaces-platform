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
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Common/fmt_Formatters.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
#include "CSP/Multiplayer/Script/EntityScriptMessages.h"
#include "CSP/Multiplayer/SpaceEntity.h"

#include <fmt/format.h>

namespace csp::multiplayer
{

constexpr const char* SCRIPT_ERROR_NO_COMPONENT = "No script component";
constexpr const char* SCRIPT_ERROR_EMPTY_SCRIPT = "Script is empty";

EntityScript::EntityScript(SpaceEntity* inEntity, csp::common::IRealtimeEngine* inRealtimeEngine, csp::common::IJSScriptRunner* scriptRunner,
    csp::common::LogSystem* logSystem)
    : m_entity(inEntity)
    , m_entityScriptComponent(nullptr)
    , m_hasLastError(false)
    , m_hasBinding(false)
    , m_realtimeEnginePtr(inRealtimeEngine)
    , m_logSystem(logSystem)
    , m_scriptRunner(scriptRunner)
{
}

EntityScript::~EntityScript() { Shutdown(); }

bool EntityScript::Invoke()
{
    if (m_logSystem != nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("EntityScript::Invoke called for {}", m_entity->GetName()).c_str());
    }

    CheckBinding();

    m_hasLastError = false;
    m_lastError = "Unknown Error";

    if (m_entityScriptComponent == nullptr)
    {
        m_hasLastError = true;
        m_lastError = SCRIPT_ERROR_NO_COMPONENT;
    }
    else
    {
        const csp::common::String& scriptSource = m_entityScriptComponent->GetScriptSource();

        if (!scriptSource.IsEmpty())
        {
            m_hasLastError = !m_scriptRunner->RunScript(m_entity->GetId(), scriptSource);
        }
        else
        {
            m_hasLastError = true;
            m_lastError = SCRIPT_ERROR_EMPTY_SCRIPT;
        }
    }

    if (m_hasLastError)
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Error, fmt::format("Script Error: {}", m_lastError).c_str());
        }
    }

    return !m_hasLastError;
}

void EntityScript::RunScript(const csp::common::String& scriptSource)
{
    if (m_realtimeEnginePtr == nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Fatal, "Null RealtimeEngine when trying to run script. Aborting Operation.");
        return;
    }

    // If offline, just run the script
    if (m_realtimeEnginePtr->GetRealtimeEngineType() != csp::common::RealtimeEngineType::Online)
    {
        m_scriptRunner->RunScript(m_entity->GetId(), scriptSource);
        return;
    }

    // Otherwise we're online
    auto* onlineRealtimeEngine = static_cast<csp::multiplayer::OnlineRealtimeEngine*>(m_realtimeEnginePtr);
    if (onlineRealtimeEngine->CheckIfWeShouldRunScriptsLocally())
    {
        m_scriptRunner->RunScript(m_entity->GetId(), scriptSource);
    }
    else
    {

        onlineRealtimeEngine->RunScriptRemotely(m_entity->GetId(), scriptSource);
    }
}

void EntityScript::SetScriptSource(const csp::common::String& inScriptSource)
{
    if (m_logSystem != nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::VeryVerbose,
            fmt::format("EntityScript::SetScriptSource called for {0}\nSource: {1}", m_entity->GetName(), inScriptSource).c_str());
        m_logSystem->LogMsg(csp::common::LogLevel::VeryVerbose, "--EndScriptSource--");
    }

    if (m_entityScriptComponent == nullptr)
    {
        m_entityScriptComponent = (ScriptSpaceComponent*)(m_entity->AddComponent(ComponentType::ScriptData));
    }

    m_entityScriptComponent->SetScriptSource(inScriptSource);

    m_entity->QueueUpdate();
}

bool EntityScript::HasError() { return m_hasLastError; }

bool EntityScript::HasEntityScriptComponent() { return m_entityScriptComponent != nullptr; }

csp::common::String EntityScript::GetErrorText() { return m_lastError; }

void EntityScript::SetScriptSpaceComponent(ScriptSpaceComponent* inEnityScriptComponent)
{
    m_entityScriptComponent = inEnityScriptComponent;
    m_scriptRunner->CreateContext(m_entity->GetId());
}

csp::common::String EntityScript::GetScriptSource()
{
    if (m_entityScriptComponent == nullptr)
    {
        return csp::common::String();
    }

    return m_entityScriptComponent->GetScriptSource();
}

void EntityScript::RegisterSourceAsModule()
{
    if (m_entityScriptComponent != nullptr)
    {
        m_scriptRunner->SetModuleSource(m_entity->GetName(), GetScriptSource());
    }
}

void EntityScript::SetOwnerId(uint64_t clientId)
{
    if (m_entityScriptComponent != nullptr)
    {
        if (GetOwnerId() != clientId)
        {
            m_entityScriptComponent->SetOwnerId(clientId);
            m_entity->QueueUpdate();
        }
    }
}

uint64_t EntityScript::GetOwnerId() const
{
    if (m_entityScriptComponent != nullptr)
    {
        return m_entityScriptComponent->GetOwnerId();
    }

    return 0;
}

void EntityScript::Shutdown()
{
    m_scriptRunner->ClearModuleSource(m_entity->GetName());
    m_scriptRunner->DestroyContext(m_entity->GetId());
}

void EntityScript::OnSourceChanged(const csp::common::String& inScriptSource)
{
    if (m_logSystem != nullptr)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("OnSourceChanged: {}\n", inScriptSource).c_str());
    }

    if (m_entityScriptComponent != nullptr)
    {
        m_messageMap.clear();
        m_propertyMap.clear();

        m_scriptRunner->ResetContext(m_entity->GetId());
        m_hasBinding = false; // we've reset the context which means this script is no longer bound

        m_scriptRunner->SetModuleSource(m_entity->GetName(), inScriptSource);

        Bind();
    }
}

// Called when an entity has been created
void EntityScript::Bind()
{
    if (m_entityScriptComponent != nullptr)
    {
        m_scriptRunner->BindContext(m_entity->GetId());
        m_hasBinding = true;
    }
}

void EntityScript::CheckBinding()
{
    if (!m_hasBinding)
    {
        Bind();
    }
}

void EntityScript::SubscribeToPropertyChange(int32_t componentId, int32_t propertyKey, csp::common::String message)
{
    PropertyChangeKey key = std::make_pair(componentId, propertyKey);
    PropertyChangeMap::iterator it = m_propertyMap.find(key);

    if (it == m_propertyMap.end())
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::VeryVerbose,
                fmt::format("SubscribeToPropertyChange: ({0}, {1}) {2}\n", componentId, propertyKey, message).c_str());
        }

        m_propertyMap.insert(PropertyChangeMap::value_type(key, message));
    }
}

void EntityScript::OnPropertyChanged(int32_t componentId, int32_t propertyKey)
{
    PropertyChangeKey key = std::make_pair(componentId, propertyKey);
    PropertyChangeMap::iterator it = m_propertyMap.find(key);

    if (it != m_propertyMap.end())
    {
        const csp::common::String& message = it->second;

        // Generate a call to the callback with the correct parameters
        csp::common::String paramJson = csp::common::StringFormat("{\"id\": %d, \"key\": %d}", componentId, propertyKey);

        PostMessageToScript(message, paramJson);
    }
}

void EntityScript::SubscribeToMessage(const csp::common::String message, const csp::common::String onMessageCallback)
{
    SubscribedMessageMap::iterator it = m_messageMap.find(message);

    if (it == m_messageMap.end())
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("SubscribeToMessage: {} -> {}\n", message, onMessageCallback).c_str());
        }

        m_messageMap.insert(SubscribedMessageMap::value_type(message, onMessageCallback));
    }
}

void EntityScript::PostMessageToScript(const csp::common::String message, const csp::common::String messageParamsJson)
{
    SubscribedMessageMap::iterator it = m_messageMap.find(message);

    if (it != m_messageMap.end())
    {
        const csp::common::String& onMessageCallback = it->second;

        // Generate a call to the callback with the correct parameters
        csp::common::String scriptText
            = csp::common::StringFormat("%s('%s','%s')", onMessageCallback.c_str(), message.c_str(), messageParamsJson.c_str());

        if (message != SCRIPT_MSG_ENTITY_TICK)
        {
            if (m_logSystem != nullptr)
            {
                m_logSystem->LogMsg(csp::common::LogLevel::VeryVerbose, fmt::format("PostMessageToScript: {}\n", scriptText).c_str());
            }
        }

        RunScript(scriptText.c_str());
    }
}

} // namespace csp::multiplayer
