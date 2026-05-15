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

#include "CSP/Multiplayer/ComponentSchema.h"

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "Systems/Conversation/ConversationSystemInternal.h"
#include "Systems/ResultHelpers.h"

// Needs broken
#include "CSP/Systems/SystemsManager.h"

using namespace csp::systems;

namespace csp::multiplayer
{

namespace
{
    bool EnsureValidConversationId(const csp::common::String& conversationId, csp::common::LogSystem* logSystem)
    {
        if (conversationId.IsEmpty())
        {
            if (logSystem != nullptr)
            {
                logSystem->LogMsg(csp::common::LogLevel::Log,
                    "This component does not have an associated conversation. "
                    "Call CreateConversation to create a new conversation for this component");
            }
            return false;
        }

        return true;
    }
}

const auto Schema = ComponentSchema {
    static_cast<ComponentSchema::TypeIdType>(ComponentType::Conversation),
    "Conversation",
    csp::common::Array<ComponentProperty> {
        {
            static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::ConversationId),
            {}, // not exposed to scripting
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::IsActive),
            "isActive",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::IsVisible),
            "isVisible",
            true,
        },
        {
            static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::Position),
            "position",
            csp::common::Vector3 { 0, 0, 0 },
        },
        {
            static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::Rotation),
            "rotation",
            csp::common::Vector4 { 0, 0, 0, 1 },
        },
        {
            static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::Title),
            "title",
            "",
        },
        {
            static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::Resolved),
            "resolved",
            false,
        },
        {
            static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::ConversationCameraPosition),
            "conversationCameraPosition",
            csp::common::Vector3 { 0, 0, 0 },
        },
        {
            static_cast<ComponentProperty::KeyType>(ConversationPropertyKeys::ConversationCameraRotation),
            "conversationCameraRotation",
            csp::common::Vector4 { 0, 0, 0, 1 },
        },
    },
};

const ComponentSchema& ConversationSpaceComponent::GetSchema() { return Schema; }

csp::multiplayer::ConversationSpaceComponent::ConversationSpaceComponent(csp::common::LogSystem* logSystem, SpaceEntity* parent)
    : ComponentBase(Schema, logSystem, parent)
{
}

void ConversationSpaceComponent::CreateConversation(const csp::common::String& message, StringResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (!conversationId.IsEmpty())
    {
        if (m_logSystem != nullptr)
        {
            m_logSystem->LogMsg(csp::common::LogLevel::Error, "This component already has an associated conversation.");
        }

        if (callback)
        {
            callback(MakeInvalid<StringResult>());
        }

        return;
    }

    const auto createConversationCallback = [this, callback](const StringResult& result)
    {
        // Set this components conversation id from the result.
        SetConversationId(result.GetValue());
        if (callback)
        {
            callback(result);
        }
    };

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->CreateConversation(message, createConversationCallback);
}

void ConversationSpaceComponent::DeleteConversation(systems::NullResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<NullResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->DeleteConversation(conversationId, callback);
}

void ConversationSpaceComponent::AddMessage(const csp::common::String& message, MessageResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<MessageResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->AddMessage(conversationId, message, callback);
}

void ConversationSpaceComponent::DeleteMessage(const csp::common::String& messageId, systems::NullResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<MessageResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->DeleteMessage(conversationId, messageId, callback);
}

void ConversationSpaceComponent::GetMessagesFromConversation(
    const csp::common::Optional<int>& resultsSkipNumber, const csp::common::Optional<int>& resultsMaxNumber, MessageCollectionResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<MessageCollectionResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->GetMessagesFromConversation(conversationId, resultsSkipNumber, resultsMaxNumber, callback);
}

void ConversationSpaceComponent::GetConversationInfo(ConversationResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<ConversationResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->GetConversationInfo(conversationId, callback);
}

void ConversationSpaceComponent::UpdateConversation(const MessageUpdateParams& newData, ConversationResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<ConversationResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->UpdateConversation(conversationId, newData, callback);
}

void ConversationSpaceComponent::GetMessageInfo(const csp::common::String& messageId, MessageResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<MessageResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->GetMessageInfo(conversationId, messageId, callback);
}

void ConversationSpaceComponent::UpdateMessage(
    const csp::common::String& messageId, const MessageUpdateParams& newData, MessageResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<MessageResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->UpdateMessage(conversationId, messageId, newData, callback);
}

void ConversationSpaceComponent::GetNumberOfReplies(NumberOfRepliesResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<NumberOfRepliesResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->GetNumberOfReplies(conversationId, callback);
}

void ConversationSpaceComponent::GetConversationAnnotation(multiplayer::AnnotationResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<multiplayer::AnnotationResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->GetConversationAnnotation(conversationId, callback);
}

void ConversationSpaceComponent::SetConversationAnnotation(const multiplayer::AnnotationUpdateParams& annotationParams,
    const systems::BufferAssetDataSource& annotation, const systems::BufferAssetDataSource& annotationThumbnail,
    multiplayer::AnnotationResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<multiplayer::AnnotationResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->SetConversationAnnotation(conversationId, annotationParams, annotation, annotationThumbnail, callback);
}

