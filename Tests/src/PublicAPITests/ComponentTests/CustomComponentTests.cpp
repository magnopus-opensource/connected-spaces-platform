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

#include "../AssetSystemTestHelpers.h"
#include "../SpaceSystemTestHelpers.h"
#include "../UserSystemTestHelpers.h"
#include "Awaitable.h"
#include "CSP/CSPFoundation.h"
#include "CSP/Common/Optional.h"
#include "CSP/Multiplayer/Components/CustomSpaceComponent.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "TestHelpers.h"

#include "gtest/gtest.h"
#include <chrono>
#include <filesystem>
#include <thread>

using namespace csp::multiplayer;
using namespace std::chrono_literals;

namespace
{

bool RequestPredicate(const csp::systems::ResultBase& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, CustomTests, SetGetCustomPropertyTest)
{
    SpaceEntity* mySpaceEntity = new SpaceEntity();
    CustomSpaceComponent myCustomComponent(csp::systems::SystemsManager::Get().GetLogSystem(), mySpaceEntity);

    const csp::common::String propertyKey("MyPropertyKey");
    const csp::common::String myString("MyTestString");
    csp::common::ReplicatedValue testStringValue(myString);

    myCustomComponent.SetCustomProperty(propertyKey, testStringValue);

    EXPECT_TRUE(myCustomComponent.GetCustomProperty(propertyKey) == testStringValue);
}

CSP_PUBLIC_TEST(CSPEngine, CustomTests, CustomComponentTest)
{
    SetRandSeed();

    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* userSystem = systemsManager.GetUserSystem();
    auto* spaceSystem = systemsManager.GetSpaceSystem();

    const csp::common::String objectName = "Object 1";
    const csp::common::String applicationOrigin = "Application Origin 1";

    // Current default properties:
    // - ComponentName
    const int defaultComponentProps = 1;

    // Log in
    csp::common::String userId;
    LogInAsNewTestUser(userSystem, userId);

    // Create space
    csp::systems::Space space;
    CreateDefaultTestSpace(spaceSystem, space);

    {
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        realtimeEngine->SetRemoteEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

        // Create object to represent the Custom fields
        SpaceTransform objectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(realtimeEngine.get(), CreateEntity, objectName, objectTransform, csp::common::Optional<uint64_t> {});

        // Create custom component
        auto* customComponent = (CustomSpaceComponent*)CreatedObject->AddComponent(ComponentType::Custom);

        EXPECT_EQ(customComponent->GetCustomPropertyKeys().Size(), 0);

        // Spectify the application origin and verify
        customComponent->SetApplicationOrigin(applicationOrigin);

        EXPECT_EQ(customComponent->GetApplicationOrigin(), applicationOrigin);

        // Vector Check
        {
            customComponent->SetCustomProperty("Vector3", csp::common::ReplicatedValue({ 10, 10, 10 }));

            EXPECT_EQ(customComponent->GetCustomProperty("Vector3").GetVector3(), csp::common::Vector3({ 10, 10, 10 }));

            customComponent->SetCustomProperty("Vector4", csp::common::ReplicatedValue({ 10, 10, 10, 10 }));

            EXPECT_EQ(customComponent->GetCustomProperty("Vector4").GetVector4(), csp::common::Vector4({ 10, 10, 10, 10 }));
        }

        // String Check
        {
            customComponent->SetCustomProperty("String", csp::common::ReplicatedValue("OKO"));

            EXPECT_EQ(customComponent->GetCustomProperty("String").GetString(), "OKO");
        }

        // Boolean Check
        {
            customComponent->SetCustomProperty("Boolean", csp::common::ReplicatedValue(true));

            EXPECT_EQ(customComponent->GetCustomProperty("Boolean").GetBool(), true);
        }

        // Integer Check
        {
            customComponent->SetCustomProperty("Integer", csp::common::ReplicatedValue(int64_t(1)));

            EXPECT_EQ(customComponent->GetCustomProperty("Integer").GetInt(), int64_t(1));
        }

        // Float Check
        {
            customComponent->SetCustomProperty("Float", csp::common::ReplicatedValue(1.00f));

            EXPECT_EQ(customComponent->GetCustomProperty("Float").GetFloat(), 1.00f);
        }

        // Has Key Check
        {
            EXPECT_EQ(customComponent->HasCustomProperty("Boolean"), true);
            EXPECT_EQ(customComponent->HasCustomProperty("BooleanFalse"), false);
        }

        // Key Size
        {
            // Custom properties including application origin + default props
            EXPECT_EQ(customComponent->GetNumProperties(), 7 + defaultComponentProps);
        }

        // Remove Key
        {
            customComponent->RemoveCustomProperty("Boolean");

            // Custom properties including application origin + default props
            EXPECT_EQ(customComponent->GetNumProperties(), 6 + defaultComponentProps);
        }

        // List Check
        {
            auto keys = customComponent->GetCustomPropertyKeys();

            EXPECT_EQ(keys.Size(), 5);
        }

        // Queue update process before exiting space
        realtimeEngine->QueueEntityUpdate(CreatedObject);
        realtimeEngine->ProcessPendingEntityOperations();

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);

        // Ensure component data has been written to database by chs before entering the space again
        // This is due to an enforced 2 second chs database write delay
        std::this_thread::sleep_for(7s);
    }

