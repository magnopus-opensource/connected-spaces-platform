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

#include "CSP/Multiplayer/EventBus.h"

namespace csp::web
{

class WebClient;

}

namespace csp::multiplayer
{

class EventBus;
class EventDeserialiser;
class MultiplayerConnection;

} // namespace csp::multiplayer

namespace signalr
{

class value;

} // namespace signalr

namespace csp::systems
{
/// @brief Base class for all Connected Spaces Platform Systems, which enforces passing of a WebClient or EventBus instance in the constructor of each
/// System.
class CSP_API CSP_NO_DISPOSE SystemBase
{
    friend class csp::multiplayer::MultiplayerConnection;

protected:
    CSP_NO_EXPORT SystemBase(csp::web::WebClient* InWebClient, csp::multiplayer::EventBus* InEventBus);
    CSP_NO_EXPORT SystemBase(csp::multiplayer::EventBus* InEventBus);

    csp::web::WebClient* WebClient;
    csp::multiplayer::EventBus* EventBusPtr;

public:
    /// @brief Destructor of the SystemBase base class.
    virtual ~SystemBase();

    /// @brief Registers the system to listen for the default event.
    virtual void RegisterSystemCallback();

    /// @brief Deregisters the system from listening for the default event.
    virtual void DeregisterSystemCallback();

    /// @brief Deserialises the event values of the system.
    /// @param EventValues std::vector<signalr::value> : event values to deserialise
    CSP_NO_EXPORT virtual void OnEvent(const std::vector<signalr::value>& EventValues);

    /// @brief Sets a callback for a default event.
    /// @param Callback csp::multiplayer::EventBus::ParameterisedCallbackHandler: Callback to receive data for the system that has been changed.
    CSP_EVENT void SetSystemCallback(csp::multiplayer::EventBus::ParameterisedCallbackHandler Callback);

private:
    SystemBase(); // This constructor is only provided to appease the wrapper generator and should not be used

    csp::multiplayer::EventBus::ParameterisedCallbackHandler SystemCallback;
};

} // namespace csp::systems
