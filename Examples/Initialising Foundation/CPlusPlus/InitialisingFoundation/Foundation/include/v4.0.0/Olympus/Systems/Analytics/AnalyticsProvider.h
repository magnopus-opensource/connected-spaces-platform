#pragma once

#include "Olympus/Systems/Analytics/AnalyticsSystemUtils.h"

class EventPayloadImpl;

namespace oly_systems
{

using MetricValue = oly_multiplayer::ReplicatedValue;

/// @ingroup Analytics System
/// @brief Interface for an Analytics Provider
class OLY_API IAnalyticsProvider
{
public:
    OLY_START_IGNORE
    IAnalyticsProvider(const IAnalyticsProvider&) = delete;
    IAnalyticsProvider(IAnalyticsProvider&&) = delete;

    IAnalyticsProvider& operator=(const IAnalyticsProvider&) = delete;
    IAnalyticsProvider& operator=(IAnalyticsProvider&&) = delete;

    virtual ~IAnalyticsProvider() = default;

    virtual void Log(AnalyticsEvent* Event) = 0;
    OLY_END_IGNORE

protected:
    IAnalyticsProvider() = default;
};

} // namespace oly_systems
