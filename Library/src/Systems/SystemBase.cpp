/*
 * Copyright 2024 Magnopus LLC

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
#include "CSP/Systems/SystemBase.h"

#include "Multiplayer/EventSerialisation.h"

namespace csp::systems
{

SystemBase::SystemBase()
    : WebClient(nullptr)
    , EventBusPtr(nullptr)
{
}

SystemBase::SystemBase(csp::web::WebClient* InWebClient, csp::multiplayer::EventBus* InEventBus)
    : WebClient(InWebClient)
    , EventBusPtr(InEventBus)
{
    RegisterSystemCallback();
}

SystemBase::SystemBase(csp::multiplayer::EventBus* InEventBus)
    : WebClient(nullptr)
    , EventBusPtr(InEventBus)
{
    RegisterSystemCallback();
}

SystemBase::~SystemBase()
{
    DeregisterSystemCallback();
    EventBusPtr = nullptr;
}

void SystemBase::RegisterSystemCallback()
{
    // Do nothing.
}

void SystemBase::DeregisterSystemCallback()
{
    // Do nothing.
}

void SystemBase::OnEvent(const std::vector<signalr::value>& EventValues)
{
    if (!SystemCallback)
    {
        return;
    }

    csp::multiplayer::EventDeserialiser Deserialiser;
    Deserialiser.Parse(EventValues);

    SystemCallback(true, Deserialiser.GetEventData());
}

void SystemBase::SetSystemCallback(csp::multiplayer::EventBus::ParameterisedCallbackHandler Callback)
{
    SystemCallback = Callback;
    RegisterSystemCallback();
}

} // namespace csp::systems
