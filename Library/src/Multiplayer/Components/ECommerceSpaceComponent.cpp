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
#include "CSP/Multiplayer/Components/ECommerceSpaceComponent.h"

#include "Debug/Logging.h"
#include "Memory/Memory.h"
#include "Multiplayer/Script/ComponentBinding/ECommerceSpaceComponentScriptInterface.h"

namespace csp::multiplayer
{

ECommerceSpaceComponent::ECommerceSpaceComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::ECommerce, Parent)
{
	Properties[static_cast<uint32_t>(ECommercePropertyKeys::Position)]	= csp::common::Vector3 {0, 0, 0};
	Properties[static_cast<uint32_t>(ECommercePropertyKeys::ProductId)] = "";

	SetScriptInterface(CSP_NEW ECommerceSpaceComponentScriptInterface(this));
}

const csp::common::Vector3& ECommerceSpaceComponent::GetPosition() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(ECommercePropertyKeys::Position));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::Vector3)
	{
		return RepVal.GetVector3();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultVector3();
}

void ECommerceSpaceComponent::SetPosition(const csp::common::Vector3& Value)
{
	SetProperty(static_cast<uint32_t>(ECommercePropertyKeys::Position), Value);
}

csp::common::String ECommerceSpaceComponent::GetProductId() const
{
	if (const auto& RepVal = GetProperty(static_cast<uint32_t>(ECommercePropertyKeys::ProductId));
		RepVal.GetReplicatedValueType() == ReplicatedValueType::String)
	{
		return RepVal.GetString();
	}

	CSP_LOG_ERROR_MSG("Underlying ReplicatedValue not valid");
	return ReplicatedValue::GetDefaultString();
}

void ECommerceSpaceComponent::SetProductId(csp::common::String Value)
{
	SetProperty(static_cast<uint32_t>(ECommercePropertyKeys::ProductId), Value);
}

} // namespace csp::multiplayer
