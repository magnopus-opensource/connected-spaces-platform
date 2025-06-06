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
#pragma once

#include "Multiplayer/Script/ComponentScriptInterface.h"

#include <map>
#include <string>

namespace csp::multiplayer
{

class ConversationSpaceComponent;

class ConversationSpaceComponentScriptInterface : public ComponentScriptInterface
{
public:
    ConversationSpaceComponentScriptInterface(ConversationSpaceComponent* InComponent = nullptr);

    // TODO ASYNC FUNCTIONS

    DECLARE_SCRIPT_PROPERTY(bool, IsVisible);
    DECLARE_SCRIPT_PROPERTY(bool, IsActive);
    DECLARE_SCRIPT_PROPERTY(Vector3, Position);
    DECLARE_SCRIPT_PROPERTY(Vector4, Rotation);
    DECLARE_SCRIPT_PROPERTY(std::string, Title);
    DECLARE_SCRIPT_PROPERTY(bool, Resolved);
    DECLARE_SCRIPT_PROPERTY(Vector3, ConversationCameraPosition);
    DECLARE_SCRIPT_PROPERTY(Vector4, ConversationCameraRotation);
};

} // namespace csp::multiplayer
