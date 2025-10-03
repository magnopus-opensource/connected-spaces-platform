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
#include "CSP/Common/Systems/Log/LogSystem.h"

#include "Common/Logger.h"

#if defined(CSP_ANDROID)
#include <android/log.h>
#endif

namespace csp::common
{

struct LogCallbacks
{
    void Clear()
    {
        LogCallback = nullptr;
        EventCallback = nullptr;
        BeginMarkerCallback = nullptr;
        EndMarkerCallback = nullptr;
    }

    LogSystem::LogCallbackHandler LogCallback;
    LogSystem::EventCallbackHandler EventCallback;
    LogSystem::BeginMarkerCallbackHandler BeginMarkerCallback;
    LogSystem::EndMarkerCallbackHandler EndMarkerCallback;
};

LogSystem::LogSystem()
{
    // Allocate internally to avoid warning C425 'needs to have dll-interface to be used by clients'
    Callbacks = new LogCallbacks();
}

LogSystem::~LogSystem() { delete Callbacks; }

void LogSystem::SetLogCallback(LogCallbackHandler InLogCallback) { Callbacks->LogCallback = std::move(InLogCallback); }

void LogSystem::SetEventCallback(EventCallbackHandler InEventCallback) { Callbacks->EventCallback = std::move(InEventCallback); }

void LogSystem::SetBeginMarkerCallback(BeginMarkerCallbackHandler InBeginCallback) { Callbacks->BeginMarkerCallback = std::move(InBeginCallback); }

void LogSystem::SetEndMarkerCallback(EndMarkerCallbackHandler InEndCallback) { Callbacks->EndMarkerCallback = std::move(InEndCallback); }

void LogSystem::SetSystemLevel(const csp::common::LogLevel InSystemLevel) { SystemLevel = InSystemLevel; }

csp::common::LogLevel LogSystem::GetSystemLevel() { return SystemLevel; }

bool LogSystem::LoggingEnabled(const csp::common::LogLevel Level) { return Level <= SystemLevel; }

void LogSystem::LogMsg(const csp::common::LogLevel Level, const csp::common::String& InMessage)
{
    if (!LoggingEnabled(Level))
    {
        return;
    }

    // Log to our Connected Spaces Platform file system.
    LogToFile(InMessage);

    if (Callbacks->LogCallback != nullptr)
    {
        // Send message to clients to display the log on the client side.
        Callbacks->LogCallback(Level, InMessage);
    }
    else
    {
#if defined(CSP_WASM)
        printf("%s\n", InMessage.c_str());
#endif

#if defined(CSP_ANDROID)
        __android_log_print(ANDROID_LOG_VERBOSE, "CSP", InMessage.c_str());
#endif
    }
}

void LogSystem::LogEvent(const csp::common::String& InEvent)
{
    if (Callbacks->EventCallback != nullptr)
    {
        // Send message to clients to display the log on the client side.
        Callbacks->EventCallback(InEvent);
    }
}

void LogSystem::BeginMarker(const csp::common::String& InMarker)
{
    if (Callbacks->BeginMarkerCallback != nullptr)
    {
        // Send message to clients to display the log on the client side.
        Callbacks->BeginMarkerCallback(InMarker);
    }
}

void LogSystem::EndMarker()
{
    if (Callbacks->EndMarkerCallback != nullptr)
    {
        // Send message to clients to display the log on the client side.
        Callbacks->EndMarkerCallback(nullptr);
    }
}

void LogSystem::LogToFile(const csp::common::String& InMessage) { CSP_LOG(InMessage.c_str()); }

void LogSystem::ClearAllCallbacks() { Callbacks->Clear(); }

} // namespace csp::systems
