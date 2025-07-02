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
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Debug/Logging.h"
#include "Mocks/SignalRConnectionMock.h"
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

/* We need to unset the mock logger before CSP shuts down,
 * because you get interdependent memory errors in the "Foundation shutdown"
 * log if you don't. (Another reason we don't want to be starting/stopping
 * ALL of CSP in these tests really.)
 */
struct RAIIMockLogger
{
    RAIIMockLogger() { csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(MockLogCallback.AsStdFunction()); }
    ~RAIIMockLogger() { csp::systems::SystemsManager::Get().GetLogSystem()->SetLogCallback(nullptr); }
    ::testing::MockFunction<void(const csp::common::String&)> MockLogCallback;
};

}

CSP_PUBLIC_TEST(CSPEngine, SpaceEntitySystemTests, TestSuccessInRemoteGenerateNewAvatarId)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* SpaceEntitySystem = SystemsManager.GetSpaceEntitySystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    auto SignalRMock = std::unique_ptr<SignalRConnectionMock>(new SignalRConnectionMock());
    SpaceEntitySystem->SetConnection(SignalRMock.get());

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

    SpaceEntitySystem->RemoteGenerateNewAvatarId()
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

    // During destruction (test cleanup) CSP can access the connection.
    // We can't leave the main Mock dangling because it needs to run RAII test assertion behaviour, so use a throwaway.
    SignalRConnectionMock* ThrowawaySignalRMock = new SignalRConnectionMock();
    SpaceEntitySystem->SetConnection(ThrowawaySignalRMock);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceEntitySystemTests, TestErrorInRemoteGenerateNewAvatarId)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* SpaceEntitySystem = SystemsManager.GetSpaceEntitySystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    auto SignalRMock = std::unique_ptr<SignalRConnectionMock>(new SignalRConnectionMock());
    SpaceEntitySystem->SetConnection(SignalRMock.get());

    // SignalR populates an exception
    EXPECT_CALL(
        *SignalRMock, Invoke(Connection->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::GENERATE_OBJECT_IDS), ::testing::_, ::testing::_))
        .WillOnce(
            [](const std::string& /**/, const signalr::value& /**/, std::function<void(const signalr::value&, std::exception_ptr)> /**/) {
                return async::make_task(
                    std::make_tuple(signalr::value("Irrelevant value"), std::make_exception_ptr(std::runtime_error("mock exception"))));
            });

    SpaceEntitySystem->RemoteGenerateNewAvatarId()
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

    // During destruction (test cleanup) CSP can access the connection.
    // We can't leave the main Mock dangling because it needs to run RAII test assertion behaviour, so use a throwaway.
    SignalRConnectionMock* ThrowawaySignalRMock = new SignalRConnectionMock();
    SpaceEntitySystem->SetConnection(ThrowawaySignalRMock);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceEntitySystemTests, TestSuccessInSendNewAvatarObjectMessage)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* SpaceEntitySystem = SystemsManager.GetSpaceEntitySystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    auto SignalRMock = std::unique_ptr<SignalRConnectionMock>(new SignalRConnectionMock());
    SpaceEntitySystem->SetConnection(SignalRMock.get());

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
            SpaceEntitySystem->SendNewAvatarObjectMessage(
                "Username", LoginState, UserTransform, IsVisible, "AvatarId", AvatarState::Idle, AvatarPlayMode::Default))
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

    // During destruction (test cleanup) CSP can access the connection.
    // We can't leave the main Mock dangling because it needs to run RAII test assertion behaviour, so use a throwaway.
    SignalRConnectionMock* ThrowawaySignalRMock = new SignalRConnectionMock();
    SpaceEntitySystem->SetConnection(ThrowawaySignalRMock);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceEntitySystemTests, TestErrorInSendNewAvatarObjectMessage)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* SpaceEntitySystem = SystemsManager.GetSpaceEntitySystem();
    auto* Connection = SystemsManager.GetMultiplayerConnection();

    auto SignalRMock = std::unique_ptr<SignalRConnectionMock>(new SignalRConnectionMock());
    SpaceEntitySystem->SetConnection(SignalRMock.get());

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
            SpaceEntitySystem->SendNewAvatarObjectMessage(
                "Username", LoginState, UserTransform, IsVisible, "AvatarId", AvatarState::Idle, AvatarPlayMode::Default))
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

    // During destruction (test cleanup) CSP can access the connection.
    // We can't leave the main Mock dangling because it needs to run RAII test assertion behaviour, so use a throwaway.
    SignalRConnectionMock* ThrowawaySignalRMock = new SignalRConnectionMock();
    SpaceEntitySystem->SetConnection(ThrowawaySignalRMock);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceEntitySystemTests, TestSuccessInCreateNewLocalAvatar)
{
    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* SpaceEntitySystem = SystemsManager.GetSpaceEntitySystem();

    auto SignalRMock = std::unique_ptr<SignalRConnectionMock>(new SignalRConnectionMock());
    SpaceEntitySystem->SetConnection(SignalRMock.get());

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
            SpaceEntitySystem->CreateNewLocalAvatar(
                Username, LoginState, UserTransform, IsVisible, AvatarId, AvatarState, AvatarPlayMode, MockCallback.AsStdFunction()));

    // During destruction (test cleanup) CSP can access the connection.
    // We can't leave the main Mock dangling because it needs to run RAII test assertion behaviour, so use a throwaway.
    SignalRConnectionMock* ThrowawaySignalRMock = new SignalRConnectionMock();
    SpaceEntitySystem->SetConnection(ThrowawaySignalRMock);
}

