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
    void HashCombine(std::size_t& InHash, std::size_t Other) const { InHash ^= Other + 0x9e3779b9 + (InHash << 6) + (InHash >> 2); }

    std::size_t operator()(const csp::events::EventId& Id) const
    {
        std::size_t Hash = Id.EventNamespace;
        HashCombine(Hash, Id.EventName);

        return Hash;
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

    EventDispatcher& GetDispatcher(const EventId& Id);

    void EnqueueEvent(const Event* InEvent);

    void RegisterListener(const EventId& Id, EventListener* InListener);
    void UnRegisterListener(const EventId& Id, EventListener* InListener);
    void UnRegisterAllListeners();
    void ProcessEvents();

private:
    csp::Queue<const Event*> EventQueue;

    // Define eastl map using above defined hasher and our custom allocator
    using DispatcherMap = std::unordered_map<EventId, EventDispatcher, std::hash<EventId>, std::equal_to<EventId>>;

    DispatcherMap Dispatchers;
};

EventSystemImpl::EventSystemImpl() { }

EventSystemImpl::~EventSystemImpl() { }

EventDispatcher& EventSystemImpl::GetDispatcher(const EventId& Id)
{
    DispatcherMap::iterator it = Dispatchers.find(Id);
    if (it != Dispatchers.end())
    {
        // Use existing dispatcher
        return it->second;
    }
    else
    {
        // Add new dispatcher
        auto result = Dispatchers.insert(DispatcherMap::value_type(Id, EventDispatcher(Id)));
        return result.first->second;
    }
}

void EventSystemImpl::EnqueueEvent(const Event* InEvent) { EventQueue.Enqueue(InEvent); }

void EventSystemImpl::RegisterListener(const EventId& Id, EventListener* InListener)
{
    EventDispatcher& Dispatcher = GetDispatcher(Id);
    Dispatcher.RegisterListener(InListener);
}

void EventSystemImpl::UnRegisterListener(const EventId& Id, EventListener* InListener)
{
    EventDispatcher& Dispatcher = GetDispatcher(Id);
    Dispatcher.UnRegisterListener(InListener);
}

void EventSystemImpl::UnRegisterAllListeners() { Dispatchers.clear(); }

void EventSystemImpl::ProcessEvents()
{
    while (EventQueue.IsEmpty() == false)
    {
        auto QueuedItem = EventQueue.Dequeue();

        const Event* QueuedEvent = QueuedItem.value();
        const EventId& Id = QueuedEvent->GetId();
        GetDispatcher(Id).Dispatch(*QueuedEvent);

        delete (QueuedEvent);
    }
}

// Public Event System Implementation

EventSystem& EventSystem::Get()
{
    static EventSystem TheEventSystem;
    return TheEventSystem;
}

EventSystem::EventSystem()
    : Impl(new EventSystemImpl())
{
}

EventSystem::~EventSystem() { delete (Impl); }

Event* EventSystem::AllocateEvent(const EventId& Id) { return new Event(Id); }

void EventSystem::EnqueueEvent(const Event* InEvent)
{
    if (Impl)
    {
        Impl->EnqueueEvent(InEvent);
    }
}

void EventSystem::RegisterListener(const EventId& Id, EventListener* InListener)
{
    if (Impl)
    {
        Impl->RegisterListener(Id, InListener);
    }
}

void EventSystem::UnRegisterListener(const EventId& Id, EventListener* InListener)
{
    if (Impl)
    {
        Impl->UnRegisterListener(Id, InListener);
    }
}

void EventSystem::UnRegisterAllListeners()
{
    if (Impl)
    {
        Impl->UnRegisterAllListeners();
    }
}

void EventSystem::ProcessEvents()
{
    if (Impl)
    {
        Impl->ProcessEvents();
    }
}

} // namespace csp::events
