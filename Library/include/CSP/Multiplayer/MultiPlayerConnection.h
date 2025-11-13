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

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/NetworkEventData.h"
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Multiplayer/MultiplayerHubMethods.h"

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

CSP_START_IGNORE
class CSPEngine_MultiplayerTests_SignalRConnectionTest_Test;
CSP_END_IGNORE

namespace async
{
CSP_START_IGNORE
template <typename T> class task;
CSP_END_IGNORE
}

namespace signalr
{
class value;
} // namespace signalr

namespace csp::common
{
class LogSystem;
class ReplicatedValue;
class IRealtimeEngine;
class IAuthContext;
class IRealtimeEngine;
}

/// @brief Namespace that encompasses everything in the multiplayer system
namespace csp::multiplayer
{
class OnlineRealtimeEngine;
class ClientElectionManager;
class ISignalRConnection;
class NetworkEventManagerImpl;
class IWebSocketClient;
class NetworkEventBus;

/// @brief Enum used to specify the current state of the multiplayer connection.
enum class ConnectionState
{
    Connecting,
    Connected,
    Disconnecting,
    Disconnected
};

/// @ingroup Multiplayer
/// @brief Handling of all multiplayer connection functionality, such as connect, disconnect, entity replication and network events.
class CSP_API MultiplayerConnection
{
public:
    /** @cond DO_NOT_DOCUMENT */
    friend class ::CSPEngine_MultiplayerTests_SignalRConnectionTest_Test;
    /** @endcond */

    // Simple callback that receives an error code.
    typedef std::function<void(ErrorCode)> ErrorCodeCallbackHandler;

    // The callback for disconnection, contains a string with a reason for disconnection.
    typedef std::function<void(const csp::common::String&)> DisconnectionCallbackHandler;

    // The callback for connection, contains a string with a status of connection.
    typedef std::function<void(const csp::common::String&)> ConnectionCallbackHandler;

    // The callback for network interruption, contains a string showing failure.
    typedef std::function<void(const csp::common::String&)> NetworkInterruptionCallbackHandler;

    /// @brief Sets a callback for a disconnection event.
    /// @param Callback DisconnectionCallbackHandler : The callback for disconnection, contains a string with a reason for disconnection.
    CSP_EVENT void SetDisconnectionCallback(DisconnectionCallbackHandler Callback);

    /// @brief Sets a callback for a connection event
    /// @param Callback ConnectionCallbackHandler : The callback for connection, contains a string with a status of connection (Success, Failure etc).
    CSP_EVENT void SetConnectionCallback(ConnectionCallbackHandler Callback);

    /// @brief Sets a callback for a network interruption event.
    /// Connection isn't recoverable after this point and Disconnect should be called.
    /// @param Callback NetworkInterruptionCallbackHandler : The callback for network interruption, contains a string showing failure.
    CSP_EVENT void SetNetworkInterruptionCallback(NetworkInterruptionCallbackHandler Callback);

    /// @brief Requests the ClientID.
    /// @return uint64_t the ClientID for this connection.
    uint64_t GetClientId() const;

    /// @brief Gets the current connection state.
    /// @return A ConnectionState enum value.
    ConnectionState GetConnectionState() const;

    /// @brief Sets the Self Messaging flag for this client.
    /// This allows a client to declare that it wishes to receive every patch and object message it sends.
    /// @warning Don't use this function if you aren't sure of the consequences, it's very unlikely that a client would want to use this!
    /// @param AllowSelfMessaging bool : True to allow and false to disallow.
    /// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
    CSP_ASYNC_RESULT void SetAllowSelfMessagingFlag(const bool AllowSelfMessaging, ErrorCodeCallbackHandler Callback);

    /// @brief Gets the bool representing if we're using self-messaging or not.
    /// @return True if self messaging is allowed, false otherwise.
    bool GetAllowSelfMessagingFlag() const;

    /// @brief Parse a SignalR multiplayer error. Unpacks the exception and forwards to @ref ParseMultiplayerError
    /// @param std::exception_ptr Exception : Pointer to the exception to parse.
    /// @return std::pair<ErrorCode, std::string> First element being the deduced error code, second being the exception message.
    CSP_NO_EXPORT static std::pair<ErrorCode, std::string> ParseMultiplayerErrorFromExceptionPtr(std::exception_ptr Exception);