CSP_PUBLIC_TEST(CSPEngine, SpaceEntitySystemTests, TestErrorLoggedFromWholeCreateAvatarChain)
{
    RAIIMockLogger MockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(csp::common::LogLevel::Log);

    auto& SystemsManager = csp::systems::SystemsManager::Get();
    auto* SpaceEntitySystem = SystemsManager.GetSpaceEntitySystem();

    auto SignalRMock = std::unique_ptr<SignalRConnectionMock>(new SignalRConnectionMock());
    SpaceEntitySystem->SetConnection(SignalRMock.get());

    // SignalR populates an exception
    EXPECT_CALL(*SignalRMock, Invoke)
        .WillOnce(
            [](const std::string& /**/, const signalr::value& /**/, std::function<void(const signalr::value&, std::exception_ptr)> /**/) {
                return async::make_task(
                    std::make_tuple(signalr::value("Irrelevent Value"), std::make_exception_ptr(std::runtime_error("mock exception"))));
            });

    using MockEntityCreatedCallback = testing::MockFunction<void(SpaceEntity*)>;
    MockEntityCreatedCallback MockCallback;

    // Expect the callback gets nullptr (not the greatest error return...)
    EXPECT_CALL(MockCallback, Call(nullptr));

    // Expect that we log the error message
    const csp::common::String ErrorMsg = "Failed to create Avatar. Exception: mock exception";
    EXPECT_CALL(MockLogger.MockLogCallback, Call(ErrorMsg)).Times(1);

    const SpaceTransform& UserTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool IsVisible = true;

    const auto LoginState = SystemsManager.GetUserSystem()->GetLoginState();

    SpaceEntitySystem->CreateAvatar(
        "Username", LoginState, UserTransform, IsVisible, AvatarState::Idle, "AvatarId", AvatarPlayMode::Default, MockCallback.AsStdFunction());

    // During destruction (test cleanup) CSP can access the connection.
    // We can't leave the main Mock dangling because it needs to run RAII test assertion behaviour, so use a throwaway.
    SignalRConnectionMock* ThrowawaySignalRMock = new SignalRConnectionMock();
    SpaceEntitySystem->SetConnection(ThrowawaySignalRMock);
}
