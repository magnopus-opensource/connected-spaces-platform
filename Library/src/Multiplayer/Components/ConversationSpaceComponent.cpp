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
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "Multiplayer/Script/ComponentBinding/ConversationSpaceComponentScriptInterface.h"
#include "Systems/Conversation/ConversationSystemInternal.h"
#include "Systems/ResultHelpers.h"

// Needs broken
#include "CSP/Systems/SystemsManager.h"

using namespace csp::systems;

namespace csp::multiplayer
{

namespace
{
    bool EnsureValidConversationId(const csp::common::String& ConversationId, csp::common::LogSystem* LogSystem)
    {
        if (ConversationId.IsEmpty())
        {
            if (LogSystem != nullptr)
            {
                LogSystem->LogMsg(csp::common::LogLevel::Log,
                    "This component does not have an associated conversation. "
                    "Call CreateConversation to create a new conversation for this component");
            }
            return false;
        }

        return true;
    }
}

csp::multiplayer::ConversationSpaceComponent::ConversationSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(ComponentType::Conversation, LogSystem, Parent)
{
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::ConversationId)] = "";
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::IsActive)] = true;
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::IsVisible)] = true;
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::Position)] = csp::common::Vector3 { 0, 0, 0 };
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::Rotation)] = csp::common::Vector4 { 0, 0, 0, 1 };
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::Title)] = "";
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::Resolved)] = false;
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraPosition)] = csp::common::Vector3 { 0, 0, 0 };
    Properties[static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraRotation)] = csp::common::Vector4 { 0, 0, 0, 1 };

    SetScriptInterface(new ConversationSpaceComponentScriptInterface(this));
}

void ConversationSpaceComponent::CreateConversation(const csp::common::String& Message, StringResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (!ConversationId.IsEmpty())
    {
        if (LogSystem != nullptr)
        {
            LogSystem->LogMsg(csp::common::LogLevel::Error, "This component already has an associated conversation.");
        }

        if (Callback)
        {
            Callback(MakeInvalid<StringResult>());
        }

        return;
    }

    const auto CreateConversationCallback = [this, Callback](const StringResult& Result)
    {
        // Set this components conversation id from the result.
        SetConversationId(Result.GetValue());
        if (Callback)
        {
            Callback(Result);
        }
    };

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->CreateConversation(Message, CreateConversationCallback);
}

void ConversationSpaceComponent::DeleteConversation(systems::NullResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<NullResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->DeleteConversation(ConversationId, Callback);
}

void ConversationSpaceComponent::AddMessage(const csp::common::String& Message, MessageResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<MessageResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->AddMessage(ConversationId, Message, Callback);
}

void ConversationSpaceComponent::DeleteMessage(const csp::common::String& MessageId, systems::NullResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<MessageResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->DeleteMessage(ConversationId, MessageId, Callback);
}

void ConversationSpaceComponent::GetMessagesFromConversation(
    const csp::common::Optional<int>& ResultsSkipNumber, const csp::common::Optional<int>& ResultsMaxNumber, MessageCollectionResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<MessageCollectionResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->GetMessagesFromConversation(ConversationId, ResultsSkipNumber, ResultsMaxNumber, Callback);
}

void ConversationSpaceComponent::GetConversationInfo(ConversationResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<ConversationResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->GetConversationInfo(ConversationId, Callback);
}

void ConversationSpaceComponent::UpdateConversation(const MessageUpdateParams& NewData, ConversationResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<ConversationResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->UpdateConversation(ConversationId, NewData, Callback);
}

void ConversationSpaceComponent::GetMessageInfo(const csp::common::String& MessageId, MessageResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<MessageResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->GetMessageInfo(ConversationId, MessageId, Callback);
}

void ConversationSpaceComponent::UpdateMessage(
    const csp::common::String& MessageId, const MessageUpdateParams& NewData, MessageResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<MessageResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->UpdateMessage(ConversationId, MessageId, NewData, Callback);
}

void ConversationSpaceComponent::GetNumberOfReplies(NumberOfRepliesResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<NumberOfRepliesResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->GetNumberOfReplies(ConversationId, Callback);
}

void ConversationSpaceComponent::GetConversationAnnotation(multiplayer::AnnotationResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<multiplayer::AnnotationResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->GetConversationAnnotation(ConversationId, Callback);
}

