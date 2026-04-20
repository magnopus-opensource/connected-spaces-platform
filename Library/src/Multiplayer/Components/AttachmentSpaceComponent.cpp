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

#include "CSP/Multiplayer/Components/AttachmentSpaceComponent.h"

#include "Multiplayer/Script/ComponentBinding/AttachmentSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

AttachmentSpaceComponent::AttachmentSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent)
    : ComponentBase(ComponentType::Attachment, LogSystem, Parent)
{
    Properties[static_cast<uint32_t>(AttachmentPropertyKeys::AnchorPath)] = "";
    Properties[static_cast<uint32_t>(AttachmentPropertyKeys::AttachedPosition)] = csp::common::Vector3::Zero();
    Properties[static_cast<uint32_t>(AttachmentPropertyKeys::AttachedRotation)] = csp::common::Vector4 { 0, 0, 0, 1 };
    Properties[static_cast<uint32_t>(AttachmentPropertyKeys::AttachedScale)] = csp::common::Vector3::One();

    SetScriptInterface(new AttachmentSpaceComponentScriptInterface(this));
}

const csp::common::String& AttachmentSpaceComponent::GetAnchorPath() const
{
    return GetStringProperty(static_cast<uint32_t>(AttachmentPropertyKeys::AnchorPath));
}

void AttachmentSpaceComponent::SetAnchorPath(const csp::common::String& Value)
{
    SetProperty(static_cast<uint32_t>(AttachmentPropertyKeys::AnchorPath), Value);
}

const csp::common::Vector3& AttachmentSpaceComponent::GetAttachedPosition() const
{
    return GetVector3Property(static_cast<uint32_t>(AttachmentPropertyKeys::AttachedPosition));
}

void AttachmentSpaceComponent::SetAttachedPosition(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(AttachmentPropertyKeys::AttachedPosition), Value);
}

const csp::common::Vector4& AttachmentSpaceComponent::GetAttachedRotation() const
{
    return GetVector4Property(static_cast<uint32_t>(AttachmentPropertyKeys::AttachedRotation));
}

void AttachmentSpaceComponent::SetAttachedRotation(const csp::common::Vector4& Value)
{
    SetProperty(static_cast<uint32_t>(AttachmentPropertyKeys::AttachedRotation), Value);
}

const csp::common::Vector3& AttachmentSpaceComponent::GetAttachedScale() const
{
    return GetVector3Property(static_cast<uint32_t>(AttachmentPropertyKeys::AttachedScale));
}

void AttachmentSpaceComponent::SetAttachedScale(const csp::common::Vector3& Value)
{
    SetProperty(static_cast<uint32_t>(AttachmentPropertyKeys::AttachedScale), Value);
}

} // namespace csp::multiplayer