void ConversationSpaceComponent::DeleteConversationAnnotation(systems::NullResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<NullResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->DeleteConversationAnnotation(conversationId, callback);
}

void ConversationSpaceComponent::GetAnnotation(const csp::common::String& messageId, AnnotationResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<multiplayer::AnnotationResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->GetAnnotation(conversationId, messageId, callback);
}

void ConversationSpaceComponent::SetAnnotation(const csp::common::String& messageId, const multiplayer::AnnotationUpdateParams& updateParams,
    const systems::BufferAssetDataSource& annotation, const systems::BufferAssetDataSource& annotationThumbnail,
    multiplayer::AnnotationResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<multiplayer::AnnotationResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->SetAnnotation(conversationId, messageId, updateParams, annotation, annotationThumbnail, callback);
}

void ConversationSpaceComponent::DeleteAnnotation(const csp::common::String& messageId, systems::NullResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<multiplayer::AnnotationResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->DeleteAnnotation(conversationId, messageId, callback);
}

void ConversationSpaceComponent::GetAnnotationThumbnailsForConversation(multiplayer::AnnotationThumbnailCollectionResultCallback callback)
{
    const common::String& conversationId = GetConversationId();

    if (EnsureValidConversationId(conversationId, m_logSystem) == false)
    {
        if (callback)
        {
            callback(MakeInvalid<multiplayer::AnnotationThumbnailCollectionResult>());
        }
        return;
    }

    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->GetAnnotationThumbnailsForConversation(conversationId, callback);
}

void ConversationSpaceComponent::SetConversationUpdateCallback(ConversationUpdateCallbackHandler callback)
{
    m_conversationUpdateCallback = callback;

    // Flush events now that we have a callback, as we may have events stored for us.
    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->FlushEvents();
}

bool ConversationSpaceComponent::GetIsVisible() const { return GetBooleanProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsVisible)); }

void ConversationSpaceComponent::SetIsVisible(const bool value) { SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsVisible), value); }

bool ConversationSpaceComponent::GetIsActive() const { return GetBooleanProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsActive)); }

/* IPositionComponent */

const csp::common::Vector3& ConversationSpaceComponent::GetPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ConversationPropertyKeys::Position));
}

void ConversationSpaceComponent::SetPosition(const csp::common::Vector3& value)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Position), value);
}

/* IRotationComponent */

const csp::common::Vector4& ConversationSpaceComponent::GetRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(ConversationPropertyKeys::Rotation));
}

void ConversationSpaceComponent::SetRotation(const csp::common::Vector4& value)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Rotation), value);
}

void ConversationSpaceComponent::SetIsActive(const bool value) { SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::IsActive), value); }

void ConversationSpaceComponent::SetTitle(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Title), value);
}

const csp::common::String& ConversationSpaceComponent::GetTitle() const
{
    return GetStringProperty(static_cast<uint32_t>(ConversationPropertyKeys::Title));
}

void ConversationSpaceComponent::SetResolved(bool value) { SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::Resolved), value); }

bool ConversationSpaceComponent::GetResolved() const { return GetBooleanProperty(static_cast<uint32_t>(ConversationPropertyKeys::Resolved)); }

void ConversationSpaceComponent::SetConversationCameraPosition(const csp::common::Vector3& inValue)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraPosition), inValue);
}

const csp::common::Vector3& ConversationSpaceComponent::GetConversationCameraPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraPosition));
}

void ConversationSpaceComponent::SetConversationCameraRotation(const csp::common::Vector4& inValue)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraRotation), inValue);
}

const csp::common::Vector4& ConversationSpaceComponent::GetConversationCameraRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(ConversationPropertyKeys::ConversationCameraRotation));
}

void ConversationSpaceComponent::OnCreated()
{
    // Register component to the ConversationSystem to receive conversation events
    // now that the component has been created.
    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->RegisterComponent(this);
}

void ConversationSpaceComponent::OnRemove()
{
    // Deregister component from the ConversationSystem to stop receiving conversation events
    // now that the component has been removed.
    auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
    conversationSystem->DeregisterComponent(this);
}

void ConversationSpaceComponent::OnLocalDelete()
{
    // The component has been deleted by this client,
    // also delete the conversation
    const auto callback = [](const NullResult& /*Result*/) {};
    DeleteConversation(callback);
}

void ConversationSpaceComponent::SetPropertyFromPatch(uint32_t key, const csp::common::ReplicatedValue& value)
{
    ComponentBase::SetPropertyFromPatch(key, value);

    if (key == static_cast<uint32_t>(ConversationPropertyKeys::ConversationId) && value.GetString() != "")
    {
        // If the conversaiton id has been updated, flush the event buffer to send any queued events to this component, because
        // the conversation system looks up the corrosponding events components using this id
        auto* conversationSystem = SystemsManager::Get().GetConversationSystem();
        conversationSystem->FlushEvents();
    }
}

void ConversationSpaceComponent::SetConversationId(const csp::common::String& value)
{
    SetProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationId), value);
}

void ConversationSpaceComponent::RemoveConversationId() { RemoveProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationId)); }

const csp::common::String& ConversationSpaceComponent::GetConversationId() const
{
    return GetStringProperty(static_cast<uint32_t>(ConversationPropertyKeys::ConversationId));
}

} // namespace csp::multiplayer
