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

#include "Events/Event.h"
#include "Events/EventListener.h"

#include <list>

namespace csp::events
{

using EventCallbackList = std::list<csp::events::EventListener*>;

class EventDispatcher
{
public:
    EventDispatcher(const EventId& inId);

    void RegisterListener(EventListener* inListener);
    void UnRegisterListener(EventListener* inListener);

    void Dispatch(const Event& inEvent);

private:
    EventId m_id;
    EventCallbackList m_callbackList;
};

} // namespace csp::events