void ConversationSpaceComponent::SetConversationAnnotation(const multiplayer::AnnotationUpdateParams& AnnotationParams,
    const systems::BufferAssetDataSource& Annotation, const systems::BufferAssetDataSource& AnnotationThumbnail,
    multiplayer::AnnotationResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<multiplayer::AnnotationResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->SetConversationAnnotation(ConversationId, AnnotationParams, Annotation, AnnotationThumbnail, Callback);
}

void ConversationSpaceComponent::DeleteConversationAnnotation(systems::NullResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<NullResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->DeleteConversationAnnotation(ConversationId, Callback);
}

void ConversationSpaceComponent::GetAnnotation(const csp::common::String& MessageId, AnnotationResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<multiplayer::AnnotationResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->GetAnnotation(ConversationId, MessageId, Callback);
}

void ConversationSpaceComponent::SetAnnotation(const csp::common::String& MessageId, const multiplayer::AnnotationUpdateParams& UpdateParams,
    const systems::BufferAssetDataSource& Annotation, const systems::BufferAssetDataSource& AnnotationThumbnail,
    multiplayer::AnnotationResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<multiplayer::AnnotationResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->SetAnnotation(ConversationId, MessageId, UpdateParams, Annotation, AnnotationThumbnail, Callback);
}

void ConversationSpaceComponent::DeleteAnnotation(const csp::common::String& MessageId, systems::NullResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<multiplayer::AnnotationResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->DeleteAnnotation(ConversationId, MessageId, Callback);
}

void ConversationSpaceComponent::GetAnnotationThumbnailsForConversation(multiplayer::AnnotationThumbnailCollectionResultCallback Callback)
{
    const common::String& ConversationId = GetConversationId();

    if (EnsureValidConversationId(ConversationId, LogSystem) == false)
    {
        if (Callback)
        {
            Callback(MakeInvalid<multiplayer::AnnotationThumbnailCollectionResult>());
        }
        return;
    }

    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->GetAnnotationThumbnailsForConversation(ConversationId, Callback);
}

void ConversationSpaceComponent::SetConversationUpdateCallback(ConversationUpdateCallbackHandler Callback)
{
    ConversationUpdateCallback = Callback;

    // Flush events now that we have a callback, as we may have events stored for us.
    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->FlushEvents();
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

void ConversationSpaceComponent::SetIsActive(const bool Value) { SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsActive), Value); }

void ConversationSpaceComponent::SetTitle(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Title), Value);
}

const csp::common::String& ConversationSpaceComponent::GetTitle() const
{
    return GetStringProperty(static_cast<uint32_t>(ConversationPropertyKeys::Title));
}

void ConversationSpaceComponent::SetResolved(bool Value) { SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Resolved), Value); }

bool ConversationSpaceComponent::GetResolved() const { return GetBooleanProperty(static_cast<uint32_t>(ConversationPropertyKeys::Resolved)); }

void ConversationSpaceComponent::SetConversationCameraPosition(const csp::common::Vector3& InValue)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraPosition), InValue);
}

const csp::common::Vector3& ConversationSpaceComponent::GetConversationCameraPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraPosition));
}

void ConversationSpaceComponent::SetConversationCameraRotation(const csp::common::Vector4& InValue)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraRotation), InValue);
}

const csp::common::Vector4& ConversationSpaceComponent::GetConversationCameraRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraRotation));
}

void ConversationSpaceComponent::OnCreated()
{
    // Register component to the ConversationSystem to receive conversation events
    // now that the component has been created.
    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->RegisterComponent(this);
}

void ConversationSpaceComponent::OnRemove()
{
    // Deregister component from the ConversationSystem to stop receiving conversation events
    // now that the component has been removed.
    auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
    ConversationSystem->DeregisterComponent(this);
}

void ConversationSpaceComponent::OnLocalDelete()
{
    // The component has been deleted by this client,
    // also delete the conversation
    const auto Callback = [](const NullResult& /*Result*/) {};
    DeleteConversation(Callback);
}

void ConversationSpaceComponent::SetPropertyFromPatch(uint32_t Key, const csp::common::ReplicatedValue& Value)
{
    ComponentBase::SetPropertyFromPatch(Key, Value);

    if (Key == static_cast<uint32_t>(ConversationPropertyKeys::ConversationId) && Value.GetString() != "")
    {
        // If the conversaiton id has been updated, flush the event buffer to send any queued events to this component, because
        // the conversation system looks up the corrosponding events components using this id
        auto* ConversationSystem = SystemsManager::Get().GetConversationSystem();
        ConversationSystem->FlushEvents();
    }
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
