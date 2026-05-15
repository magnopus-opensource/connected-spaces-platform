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
#include "CSP/Multiplayer/ComponentSchema.h"
#include "CSP/Multiplayer/ContinuationUtils.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/SpaceEntity.h"
#include "CSP/Systems/Script/ScriptSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Debug/Logging.h"
#include "Mocks/SignalRConnectionMock.h"
#include "Multiplayer/MCS/MCSTypes.h"
#include "Multiplayer/SpaceEntityStatePatcher.h"
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

class MockScriptRunner : public csp::common::IJSScriptRunner
{
    bool RunScript(int64_t, const csp::common::String&) override { return false; }
    void RegisterScriptBinding(csp::common::IScriptBinding*) override { }
    void UnregisterScriptBinding(csp::common::IScriptBinding*) override { }
    bool BindContext(int64_t) override { return false; }
    bool ResetContext(int64_t) override { return false; }
    void* GetContext(int64_t) override { return nullptr; }
    void* GetModule(int64_t, const csp::common::String&) override { return nullptr; }
    bool CreateContext(int64_t) override { return false; }
    bool DestroyContext(int64_t) override { return false; }
    void SetModuleSource(csp::common::String, csp::common::String) override { }
    void ClearModuleSource(csp::common::String) override { }
};

}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, TestSuccessInRemoteGenerateNewAvatarId)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* connection = systemsManager.GetMultiplayerConnection();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    EXPECT_CALL(*m_webClientMock, SendRequest).Times(0);

    // SignalR populates a result and not an exception
    EXPECT_CALL(
        *m_signalRMock, Invoke(connection->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::GENERATE_OBJECT_IDS), ::testing::_, ::testing::_))
        .WillOnce(
            [](const std::string& /**/, const signalr::value& /**/,
                std::function<void(const signalr::value&, std::exception_ptr)> /**/) -> async::task<std::tuple<signalr::value, std::exception_ptr>>
            {
                // For some reason the ID value has to be an array :/
                std::vector<signalr::value> ids;
                ids.emplace_back(signalr::value(uint64_t(55)));
                // Construct a signalr::value that holds an array of those IDs
                signalr::value value(ids);

                return async::make_task(std::make_tuple(value, std::exception_ptr(nullptr)));
            });

    realtimeEngine->RemoteGenerateNewAvatarId()
        .then(async::inline_scheduler(),
            [](async::task<uint64_t> result)
            {
                EXPECT_FALSE(result.get_exception());
                EXPECT_EQ(result.get(), uint64_t(55));
            })
        .then(async::inline_scheduler(),
            [](async::task<void> checkForErrorsTask)
            { EXPECT_FALSE(checkForErrorsTask.get_exception()); }); // This is to be paranoid and guard against errors in writing the test, as async++
                                                                    // will catch exceptions and convert to a friendly cancel if they occur.
}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, TestErrorInRemoteGenerateNewAvatarId)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* connection = systemsManager.GetMultiplayerConnection();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    EXPECT_CALL(*m_webClientMock, SendRequest).Times(0);

    // SignalR populates an exception
    EXPECT_CALL(
        *m_signalRMock, Invoke(connection->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::GENERATE_OBJECT_IDS), ::testing::_, ::testing::_))
        .WillOnce(
            [](const std::string& /**/, const signalr::value& /**/, std::function<void(const signalr::value&, std::exception_ptr)> /**/) {
                return async::make_task(
                    std::make_tuple(signalr::value("Irrelevant value"), std::make_exception_ptr(std::runtime_error("mock exception"))));
            });

    realtimeEngine->RemoteGenerateNewAvatarId()
        .then(
            [](async::task<uint64_t> result)
            {
                EXPECT_TRUE(result.get_exception());
                try
                {
                    std::rethrow_exception(result.get_exception());
                }
                catch (std::runtime_error error)
                {
                    EXPECT_EQ(std::string(error.what()), std::string("Multiplayer Error. mock exception"));
                }
            })
        .then(async::inline_scheduler(),
            [](async::task<void> checkForErrorsTask)
            { EXPECT_FALSE(checkForErrorsTask.get_exception()); }); // This is to be paranoid and guard against errors in writing the test, as async++
                                                                    // will catch exceptions and convert to a friendly cancel if they occur.
}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, TestSuccessInSendNewAvatarObjectMessage)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* connection = systemsManager.GetMultiplayerConnection();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    EXPECT_CALL(*m_webClientMock, SendRequest).Times(0);

    // SignalR populates a result and not an exception
    EXPECT_CALL(
        *m_signalRMock, Invoke(connection->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_MESSAGE), ::testing::_, ::testing::_))
        .WillOnce(
            [](const std::string& /**/, const signalr::value& /**/,
                std::function<void(const signalr::value&, std::exception_ptr)> /**/) -> async::task<std::tuple<signalr::value, std::exception_ptr>>
            { return async::make_task(std::make_tuple(signalr::value(true), std::exception_ptr(nullptr))); });

    const SpaceTransform& userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;

    const auto loginState = systemsManager.GetUserSystem()->GetLoginState();

    async::spawn(async::inline_scheduler(), []() { return uint64_t(55); }) // This continuation takes the ID as its input
        .then(async::inline_scheduler(),
            realtimeEngine->SendNewAvatarObjectMessage(
                "Username", loginState.UserId, userTransform, isVisible, "AvatarId", AvatarState::Idle, AvatarPlayMode::Default, LocomotionModel::Grounded))
        .then(async::inline_scheduler(),
            [](async::task<uint64_t> id)
            {
                EXPECT_FALSE(id.get_exception());
                EXPECT_EQ(id.get(), 55);
            })
        .then(async::inline_scheduler(),
            [](async::task<void> checkForErrorsTask)
            { EXPECT_FALSE(checkForErrorsTask.get_exception()); }); // This is to be paranoid and guard against errors in writing the test, as async++
                                                                    // will catch exceptions and convert to a friendly cancel if they occur.
}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, TestErrorInSendNewAvatarObjectMessage)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();
    auto* connection = systemsManager.GetMultiplayerConnection();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    EXPECT_CALL(*m_webClientMock, SendRequest).Times(0);

    // SignalR populates an exception
    EXPECT_CALL(
        *m_signalRMock, Invoke(connection->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_OBJECT_MESSAGE), ::testing::_, ::testing::_))
        .WillOnce(
            [](const std::string& /**/, const signalr::value& /**/,
                std::function<void(const signalr::value&, std::exception_ptr)> /**/) -> async::task<std::tuple<signalr::value, std::exception_ptr>> {
                return async::make_task(
                    std::make_tuple(signalr::value("Irrelevent Value"), std::make_exception_ptr(std::runtime_error("mock exception"))));
            });

    const SpaceTransform& userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;

    const auto loginState = systemsManager.GetUserSystem()->GetLoginState();

    async::spawn(async::inline_scheduler(), []() { return uint64_t(55); }) // This continuation takes the ID as its input
        .then(async::inline_scheduler(),
            realtimeEngine->SendNewAvatarObjectMessage(
                "Username", loginState.UserId, userTransform, isVisible, "AvatarId", AvatarState::Idle, AvatarPlayMode::Default, LocomotionModel::Grounded))
        .then(async::inline_scheduler(),
            [](async::task<uint64_t> id)
            {
                EXPECT_TRUE(id.get_exception());
                try
                {
                    std::rethrow_exception(id.get_exception());
                }
                catch (std::runtime_error error)
                {
                    EXPECT_EQ(std::string(error.what()), std::string("Multiplayer Error. mock exception"));
                }
            })
        .then(async::inline_scheduler(),
            [](async::task<void> checkForErrorsTask)
            { EXPECT_FALSE(checkForErrorsTask.get_exception()); }); // This is to be paranoid and guard against errors in writing the test, as async++
                                                                    // will catch exceptions and convert to a friendly cancel if they occur.
}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, TestSuccessInCreateNewLocalAvatar)
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    using MockEntityCreatedCallback = testing::MockFunction<void(SpaceEntity*)>;
    MockEntityCreatedCallback mockCallback;

    const csp::common::String username = "Username";
    const csp::common::String avatarId = "AvatarId";
    AvatarState avatarState = AvatarState::Flying;
    AvatarPlayMode avatarPlayMode = AvatarPlayMode::Creator;
    const auto locomotionModel = LocomotionModel::FreeCamera;
    const uint64_t id = 55;
    const SpaceTransform userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;

    EXPECT_CALL(*m_webClientMock, SendRequest).Times(0);

    EXPECT_CALL(mockCallback, Call(::testing::_))
        .WillOnce(::testing::Invoke(
            [id, &username, &avatarId, avatarState, avatarPlayMode, locomotionModel, isVisible](SpaceEntity* createdSpaceEntity)
            {
                ASSERT_NE(createdSpaceEntity, nullptr);
                ASSERT_EQ(createdSpaceEntity->GetId(), id);
                ASSERT_EQ(createdSpaceEntity->GetName(), username);

                ASSERT_EQ(createdSpaceEntity->GetComponents()->Size(), 1);

                ComponentBase* avatarComponentBase = createdSpaceEntity->GetComponent(0);
                ASSERT_EQ(avatarComponentBase->GetComponentType(), ComponentType::AvatarData);

                AvatarSpaceComponent* avatarComponent = dynamic_cast<AvatarSpaceComponent*>(avatarComponentBase);
                ASSERT_NE(avatarComponent, nullptr);
                ASSERT_EQ(avatarComponent->GetAvatarId(), avatarId);
                ASSERT_EQ(avatarComponent->GetAvatarPlayMode(), avatarPlayMode);
                ASSERT_EQ(avatarComponent->GetState(), avatarState);
                ASSERT_EQ(avatarComponent->GetLocomotionModel(), locomotionModel);
                ASSERT_EQ(avatarComponent->GetIsVisible(), isVisible);
            }));

    const auto loginState = systemsManager.GetUserSystem()->GetLoginState();

    async::spawn(async::inline_scheduler(),
        []()
        { return async::make_task(uint64_t { 55 }); }) // This continuation takes the ID (and another void return from a when_all branch) as its input
        .then(async::inline_scheduler(),
            realtimeEngine->CreateNewLocalAvatar(
                username, loginState.UserId, userTransform, isVisible, avatarId, avatarState, avatarPlayMode, locomotionModel, mockCallback.AsStdFunction()));
}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, TestErrorLoggedFromWholeCreateAvatarChain)
{
    RAIIMockLogger mockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(csp::common::LogLevel::Log);

    auto& systemsManager = csp::systems::SystemsManager::Get();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    
    EXPECT_CALL(*m_webClientMock, SendRequest).Times(0);

    // SignalR populates an exception
    EXPECT_CALL(*m_signalRMock, Invoke)
        .WillOnce(
            [](const std::string& /**/, const signalr::value& /**/, std::function<void(const signalr::value&, std::exception_ptr)> /**/)
            {
                return async::make_task(std::make_tuple(signalr::value("Irrelevent Value"),
                    std::make_exception_ptr(csp::common::continuations::ErrorCodeException(ErrorCode::None, "mock exception"))));
            });

    using MockEntityCreatedCallback = testing::MockFunction<void(SpaceEntity*)>;
    MockEntityCreatedCallback mockCallback;

    // Expect the callback gets nullptr (not the greatest error return...)
    EXPECT_CALL(mockCallback, Call(nullptr));

    // Expect that we log the error message
    const csp::common::String errorMsg = "Failed to create Avatar. Exception: Multiplayer Error. mock exception";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, errorMsg)).Times(1);

    const SpaceTransform userTransform
        = { csp::common::Vector3 { 1.452322f, 2.34f, 3.45f }, csp::common::Vector4 { 4.1f, 5.1f, 6.1f, 7.1f }, csp::common::Vector3 { 1, 1, 1 } };
    bool isVisible = true;

    const auto loginState = systemsManager.GetUserSystem()->GetLoginState();

    realtimeEngine->CreateAvatar("Username", loginState.UserId, userTransform, isVisible, AvatarState::Idle, "AvatarId", AvatarPlayMode::Default,
        LocomotionModel::Grounded, mockCallback.AsStdFunction());
}

