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
#include "CSP/Multiplayer/LocalScript/LocalScriptSystem.h"

#include "CSP/Common/StringFormat.h"
#include "CSP/Multiplayer/Components/ScriptSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/Script/localscript-libs/eventbus.h"
#include "CSP/Systems/SystemsManager.h"
#include "Debug/Logging.h"
#include "quickjspp.hpp"


namespace csp::multiplayer
{

constexpr const char* SCRIPT_ERROR_NO_COMPONENT = "No script component";
constexpr const char* SCRIPT_ERROR_EMPTY_SCRIPT = "Script is empty";

LocalScriptSystem::LocalScriptSystem(SpaceEntity* InEntity, SpaceEntitySystem* InSpaceEntitySystem)
    : ScriptSystem(csp::systems::SystemsManager::Get().GetScriptSystem())
    , Entity(InEntity)
    , LocalScriptSystemComponent(nullptr)
    , HasLastError(false)
    , HasBinding(false)
    , SpaceEntitySystemPtr(InSpaceEntitySystem)
{
}

LocalScriptSystem::~LocalScriptSystem() { Shutdown(); }

bool LocalScriptSystem::Invoke()
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "LocalScriptSystem::Invoke called for %s", Entity->GetName().c_str());

    CheckBinding();

    HasLastError = false;
    LastError = "Unknown Error";

    if (LocalScriptSystemComponent == nullptr)
    {
        HasLastError = true;
        LastError = SCRIPT_ERROR_NO_COMPONENT;
    }
    else
    {
        const csp::common::String& ScriptSource = LocalScriptSystemComponent->GetScriptSource();

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

void LocalScriptSystem::RunScript(const csp::common::String& ScriptSource)
{
    ScriptSystem->RunScript(Entity->GetId(), ScriptSource);
}

void LocalScriptSystem::SetScriptSource(const csp::common::String& InScriptSource)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "LocalScriptSystem::SetScriptSource called for %s\nSource:", Entity->GetName().c_str(),
        InScriptSource.c_str());
    CSP_LOG_MSG(csp::systems::LogLevel::VeryVerbose, "--EndScriptSource--");

    if (LocalScriptSystemComponent == nullptr)
    {
        LocalScriptSystemComponent = (ScriptSpaceComponent*)(Entity->AddComponent(ComponentType::ScriptData));
    }

    LocalScriptSystemComponent->SetScriptSource(InScriptSource);

    Entity->MarkForUpdate();
}

bool LocalScriptSystem::HasError() { return HasLastError; }

csp::common::String LocalScriptSystem::GetErrorText() { return LastError; }

void LocalScriptSystem::SetScriptSpaceComponent(ScriptSpaceComponent* InEnityScriptComponent)
{
    LocalScriptSystemComponent = InEnityScriptComponent;
    ScriptSystem->CreateContext(Entity->GetId());
    qjs::Context* context = (qjs::Context*) ScriptSystem->GetContext(Entity->GetId());
    qjs::Context::Module* Module = (qjs::Context::Module*)ScriptSystem->GetModule(Entity->GetId(), "CSP");

    auto Fn = [this](const char* Str)
    {
        ScriptSystem->FireLocalScriptCommand(Str);
    };
 
    try {
       Module->function("sendMessage", Fn);
    } catch (const std::exception& e) {
        CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "LocalScriptSystem::Exception called for %s", e.what());
    }
    
    qjs::Context::Module* CommonModule = (qjs::Context::Module*)ScriptSystem->GetModule(Entity->GetId(), "common");
    ScriptSystem->SetModuleSource("common", csp::localscripts::EventBusScript.c_str());
    JSValue module = context->eval(csp::localscripts::EventBusScript.c_str(), "common", JS_EVAL_TYPE_MODULE);
    if (JS_IsException(module)) {
    // Handle error
            CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "LocalScriptSystem::Exception called%s","");

    }
}

csp::common::String LocalScriptSystem::GetScriptSource()
{
    if (LocalScriptSystemComponent == nullptr)
    {
        return csp::common::String();
    }

    return LocalScriptSystemComponent->GetScriptSource();
}

