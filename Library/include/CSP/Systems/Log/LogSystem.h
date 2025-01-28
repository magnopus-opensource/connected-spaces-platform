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

#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"

#include <functional>

namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

namespace csp::systems
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

/// @brief A Connected Spaces Platform level Logger for debugging or printing to console, also handles logging to a file.
/// Contains a callback system that allows clients to react to specific logs or events.
class CSP_API LogSystem
{
    CSP_START_IGNORE
    /** @cond DO_NOT_DOCUMENT */
    friend class SystemsManager;
    friend void csp::memory::Delete<LogSystem>(LogSystem* Ptr);
    /** @endcond */
    CSP_END_IGNORE

public:
    typedef std::function<void(const csp::common::String&)> LogCallbackHandler;
    typedef std::function<void(const csp::common::String&)> EventCallbackHandler;
    typedef std::function<void(const csp::common::String&)> BeginMarkerCallbackHandler;

    // @todo This doesn't need a parameter, but wrapper gen fails without one
    typedef std::function<void(void*)> EndMarkerCallbackHandler;

    /// @brief Set a callback for handling a log. Can be used to debug Connected Spaces Platform within a client application.
    /// @param InLogCallback The callback to execute when a log occurs.
    CSP_EVENT void SetLogCallback(LogCallbackHandler InLogCallback);

    /// @brief Set a callback for handling an event log. Can be used to debug Connected Spaces Platform within a client application.
    /// @param InEventCallback The callback to execute when an event log occurs.
    CSP_EVENT void SetEventCallback(EventCallbackHandler InEventCallback);

    /// @brief Set a callback for handling a begin marker event. Can be used to debug Connected Spaces Platform within a client application.
    /// @param InBeginCallback The callback to execute when the marker begins.
    CSP_EVENT void SetBeginMarkerCallback(BeginMarkerCallbackHandler InBeginCallback);

    /// @brief Set a callback for handling an end marker event. Can be used to debug Connected Spaces Platform within a client application.
    /// @param InEndCallback The callback to execute when the marker ends.
    CSP_EVENT void SetEndMarkerCallback(EndMarkerCallbackHandler InEndCallback);

    /// @brief Set the verbosity of logging for a system-wide level.
    /// @param InSystemLevel The level to set the system logging to.
    void SetSystemLevel(const csp::systems::LogLevel InSystemLevel);

    /// @brief Retreive the log verbosity level.
    csp::systems::LogLevel GetSystemLevel();

    /// @brief Check if we currently log a specified log verbosity level.
    /// @param Level The level to check.
    bool LoggingEnabled(const csp::systems::LogLevel Level);

    /// @brief Log a message at a specific verbosity level.
    /// @param Level The level to log this message at.
    /// @param InMessage The message to be logged.
    void LogMsg(const csp::systems::LogLevel Level, const csp::common::String& InMessage);
    /// @brief Log an event.
    /// @param InEvent The event to be logged.
    void LogEvent(const csp::common::String& InEvent);

    /// @brief Specify a 'Marker' event which can be used to communicate a certain process occurring, usually for debugging.
    void BeginMarker(const csp::common::String& InMarker);
    /// @brief End a 'Marker' event.
    void EndMarker();

    /// @brief Clears all logging callbacks.
    void ClearAllCallbacks();

private:
    LogSystem();
    ~LogSystem();

    csp::systems::LogLevel SystemLevel = LogLevel::All;

    void LogToFile(const csp::common::String& InMessage);

    // Allocate internally to avoid warning C4251 'needs to have dll-interface to be used by clients'
    struct LogCallbacks* Callbacks;
};

} // namespace csp::systems