// This ensures the callback fires only once, with nullptr if the internal GenerateObjectIds fails.
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, CreateEntityGenerateObjectIdsFailureTest)
{
    RAIIMockLogger mockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(csp::common::LogLevel::Log);

    auto& systemsManager = csp::systems::SystemsManager::Get();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    EXPECT_CALL(*m_webClientMock, SendRequest).Times(0);

    // SignalR populates an exception
    EXPECT_CALL(*m_signalRMock, Invoke)
        .WillOnce(
            [](const std::string& method, const signalr::value& /**/, std::function<void(const signalr::value&, std::exception_ptr)> callback)
            {
                // Create a method map to get the correct hub method string value
                csp::multiplayer::MultiplayerHubMethodMap hubMethods;

                if (method == hubMethods.Get(csp::multiplayer::MultiplayerHubMethod::GENERATE_OBJECT_IDS))
                {
                    // Fail this method by returning an exception
                    signalr::value value {};
                    const auto exceptionPtr = std::make_exception_ptr(std::runtime_error{ "fail" });

                    callback(signalr::value {}, exceptionPtr);

                    return async::make_task(std::make_tuple(value, exceptionPtr));
                }
                else
                {
                    signalr::value value {};
                    return async::make_task(std::make_tuple(value, std::exception_ptr { nullptr }));
                }
            });

    using MockEntityCreatedCallback = testing::MockFunction<void(SpaceEntity*)>;
    MockEntityCreatedCallback mockCallback;

    // Expect the callback is called only once, returning nullptr (not the greatest error return...)
    EXPECT_CALL(mockCallback, Call(nullptr)).Times(1);

    // Expect that we log the error message once and only once
    const csp::common::String errorMsg = "Failed to generate object ID.";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, testing::HasSubstr(errorMsg))).Times(1);

    const SpaceTransform transform = { csp::common::Vector3::Zero(), csp::common::Vector4::Identity(), csp::common::Vector3::One() };

    realtimeEngine->CreateEntity("Mock Entity", transform, nullptr, mockCallback.AsStdFunction());
}

