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
#include "Events/Event.h"
#include "Common/Wrappers.h"

#include <map>

namespace csp::events
{

class EventPayloadImpl
{
public:
    EventPayloadImpl();
    ~EventPayloadImpl();

    void AddInt(const char* key, const int value);
    void AddString(const char* key, const char* value);
    void AddFloat(const char* key, const float value);
    void AddBool(const char* key, const bool value);

    int GetInt(const char* key) const;
    const char* GetString(const char* key) const;
    float GetFloat(const char* key) const;
    bool GetBool(const char* key) const;

private:
    enum EParamType
    {
        TypeInt,
        TypeFloat,
        TypeString,
        TypeBool
    };

    struct EventParam
    {
        EventParam(EParamType type)
            : ParamType(type)
        {
        }

        EventParam(const EventParam& other)
        {
            ParamType = other.ParamType;

            switch (ParamType)
            {
            case TypeInt:
                IntParam = other.IntParam;
                break;

            case TypeFloat:
                FloatParam = other.FloatParam;
                break;

            case TypeBool:
                BoolParam = other.BoolParam;
                break;

            case TypeString:
                SetString(other.StringParam);
                break;

            default:
                break; // Unknown type
            }
        }

        ~EventParam()
        {
            if (ParamType == TypeString)
            {
                std::free(StringParam);
            }
        }

        void SetString(const char* inString)
        {
            size_t stringLen = strlen(inString);
            StringParam = (char*)std::malloc(stringLen + 1);

            STRCPY(StringParam, stringLen + 1, inString);
        }

        EParamType ParamType;

        union
        {
            int IntParam;
            float FloatParam;
            char* StringParam;
            bool BoolParam;
        };
    };

    using ParamMap = std::map<std::string, EventParam>;

    ParamMap m_parameters;
};

EventPayloadImpl::EventPayloadImpl() { }

EventPayloadImpl::~EventPayloadImpl() { }

void EventPayloadImpl::AddInt(const char* key, const int value)
{
    EventParam param(TypeInt);
    param.IntParam = value;
    m_parameters.insert(ParamMap::value_type(key, param));
}

void EventPayloadImpl::AddString(const char* key, const char* value)
{
    EventParam param(TypeString);
    param.SetString(value);
    m_parameters.insert(ParamMap::value_type(key, param));
}

void EventPayloadImpl::AddFloat(const char* key, const float value)
{
    EventParam param(TypeFloat);
    param.FloatParam = value;
    m_parameters.insert(ParamMap::value_type(key, param));
}

void EventPayloadImpl::AddBool(const char* key, const bool value)
{
    EventParam param(TypeBool);
    param.BoolParam = value;
    m_parameters.insert(ParamMap::value_type(key, param));
}

int EventPayloadImpl::GetInt(const char* key) const
{
    ParamMap::const_iterator it = m_parameters.find(key);
    if (it != m_parameters.end())
    {
        assert(it->second.ParamType == TypeInt);
        return it->second.IntParam;
    }

    return 0;
}

const char* EventPayloadImpl::GetString(const char* key) const
{
    ParamMap::const_iterator it = m_parameters.find(key);
    if (it != m_parameters.end())
    {
        assert(it->second.ParamType == TypeString);
        return it->second.StringParam;
    }

    return nullptr;
}

float EventPayloadImpl::GetFloat(const char* key) const
{
    ParamMap::const_iterator it = m_parameters.find(key);
    if (it != m_parameters.end())
    {
        assert(it->second.ParamType == TypeFloat);
        return it->second.FloatParam;
    }

    return 0.0f;
}

bool EventPayloadImpl::GetBool(const char* key) const
{
    ParamMap::const_iterator it = m_parameters.find(key);
    if (it != m_parameters.end())
    {
        assert(it->second.ParamType == TypeBool);
        return it->second.BoolParam;
    }

    return false;
}

Event::Event(const EventId& inId)
    : m_id(inId)
    , m_impl(new EventPayloadImpl())
{
}

Event::~Event() { delete (m_impl); }

void Event::AddInt(const char* key, const int value) { m_impl->AddInt(key, value); }

void Event::AddString(const char* key, const char* value) { m_impl->AddString(key, value); }

void Event::AddFloat(const char* key, const float value) { m_impl->AddFloat(key, value); }

void Event::AddBool(const char* key, const bool value) { m_impl->AddBool(key, value); }

int Event::GetInt(const char* key) const { return m_impl->GetInt(key); }

const char* Event::GetString(const char* key) const { return m_impl->GetString(key); }

float Event::GetFloat(const char* key) const { return m_impl->GetFloat(key); }

bool Event::GetBool(const char* key) const { return m_impl->GetBool(key); }

const EventId& Event::GetId() const { return m_id; }

} // namespace csp::events
