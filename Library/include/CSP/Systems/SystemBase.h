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

#include "CSP/Multiplayer/NetworkEventBus.h"

namespace csp::common
{
class LogSystem;
}

namespace csp::web
{

class WebClient;

}

namespace csp::multiplayer
{

class NetworkEventBus;
class EventDeserialiser;
class MultiplayerConnection;

} // namespace csp::multiplayer

namespace signalr
{

class value;

} // namespace signalr

namespace csp::systems
{
/// @brief Base class for all Connected Spaces Platform Systems, which enforces passing of a WebClient or NetworkEventBus instance in the constructor
/// of each System.
class CSP_API CSP_NO_DISPOSE SystemBase
{
    friend class csp::multiplayer::MultiplayerConnection;

protected:
    CSP_NO_EXPORT SystemBase(csp::web::WebClient* InWebClient, csp::multiplayer::NetworkEventBus* InEventBus, csp::common::LogSystem* LogSystem);
    CSP_NO_EXPORT SystemBase(csp::multiplayer::NetworkEventBus* InEventBus, csp::common::LogSystem* LogSystem);

    csp::web::WebClient* WebClient;
    csp::multiplayer::NetworkEventBus* EventBusPtr;

public:
    /// @brief Destructor of the SystemBase base class.
    virtual ~SystemBase();

    /// @brief Registers the system to listen for the default event.
    virtual void RegisterSystemCallback();

    /// @brief Deregisters the system from listening for the default event.
    virtual void DeregisterSystemCallback();

protected:
    // EM, June2025: Having this on system base makes it quite simple to quit using the singleton macros in all the systems if we wanted to.
    // Should NOT be null. The only reason this is a pointer is because we can't get the wrapper gen work required to support reference injections
    // done...
    csp::common::LogSystem* LogSystem;

private:
    SystemBase(); // This constructor is only provided to appease the wrapper generator and should not be used
};

} // namespace csp::systems
