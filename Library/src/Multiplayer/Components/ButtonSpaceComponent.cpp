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
#include "CSP/Multiplayer/Components/ButtonSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/ButtonSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

ButtonSpaceComponent::ButtonSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::Button, Parent)
{
	Properties[static_cast<uint32_t>(ButtonPropertyKeys::LabelText)]		 = "";
	Properties[static_cast<uint32_t>(ButtonPropertyKeys::IconAssetId)]		 = "";
	Properties[static_cast<uint32_t>(ButtonPropertyKeys::AssetCollectionId)] = "";
	Properties[static_cast<uint32_t>(ButtonPropertyKeys::Position)]			 = csp::common::Vector3 {0, 0, 0};
	Properties[static_cast<uint32_t>(ButtonPropertyKeys::Rotation)]			 = csp::common::Vector4 {0, 0, 0, 1};
	Properties[static_cast<uint32_t>(ButtonPropertyKeys::Scale)]			 = csp::common::Vector3 {1, 1, 1};
	Properties[static_cast<uint32_t>(ButtonPropertyKeys::IsVisible)]		 = true;
	Properties[static_cast<uint32_t>(ButtonPropertyKeys::IsEnabled)]		 = true;
	Properties[static_cast<uint32_t>(ButtonPropertyKeys::IsARVisible)]		 = true;

	SetScriptInterface(CSP_NEW ButtonSpaceComponentScriptInterface(this));
}

const csp::common::String& ButtonSpaceComponent::GetLabelText() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(ButtonPropertyKeys::LabelText));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void ButtonSpaceComponent::SetLabelText(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::LabelText), Value);
}

const csp::common::String& ButtonSpaceComponent::GetIconAssetId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IconAssetId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void ButtonSpaceComponent::SetIconAssetId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IconAssetId), Value);
}

const csp::common::String& ButtonSpaceComponent::GetAssetCollectionId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(ButtonPropertyKeys::AssetCollectionId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void ButtonSpaceComponent::SetAssetCollectionId(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::AssetCollectionId), Value);
}

const csp::common::Vector3& ButtonSpaceComponent::GetPosition() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(ButtonPropertyKeys::Position));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultVector3();
}

void ButtonSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::Position), Value);
}

const csp::common::Vector4& ButtonSpaceComponent::GetRotation() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(ButtonPropertyKeys::Rotation));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector4)
	{
		return RepVal.GetVector4();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultVector4();
}

void ButtonSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
	SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::Rotation), Value);
}

const csp::common::Vector3& ButtonSpaceComponent::GetScale() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(ButtonPropertyKeys::Scale));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultVector3();
}

void ButtonSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::Scale), Value);
}

/* IClickableComponent */

bool ButtonSpaceComponent::GetIsEnabled() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsEnabled));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void ButtonSpaceComponent::SetIsEnabled(bool Value)
{
	SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsEnabled), Value);
}

/* IVisibleComponent */

bool ButtonSpaceComponent::GetIsVisible() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsVisible));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void ButtonSpaceComponent::SetIsVisible(bool Value)
{
	SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsVisible), Value);
}

bool ButtonSpaceComponent::GetIsARVisible() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsARVisible));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void ButtonSpaceComponent::SetIsARVisible(bool Value)
{
	SetProperty(static_cast<uint32_t>(ButtonPropertyKeys::IsARVisible), Value);
}

} // namespace csp::multiplayer
