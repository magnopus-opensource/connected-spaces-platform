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
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "Multiplayer/EventSerialisation.h"

#include <map>

namespace csp::systems
{

class SpaceSystem;
class SystemsManager;
class SystemBase;

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

/// @ingroup Multiplayer
/// @brief Handling of all network events.
class CSP_API EventBus
{
public:
	/** @cond DO_NOT_DOCUMENT */
	friend class csp::systems::SpaceSystem;
	friend class csp::systems::SystemsManager;
	friend class MultiplayerConnection;
	friend void csp::memory::Delete<EventBus>(EventBus* Ptr);
	/** @endcond */

	// The callback used to register to listen to network events.
	typedef std::function<void(bool, const csp::common::Array<ReplicatedValue>&)> ParameterisedCallbackHandler;

	/// @brief Registers a callback to listen for the named event.
	/// @param EventName csp::common::String : The identifying name for the event to listen for.
	/// @param Callback ParameterisedCallbackHandler : A callback to register for the event which contains the parameter payload data.
	void ListenEvent(const csp::common::String& EventName, ParameterisedCallbackHandler Callback);

	/// @brief Registers a system to listen for the named event.
	/// @param EventName csp::common::String : The identifying name for the event to listen for.
	/// @param System csp::systems::SystemBase* : A pointer to the system which wants to register for the event.
	CSP_NO_EXPORT void ListenEvent(const csp::common::String& EventName, csp::systems::SystemBase* System);

	/// @brief Registers a callback and a system to listen for the named event.
	/// @param EventName csp::common::String : The identifying name for the event to listen for.
	/// @param Callback ParameterisedCallbackHandler : A callback to register for the event which contains the parameter payload data.
	/// @param System csp::systems::SystemBase* : A pointer to the system which wants to register for the event.
	CSP_NO_EXPORT void ListenEvent(const csp::common::String& EventName, ParameterisedCallbackHandler Callback, csp::systems::SystemBase* System);

	/// @brief Stops the event bus from listening for a particular event.
	/// @param EventName csp::common::String : The identifying name for the event to stop listening for.
	void StopListenEvent(const csp::common::String& EventName);

	/// @brief Instructs the event bus to start listening to messages
	void StartEventMessageListening();

private:
	EventBus();
	~EventBus();

	EventBus(const EventBus& InBoundConnection);
	EventBus(MultiplayerConnection* InMultiplayerConnection);

	class MultiplayerConnection* MultiplayerConnectionInst;

	// TODO: Replace these with pointers! We can't use STL containers as class fields due to the fact that the class size will
	//   change depending on which runtime is used.
	typedef std::vector<ParameterisedCallbackHandler> Callbacks;
	typedef std::map<csp::common::String, std::pair<Callbacks, csp::systems::SystemBase*>> EventMaps;
	EventMaps EventMap;
};

} // namespace csp::multiplayer
