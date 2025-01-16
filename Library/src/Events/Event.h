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
#include "Events/EventId.h"

#include <functional>

namespace csp::events
{

class CSP_API Event
{
    friend class EventSystem;

public:
    ~Event();

    // Add Payload params
    void AddInt(const char* Key, const int Value);
    void AddString(const char* Key, const char* Value);
    void AddFloat(const char* Key, const float Value);
    void AddBool(const char* Key, const bool Value);

    // Get Payload params
    const int GetInt(const char* Key) const;
    const char* GetString(const char* Key) const;
    const float GetFloat(const char* Key) const;
    bool GetBool(const char* Key) const;

    const EventId& GetId() const;

private:
    Event(const EventId& InId);

    EventId Id;
    class EventPayloadImpl* Impl;
};

} // namespace csp::events
