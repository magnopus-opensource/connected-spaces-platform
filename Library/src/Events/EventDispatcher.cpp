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
#include "Events/EventDispatcher.h"

namespace csp::events
{

EventDispatcher::EventDispatcher(const EventId& inId)
    : m_id(inId)
{
}

void EventDispatcher::RegisterListener(EventListener* inListener)
{
    // Check it's not there already
    m_callbackList.remove(inListener);

    m_callbackList.push_back(inListener);
}

void EventDispatcher::UnRegisterListener(EventListener* inListener) { m_callbackList.remove(inListener); }

void EventDispatcher::Dispatch(const Event& inEvent)
{
    for (auto callback : m_callbackList)
    {
        callback->OnEvent(inEvent);
    }
}

} // namespace csp::events
