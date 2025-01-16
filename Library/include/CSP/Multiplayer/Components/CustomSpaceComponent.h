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

/// @file CustomSpaceComponent.h
/// @brief Definitions and support for custom components.

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/ComponentBase.h"

namespace csp::multiplayer
{

/// @brief Enumerates the list of properties that can be replicated for a custom component.
enum class CustomComponentPropertyKeys
{
    ApplicationOrigin,
    CustomPropertyList,
    Num
};

/// @ingroup CustomSpaceComponent
/// @brief Can be used to prototype new component types or to support the replication of custom data.
/// @note This class is used to replicate properties of primitive types of properties across multiple platforms.
///       The supported primitive types are the ones supported by the "ReplicatedValue" class.
class CSP_API CustomSpaceComponent : public ComponentBase
{
public:
    /// @brief Constructs the custom space component, and associates it with the specified Parent space entity.
    /// @param Parent The Space entity that owns this component.
    CustomSpaceComponent(SpaceEntity* Parent);

    /// @brief Gets a string that identifies the application origin.
    /// @return The application origin for which this component has been generated.
    const csp::common::String& GetApplicationOrigin() const;

    /// @brief Sets a string that identifies the application origin.
    /// @param Value The application origin for which this component has been generated.
    void SetApplicationOrigin(const csp::common::String& Value);

    /// @brief Checks if the property with the specified Key exists in the list of replicated properties.
    /// @param Key Uniquely identifies the property for which the check is performed.
    /// @return True if the property with the specified Key exists in the list of replicated properties, false otherwise.
    bool HasCustomProperty(const csp::common::String& Key) const;

    /// @brief Retrieves the replicated value of the property identified by the specified Key.
    /// @param Key The ID of the property of which the value will be retrieved.
    /// @return The value of the property identified by the provided Key.
    const ReplicatedValue& GetCustomProperty(const csp::common::String& Key) const;

    /// @brief Sets a custom property by specifying a unique Key and its relative property Value.
    /// @param Key Uniquely identifies this new property.
    /// @param Value The value to store for this new property.
    void SetCustomProperty(const csp::common::String& Key, const ReplicatedValue& Value);

    /// @brief Removes the specified property by Key.
    /// @param Key The ID of the property that will be removed.
    void RemoveCustomProperty(const csp::common::String& Key);

    /// @brief Retrieves the list of all the keys of the properties available in the list of replicated values.
    /// @return The list of available property keys.
    csp::common::List<csp::common::String> GetCustomPropertyKeys() const;

    /// @brief Returns the amount of properties currently stored in the list of replicated values.
    /// @return How many properties are currently stored in the list of replicated values of this component.
    int32_t GetNumProperties() const;

    /// @brief Returns the hash of the provided Key.
    /// @param Key The key for which an hash will be produced.
    /// @return The resulting hash for the provided Key.
    uint32_t GetCustomPropertySubscriptionKey(const csp::common::String& Key) const;

private:
    void AddKey(const csp::common::String& Key);
    void RemoveKey(const csp::common::String& Key);
};

} // namespace csp::multiplayer
