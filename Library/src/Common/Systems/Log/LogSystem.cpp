/*
 * Copyright 2025 Magnopus LLC

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
    m_callbacks = new LogCallbacks();
}

LogSystem::~LogSystem() { delete m_callbacks; }

void LogSystem::SetLogCallback(LogCallbackHandler inLogCallback) { m_callbacks->LogCallback = std::move(inLogCallback); }

void LogSystem::SetEventCallback(EventCallbackHandler inEventCallback) { m_callbacks->EventCallback = std::move(inEventCallback); }

void LogSystem::SetBeginMarkerCallback(BeginMarkerCallbackHandler inBeginCallback) { m_callbacks->BeginMarkerCallback = std::move(inBeginCallback); }

void LogSystem::SetEndMarkerCallback(EndMarkerCallbackHandler inEndCallback) { m_callbacks->EndMarkerCallback = std::move(inEndCallback); }

void LogSystem::SetSystemLevel(const csp::common::LogLevel inSystemLevel) { m_systemLevel = inSystemLevel; }

csp::common::LogLevel LogSystem::GetSystemLevel() { return m_systemLevel; }

bool LogSystem::LoggingEnabled(const csp::common::LogLevel level) { return level <= m_systemLevel; }

void LogSystem::LogMsg(const csp::common::LogLevel level, const csp::common::String& inMessage)
{
    if (!LoggingEnabled(level))
    {
        return;
    }

    // Log to our Connected Spaces Platform file system.
    LogToFile(level, inMessage);

    if (m_callbacks->LogCallback != nullptr)
    {
        // Send message to clients to display the log on the client side.
        m_callbacks->LogCallback(level, inMessage);
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

void LogSystem::LogEvent(const csp::common::String& inEvent)
{
    if (m_callbacks->EventCallback != nullptr)
    {
        // Send message to clients to display the log on the client side.
        m_callbacks->EventCallback(inEvent);
    }
}

void LogSystem::BeginMarker(const csp::common::String& inMarker)
{
    if (m_callbacks->BeginMarkerCallback != nullptr)
    {
        // Send message to clients to display the log on the client side.
        m_callbacks->BeginMarkerCallback(inMarker);
    }
}

void LogSystem::EndMarker()
{
    if (m_callbacks->EndMarkerCallback != nullptr)
    {
        // Send message to clients to display the log on the client side.
        m_callbacks->EndMarkerCallback(nullptr);
    }
}

void LogSystem::LogToFile(const csp::common::LogLevel level, const csp::common::String& inMessage) { CSP_LOG(level, inMessage.c_str()); }

void LogSystem::ClearAllCallbacks() { m_callbacks->Clear(); }

} // namespace csp::systems
