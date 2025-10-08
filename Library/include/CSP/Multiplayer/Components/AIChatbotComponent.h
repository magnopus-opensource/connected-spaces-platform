/*
 * Copyright 2025 Magnopus LLC

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

/// @file AIChatbotComponent.h
/// @brief Definitions and support for AI chatbot components.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Multiplayer/ComponentBase.h"
#include "CSP/Multiplayer/Components/Interfaces/ITransformComponent.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for a AI chatbot component.
enum class AIChatbotPropertyKeys
{
    Position = 0,
    Rotation,
    Scale,
    ContextAssetId,
    GuardrailAssetId,
    VisualState,
    Num
};

/// @brief Enumerates the list of potential visual states that can be replicated for a AI chatbot component.
enum class AIChatbotVisualState
{
    None
};

class CSP_API AIChatbotSpaceComponent : public ComponentBase, public ITransformComponent
{
public:
    AIChatbotSpaceComponent(csp::common::LogSystem* LogSystem, SpaceEntity* Parent);

    /// @brief Gets the ID of the context asset associated with this AI chatbot.
    /// @return The ID of the context asset assicuated with this AI chatbot.
    const csp::common::String& GetContextAssetId() const;

    /// @brief Sets the ID of the context asset associated with this AI chatbot.
    /// @param Value The ID of the context asset assicuated with this AI chatbot.
    void SetContextAssetId(const csp::common::String& Value);

    /// @brief Gets the ID of the guardrail asset associated with this AI chatbot.
    /// @return The ID of the guardrail asset assicuated with this AI chatbot.
    const csp::common::String& GetGuardrailAssetId() const;

    /// @brief Sets the ID of the guardrail asset associated with this AI chatbot.
    /// @param Value The ID of the guardrail asset assicuated with this AI chatbot.
    void SetGuardrailAssetId(const csp::common::String& Value);

    /// @brief Retrieves the visual state of the AI chatbot for this component.
    /// @return The playback state of the AI chatbot.
    AIChatbotVisualState GetVisualState() const;

    /// @brief Sets the visual state of the AI chatbot for this component.
    /// @param Value The visual state of the AI chatbot.
    void SetVisualState(AIChatbotVisualState Value);

    /// \addtogroup ITransformComponent
    /// @{
    /// @copydoc IPositionComponent::GetPosition()
    const csp::common::Vector3& GetPosition() const override;
    /// @copydoc IPositionComponent::SetPosition()
    void SetPosition(const csp::common::Vector3& InValue) override;
    /// @copydoc IRotationComponent::GetRotation()
    const csp::common::Vector4& GetRotation() const override;
    /// @copydoc IRotationComponent::SetRotation()
    void SetRotation(const csp::common::Vector4& InValue) override;
    /// @copydoc IScaleComponent::GetScale()
    const csp::common::Vector3& GetScale() const override;
    /// @copydoc IScaleComponent::SetScale()
    void SetScale(const csp::common::Vector3& InValue) override;
    /// @copydoc ITransformComponent::GetTransform()
    SpaceTransform GetTransform() const override;
    /// @copydoc ITransformComonent::SetTransform()
    void SetTransform(const SpaceTransform& InValue) override;
    /// @}
};

} // namespace csp::multiplayer