// This ensures the callback fires only once, with nullptr if the internal SendObjectMessage fails.
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, CreateEntitySendObjectMessageFailureTest)
{
    RAIIMockLogger mockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(csp::common::LogLevel::Log);

    auto& systemsManager = csp::systems::SystemsManager::Get();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    bool sendObjectMessageCalled = false;

    EXPECT_CALL(*m_webClientMock, SendRequest).Times(0);

    // SignalR populates an exception
    EXPECT_CALL(*m_signalRMock, Invoke)
        .WillRepeatedly(
            [&sendObjectMessageCalled](
                const std::string& method, const signalr::value& /**/, std::function<void(const signalr::value&, std::exception_ptr)> callback)
            {
                // Create a method map to get the correct hub method string value
                csp::multiplayer::MultiplayerHubMethodMap hubMethods;

                if (method == hubMethods.Get(csp::multiplayer::MultiplayerHubMethod::SEND_OBJECT_MESSAGE))
                {
                    sendObjectMessageCalled = true;

                    // Fail this method by returning an exception
                    signalr::value value {};
                    const auto exceptionPtr = std::make_exception_ptr(std::runtime_error { "fail" });

                    callback(signalr::value {}, exceptionPtr);

                    return async::make_task(std::make_tuple(value, exceptionPtr));
                }
                else
                {
                    // Don't return an exception, as we want this call to succeed
                    const auto exceptionPtr = std::exception_ptr { nullptr };

                    // Return a valid object id
                    std::vector<signalr::value> paramsV { signalr::value { static_cast<uint64_t>(1ull) } };
                    signalr::value params { paramsV };

                    callback(params, exceptionPtr);

                    return async::make_task(std::make_tuple(params, exceptionPtr));
                }
            });

    using MockEntityCreatedCallback = testing::MockFunction<void(SpaceEntity*)>;
    MockEntityCreatedCallback mockCallback;

    // Expect the callback is called only once, returning nullptr (not the greatest error return...)
    EXPECT_CALL(mockCallback, Call(nullptr)).Times(1);

    // Expect that we log the error message once and only once
    const csp::common::String errorMsg = "Failed to create object.";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, testing::HasSubstr(errorMsg))).Times(1);

    const SpaceTransform transform = { csp::common::Vector3::Zero(), csp::common::Vector4::Identity(), csp::common::Vector3::One() };
    realtimeEngine->CreateEntity("Mock Entity", transform, nullptr, mockCallback.AsStdFunction());

    EXPECT_TRUE(sendObjectMessageCalled);
}

