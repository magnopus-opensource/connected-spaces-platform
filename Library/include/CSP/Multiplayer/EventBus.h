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

#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"

#include <map>
#include <optional>

namespace csp::common
{
class LogSystem;
}

namespace csp::systems
{

class SpaceSystem;
class SystemsManager;
class SystemBase;

} // namespace csp::systems

namespace async
{
CSP_START_IGNORE
template <typename T> class event_task;
template <typename T> class task;
CSP_END_IGNORE
}

/// @brief Namespace that encompasses everything in the multiplayer system
namespace csp::multiplayer
{

enum class ErrorCode;

/// @ingroup Multiplayer
/// @brief Handling of all network events.
class CSP_API EventBus
{
public:
    typedef std::function<void(csp::multiplayer::ErrorCode)> ErrorCodeCallbackHandler;

    // The callback used to register to listen to events.
    typedef std::function<void(bool, const csp::common::Array<csp::common::ReplicatedValue>&)> ParameterisedCallbackHandler;

    /// @brief Sends a network event by EventName to all currently connected clients.
    /// @param EventName csp::common::String : The identifying name for the event.
    /// @param Args csp::common::Array<csp::common::ReplicatedValue> : An array of arguments (ReplicatedValue) to be passed as part of the event
    /// payload.
    /// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
    CSP_ASYNC_RESULT void SendNetworkEvent(
        const csp::common::String& EventName, const csp::common::Array<csp::common::ReplicatedValue>& Args, ErrorCodeCallbackHandler Callback);
    CSP_NO_EXPORT async::task<std::optional<csp::multiplayer::ErrorCode>> SendNetworkEvent(
        const csp::common::String& EventName, const csp::common::Array<csp::common::ReplicatedValue>& Args);

    /// @brief Sends a network event by EventName, to TargetClientId.
    /// @param EventName csp::common::String : The identifying name for the event.
    /// @param Args csp::common::Array<csp::common::ReplicatedValue> : An array of arguments (ReplicatedValue) to be passed as part of the event
    /// payload.
    /// @param TargetClientId uint64_t : The client ID to send the event to.
    /// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
    CSP_ASYNC_RESULT void SendNetworkEventToClient(const csp::common::String& EventName, const csp::common::Array<csp::common::ReplicatedValue>& Args,
        uint64_t TargetClientId, ErrorCodeCallbackHandler Callback);

    /// @brief Registers a system to listen for the named event, where the system can define its
    /// @brief own callback and deserialiser.
    /// @param EventName csp::common::String : The identifying name for the event to listen for.
    /// @param System csp::systems::SystemBase* : A pointer to the system which wants to register for the event.
    CSP_NO_EXPORT void ListenNetworkEvent(const csp::common::String& EventName, csp::systems::SystemBase* System);

    /// @brief Registers a callback to listen for the named event
    /// @param EventName csp::common::String : The identifying name for the event to listen for.
    /// @param Callback ParameterisedCallbackHandler : A callback to register for the event which contains the parameter payload data.
    void ListenNetworkEvent(const csp::common::String& EventName, ParameterisedCallbackHandler Callback);

    /// @brief Stops the event bus from listening for a particular event, for any system or callback
    /// @brief that were registered.
    /// @param EventName csp::common::String : The identifying name for the event to stop listening for.
    void StopListenNetworkEvent(const csp::common::String& EventName);

    /// @brief Instructs the event bus to start listening to messages
    void StartEventMessageListening();

    /// @brief EventBus constructor
    /// @param InMultiplayerConnection MultiplayerConnection* : the multiplayer connection to construct the event bus with
    CSP_NO_EXPORT EventBus(MultiplayerConnection* InMultiplayerConnection, csp::common::LogSystem& LogSystem);

    /// @brief EventBus destructor
    CSP_NO_EXPORT ~EventBus();

private:
    EventBus();

    class MultiplayerConnection* MultiplayerConnectionInst;

    std::map<csp::common::String, ParameterisedCallbackHandler> CallbacksNetworkEventMap;
    std::map<csp::common::String, csp::systems::SystemBase*> SystemsNetworkEventMap;

    csp::common::LogSystem& LogSystem;
};

} // namespace csp::multiplayer
