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

bool RequestPredicate(const csp::systems::ResultBase& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; }

} // namespace

CSP_PUBLIC_TEST(CSPEngine, CustomTests, SetGetCustomPropertyTest)
{
    SpaceEntity* MySpaceEntity = new SpaceEntity();
    CustomSpaceComponent MyCustomComponent(csp::systems::SystemsManager::Get().GetLogSystem(), MySpaceEntity);

    const csp::common::String PropertyKey("MyPropertyKey");
    const csp::common::String MyString("MyTestString");
    csp::common::ReplicatedValue TestStringValue(MyString);

    MyCustomComponent.SetCustomProperty(PropertyKey, TestStringValue);

    EXPECT_TRUE(MyCustomComponent.GetCustomProperty(PropertyKey) == TestStringValue);
}

CSP_PUBLIC_TEST(CSPEngine, CustomTests, CustomComponentTest)
{
    SetRandSeed();

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* UserSystem = SystemsManager.GetUserSystem();
    auto* SpaceSystem = SystemsManager.GetSpaceSystem();

    const char* TestSpaceName = "CSP-UNITTEST-SPACE-MAG";
    const char* TestSpaceDescription = "CSP-UNITTEST-SPACEDESC-MAG";

    const char* TestSpaceName2 = "CSP-UNITTEST-SPACE-MAG-2";

    const csp::common::String ObjectName = "Object 1";
    const csp::common::String ApplicationOrigin = "Application Origin 1";

    char UniqueSpaceName[256];
    SPRINTF(UniqueSpaceName, "%s-%s", TestSpaceName, GetUniqueString().c_str());

    char UniqueSpaceName2[256];
    SPRINTF(UniqueSpaceName2, "%s-%s", TestSpaceName2, GetUniqueString().c_str());

    // Current default properties:
    // - ComponentName
    const int DefaultComponentProps = 1;

    // Log in
    csp::common::String UserId;
    LogInAsNewTestUser(UserSystem, UserId);

    // Create space
    csp::systems::Space Space;
    CreateSpace(
        SpaceSystem, UniqueSpaceName, TestSpaceDescription, csp::systems::SpaceAttributes::Private, nullptr, nullptr, nullptr, nullptr, Space);

    {
        std::unique_ptr<csp::multiplayer::SpaceEntitySystem> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
        RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());

        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        RealtimeEngine->SetEntityCreatedCallback([](csp::multiplayer::SpaceEntity* /*Entity*/) {});

        // Create object to represent the Custom fields
        SpaceTransform ObjectTransform = { csp::common::Vector3::Zero(), csp::common::Vector4::Zero(), csp::common::Vector3::One() };
        auto [CreatedObject] = AWAIT(RealtimeEngine.get(), CreateEntity, ObjectName, ObjectTransform, csp::common::Optional<uint64_t> {});

        // Create custom component
        auto* CustomComponent = (CustomSpaceComponent*)CreatedObject->AddComponent(ComponentType::Custom);

        EXPECT_EQ(CustomComponent->GetCustomPropertyKeys().Size(), 0);

        // Spectify the application origin and verify
        CustomComponent->SetApplicationOrigin(ApplicationOrigin);

        EXPECT_EQ(CustomComponent->GetApplicationOrigin(), ApplicationOrigin);

        // Vector Check
        {
            CustomComponent->SetCustomProperty("Vector3", csp::common::ReplicatedValue({ 10, 10, 10 }));

            EXPECT_EQ(CustomComponent->GetCustomProperty("Vector3").GetVector3(), csp::common::Vector3({ 10, 10, 10 }));

            CustomComponent->SetCustomProperty("Vector4", csp::common::ReplicatedValue({ 10, 10, 10, 10 }));

            EXPECT_EQ(CustomComponent->GetCustomProperty("Vector4").GetVector4(), csp::common::Vector4({ 10, 10, 10, 10 }));
        }

        // String Check
        {
            CustomComponent->SetCustomProperty("String", csp::common::ReplicatedValue("OKO"));

            EXPECT_EQ(CustomComponent->GetCustomProperty("String").GetString(), "OKO");
        }

        // Boolean Check
        {
            CustomComponent->SetCustomProperty("Boolean", csp::common::ReplicatedValue(true));

            EXPECT_EQ(CustomComponent->GetCustomProperty("Boolean").GetBool(), true);
        }

        // Integer Check
        {
            CustomComponent->SetCustomProperty("Integer", csp::common::ReplicatedValue(int64_t(1)));

            EXPECT_EQ(CustomComponent->GetCustomProperty("Integer").GetInt(), int64_t(1));
        }

        // Float Check
        {
            CustomComponent->SetCustomProperty("Float", csp::common::ReplicatedValue(1.00f));

            EXPECT_EQ(CustomComponent->GetCustomProperty("Float").GetFloat(), 1.00f);
        }

        // Has Key Check
        {
            EXPECT_EQ(CustomComponent->HasCustomProperty("Boolean"), true);
            EXPECT_EQ(CustomComponent->HasCustomProperty("BooleanFalse"), false);
        }

        // Key Size
        {
            // Custom properties including application origin + default props
            EXPECT_EQ(CustomComponent->GetNumProperties(), 7 + DefaultComponentProps);
        }

        // Remove Key
        {
            CustomComponent->RemoveCustomProperty("Boolean");

            // Custom properties including application origin + default props
            EXPECT_EQ(CustomComponent->GetNumProperties(), 6 + DefaultComponentProps);
        }

        // List Check
        {
            auto Keys = CustomComponent->GetCustomPropertyKeys();

            EXPECT_EQ(Keys.Size(), 5);
        }

        // Queue update process before exiting space
        RealtimeEngine->QueueEntityUpdate(CreatedObject);
        RealtimeEngine->ProcessPendingEntityOperations();

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);

        // Ensure component data has been written to database by chs before entering the space again
        // This is due to an enforced 2 second chs database write delay
        std::this_thread::sleep_for(7s);
    }

    std::this_thread::sleep_for(std::chrono::seconds(7));

    // Re-Enter space and verify contents
    {
        // Retrieve all entities
        auto GotAllEntities = false;
        SpaceEntity* LoadedObject;

        // Reload the space and verify the contents match
        std::unique_ptr<csp::multiplayer::SpaceEntitySystem> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };
        RealtimeEngine->SetEntityFetchCompleteCallback([](uint32_t) {});

        RealtimeEngine->SetEntityCreatedCallback(
            [&](SpaceEntity* Entity)
            {
                if (Entity->GetName() == ObjectName)
                {
                    GotAllEntities = true;
                    LoadedObject = Entity;
                }
            });

        auto [EnterResult] = AWAIT_PRE(SpaceSystem, EnterSpace, RequestPredicate, Space.Id, RealtimeEngine.get());
        EXPECT_EQ(EnterResult.GetResultCode(), csp::systems::EResultCode::Success);

        // Wait until loaded
        auto Start = std::chrono::steady_clock::now();
        auto Current = std::chrono::steady_clock::now();
        int64_t TestTime = 0;

        while (!GotAllEntities && TestTime < 20)
        {
            std::this_thread::sleep_for(50ms);

            Current = std::chrono::steady_clock::now();
            TestTime = std::chrono::duration_cast<std::chrono::seconds>(Current - Start).count();
        }

        ASSERT_TRUE(GotAllEntities);

        const auto& Components = *LoadedObject->GetComponents();

        EXPECT_EQ(Components.Size(), 1);

        // Retreive the custom component
        auto LoadedComponent = Components[0];

        // Verify the component type
        EXPECT_EQ(LoadedComponent->GetComponentType(), ComponentType::Custom);

        // Verify the application
        auto* CustomComponent = (CustomSpaceComponent*)LoadedComponent;

        EXPECT_EQ(CustomComponent->GetApplicationOrigin(), ApplicationOrigin);

        // List Check
        {
            auto Keys = CustomComponent->GetCustomPropertyKeys();
            EXPECT_EQ(Keys.Size(), 5);

            // Vector Check
            {
                EXPECT_EQ(CustomComponent->GetCustomProperty("Vector3").GetVector3(), csp::common::Vector3({ 10, 10, 10 }));
                EXPECT_EQ(CustomComponent->GetCustomProperty("Vector4").GetVector4(), csp::common::Vector4({ 10, 10, 10, 10 }));
            }

            // String Check
            {
                EXPECT_EQ(CustomComponent->GetCustomProperty("String").GetString(), "OKO");
            }

            // Integer Check
            {
                EXPECT_EQ(CustomComponent->GetCustomProperty("Integer").GetInt(), int64_t(1));
            }

            // Float Check
            {
                EXPECT_EQ(CustomComponent->GetCustomProperty("Float").GetFloat(), 1.00f);
            }

            // Has Missing Key Check
            {
                EXPECT_EQ(CustomComponent->HasCustomProperty("Boolean"), false);
            }
        }

        auto [ExitSpaceResult] = AWAIT_PRE(SpaceSystem, ExitSpace, RequestPredicate);
    }

    // Delete space
    DeleteSpace(SpaceSystem, Space.Id);

    // Log out
    LogOut(UserSystem);
}