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
/// @file LocalScriptSpaceComponent.h
/// @brief Definitions and support for script components.

#pragma once
#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for a script component.
enum class LocalScriptComponentPropertyKeys
{
    ScriptAssetId = 1,
    OwnerId,
    ScriptScope,
    Num
};

/// @ingroup LocalScriptSpaceComponent
/// @brief Data representation of a LocalScriptSpaceComponent.
class CSP_API LocalScriptSpaceComponent : public ComponentBase
{
public:
    /// @brief Constructs the script space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    LocalScriptSpaceComponent(SpaceEntity* Parent);

    /// @brief Gets the ID of the Script asset this Script component refers to.
    /// @return The ID of the Script asset this Script component refers to.
    const csp::common::String& GetScriptAssetId() const;

    /// @brief Sets the ID of the Script asset this Script component refers to.
    /// @param Value The ID of the Script asset this Script component refers to.
    void SetScriptAssetId(const csp::common::String& Value);

    /// @brief Gets the ID of the owner of this script component.
    /// @return The ID of the owner of this script.
    int64_t GetOwnerId() const;

    /// @brief Sets the ID of the owner of this script component.
    /// @param OwnerId The ID of the owner of this script.
    void SetOwnerId(int64_t OwnerId);

protected:
    void SetPropertyFromPatch(uint32_t Key, const ReplicatedValue& Value) override;
    void OnRemove() override;
};

} // namespace csp::multiplayer
