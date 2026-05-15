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
    void AddInt(const char* key, const int value);
    void AddString(const char* key, const char* value);
    void AddFloat(const char* key, const float value);
    void AddBool(const char* key, const bool value);

    // Get Payload params
    int GetInt(const char* key) const;
    const char* GetString(const char* key) const;
    float GetFloat(const char* key) const;
    bool GetBool(const char* key) const;

    const EventId& GetId() const;

private:
    Event(const EventId& inId);

    EventId m_id;
    class EventPayloadImpl* m_impl;
};

} // namespace csp::events
