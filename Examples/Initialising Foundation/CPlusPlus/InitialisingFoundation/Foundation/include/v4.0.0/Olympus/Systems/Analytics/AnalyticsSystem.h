#pragma once

#include "AnalyticsProvider.h"
#include "Olympus/Common/Array.h"
#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"

#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

namespace oly_systems
{

class AnalyticsSystemImpl;

/// @ingroup Analytics System
/// @brief Public facing system that allows interfacing with an analytics provider.
/// Offers methods for sending events to the provider
/// Events are added to a queue to be processewd on a different thread
/// If events are unable to be send to the provider, then they will be held in the queue
class OLY_API OLY_NO_DISPOSE AnalyticsSystem
{
public:
    AnalyticsSystem();
    ~AnalyticsSystem();

    OLY_START_IGNORE
    AnalyticsSystem(const AnalyticsSystem&) = delete;
    AnalyticsSystem(AnalyticsSystem&&) = delete;

    AnalyticsSystem& operator=(const AnalyticsSystem&) = delete;
    AnalyticsSystem& operator=(AnalyticsSystem&&) = delete;
    OLY_END_IGNORE

    /// @brief Send an event
    /// @param Event AnalyticsEvent
    void Log(AnalyticsEvent* Event);

    OLY_START_IGNORE
    void RegisterProvider(IAnalyticsProvider* Provider);
    void DeregisterProvider(IAnalyticsProvider* Provider);
    OLY_END_IGNORE

    OLY_START_IGNORE
    static const int QueueSize = 1024;
    OLY_END_IGNORE

private:
    AnalyticsSystemImpl* Impl;
};
} // namespace oly_systems
