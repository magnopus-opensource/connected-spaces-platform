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

#include "CSP/Multiplayer/Components/ConversationSpaceComponent.h"

#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "CSP/Web/HTTPResponseCodes.h"
#include "CallHelpers.h"
#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/ConversationSpaceComponentScriptInterface.h"
#include "Systems/ResultHelpers.h"

#include <msgpack/v1/object_fwd_decl.hpp>

using namespace csp::systems;

namespace csp::multiplayer
{

csp::multiplayer::ConversationSpaceComponent::ConversationSpaceComponent(SpaceEntity* Parent)
    : ComponentBase(ComponentType::Conversation, Parent)
    , ConversationSystem(csp::systems::SystemsManager::Get().GetMultiplayerConnection()->GetConversationSystem())
{
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::ConversationId)] = "";
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::IsActive)] = true;
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::IsVisible)] = true;
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::Position)] = csp::common::Vector3 { 0, 0, 0 };
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::Rotation)] = csp::common::Vector4 { 0, 0, 0, 1 };
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::Title)] = "";
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::Date)] = "";
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::NumberOfReplies)] = static_cast<int64_t>(0);

    SetScriptInterface(CSP_NEW ConversationSpaceComponentScriptInterface(this));
}

void ConversationSpaceComponent::CreateConversation(const csp::common::String& Message, StringResultCallback Callback)
{
    if (!GetConversationId().IsEmpty())
    {
        CSP_LOG_WARN_MSG("This component already has an associated conversation! No new conversation was created as a result.");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<StringResult>());

        return;
    }

    const StringResultCallback CreateConversationIdCallback = [=](const StringResult& StringResult)
    {
        if (StringResult.GetResultCode() == csp::systems::EResultCode::Success)
        {
            SetConversationId(StringResult.GetValue());
        }

        INVOKE_IF_NOT_NULL(Callback, StringResult);
    };

    ConversationSystem->CreateConversation(Message, CreateConversationIdCallback);
}

bool ConversationSpaceComponent::MoveConversationFromComponent(ConversationSpaceComponent& OtherConversationComponent)
{
    if (!GetConversationId().IsEmpty())
    {
        CSP_LOG_WARN_MSG("This component already has an associated conversation! The conversation was not moved as a result.");

        return false;
    }

    SetConversationId(OtherConversationComponent.GetConversationId());
    OtherConversationComponent.RemoveConversationId();

    return true;
}

void ConversationSpaceComponent::DeleteConversation(csp::systems::NullResultCallback Callback)
{
    if (GetConversationId().IsEmpty())
    {
        CSP_LOG_ERROR_MSG("The conversation ID passed to DeleteConversation was empty! No update to the conversation was issued as a result.");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

        return;
    }

    ConversationSystem->DeleteConversation(GetConversationId(), Callback);
}

void ConversationSpaceComponent::AddMessage(const csp::common::String& Message, MessageResultCallback Callback)
{
    if (GetConversationId().IsEmpty())
    {
        CSP_LOG_ERROR_MSG("The conversation ID passed to AddMessage was empty! No update to the conversation was issued as a result.");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MessageResult>());

        return;
    }

    csp::systems::ProfileResultCallback GetProfileCallback = [=](const csp::systems::ProfileResult& GetProfileResult)
    {
        if (GetProfileResult.GetResultCode() == csp::systems::EResultCode::InProgress)
        {
            return;
        }

        if (GetProfileResult.GetResultCode() == csp::systems::EResultCode::Failed)
        {
            const MessageResult InternalResult(GetProfileResult.GetResultCode(), GetProfileResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        this->ConversationSystem->AddMessageToConversation(GetConversationId(), GetProfileResult.GetProfile().DisplayName, Message, Callback);
    };

    auto* UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();
    UserSystem->GetProfileByUserId(UserSystem->GetLoginState().UserId, GetProfileCallback);
}

void ConversationSpaceComponent::GetMessage(const csp::common::String& MessageId, MessageResultCallback Callback)
{
    ConversationSystem->GetMessage(MessageId, Callback);
}

void ConversationSpaceComponent::GetAllMessages(MessageCollectionResultCallback Callback)
{
    if (GetConversationId().IsEmpty())
    {
        CSP_LOG_ERROR_MSG("The conversation ID passed to GetAllMessages was empty! No update to the conversation was issued as a result.");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<MessageCollectionResult>());

        return;
    }

    ConversationSystem->GetMessagesFromConversation(GetConversationId(), nullptr, nullptr, Callback);
}

void ConversationSpaceComponent::DeleteMessage(const csp::common::String& MessageId, NullResultCallback Callback)
{
    ConversationSystem->DeleteMessage(MessageId, Callback);
}

bool ConversationSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsVisible)); }

void ConversationSpaceComponent::SetIsVisible(const bool Value) { SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsVisible), Value); }

bool ConversationSpaceComponent::GetIsActive() const { return GetBooleanProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsActive)); }

/* IPositionComponent */

const csp::common::Vector3& ConversationSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ConversationPropertyKeys::Position));
}

void ConversationSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Position), Value);
}

/* IRotationComponent */

const csp::common::Vector4& ConversationSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(ConversationPropertyKeys::Rotation));
}

void ConversationSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Rotation), Value);
}

void ConversationSpaceComponent::GetConversationInfo(ConversationResultCallback Callback)
{
    if (GetConversationId().IsEmpty())
    {
        CSP_LOG_ERROR_MSG("This component does not have an associated conversation.");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<ConversationResult>());

        return;
    }

    ConversationSystem->GetConversationInformation(GetConversationId(), Callback);
}

void ConversationSpaceComponent::SetConversationInfo(const ConversationInfo& ConversationData, ConversationResultCallback Callback)
{
    if (GetConversationId().IsEmpty())
    {
        CSP_LOG_ERROR_MSG("This component does not have an associated conversation.");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<ConversationResult>());

        return;
    }

    ConversationSystem->SetConversationInformation(GetConversationId(), ConversationData, Callback);
}

void ConversationSpaceComponent::GetMessageInfo(const csp::common::String& MessageId, MessageResultCallback Callback)
{
    ConversationSystem->GetMessageInformation(MessageId, Callback);
}

void ConversationSpaceComponent::SetMessageInfo(const csp::common::String& MessageId, const MessageInfo& MessageData, MessageResultCallback Callback)
{
    ConversationSystem->SetMessageInformation(MessageId, MessageData, Callback);
}

void ConversationSpaceComponent::SetIsActive(const bool Value) { SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsActive), Value); }

void ConversationSpaceComponent::SetTitle(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Title), Value);
}

const csp::common::String& ConversationSpaceComponent::GetTitle() const
{
    return GetStringProperty(static_cast<uint32_t>(ConversationPropertyKeys::Title));
}

void ConversationSpaceComponent::SetDate(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Date), Value);
}

const csp::common::String& ConversationSpaceComponent::GetDate() const
{
    return GetStringProperty(static_cast<uint32_t>(ConversationPropertyKeys::Date));
}

void ConversationSpaceComponent::SetNumberOfReplies(const int64_t Value)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::NumberOfReplies), Value);
}

const int64_t ConversationSpaceComponent::GetNumberOfReplies() const
{
    return GetIntegerProperty(static_cast<uint32_t>(ConversationPropertyKeys::NumberOfReplies));
}

void ConversationSpaceComponent::SetConversationId(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationId), Value);
}

void ConversationSpaceComponent::RemoveConversationId() { RemoveProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationId)); }

const csp::common::String& ConversationSpaceComponent::GetConversationId() const
{
    return GetStringProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationId));
}

} // namespace csp::multiplayer