// This ensures the callback fires only once, with false if the internal SendObjectPatches fails.
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, DestroyEntitySendObjectPatchesFailureTest)
{
    RAIIMockLogger mockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(csp::common::LogLevel::Log);

    auto& systemsManager = csp::systems::SystemsManager::Get();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    bool sendObjectPatchesCalled = false;

    using MockEntityDestroyedCallback = testing::MockFunction<void(bool)>;
    MockEntityDestroyedCallback mockCallback;

    // Expect the callback is called only once, returning false
    EXPECT_CALL(mockCallback, Call(false)).Times(1);

    // Expect that we log the error message once and only once
    const csp::common::String errorMsg = "Failed to destroy entity.";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, testing::HasSubstr(errorMsg))).Times(1);

    EXPECT_CALL(*m_webClientMock, SendRequest).Times(0);

    // SignalR populates an exception
    EXPECT_CALL(*m_signalRMock, Invoke)
        .WillRepeatedly(
            [&sendObjectPatchesCalled, &realtimeEngine, &mockLogger](
                const std::string& method, const signalr::value&, std::function<void(const signalr::value&, std::exception_ptr)> callback)
            {
                // Create a method map to get the correct hub method string value
                csp::multiplayer::MultiplayerHubMethodMap hubMethods;

                if (method == hubMethods.Get(csp::multiplayer::MultiplayerHubMethod::GENERATE_OBJECT_IDS))
                {
                    // Don't return an exception, as we want this call to succeed
                    const auto exceptionPtr = std::exception_ptr { nullptr };

                    // Return a valid object id
                    std::vector<signalr::value> paramsV { signalr::value { static_cast<uint64_t>(1ull) } };
                    signalr::value params { paramsV };

                    callback(params, exceptionPtr);

                    return async::make_task(std::make_tuple(params, exceptionPtr));
                }
                else if (method == hubMethods.Get(csp::multiplayer::MultiplayerHubMethod::SEND_OBJECT_MESSAGE))
                {
                    // Don't return an exception, as we want this call to succeed
                    const auto exceptionPtr = std::exception_ptr { nullptr };

                    // Create a space entity patch
                    MockScriptRunner runner;
                    SpaceEntity entity { realtimeEngine.get(), runner, csp::systems::SystemsManager::Get().GetLogSystem() };
                    csp::multiplayer::mcs::ObjectPatch patch = entity.GetStatePatcher()->CreateObjectPatch();
                    csp::multiplayer::SignalRSerializer serializer;
                    serializer.WriteValue(patch);

                    std::vector<signalr::value> paramsV { serializer.Get() };
                    signalr::value params { paramsV };

                    callback(params, exceptionPtr);

                    return async::make_task(std::make_tuple(params, exceptionPtr));
                }
                else
                {
                    sendObjectPatchesCalled = true;

                    // Fail this method by returning an exception
                    signalr::value value {};
                    const auto exceptionPtr = std::make_exception_ptr(std::runtime_error { "fail" });

                    callback(signalr::value {}, exceptionPtr);

                    return async::make_task(std::make_tuple(value, exceptionPtr));
                }
            });

    const SpaceTransform transform = { csp::common::Vector3::Zero(), csp::common::Vector4::Identity(), csp::common::Vector3::One() };
    SpaceEntity* entity = nullptr;
    realtimeEngine->CreateEntity("Mock Entity", transform, nullptr, [&entity](SpaceEntity* createdEntity) { entity = createdEntity; });

    entity->Destroy(mockCallback.AsStdFunction());

    EXPECT_TRUE(sendObjectPatchesCalled);
}