    // Re-Enter space and verify contents
    {
        // Retrieve all entities
        auto gotAllEntities = false;
        SpaceEntity* loadedObject;

        // Reload the space and verify the contents match
        std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };
        realtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        realtimeEngine->SetRemoteEntityCreatedCallback(
            [&](SpaceEntity* entity)
            {
                if (entity->GetName() == objectName)
                {
                    gotAllEntities = true;
                    loadedObject = entity;
                }
            });

        auto [EnterResult] = AWAIT_PRE(spaceSystem, EnterSpace, RequestPredicate, space.Id, realtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        // Wait until loaded
        auto start = std::chrono::steady_clock::now();
        auto current = std::chrono::steady_clock::now();
        int64_t testTime = 0;

        while (!gotAllEntities && testTime < 20)
        {
            std::this_thread::sleep_for(50ms);

            current = std::chrono::steady_clock::now();
            testTime = std::chrono::duration_cast<std::chrono::seconds>(current - start).count();
        }

        ASSERT_TRUE(gotAllEntities);

        const auto& components = *loadedObject->GetComponents();

        EXPECT_EQ(components.Size(), 1);

        // Retreive the custom component
        auto loadedComponent = components[0];

        // Verify the component type
        EXPECT_EQ(loadedComponent->GetComponentType(), ComponentType::Custom);

        // Verify the application
        auto* customComponent = (CustomSpaceComponent*)loadedComponent;

        EXPECT_EQ(customComponent->GetApplicationOrigin(), applicationOrigin);

        // List Check
        {
            auto keys = customComponent->GetCustomPropertyKeys();
            EXPECT_EQ(keys.Size(), 5);

            // Vector Check
            {
                EXPECT_EQ(customComponent->GetCustomProperty("Vector3").GetVector3(), csp::common::Vector3({ 10, 10, 10 }));
                EXPECT_EQ(customComponent->GetCustomProperty("Vector4").GetVector4(), csp::common::Vector4({ 10, 10, 10, 10 }));
            }

            // String Check
            {
                EXPECT_EQ(customComponent->GetCustomProperty("String").GetString(), "OKO");
            }

            // Integer Check
            {
                EXPECT_EQ(customComponent->GetCustomProperty("Integer").GetInt(), int64_t(1));
            }

            // Float Check
            {
                EXPECT_EQ(customComponent->GetCustomProperty("Float").GetFloat(), 1.00f);
            }

            // Has Missing Key Check
            {
                EXPECT_EQ(customComponent->HasCustomProperty("Boolean"), false);
            }
        }

        auto [ExitSpaceResult] = AWAIT_PRE(spaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(spaceSystem, space.Id);

    // Log out
    LogOut(userSystem);
}