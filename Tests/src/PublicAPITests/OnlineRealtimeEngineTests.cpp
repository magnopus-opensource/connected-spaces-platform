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
#include "CSP/Common/ContinuationUtils.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Debug/Logging.h"
#include "Mocks/SignalRConnectionMock.h"
#include "RAIIMockLogger.h"
#include "TestHelpers.h"

#include "signalrclient/signalr_value.h"
#include "gtest/gtest.h"
#include <memory>

using namespace csp::multiplayer;

namespace
{
class MockEntityCreatedCallback
{
public:
    MOCK_METHOD(void, Call, (csp::multiplayer::SpaceEntity*), ());
};

}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, TestSuccessInRemoteGenerateNewAvatarId)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };

    // SignalR populates a result and not an exception
    EXPECT_CALL(
        *SignalRMock, Invoke(Connection->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::GENERATE_OBJECT_IDS), ::testing::_, ::testing::_))
        .WillOnce(
            [](const std::string& /**/, const signalr::value& /**/,
                std::function<void(const signalr::value&, std::exception_ptr)> /**/) -> async::task<std::tuple<signalr::value, std::exception_ptr>>
            {
                // For some reason the ID value has to be an array :/
                std::vector<signalr::value> ids;
                ids.emplace_back(signalr::value(uint64_t(55)));
                // Construct a signalr::value that holds an array of those IDs
                signalr::value Value(ids);

                return async::make_task(std::make_tuple(Value, std::exception_ptr(nullptr)));
            });

    RealtimeEngine->RemoteGenerateNewAvatarId()
        .then(async::inline_scheduler(),
            [](async::shared_task<uint64_t> Result)
            {
                EXPECT_FALSE(Result.get_exception());
                EXPECT_EQ(Result.get(), uint64_t(55));
            })
        .then(async::inline_scheduler(),
            [](async::task<void> CheckForErrorsTask)
            { EXPECT_FALSE(CheckForErrorsTask.get_exception()); }); // This is to be paranoid and guard against errors in writing the test, as async++
                                                                    // will catch exceptions and convert to a friendly cancel if they occur.
}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, TestErrorInRemoteGenerateNewAvatarId)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };

    // SignalR populates an exception
    EXPECT_CALL(
        *SignalRMock, Invoke(Connection->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::GENERATE_OBJECT_IDS), ::testing::_, ::testing::_))
        .WillOnce(
            [](const std::string& /**/, const signalr::value& /**/, std::function<void(const signalr::value&, std::exception_ptr)> /**/) {
                return async::make_task(
                    std::make_tuple(signalr::value("Irrelevant value"), std::make_exception_ptr(std::runtime_error("mock exception"))));
            });

    RealtimeEngine->RemoteGenerateNewAvatarId()
        .then(
            [](async::shared_task<uint64_t> Result)
            {
                EXPECT_TRUE(Result.get_exception());
                try
                {
                    std::rethrow_exception(Result.get_exception());
                }
                catch (std::runtime_error error)
                {
                    EXPECT_EQ(std::string(error.what()), std::string("mock exception"));
                }
            })
        .then(async::inline_scheduler(),
            [](async::task<void> CheckForErrorsTask)
            { EXPECT_FALSE(CheckForErrorsTask.get_exception()); }); // This is to be paranoid and guard against errors in writing the test, as async++
                                                                    // will catch exceptions and convert to a friendly cancel if they occur.
}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, TestSuccessInSendNewAvatarObjectMessage)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };

    // SignalR populates a result and not an exception
    EXPECT_CALL(
        *SignalRMock, Invoke(Connection->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_MESSAGE), ::testing::_, ::testing::_))
        .WillOnce(
            [](const std::string& /**/, const signalr::value& /**/,
                std::function<void(const signalr::value&, std::exception_ptr)> /**/) -> async::task<std::tuple<signalr::value, std::exception_ptr>>
            { return async::make_task(std::make_tuple(signalr::value(true), std::exception_ptr(nullptr))); });

    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;

    const auto LoginState = SystemsManager.GetUserSystem()->GetLoginState();

    async::spawn(async::inline_scheduler(), []() { return uint64_t(55); }) // This continuation takes the ID as its input
        .then(async::inline_scheduler(),
            RealtimeEngine->SendNewAvatarObjectMessage(
                "Username", LoginState.UserId, UserTransform, IsVisible, "AvatarId", AvatarState::Idle, AvatarPlayMode::Default))
        .then(async::inline_scheduler(),
            [](std::tuple<const signalr::value&, std::exception_ptr> Results)
            {
                auto [Result, Exception] = Results;
                EXPECT_EQ(Result.as_bool(), true);
                EXPECT_FALSE(Exception);
            })
        .then(async::inline_scheduler(),
            [](async::task<void> CheckForErrorsTask)
            { EXPECT_FALSE(CheckForErrorsTask.get_exception()); }); // This is to be paranoid and guard against errors in writing the test, as async++
                                                                    // will catch exceptions and convert to a friendly cancel if they occur.
}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, TestErrorInSendNewAvatarObjectMessage)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };

    // SignalR populates an exception
    EXPECT_CALL(
        *SignalRMock, Invoke(Connection->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_MESSAGE), ::testing::_, ::testing::_))
        .WillOnce(
            [](const std::string& /**/, const signalr::value& /**/,
                std::function<void(const signalr::value&, std::exception_ptr)> /**/) -> async::task<std::tuple<signalr::value, std::exception_ptr>> {
                return async::make_task(
                    std::make_tuple(signalr::value("Irrelevent Value"), std::make_exception_ptr(std::runtime_error("mock exception"))));
            });

    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;

    const auto LoginState = SystemsManager.GetUserSystem()->GetLoginState();

    async::spawn(async::inline_scheduler(), []() { return uint64_t(55); }) // This continuation takes the ID as its input
        .then(async::inline_scheduler(),
            RealtimeEngine->SendNewAvatarObjectMessage(
                "Username", LoginState.UserId, UserTransform, IsVisible, "AvatarId", AvatarState::Idle, AvatarPlayMode::Default))
        .then(async::inline_scheduler(),
            [](std::tuple<const signalr::value&, std::exception_ptr> Results)
            {
                auto [Result, Exception] = Results;
                EXPECT_TRUE(Exception);
                try
                {
                    std::rethrow_exception(Exception);
                }
                catch (std::runtime_error error)
                {
                    EXPECT_EQ(std::string(error.what()), std::string("mock exception"));
                }
            })
        .then(async::inline_scheduler(),
            [](async::task<void> CheckForErrorsTask)
            { EXPECT_FALSE(CheckForErrorsTask.get_exception()); }); // This is to be paranoid and guard against errors in writing the test, as async++
                                                                    // will catch exceptions and convert to a friendly cancel if they occur.
}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, TestSuccessInCreateNewLocalAvatar)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };

    using MockEntityCreatedCallback = testing::MockFunction<void(SpaceEntity*)>;
    MockEntityCreatedCallback MockCallback;

    const csp::common::String Username = "Username";
    const csp::common::String AvatarId = "AvatarId";
    AvatarState AvatarState = AvatarState::Flying;
    AvatarPlayMode AvatarPlayMode = AvatarPlayMode::Creator;
    const uint64_t Id = 55;
    const SpaceTransform UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;

    EXPECT_CALL(MockCallback, Call(::testing::_))
        .WillOnce(::testing::Invoke(
            [Id, &Username, &AvatarId, AvatarState, AvatarPlayMode, &UserTransform, IsVisible](SpaceEntity* CreatedSpaceEntity)
            {
                ASSERT_NE(CreatedSpaceEntity, nullptr);
                ASSERT_EQ(CreatedSpaceEntity->GetId(), Id);
                ASSERT_EQ(CreatedSpaceEntity->GetName(), Username);

                ASSERT_EQ(CreatedSpaceEntity->GetComponents()->Size(), 1);

                ComponentBase* AvatarComponentBase = CreatedSpaceEntity->GetComponent(0);
                ASSERT_EQ(AvatarComponentBase->GetComponentType(), ComponentType::AvatarData);

                AvatarSpaceComponent* AvatarComponent = dynamic_cast<AvatarSpaceComponent*>(AvatarComponentBase);
                ASSERT_NE(AvatarComponent, nullptr);
                ASSERT_EQ(AvatarComponent->GetAvatarId(), AvatarId);
                ASSERT_EQ(AvatarComponent->GetAvatarPlayMode(), AvatarPlayMode);
                ASSERT_EQ(AvatarComponent->GetState(), AvatarState);
                ASSERT_EQ(AvatarComponent->GetIsVisible(), IsVisible);
            }));

    const auto LoginState = SystemsManager.GetUserSystem()->GetLoginState();

    async::spawn(async::inline_scheduler(),
        []()
        {
            return std::make_tuple(async::make_task(uint64_t { 55 }).share(), async::make_task());
        }) // This continuation takes the ID (and another void return from a when_all branch) as its input
        .then(async::inline_scheduler(),
            RealtimeEngine->CreateNewLocalAvatar(
                Username, LoginState.UserId, UserTransform, IsVisible, AvatarId, AvatarState, AvatarPlayMode, MockCallback.AsStdFunction()));
}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, TestErrorLoggedFromWholeCreateAvatarChain)
{
    RAIIMockLogger MockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(csp::common::LogLevel::Log);

    auto& SystemsManager = csp::systems::SystemsManager::Get();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> RealtimeEngine { SystemsManager.MakeOnlineRealtimeEngine() };

    // SignalR populates an exception
    EXPECT_CALL(*SignalRMock, Invoke)
        .WillOnce(
            [](const std::string& /**/, const signalr::value& /**/, std::function<void(const signalr::value&, std::exception_ptr)> /**/)
            {
                return async::make_task(std::make_tuple(signalr::value("Irrelevent Value"),
                    std::make_exception_ptr(csp::common::continuations::ErrorCodeException(ErrorCode::None, "mock exception"))));
            });

    using MockEntityCreatedCallback = testing::MockFunction<void(SpaceEntity*)>;
    MockEntityCreatedCallback MockCallback;

    // Expect the callback gets nullptr (not the greatest error return...)
    EXPECT_CALL(MockCallback, Call(nullptr));

    // Expect that we log the error message
    const csp::common::String ErrorMsg = "Failed to create Avatar. Exception: mock exception";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, ErrorMsg)).Times(1);

    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;

    const auto LoginState = SystemsManager.GetUserSystem()->GetLoginState();

    RealtimeEngine->CreateAvatar("Username", LoginState.UserId, UserTransform, IsVisible, AvatarState::Idle, "AvatarId", AvatarPlayMode::Default,
        MockCallback.AsStdFunction());
}
