#pragma once

#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"

#include <functional>

namespace oly_systems
{

enum class LogLevel
{
    NoLogging,
    Fatal,
    Error,
    Warning,
    Display,
    Log,
    Verbose,
    VeryVerbose,
    All
};

class OLY_API OLY_NO_DISPOSE LogSystem
{
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    /** @endcond */

public:
    ~LogSystem();

    typedef std::function<void(const oly_common::String&)> LogCallbackHandler;
    typedef std::function<void(const oly_common::String&)> EventCallbackHandler;
    typedef std::function<void(const oly_common::String&)> BeginMarkerCallbackHandler;

    // @todo This doesn't need a parameter, but wrapper gen fails without one
    typedef std::function<void(void*)> EndMarkerCallbackHandler;

    OLY_EVENT void SetLogCallback(LogCallbackHandler InLogCallback);
    OLY_EVENT void SetEventCallback(EventCallbackHandler InEventCallback);
    OLY_EVENT void SetBeginMarkerCallback(BeginMarkerCallbackHandler InBeginCallback);
    OLY_EVENT void SetEndMarkerCallback(EndMarkerCallbackHandler InEndCallback);

    OLY_NO_EXPORT void SetSystemLevel(const oly_systems::LogLevel InSystemLevel);
    OLY_NO_EXPORT oly_systems::LogLevel GetSystemLevel();
    OLY_NO_EXPORT bool LoggingEnabled(const oly_systems::LogLevel Level);

    OLY_NO_EXPORT void LogMsg(const oly_systems::LogLevel Level, const oly_common::String& InMessage);
    OLY_NO_EXPORT void LogEvent(const oly_common::String& InEvent);

    OLY_NO_EXPORT void BeginMarker(const oly_common::String& InMarker);
    OLY_NO_EXPORT void EndMarker();

    void ClearAllCallbacks();

private:
    LogSystem();

    oly_systems::LogLevel SystemLevel = LogLevel::All;

    void LogToFile(const oly_common::String& InMessage);

private:
    // Allocate internally to avoid warning C4251 'needs to have dll-interface to be used by clients'
    class LogCallbacks* Callbacks;
};

} // namespace oly_systems
