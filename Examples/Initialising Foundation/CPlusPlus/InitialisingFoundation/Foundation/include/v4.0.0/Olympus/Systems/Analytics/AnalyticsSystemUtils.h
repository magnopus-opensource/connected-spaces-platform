#pragma once

#include "Olympus/Common/Map.h"
#include "Olympus/Common/String.h"
#include "Olympus/Multiplayer/ReplicatedValue.h"
#include "Olympus/OlympusCommon.h"

namespace oly_systems
{
using MetricValue = oly_multiplayer::ReplicatedValue;

class OLY_API AnalyticsEvent
{
    friend class AnalyticsEventInitialiser;

public:
    void AddInt(oly_common::String Key, int64_t Value);
    void AddString(oly_common::String Key, const oly_common::String& Value);
    void AddFloat(oly_common::String Key, float Value);
    void AddBool(oly_common::String Key, bool Value);

    const int64_t GetInt(oly_common::String Key) const;
    const oly_common::String& GetString(oly_common::String Key) const;
    const float GetFloat(oly_common::String Key) const;
    bool GetBool(oly_common::String Key) const;

    const oly_common::String& GetTag() const;

    OLY_START_IGNORE
    const oly_common::Map<oly_common::String, MetricValue>& GetParams() const;
    OLY_END_IGNORE

private:
    AnalyticsEvent(const oly_common::String& Tag);

    oly_common::String Tag;

    oly_common::Map<oly_common::String, MetricValue> Parameters;
};

class OLY_API AnalyticsEventInitialiser
{
public:
    static AnalyticsEvent* Initialise(const oly_common::String Tag);
    static void DeInitialise(AnalyticsEvent* Event);
};

#define INIT_EVENT(T) oly_systems::AnalyticsEventInitialiser::Initialise(T)
#define DEINIT_EVENT(E) oly_systems::AnalyticsEventInitialiser::DeInitialise(E)

} // namespace oly_systems
