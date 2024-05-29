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
#include "CSP/Multiplayer/Components/TextSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/TextSpaceComponentScriptInterface.h"


namespace csp::multiplayer
{

TextSpaceComponent::TextSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::Image, Parent)
{
	Properties[static_cast<uint32_t>(TextPropertyKeys::Position)]			 = csp::common::Vector3::Zero();
	Properties[static_cast<uint32_t>(TextPropertyKeys::Rotation)]			 = csp::common::Vector4 {0, 0, 0, 1};
	Properties[static_cast<uint32_t>(TextPropertyKeys::Scale)]				 = csp::common::Vector3::One();
	Properties[static_cast<uint32_t>(TextPropertyKeys::Text)]				 = "";
	Properties[static_cast<uint32_t>(TextPropertyKeys::TextColor)]			 = csp::common::Vector3::Zero();
	Properties[static_cast<uint32_t>(TextPropertyKeys::BackgroundColor)]	 = csp::common::Vector3::Zero();
	Properties[static_cast<uint32_t>(TextPropertyKeys::IsBackgroundVisible)] = true;
	Properties[static_cast<uint32_t>(TextPropertyKeys::Width)]				 = 1.0f;
	Properties[static_cast<uint32_t>(TextPropertyKeys::Height)]				 = 1.0f;
	Properties[static_cast<uint32_t>(TextPropertyKeys::BillboardMode)]		 = static_cast<int64_t>(TextSpaceComponentBillboardMode::Off);
	Properties[static_cast<uint32_t>(TextPropertyKeys::IsVisible)]			 = false;
	Properties[static_cast<uint32_t>(TextPropertyKeys::IsARVisible)]		 = false;

	SetScriptInterface(CSP_NEW TextSpaceComponentScriptInterface(this));
}


/* ITransformComponent */

const csp::common::String& TextSpaceComponent::GetText() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(TextPropertyKeys::Text));
	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}
	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultString();
}

void TextSpaceComponent::SetText(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(TextPropertyKeys::Text), Value);
}

const csp::common::Vector3& TextSpaceComponent::GetTextColor() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(TextPropertyKeys::TextColor));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void TextSpaceComponent::SetTextColor(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(TextPropertyKeys::TextColor), Value);
}

const csp::common::Vector3& TextSpaceComponent::GetBackgroundColor() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(TextPropertyKeys::BackgroundColor));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void TextSpaceComponent::SetBackgroundColor(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(TextPropertyKeys::BackgroundColor), Value);
}

bool TextSpaceComponent::GetIsBackgroundVisible() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(TextPropertyKeys::IsBackgroundVisible));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void TextSpaceComponent::SetIsBackgroundVisible(float InValue)
{
	SetProperty(static_cast<uint32_t>(TextPropertyKeys::IsBackgroundVisible), InValue);
}

float TextSpaceComponent::GetWidth() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(TextPropertyKeys::Width));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Float)
	{
		return RepVal.GetFloat();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0.0f;
}

void TextSpaceComponent::SetWidth(float InValue)
{
	SetProperty(static_cast<uint32_t>(TextPropertyKeys::Width), InValue);
}

float TextSpaceComponent::GetHeight() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(TextPropertyKeys::Height));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Float)
	{
		return RepVal.GetFloat();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return 0.0f;
}

void TextSpaceComponent::SetHeight(bool InValue)
{
	SetProperty(static_cast<uint32_t>(TextPropertyKeys::Height), InValue);
}

const csp::common::Vector3& TextSpaceComponent::GetPosition() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(TextPropertyKeys::Position));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void TextSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(TextPropertyKeys::Position), Value);
}


const csp::common::Vector4& TextSpaceComponent::GetRotation() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(TextPropertyKeys::Rotation));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector4)
	{
		return RepVal.GetVector4();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector4();
}

void TextSpaceComponent::SetRotation(const csp::common::Vector4& Value)
{
	SetProperty(static_cast<uint32_t>(TextPropertyKeys::Rotation), Value);
}


const csp::common::Vector3& TextSpaceComponent::GetScale() const
{
	const auto& RepVal = GetProperty(static_cast<uint32_t>(TextPropertyKeys::Scale));

	if (RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");

	return ReplicatedValue::GetDefaultVector3();
}

void TextSpaceComponent::SetScale(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(TextPropertyKeys::Scale), Value);
}


SpaceTransform TextSpaceComponent::GetTransform() const
{
	SpaceTransform Transform;
	Transform.Position = GetPosition();
	Transform.Rotation = GetRotation();
	Transform.Scale	   = GetScale();

	return Transform;
}

void TextSpaceComponent::SetTransform(const SpaceTransform& InValue)
{
	SetPosition(InValue.Position);
	SetRotation(InValue.Rotation);
	SetScale(InValue.Scale);
}


/* IVisibleComponent */

bool TextSpaceComponent::GetIsVisible() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(TextPropertyKeys::IsVisible));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void TextSpaceComponent::SetIsVisible(bool Value)
{
	SetProperty(static_cast<uint32_t>(TextPropertyKeys::IsVisible), Value);
}

bool TextSpaceComponent::GetIsARVisible() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(TextPropertyKeys::IsARVisible));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Boolean)
	{
		return RepVal.GetBool();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return false;
}

void TextSpaceComponent::SetIsARVisible(bool Value)
{
	SetProperty(static_cast<uint32_t>(TextPropertyKeys::IsARVisible), Value);
}

TextSpaceComponentBillboardMode TextSpaceComponent::GetBillboardMode() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(TextPropertyKeys::BillboardMode));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Integer)
	{
		return static_cast<TextSpaceComponentBillboardMode>(RepVal.GetInt());
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return TextSpaceComponentBillboardMode::Off;
}

void TextSpaceComponent::SetBillboardMode(TextSpaceComponentBillboardMode Value)
{
	SetProperty(static_cast<uint32_t>(TextPropertyKeys::BillboardMode), static_cast<int64_t>(Value));
}
} // namespace csp::multiplayer
