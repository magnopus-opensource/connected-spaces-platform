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
#ifndef SKIP_INTERNAL_TESTS

#include "CSP/CSPFoundation.h"
#include "Events/EventSystem.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"

using namespace csp::events;

const EventId kTestEventId = EventId("TestEvent", "Test");

class LoginEventHandler : public EventListener
{
public:
    LoginEventHandler() { }

    virtual void OnEvent(const Event& InEvent) override
    {
        if (InEvent.GetId() == USERSERVICE_LOGIN_EVENT_ID)
        {
            std::cerr << "LoginEvent Received" << std::endl;

            EXPECT_TRUE(strcmp(InEvent.GetString("UserId"), "MyUserId") == 0);
            EXPECT_TRUE(InEvent.GetInt("TestInt") == 384);
        }
        else
        {
            FAIL();
        }
    }
};

class TestEventHandler : public EventListener
{
public:
    TestEventHandler() { }

    virtual void OnEvent(const Event& InEvent) override
    {
        if (InEvent.GetId() == kTestEventId)
        {
            EXPECT_TRUE(InEvent.GetFloat("TestFloat") == 3.14f);
            EXPECT_TRUE(InEvent.GetBool("TestBool") == true);

            std::cerr << "TestEvent Received" << std::endl;
        }
        else
        {
            FAIL();
        }
    }
};

class AllEventHandler : public EventListener
{
public:
    AllEventHandler() { }

    virtual void OnEvent(const Event& InEvent) override
    {
        if (InEvent.GetId() == kTestEventId)
        {
            EXPECT_TRUE(InEvent.GetFloat("TestFloat") == 3.14f);
            EXPECT_TRUE(InEvent.GetBool("TestBool") == true);

            std::cerr << "TestEvent Received" << std::endl;
        }
        else if (InEvent.GetId() == USERSERVICE_LOGIN_EVENT_ID)
        {
            EXPECT_TRUE(strcmp(InEvent.GetString("UserId"), "MyUserId") == 0);
            EXPECT_TRUE(InEvent.GetInt("TestInt") == 384);

            std::cerr << "LoginEvent Received" << std::endl;
        }
        else
        {
            FAIL();
        }
    }
};

CSP_INTERNAL_TEST(CSPEngine, EventTests, EventSystemTest)
{
    EventSystem& OlyEvents = EventSystem::Get();

    Event* LoginEvent = OlyEvents.AllocateEvent(USERSERVICE_LOGIN_EVENT_ID);

    LoginEvent->AddString("UserId", "MyUserId");
    LoginEvent->AddInt("TestInt", 384);

    Event* TestEvent = OlyEvents.AllocateEvent(kTestEventId);

    TestEvent->AddFloat("TestFloat", 3.14f);
    TestEvent->AddBool("TestBool", true);

    LoginEventHandler LoginHandler;
    TestEventHandler TestHandler;
    AllEventHandler AllHandler;

    OlyEvents.RegisterListener(USERSERVICE_LOGIN_EVENT_ID, &LoginHandler);
    OlyEvents.RegisterListener(USERSERVICE_LOGIN_EVENT_ID, &AllHandler);
    OlyEvents.RegisterListener(kTestEventId, &TestHandler);
    OlyEvents.RegisterListener(kTestEventId, &AllHandler);

    OlyEvents.EnqueueEvent(LoginEvent);
    OlyEvents.EnqueueEvent(TestEvent);
    OlyEvents.ProcessEvents();

    OlyEvents.UnRegisterListener(USERSERVICE_LOGIN_EVENT_ID, &LoginHandler);
    OlyEvents.UnRegisterListener(USERSERVICE_LOGIN_EVENT_ID, &AllHandler);
    OlyEvents.UnRegisterListener(kTestEventId, &TestHandler);
    OlyEvents.UnRegisterListener(kTestEventId, &AllHandler);
}

#endif