// This ensures the callback fires only once, with nullptr if the internal GenerateObjectIds fails.
CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, CreateAvatarGenerateObjectIdsFailureTest)
{
    RAIIMockLogger mockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(csp::common::LogLevel::Log);

    auto& systemsManager = csp::systems::SystemsManager::Get();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    EXPECT_CALL(*m_webClientMock, SendRequest).Times(0);

    // SignalR populates an exception
    EXPECT_CALL(*m_signalRMock, Invoke)
        .WillOnce(
            [](const std::string& method, const signalr::value& /**/, std::function<void(const signalr::value&, std::exception_ptr)> callback)
            {
                // Create a method map to get the correct hub method string value
                csp::multiplayer::MultiplayerHubMethodMap hubMethods;

                if (method == hubMethods.Get(csp::multiplayer::MultiplayerHubMethod::GENERATE_OBJECT_IDS))
                {
                    // Fail this method by returning an exception
                    signalr::value value {};
                    const auto exceptionPtr
                        = std::make_exception_ptr(csp::common::continuations::ErrorCodeException { csp::multiplayer::ErrorCode::Unknown, "fail" });

                    callback(value, exceptionPtr);
                    return async::make_task(std::make_tuple(value, exceptionPtr));
                }
                else
                {
                    signalr::value value {};
                    return async::make_task(std::make_tuple(value, std::exception_ptr { nullptr }));
                }
            });

    using MockEntityCreatedCallback = testing::MockFunction<void(SpaceEntity*)>;
    MockEntityCreatedCallback mockCallback;

    // Expect the callback is called only once, returning nullptr (not the greatest error return...)
    EXPECT_CALL(mockCallback, Call(nullptr)).Times(1);

    // Expect that we log the error message once and only once
    const csp::common::String errorMsg = "fail";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, testing::HasSubstr(errorMsg))).Times(1);

    const SpaceTransform transform = { csp::common::Vector3::Zero(), csp::common::Vector4::Identity(), csp::common::Vector3::One() };
    const auto loginState = systemsManager.GetUserSystem()->GetLoginState();

    realtimeEngine->CreateAvatar("Username", loginState.UserId, transform, true, AvatarState::Idle, "AvatarId", AvatarPlayMode::Default,
        LocomotionModel::Grounded, mockCallback.AsStdFunction());
}

CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, CreateAvatarSendObjectMessageFailureTest)
{
    RAIIMockLogger mockLogger {};
    csp::systems::SystemsManager::Get().GetLogSystem()->SetSystemLevel(csp::common::LogLevel::Log);

    auto& systemsManager = csp::systems::SystemsManager::Get();

    std::unique_ptr<csp::multiplayer::OnlineRealtimeEngine> realtimeEngine { systemsManager.MakeOnlineRealtimeEngine() };

    bool sendObjectMessageCalled = false;

    EXPECT_CALL(*m_webClientMock, SendRequest).Times(0);

    // SignalR populates an exception
    EXPECT_CALL(*m_signalRMock, Invoke)
        .WillRepeatedly(
            [&sendObjectMessageCalled](
                const std::string& method, const signalr::value& /**/, std::function<void(const signalr::value&, std::exception_ptr)> callback)
            {
                // Create a method map to get the correct hub method string value
                csp::multiplayer::MultiplayerHubMethodMap hubMethods;

                if (method == hubMethods.Get(csp::multiplayer::MultiplayerHubMethod::SEND_OBJECT_MESSAGE))
                {
                    sendObjectMessageCalled = true;

                    // Fail this method by returning an exception
                    signalr::value value {};
                    const auto exceptionPtr
                        = std::make_exception_ptr(csp::common::continuations::ErrorCodeException { csp::multiplayer::ErrorCode::Unknown, "fail" });

                    callback(value, exceptionPtr);
                    return async::make_task(std::make_tuple(value, exceptionPtr));
                }
                else
                {
                    // Don't return an exception, as we want this call to succeed
                    const auto exceptionPtr = std::exception_ptr { nullptr };

                    // Return a valid object id
                    std::vector<signalr::value> paramsV { signalr::value { static_cast<uint64_t>(1ull) } };
                    signalr::value params { paramsV };

                    callback(params, exceptionPtr);

                    return async::make_task(std::make_tuple(params, exceptionPtr));
                }
            });

    using MockEntityCreatedCallback = testing::MockFunction<void(SpaceEntity*)>;
    MockEntityCreatedCallback mockCallback;

    // Expect the callback is called only once, returning nullptr (not the greatest error return...)
    EXPECT_CALL(mockCallback, Call(nullptr)).Times(1);

    // Expect that we log the error message once and only once
    const csp::common::String errorMsg = "fail";
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, testing::HasSubstr(errorMsg))).Times(1);

    const SpaceTransform transform = { csp::common::Vector3::Zero(), csp::common::Vector4::Identity(), csp::common::Vector3::One() };
    const auto loginState = systemsManager.GetUserSystem()->GetLoginState();

    realtimeEngine->CreateAvatar("Username", loginState.UserId, transform, true, AvatarState::Idle, "AvatarId", AvatarPlayMode::Default,
        LocomotionModel::Grounded, mockCallback.AsStdFunction());
}


CSP_PUBLIC_TEST_WITH_MOCKS(CSPEngine, OnlineRealtimeEngineTests, ConstructWithComponentSchema) 
{
    auto& systemsManager = csp::systems::SystemsManager::Get();

    const auto exampleSchemaId = ComponentSchema::TypeIdType{666};

    const auto components = csp::common::Array<csp::multiplayer::ComponentSchema> {
        {
            exampleSchemaId,
            "Example",
            csp::common::Array<ComponentProperty> {
                {
                    ComponentProperty::KeyType { 42 },
                    "value",
                    "DefaultValue",
                },
            },
        },
    };

    const auto engine = OnlineRealtimeEngine {
        *systemsManager.GetMultiplayerConnection(),
        *systemsManager.GetLogSystem(),
        *systemsManager.GetEventBus(),
        *systemsManager.GetScriptSystem(),
        components,
    };

    EXPECT_TRUE(engine.GetComponentSchemaRegistry()->HasKey(exampleSchemaId));
}