    /// @brief Parse a SignalR multiplayer error.
    /// @param const std::exception& Exception : The exception to parse
    /// @return std::pair<ErrorCode, std::string> First element being the deduced error code, second being the exception message.
    CSP_NO_EXPORT static std::pair<ErrorCode, std::string> ParseMultiplayerError(const std::exception& Exception);

    /// @brief Create a default SignalRConnection configured to the configured MultiplayerServiceURI
    /// @return ISignalRConnection* Pointer to SignalR connection. The caller should take ownership of the pointer.
    CSP_NO_EXPORT static ISignalRConnection* MakeSignalRConnection(csp::common::IAuthContext& AuthContext);

    /// @brief Start the connection and register to start receiving updates from the server.
    /// Connect should be called after LogIn and before EnterSpace.
    /// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
    CSP_NO_EXPORT void Connect(ErrorCodeCallbackHandler Callback, [[maybe_unused]] const csp::common::String& MultiplayerUri,
        const csp::common::String& AccessToken, const csp::common::String& DeviceId);

    /// @brief Indicates whether the multiplayer connection is established
    /// @return bool : true if connected, false otherwise
    CSP_NO_EXPORT bool IsConnected() const { return Connected; }

    /// @brief Getter for the signalR connection
    /// @return ISignalRConnection* : pointer to the signalR connection
    CSP_NO_EXPORT csp::multiplayer::ISignalRConnection* GetSignalRConnection() { return Connection; };

    /// @brief Getter for the NetworkEventManager
    /// @return NetworkEventManagerImpl* : pointer to the NetworkEventManager
    CSP_NO_EXPORT csp::multiplayer::NetworkEventManagerImpl* GetNetworkEventManager() { return NetworkEventManager; }

    /// @brief Getter for the NetworkEventBus
    /// @return NetworkEventBus* : pointer to the NetworkEventBus
    CSP_NO_EXPORT NetworkEventBus* GetEventBusPtr() { return EventBusPtr; }

    /// @brief Disconnect the multiplayer and provide a reason
    /// @param Reason csp::common::String& : the reason to disconnect
    /// @param Callback ErrorCodeCallbackHandler : a callback with failure state
    CSP_NO_EXPORT void DisconnectWithReason(const csp::common::String& Reason, ErrorCodeCallbackHandler Callback);

    CSP_START_IGNORE
    // Invoke "StartListening" on already created Connection
    CSP_NO_EXPORT async::task<std::tuple<signalr::value, std::exception_ptr>> StartListening();
    CSP_END_IGNORE

    /// @brief Subscribes the connected user to the specified space's scope.
    /// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
    /// @return async::task<std::tuple<signalr::value, std::exception_ptr>> : The async task containing the result which will be passed to the next
    /// continuation.
    CSP_NO_EXPORT async::task<std::tuple<signalr::value, std::exception_ptr>> SetScopes(csp::common::String InSpaceId);

    /// @brief Stop listening to the multiplayer
    /// @return async::task<std::tuple<signalr::value, std::exception_ptr>> : The async task containing the result which will be passed to the next
    /// continuation.
    CSP_NO_EXPORT async::task<std::tuple<signalr::value, std::exception_ptr>> StopListening();

    /// @brief Clears the connected user's subscription to their current set of scopes.
    /// @return async::task<std::tuple<signalr::value, std::exception_ptr>> : The async task containing the result which will be passed to the next
    /// continuation.
    CSP_NO_EXPORT async::task<std::tuple<signalr::value, std::exception_ptr>> ResetScopes();

    /// @brief MultiplayerConnection constructor
    /// @param LogSystem csp::common::LogSystem& LogSystem injection.
    /// @param Connection csp::multiplayer::ISignalRConnection& Connection injection. Multiplayer connection allows connection/disconnection, and
    /// tracks that with a boolean flag, but the connection object itself is invariant, always set.
    CSP_NO_EXPORT MultiplayerConnection(csp::common::LogSystem& LogSystem, csp::multiplayer::ISignalRConnection& Connection);

