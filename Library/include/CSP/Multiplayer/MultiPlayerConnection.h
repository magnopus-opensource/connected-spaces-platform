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
#include "CSP/Common/String.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Multiplayer/EventParameters.h"
#include "CSP/Systems/Assets/Asset.h"

#include <atomic>
#include <functional>
#include <map>
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

namespace csp::systems
{

class SpaceSystem;
class SystemsManager;
class UserSystem;

} // namespace csp::systems

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

/// @brief Namespace that encompasses everything in the multiplayer system
namespace csp::multiplayer
{

class ReplicatedValue;
class SpaceEntitySystem;
class ConversationSystem;
class ClientElectionManager;
class ISignalRConnection;
class IWebSocketClient;
class EventBus;

/// @brief Enum used to specify the current state of the multiplayer connection.
enum class ConnectionState
{
    Connecting,
    Connected,
    Disconnecting,
    Disconnected
};

/// @brief Enum used to indicate the failure state of a multiplayer request.
enum class ErrorCode
{
    None,
    Unknown,
    NotConnected,
    AlreadyConnected,
    SpaceUserLimitExceeded
};

/// @ingroup Multiplayer
/// @brief Handling of all multiplayer connection functionality, such as connect, disconnect, entity replication and network events.
class CSP_API MultiplayerConnection
{
public:
    /** @cond DO_NOT_DOCUMENT */
    friend class ConversationSystem;
    friend class csp::systems::SpaceSystem;
    friend class csp::systems::SystemsManager;
    friend class csp::systems::UserSystem;
    friend class SpaceEntityEventHandler;
    friend class SpaceEntitySystem;
    friend class ClientElectionManager;
    friend class ClientElectionEventHandler;
    friend class EventBus;
    friend class ::CSPEngine_MultiplayerTests_SignalRConnectionTest_Test;
    friend void csp::memory::Delete<MultiplayerConnection>(MultiplayerConnection* Ptr);
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

    /// @brief Gets a pointer to the conversation system.
    /// @return A pointer to the conversation system.
    ConversationSystem* GetConversationSystem() const;

    /// @brief Gets the current connection state.
    /// @return A ConnectionState enum value.
    ConnectionState GetConnectionState() const;

    /// @brief Sets the Self Messaging flag for this client.
    /// This allows a client to declare that it wishes to recieve every patch and object message it sends.
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
    CSP_NO_EXPORT static ISignalRConnection* MakeSignalRConnection();

    /// @brief Start the connection and register to start receiving updates from the server.
    /// Connect should be called after LogIn and before EnterSpace.
    /// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
    /// @param ISignalRConnection* SignalRConnection : The SignalR connection to use when talking to the server. The MultiplayerConnection takes
    /// ownership of this pointer.
    CSP_NO_EXPORT void Connect(ErrorCodeCallbackHandler Callback, ISignalRConnection* SignalRConnection);

private:
    MultiplayerConnection();
    ~MultiplayerConnection();

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
    // Invoke "StartListening" on already created Connection
    std::function<async::task<void>()> StartListening();
    CSP_END_IGNORE

    /// @brief End the multiplayer connection.
    /// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
    void Disconnect(ErrorCodeCallbackHandler Callback);
    void Stop(ExceptionCallbackHandler Callback) const;
    void StopListening(ErrorCodeCallbackHandler Callback);

    /// @brief Subscribes the connected user to the specified space's scope.
    /// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
    void SetScopes(csp::common::String InSpaceId, ErrorCodeCallbackHandler Callback);

    /// @brief Clears the connected user's subscription to their current set of scopes.
    /// @param Callback ErrorCodeCallbackHandler : a callback with failure state.
    void ResetScopes(ErrorCodeCallbackHandler Callback);

    void DisconnectWithReason(const csp::common::String& Reason, ErrorCodeCallbackHandler Callback);

    class csp::multiplayer::ISignalRConnection* Connection;
    class csp::multiplayer::IWebSocketClient* WebSocketClient;
    class NetworkEventManagerImpl* NetworkEventManager;
    ConversationSystem* ConversationSystemPtr;
    EventBus* EventBusPtr;

    uint64_t ClientId;

    DisconnectionCallbackHandler DisconnectionCallback;
    ConnectionCallbackHandler ConnectionCallback;
    NetworkInterruptionCallbackHandler NetworkInterruptionCallback;

    std::atomic_bool Connected;
    uint32_t KeepAliveSeconds = 120;

    bool AllowSelfMessaging = false;
};

} // namespace csp::multiplayer