void LocalScriptSystem::RegisterSourceAsModule()
{
    if (LocalScriptSystemComponent != nullptr)
    {
        ScriptSystem->SetModuleSource(Entity->GetName(), GetScriptSource());
    }
}

void LocalScriptSystem::SetOwnerId(uint64_t ClientId)
{
    if (LocalScriptSystemComponent != nullptr)
    {
        if (GetOwnerId() != ClientId)
        {
            LocalScriptSystemComponent->SetOwnerId(ClientId);
            Entity->MarkForUpdate();
        }
    }
}

uint64_t LocalScriptSystem::GetOwnerId() const
{
    if (LocalScriptSystemComponent != nullptr)
    {
        return LocalScriptSystemComponent->GetOwnerId();
    }

    return 0;
}

void LocalScriptSystem::Shutdown()
{
    ScriptSystem->ClearModuleSource(Entity->GetName());
    ScriptSystem->DestroyContext(Entity->GetId());
}

void LocalScriptSystem::OnSourceChanged(const csp::common::String& InScriptSource)
{
    CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "OnSourceChanged: %s\n", InScriptSource.c_str());

    if (LocalScriptSystemComponent != nullptr)
    {
        MessageMap.clear();
        PropertyMap.clear();

        ScriptSystem->ResetContext(Entity->GetId());
        HasBinding = false; // we've reset the context which means this script is no longer bound
        
        ScriptSystem->SetModuleSource(Entity->GetName(), InScriptSource); 
        qjs::Context* context = (qjs::Context*) ScriptSystem->GetContext(Entity->GetId());
        qjs::Context::Module* Module = (qjs::Context::Module*)ScriptSystem->GetModule(Entity->GetId(), "CSP");

        auto Fn = [this](const char* Str)
        {
            ScriptSystem->FireLocalScriptCommand(Str);
        };
    
        try {
            Module->function("sendMessage", Fn);
        } catch (const std::exception& e) {
            CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "LocalScriptSystem::Exception called for %s", e.what());
        }
        
        qjs::Context::Module* CommonModule = (qjs::Context::Module*)ScriptSystem->GetModule(Entity->GetId(), "common");
        ScriptSystem->SetModuleSource("common", csp::localscripts::EventBusScript.c_str());
        JSValue module = context->eval(csp::localscripts::EventBusScript.c_str(), "common", JS_EVAL_TYPE_MODULE);
        if (JS_IsException(module))
        {
            // Handle error
            CSP_LOG_FORMAT(csp::systems::LogLevel::Error, "LocalScriptSystem::Exception called%s","");
        }
        Bind();
    }
}

// Called when an entity has been created
void LocalScriptSystem::Bind()
{
    if (LocalScriptSystemComponent != nullptr)
    {
        ScriptSystem->BindContext(Entity->GetId());
        HasBinding = true;
    }
}

void LocalScriptSystem::CheckBinding()
{
    if (!HasBinding)
    {
        Bind();
    }
}

void LocalScriptSystem::SubscribeToPropertyChange(int32_t ComponentId, int32_t PropertyKey, csp::common::String Message)
{
    PropertyChangeKey Key = std::make_pair(ComponentId, PropertyKey);
    PropertyChangeMap::iterator It = PropertyMap.find(Key);

    if (It == PropertyMap.end())
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "SubscribeToPropertyChange: (%d, %d) %s\n", ComponentId, PropertyKey, Message.c_str());

        PropertyMap.insert(PropertyChangeMap::value_type(Key, Message));
    }
}

void LocalScriptSystem::OnPropertyChanged(int32_t ComponentId, int32_t PropertyKey)
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

void LocalScriptSystem::SubscribeToMessage(const csp::common::String Message, const csp::common::String OnMessageCallback)
{
    SubscribedMessageMap::iterator It = MessageMap.find(Message);

    if (It == MessageMap.end())
    {
        CSP_LOG_FORMAT(csp::systems::LogLevel::VeryVerbose, "SubscribeToMessage: %s -> %s\n", Message.c_str(), OnMessageCallback.c_str());

        MessageMap.insert(SubscribedMessageMap::value_type(Message, OnMessageCallback));
    }
}

void LocalScriptSystem::PostMessageToScript(const csp::common::String Message, const csp::common::String MessageParamsJson)
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
