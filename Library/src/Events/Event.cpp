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

#include "Common/StlString.h"
#include "Common/Wrappers.h"
#include "Memory/Memory.h"

#include <map>

namespace csp::events
{

class EventPayloadImpl
{
public:
    EventPayloadImpl();
    ~EventPayloadImpl();

    void AddInt(const char* Key, const int Value);
    void AddString(const char* Key, const char* Value);
    void AddFloat(const char* Key, const float Value);
    void AddBool(const char* Key, const bool Value);

    const int GetInt(const char* Key) const;
    const char* GetString(const char* Key) const;
    const float GetFloat(const char* Key) const;
    bool GetBool(const char* Key) const;

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
        EventParam(EParamType Type)
            : ParamType(Type)
        {
        }

        EventParam(const EventParam& Other)
        {
            ParamType = Other.ParamType;

            switch (ParamType)
            {
            case TypeInt:
                IntParam = Other.IntParam;
                break;

            case TypeFloat:
                FloatParam = Other.FloatParam;
                break;

            case TypeBool:
                BoolParam = Other.BoolParam;
                break;

            case TypeString:
                SetString(Other.StringParam);
                break;

            default:
                break; // Unknown type
            }
        }

        ~EventParam()
        {
            if (ParamType == TypeString)
            {
                CSP_FREE(StringParam);
            }
        }

        void SetString(const char* InString)
        {
            size_t StringLen = strlen(InString);
            StringParam = (char*)CSP_ALLOC(StringLen + 1);

            STRCPY(StringParam, StringLen + 1, InString);
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

    using ParamMap
        = std::map<csp::StlString, EventParam, std::less<csp::StlString>, csp::memory::StlAllocator<std::pair<const csp::StlString, EventParam>>>;

    ParamMap Parameters;
};

EventPayloadImpl::EventPayloadImpl() { }

EventPayloadImpl::~EventPayloadImpl() { }

void EventPayloadImpl::AddInt(const char* Key, const int Value)
{
    EventParam Param(TypeInt);
    Param.IntParam = Value;
    Parameters.insert(ParamMap::value_type(Key, Param));
}

void EventPayloadImpl::AddString(const char* Key, const char* Value)
{
    EventParam Param(TypeString);
    Param.SetString(Value);
    Parameters.insert(ParamMap::value_type(Key, Param));
}

void EventPayloadImpl::AddFloat(const char* Key, const float Value)
{
    EventParam Param(TypeFloat);
    Param.FloatParam = Value;
    Parameters.insert(ParamMap::value_type(Key, Param));
}

void EventPayloadImpl::AddBool(const char* Key, const bool Value)
{
    EventParam Param(TypeBool);
    Param.BoolParam = Value;
    Parameters.insert(ParamMap::value_type(Key, Param));
}

const int EventPayloadImpl::GetInt(const char* Key) const
{
    ParamMap::const_iterator it = Parameters.find(Key);
    if (it != Parameters.end())
    {
        assert(it->second.ParamType == TypeInt);
        return it->second.IntParam;
    }

    return 0;
}

const char* EventPayloadImpl::GetString(const char* Key) const
{
    ParamMap::const_iterator it = Parameters.find(Key);
    if (it != Parameters.end())
    {
        assert(it->second.ParamType == TypeString);
        return it->second.StringParam;
    }

    return nullptr;
}

const float EventPayloadImpl::GetFloat(const char* Key) const
{
    ParamMap::const_iterator it = Parameters.find(Key);
    if (it != Parameters.end())
    {
        assert(it->second.ParamType == TypeFloat);
        return it->second.FloatParam;
    }

    return 0.0f;
}

bool EventPayloadImpl::GetBool(const char* Key) const
{
    ParamMap::const_iterator it = Parameters.find(Key);
    if (it != Parameters.end())
    {
        assert(it->second.ParamType == TypeBool);
        return it->second.BoolParam;
    }

    return false;
}

Event::Event(const EventId& InId)
    : Id(InId)
    , Impl(CSP_NEW EventPayloadImpl())
{
}

Event::~Event() { CSP_DELETE(Impl); }

void Event::AddInt(const char* Key, const int Value) { Impl->AddInt(Key, Value); }

void Event::AddString(const char* Key, const char* Value) { Impl->AddString(Key, Value); }

void Event::AddFloat(const char* Key, const float Value) { Impl->AddFloat(Key, Value); }

void Event::AddBool(const char* Key, const bool Value) { Impl->AddBool(Key, Value); }

const int Event::GetInt(const char* Key) const { return Impl->GetInt(Key); }

const char* Event::GetString(const char* Key) const { return Impl->GetString(Key); }

const float Event::GetFloat(const char* Key) const { return Impl->GetFloat(Key); }

bool Event::GetBool(const char* Key) const { return Impl->GetBool(Key); }

const EventId& Event::GetId() const { return Id; }

} // namespace csp::events
