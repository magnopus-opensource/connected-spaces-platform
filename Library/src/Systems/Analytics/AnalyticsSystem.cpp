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
#include "CSP/Systems/Analytics/AnalyticsSystem.h"

#include "CSP/Systems/Analytics/AnalyticsProvider.h"
#include "CSP/Systems/Analytics/AnalyticsSystemUtils.h"
#include "Events/EventSystem.h"
#include "Memory/Memory.h"

// #include <atomic_queue/atomic_queue.h>

namespace csp::systems
{

class AnalyticsSystemImpl : public csp::events::EventListener
{
public:
    AnalyticsSystemImpl()
        : Provider { nullptr }
    {
    }

    ~AnalyticsSystemImpl() { }

    void OnEvent(const csp::events::Event& InEvent) override
    {
        /* std::scoped_lock<std::mutex> ProviderLock(ProviderMutex);

        if (Provider == nullptr)
        {
                return;
        }

        while (Queue.was_size() > 0)
        {
                auto Event = Queue.pop();
                Provider->Log(Event);

                DEINIT_EVENT(Event);
        }*/
    }

    void Log(AnalyticsEvent* Event)
    {
        /* if (Provider)
        {
                Queue.push(Event);
        }*/
    }

    void RegisterProvider(IAnalyticsProvider* InProvider) { Provider = InProvider; }

    void DeregisterProvider(IAnalyticsProvider* InProvider)
    {
        /* if (Provider == InProvider)
        {
                std::scoped_lock<std::mutex> ProviderLock(ProviderMutex);
                Provider = nullptr;
        }*/
    }

private:
    // using AnalyticsSystemQueue = atomic_queue::AtomicQueue<AnalyticsEvent*, AnalyticsSystem::QueueSize>;

    IAnalyticsProvider* Provider;
    std::mutex ProviderMutex;
    // AnalyticsSystemQueue Queue;
};

AnalyticsSystem::AnalyticsSystem()
    : Impl { CSP_NEW AnalyticsSystemImpl() }
{
    csp::events::EventSystem::Get().RegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, Impl);
}

AnalyticsSystem::~AnalyticsSystem()
{
    csp::events::EventSystem::Get().UnRegisterListener(csp::events::FOUNDATION_TICK_EVENT_ID, Impl);
    CSP_DELETE(Impl);
}

void AnalyticsSystem::Log(AnalyticsEvent* Event) { Impl->Log(Event); }

void AnalyticsSystem::RegisterProvider(IAnalyticsProvider* InProvider) { Impl->RegisterProvider(InProvider); }

void AnalyticsSystem::DeregisterProvider(IAnalyticsProvider* InProvider) { Impl->DeregisterProvider(InProvider); }

} // namespace csp::systems
