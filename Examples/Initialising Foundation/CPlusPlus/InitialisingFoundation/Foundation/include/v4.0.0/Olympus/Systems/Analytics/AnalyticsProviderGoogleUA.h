#pragma once

#include "AnalyticsProvider.h"

#include <chrono>

namespace oly_web
{
class WebClient;
}

namespace oly_systems
{

/// @ingroup Analytics System
/// @brief Analytics Provider implementation for Google Universal Analytics
class AnalyticsProviderGoogleUA : public IAnalyticsProvider
{
public:
    AnalyticsProviderGoogleUA(const oly_common::String& ClientId, const oly_common::String& PropertyTag);

    OLY_START_IGNORE
    void Log(AnalyticsEvent* Event) override;
    OLY_END_IGNORE

private:
    oly_common::String ClientId;
    oly_common::String PropertyTag;

    oly_web::WebClient* WebClient;

    std::chrono::steady_clock::time_point Start;
};

oly_common::String CreateUAEventString(const oly_common::String& ClientId, const oly_common::String& PropertyTag, oly_systems::AnalyticsEvent* Event);
} // namespace oly_systems
