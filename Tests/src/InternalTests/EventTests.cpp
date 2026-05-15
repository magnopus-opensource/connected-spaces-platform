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

    virtual void OnEvent(const Event& inEvent) override
    {
        if (inEvent.GetId() == USERSERVICE_LOGIN_EVENT_ID)
        {
            std::cerr << "LoginEvent Received" << std::endl;

            EXPECT_TRUE(strcmp(inEvent.GetString("UserId"), "MyUserId") == 0);
            EXPECT_TRUE(inEvent.GetInt("TestInt") == 384);
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

    virtual void OnEvent(const Event& inEvent) override
    {
        if (inEvent.GetId() == kTestEventId)
        {
            EXPECT_TRUE(inEvent.GetFloat("TestFloat") == 3.14f);
            EXPECT_TRUE(inEvent.GetBool("TestBool") == true);

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

    virtual void OnEvent(const Event& inEvent) override
    {
        if (inEvent.GetId() == kTestEventId)
        {
            EXPECT_TRUE(inEvent.GetFloat("TestFloat") == 3.14f);
            EXPECT_TRUE(inEvent.GetBool("TestBool") == true);

            std::cerr << "TestEvent Received" << std::endl;
        }
        else if (inEvent.GetId() == USERSERVICE_LOGIN_EVENT_ID)
        {
            EXPECT_TRUE(strcmp(inEvent.GetString("UserId"), "MyUserId") == 0);
            EXPECT_TRUE(inEvent.GetInt("TestInt") == 384);

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
    EventSystem& olyEvents = EventSystem::Get();

    Event* loginEvent = olyEvents.AllocateEvent(USERSERVICE_LOGIN_EVENT_ID);

    loginEvent->AddString("UserId", "MyUserId");
    loginEvent->AddInt("TestInt", 384);

    Event* testEvent = olyEvents.AllocateEvent(kTestEventId);

    testEvent->AddFloat("TestFloat", 3.14f);
    testEvent->AddBool("TestBool", true);

    LoginEventHandler loginHandler;
    TestEventHandler testHandler;
    AllEventHandler allHandler;

    olyEvents.RegisterListener(USERSERVICE_LOGIN_EVENT_ID, &loginHandler);
    olyEvents.RegisterListener(USERSERVICE_LOGIN_EVENT_ID, &allHandler);
    olyEvents.RegisterListener(kTestEventId, &testHandler);
    olyEvents.RegisterListener(kTestEventId, &allHandler);

    olyEvents.EnqueueEvent(loginEvent);
    olyEvents.EnqueueEvent(testEvent);
    olyEvents.ProcessEvents();

    olyEvents.UnRegisterListener(USERSERVICE_LOGIN_EVENT_ID, &loginHandler);
    olyEvents.UnRegisterListener(USERSERVICE_LOGIN_EVENT_ID, &allHandler);
    olyEvents.UnRegisterListener(kTestEventId, &testHandler);
    olyEvents.UnRegisterListener(kTestEventId, &allHandler);
}
