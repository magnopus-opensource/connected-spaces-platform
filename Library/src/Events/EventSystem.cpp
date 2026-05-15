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
#include "Events/EventSystem.h"

#include "Common/Queue.h"
#include "Events/EventDispatcher.h"

#include <unordered_map>

namespace std
{

template <> struct hash<csp::events::EventId>
{
    // This is taken from boost hash_combine, I doubt the author understood the magic number. I don't either.
    void HashCombine(std::size_t& inHash, std::size_t other) const { inHash ^= other + 0x9e3779b9 + (inHash << 6) + (inHash >> 2); }

    std::size_t operator()(const csp::events::EventId& id) const
    {
        std::size_t hash = id.EventNamespace;
        HashCombine(hash, id.EventName);

        return hash;
    }
};

} // namespace std

namespace csp::events
{

// Internal Implementation

class EventSystemImpl
{
public:
    EventSystemImpl();
    ~EventSystemImpl();

    EventDispatcher& GetDispatcher(const EventId& id);

    void EnqueueEvent(const Event* inEvent);

    void RegisterListener(const EventId& id, EventListener* inListener);
    void UnRegisterListener(const EventId& id, EventListener* inListener);
    void UnRegisterAllListeners();
    void ProcessEvents();

private:
    csp::Queue<const Event*> m_eventQueue;

    // Define eastl map using above defined hasher and our custom allocator
    using DispatcherMap = std::unordered_map<EventId, EventDispatcher, std::hash<EventId>, std::equal_to<EventId>>;

    DispatcherMap m_dispatchers;
};

EventSystemImpl::EventSystemImpl() { }

EventSystemImpl::~EventSystemImpl() { }

EventDispatcher& EventSystemImpl::GetDispatcher(const EventId& id)
{
    DispatcherMap::iterator it = m_dispatchers.find(id);
    if (it != m_dispatchers.end())
    {
        // Use existing dispatcher
        return it->second;
    }
    else
    {
        // Add new dispatcher
        auto result = m_dispatchers.insert(DispatcherMap::value_type(id, EventDispatcher(id)));
        return result.first->second;
    }
}

void EventSystemImpl::EnqueueEvent(const Event* inEvent) { m_eventQueue.Enqueue(inEvent); }

void EventSystemImpl::RegisterListener(const EventId& id, EventListener* inListener)
{
    EventDispatcher& dispatcher = GetDispatcher(id);
    dispatcher.RegisterListener(inListener);
}

void EventSystemImpl::UnRegisterListener(const EventId& id, EventListener* inListener)
{
    EventDispatcher& dispatcher = GetDispatcher(id);
    dispatcher.UnRegisterListener(inListener);
}

void EventSystemImpl::UnRegisterAllListeners() { m_dispatchers.clear(); }

void EventSystemImpl::ProcessEvents()
{
    while (m_eventQueue.IsEmpty() == false)
    {
        auto queuedItem = m_eventQueue.Dequeue();

        const Event* queuedEvent = queuedItem.value();
        const EventId& id = queuedEvent->GetId();
        GetDispatcher(id).Dispatch(*queuedEvent);

        delete (queuedEvent);
    }
}

// Public Event System Implementation

EventSystem& EventSystem::Get()
{
    static EventSystem theEventSystem;
    return theEventSystem;
}

EventSystem::EventSystem()
    : m_impl(new EventSystemImpl())
{
}

EventSystem::~EventSystem() { delete (m_impl); }

Event* EventSystem::AllocateEvent(const EventId& id) { return new Event(id); }

void EventSystem::EnqueueEvent(const Event* inEvent)
{
    if (m_impl)
    {
        m_impl->EnqueueEvent(inEvent);
    }
}

void EventSystem::RegisterListener(const EventId& id, EventListener* inListener)
{
    if (m_impl)
    {
        m_impl->RegisterListener(id, inListener);
    }
}

void EventSystem::UnRegisterListener(const EventId& id, EventListener* inListener)
{
    if (m_impl)
    {
        m_impl->UnRegisterListener(id, inListener);
    }
}

void EventSystem::UnRegisterAllListeners()
{
    if (m_impl)
    {
        m_impl->UnRegisterAllListeners();
    }
}

void EventSystem::ProcessEvents()
{
    if (m_impl)
    {
        m_impl->ProcessEvents();
    }
}

} // namespace csp::events
