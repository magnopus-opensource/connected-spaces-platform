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
#include "Multiplayer/Script/ComponentBinding/ConversationSpaceComponentScriptInterface.h"

#include "CSP/Multiplayer/Components/ConversationSpaceComponent.h"
#include "CSP/Multiplayer/SpaceEntity.h"

using namespace csp::systems;
namespace csp::multiplayer
{

ConversationSpaceComponentScriptInterface::ConversationSpaceComponentScriptInterface(ConversationSpaceComponent* InComponent)
    : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_TYPE(ConversationSpaceComponent, bool, bool, IsVisible);
DEFINE_SCRIPT_PROPERTY_TYPE(ConversationSpaceComponent, bool, bool, IsActive);
DEFINE_SCRIPT_PROPERTY_VEC3(ConversationSpaceComponent, Position);
DEFINE_SCRIPT_PROPERTY_VEC4(ConversationSpaceComponent, Rotation);
DEFINE_SCRIPT_PROPERTY_STRING(ConversationSpaceComponent, Title);
DEFINE_SCRIPT_PROPERTY_STRING(ConversationSpaceComponent, Date);
DEFINE_SCRIPT_PROPERTY_TYPE(ConversationSpaceComponent, int32_t, int32_t, NumberOfReplies);

} // namespace csp::multiplayer