    /// @brief MultiplayerConnection destructor
    CSP_NO_EXPORT ~MultiplayerConnection();

    /// @brief End the multiplayer connection.
    /// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
    CSP_NO_EXPORT void Disconnect(ErrorCodeCallbackHandler Callback);

    /// @brief Getter for the MultiplayerHubMethodMap
    /// @return MultiplayerHubMethodMap : the MultiplayerHubMethodMap instance
    CSP_NO_EXPORT MultiplayerHubMethodMap GetMultiplayerHubMethods() const { return MultiplayerHubMethods; }

    /// @brief Sets the internal reference to the OnlineRealtimeEngine. Should be called when entering a space.
    /// @param OnlineRealtimeEngine OnlineRealtimeEngine* : Non-owning pointer. Ideally this would be an owned structure, but we can't due to the
    /// wrapper generator. Remember to null this when exiting a space as object dispatch depends on that, until clients are capable of managing this
    /// themselves. Unfortunate manual state management.
    CSP_NO_EXPORT void SetOnlineRealtimeEngine(csp::multiplayer::OnlineRealtimeEngine* OnlineRealtimeEngine);

    /// @brief Get the currently set realtime engine.
    /// @return Non-owning pointer to currently set realtime engine. This should be non-null when in a space, and null before entering, or after
    /// exiting a space.
    csp::multiplayer::OnlineRealtimeEngine* GetOnlineRealtimeEngine() const;

private:
    MultiplayerConnection(const MultiplayerConnection& InBoundConnection);

    typedef std::function<void(std::exception_ptr)> ExceptionCallbackHandler;

    void Start(ExceptionCallbackHandler Callback) const;

    //<void> template tags not supported in wrapper generator. (Why we're even wrapping private methods is a mystery to me though)
    CSP_START_IGNORE
    async::task<void> Start() const;
    /* Connect Continuations */
    // Delete the entity specified by the EntityId. std::numeric_limits<uint64_t>::max() means ALL_ENTITIES_ID, and deletes everything.
    auto DeleteEntities(uint64_t EntityId) const;
    // Get the client ID and return it (does not set it locally)
    auto RequestClientId();
    CSP_END_IGNORE

    void Stop(ExceptionCallbackHandler Callback) const;

    /*
     * Bind the SignalR messages that are recieved from MCS to facilitate realtime communication.
     * These are bound for the entire lifetime of the MultiplayerConnection (conceptually Login/Logout scoped).
     * These messages are dispatched to the OnlineRealtimeEngine if it exists.
     * For the future, consider how pleasant it might be if we bound everything all at once here, and had a single
     * container of observers to recieve the bindings. That would make registering/deregistering AND lifetime the
     * same concept, and we'd have a single pipe of realtime events that were obvious to trace.
     */
    void BindOnObjectMessage();
    void BindOnObjectPatch();
    void BindOnRequestToSendObject();
    void BindOnRequestToDisconnect();
    void BindOnElectedScopeLeaderCallback();
    void BindOnVacatedScopeLeaderCallback();

    // May not be null
    class csp::multiplayer::ISignalRConnection* Connection;

    class csp::multiplayer::IWebSocketClient* WebSocketClient;
    class NetworkEventManagerImpl* NetworkEventManager;
    class NetworkEventBus* EventBusPtr;

    csp::common::LogSystem& LogSystem;

    uint64_t ClientId;

    DisconnectionCallbackHandler DisconnectionCallback;
    ConnectionCallbackHandler ConnectionCallback;
    NetworkInterruptionCallbackHandler NetworkInterruptionCallback;

    std::atomic_bool Connected;
    uint32_t KeepAliveSeconds = 120;

    bool AllowSelfMessaging = false;

    MultiplayerHubMethodMap MultiplayerHubMethods;

    /*  We currently have a circular dependency between the MultiplayerConnection and OnlineRealtimeEngine.
        This could easily be resolved by exposing an event registration from the MultiplayerConnection,
        so the OnlineRealtimeEngine can receieve events agonstically from the MultiplayerConnection (Similar to what we did with the event bus).
    */
    OnlineRealtimeEngine* MultiplayerRealtimeEngine = nullptr;
};

} // namespace csp::multiplayer
