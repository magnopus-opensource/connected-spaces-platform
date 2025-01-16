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

#include "CSP/Common/String.h"

#include <map>
#include <string>

namespace csp::systems
{
class ScriptSystem;
}

/// @brief Namespace that encompasses everything in the multiplayer system
namespace csp::multiplayer
{
class SpaceEntitySystem;

class SpaceEntity;
class ScriptSpaceComponent;

/// @brief Manages the script attached to an Entity.
///
/// Provides functions for setting the script source, subscribing to property changes and messages and other script management.
class CSP_API EntityScript
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SpaceEntity;
    /** @endcond */
    CSP_END_IGNORE

public:
    /// @brief Destroy the instance of EntityScript.
    ~EntityScript();

    /// @brief Sets the source code for the script.
    /// @param ScriptSource csp::common::String : The source as a string.
    void SetScriptSource(const csp::common::String& ScriptSource);

    /// @brief Runs the script.
    /// @return True if the script runs successfully or false if there are errors.
    bool Invoke();

    /// @brief Run a script with the given source rather than the stored source.
    ///
    /// Will be run locally or remotely depending on leader election.
    ///
    /// @param ScriptSource csp::common::String : The source to use.
    void RunScript(const csp::common::String& ScriptSource);

    /// @brief Checks if there was an error with the last script invocation.
    /// @return True if there was an error, false otherwise.
    bool HasError();

    /// @brief Gets the text of the last error if it is known or otherwise returns a default unknown error string.
    /// @return Text of the last error.
    csp::common::String GetErrorText();

    /// @brief Gets the stored script source code.
    /// @return The source as a string.
    csp::common::String GetScriptSource();

    /// @brief Sets the related component for this script.
    /// @param InEnityScriptComponent ScriptSpaceComponent : The component related to this script.
    void SetScriptSpaceComponent(ScriptSpaceComponent* InEnityScriptComponent);

    /// @brief Called when a component property changes so that a message can be passed to the script if a subscription has been setup.
    /// @param ComponentId int32_t : ID of the component that changed.
    /// @param PropertyKey int32_t : Key of the property that changed.
    void OnPropertyChanged(int32_t ComponentId, int32_t PropertyKey);

    // Script Binding Interface
    /// @brief Sets up a subscription where the given message is posted to the script when a change is made to the specified component property.
    /// @param ComponentId int32_t : The ID of the component that the property belongs to.
    /// @param PropertyKey int32_t : The key of the component property to subscribe to changes of.
    /// @param Message csp::common::String : The message that will be posted to the script when a change occurs to the property.
    CSP_NO_EXPORT void SubscribeToPropertyChange(int32_t ComponentId, int32_t PropertyKey, csp::common::String Message);

    /// @brief Sets up a subscription where the given callback in the script will be run when given message is posted to the script.
    /// @param Message csp::common::String : The message to subscribe to.
    /// @param OnMessageCallback csp::common::String : The callback that will be run in the script.
    CSP_NO_EXPORT void SubscribeToMessage(const csp::common::String Message, const csp::common::String OnMessageCallback);

    /// @brief Runs the callback associated with the given message, if a subscription has been setup, and passes the given params.
    /// @param Message csp::common::String : The message to use.
    /// @param MessageParamsJson csp::common::String : A JSON formatted string of parameters to be passed to the callback.
    void PostMessageToScript(const csp::common::String Message, const csp::common::String MessageParamsJson = "");

    /// @brief Resets binding, context and subscriptions when the source is changed for the script.
    /// @param InScriptSource csp::common::String : The new source for the script.
    void OnSourceChanged(const csp::common::String& InScriptSource);

    /// @brief Registers the script source for the related entity in the script system.
    void RegisterSourceAsModule();

    /// @brief Binds the related entity in the script system.
    void Bind();

    /// @brief Sets the owner of the script.
    /// @param ClientId uint64_t : The ID of the client to be set as owner.
    void SetOwnerId(uint64_t ClientId);

    /// @brief Get the owner of the script.
    /// @return The client ID of the owner.
    uint64_t GetOwnerId() const;

    /// @brief Removes the script source and context from the script system.
    void Shutdown();

private:
    EntityScript(SpaceEntity* InEntity, SpaceEntitySystem* InSpaceEntitySystem);

    void CheckBinding();

    csp::systems::ScriptSystem* ScriptSystem;
    SpaceEntity* Entity;
    ScriptSpaceComponent* EntityScriptComponent;

    bool HasLastError;
    csp::common::String LastError;

    using PropertyChangeKey = std::pair<int32_t, int32_t>;
    using PropertyChangeMap = std::map<PropertyChangeKey, csp::common::String>;
    PropertyChangeMap PropertyMap;

    using SubscribedMessageMap = std::map<csp::common::String, csp::common::String>;
    SubscribedMessageMap MessageMap;

    bool HasBinding;

    SpaceEntitySystem* SpaceEntitySystemPtr;
};

} // namespace csp::multiplayer
