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
#include "CSP/Common/Systems/Log/LogSystem.h"

#include "Multiplayer/NetworkEventSerialisation.h"

namespace csp::systems
{

SystemBase::SystemBase()
    : m_webClient(nullptr)
    , m_eventBusPtr(nullptr)
    , m_logSystem(nullptr)
{
}

SystemBase::SystemBase(csp::web::WebClient* inWebClient, csp::multiplayer::NetworkEventBus* eventBus, csp::common::LogSystem* logSystem)
    : m_webClient(inWebClient)
    , m_eventBusPtr(eventBus)
    , m_logSystem(logSystem)
{
}

SystemBase::SystemBase(csp::multiplayer::NetworkEventBus* eventBus, csp::common::LogSystem* logSystem)
    : m_webClient(nullptr)
    , m_eventBusPtr(eventBus)
    , m_logSystem(logSystem)
{
}

SystemBase::~SystemBase() { m_eventBusPtr = nullptr; }

void SystemBase::RegisterSystemCallback()
{
    // Do nothing.
}

} // namespace csp::systems
