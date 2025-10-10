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

#include "Multiplayer/Script/ComponentBinding/AIChatbotComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/AIChatbotComponent.h"

namespace csp::multiplayer
{

AIChatbotSpaceComponentScriptInterface::AIChatbotSpaceComponentScriptInterface(AIChatbotSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_VEC3(AIChatbotSpaceComponent, Position);

DEFINE_SCRIPT_PROPERTY_STRING(AIChatbotSpaceComponent, Voice);

DEFINE_SCRIPT_PROPERTY_STRING(AIChatbotSpaceComponent, GuardrailAssetCollectionId);

DEFINE_SCRIPT_PROPERTY_TYPE(AIChatbotSpaceComponent, csp::multiplayer::AIChatbotVisualState, int32_t, VisualState);

} // namespace csp::multiplayer
