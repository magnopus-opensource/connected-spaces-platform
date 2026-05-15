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
#include "Events/EventId.h"
#include "Common/Wrappers.h"

namespace csp::events
{

EventId::EventId(const char* inEventNamespace, const char* inEventName)
{
#if DEBUG
    STRNCPY(EventNamespaceDebug, sizeof(EventNamespaceDebug), inEventNamespace, sizeof(EventNamespaceDebug));
    STRNCPY(EventNameDebug, sizeof(EventNameDebug), inEventName, sizeof(EventNameDebug));
#endif

    EventNamespace = std::hash<std::string> { }(inEventNamespace);
    EventName = std::hash<std::string> { }(inEventName);
}

bool EventId::operator==(const EventId& other) const { return (EventName == other.EventName && EventNamespace == other.EventNamespace); }

} // namespace csp::events
