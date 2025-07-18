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

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/ContinuationUtils.h"
#include "CSP/Systems/ContinuationUtils.h"

#include "CSP/Common/SharedEnums.h"
#include "CSP/Systems/SystemsResult.h"
#include "RAIIMockLogger.h"
#include "Services/ApiBase/ApiBase.h"
#include "Systems/ResultHelpers.h"
#include "TestHelpers.h"

#include <gmock/gmock.h>
#include <thread>

using namespace csp::common;
using namespace csp::systems;

/*
 * These tests should be Internal tests, they don't depend on CSP state really.
 * That is, very frustratingly, except for the logging system.
 * It seems like it would be a good idea to be able to initialize the logging
 * system seperately of all of the other CSP systems. ... (or initialize
 * all the chunky CSP systems seperately of the core systems, which would include
 * the logger).
 */

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestReportSuccess)
{
    RAIIMockLogger MockLogger {};

    /* Specific values are irrelevent */
    csp::common::String SuccessMsg = "Mock Success Msg";
    EResultCode ResultCode = EResultCode::Success;
    csp::web::EResponseCodes HttpResultCode = csp::web::EResponseCodes::ResponseOK;
    ERequestFailureReason FailureReason = ERequestFailureReason::None;

    NullResult ExpectedResult(ResultCode, HttpResultCode, FailureReason);

    ::testing::MockFunction<void(const NullResult& Result)> MockResultCallback;
    // Expect that the callback is called with the result constructed as expected.
    EXPECT_CALL(MockResultCallback, Call(ExpectedResult)).Times(1);
    // Expect that we log the success message
    EXPECT_CALL(MockLogger.MockLogCallback, Call(SuccessMsg)).Times(1);

    csp::systems::continuations::ReportSuccess(MockResultCallback.AsStdFunction(), SuccessMsg.c_str())();
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestLogErrorAndCancel)
{
    RAIIMockLogger MockLogger {};

    /* Specific values are irrelevent */
    csp::common::String ErrorMsg = "Mock Error Msg";
    EResultCode ResultCode = EResultCode::Failed;
    csp::web::EResponseCodes HttpResultCode = csp::web::EResponseCodes::ResponseContinue;
    ERequestFailureReason FailureReason = ERequestFailureReason::SpacePublicNameDuplicate;

    NullResult ExpectedResult(ResultCode, HttpResultCode, FailureReason);

    ::testing::MockFunction<void(const NullResult& Result)> MockResultCallback;
    // Expect that the callback is called with the result constructed as expected.
    EXPECT_CALL(MockResultCallback, Call(ExpectedResult)).Times(1);
    // Expect that we log the error message
    EXPECT_CALL(MockLogger.MockLogCallback, Call(ErrorMsg)).Times(1);

    // This throws a async++::task_cancelled exception, but we don't want to link that lib in the tests, so just expect any exception.
    ASSERT_ANY_THROW(csp::systems::continuations::LogHTTPErrorAndCancelContinuation(
        MockResultCallback.AsStdFunction(), ErrorMsg.c_str(), ResultCode, HttpResultCode, FailureReason));
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestAssertRequestSuccessOrErrorFromResultWhenSuccess)
{
    RAIIMockLogger MockLogger {};

    csp::common::String ErrorMsg = "Mock Error Msg";
    csp::common::String SuccessMsg = "Mock Success Msg";

    ::testing::MockFunction<void(const NullResult& Result)> MockResultCallback;

    // When we succeed, we should just log, and forward the result, (dont call callback) //

    EXPECT_CALL(MockResultCallback, Call(::testing::_)).Times(0);
    EXPECT_CALL(MockLogger.MockLogCallback, Call(SuccessMsg)).Times(1);

    NullResult SuccessResult(EResultCode::Success, 200, ERequestFailureReason::None);
    ASSERT_EQ(csp::systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(
                  MockResultCallback.AsStdFunction(), SuccessMsg.c_str(), ErrorMsg.c_str(), {}, {}, {}, csp::common::LogLevel::Log)(SuccessResult),
        SuccessResult);
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestAssertRequestSuccessOrErrorFromResultWhenError)
{
    RAIIMockLogger MockLogger {};

    /* Specific values are irrelevent */
    csp::common::String ErrorMsg = "Mock Error Msg";
    csp::common::String SuccessMsg = "Mock Success Msg";
    EResultCode ResultCode = EResultCode::Failed;
    csp::web::EResponseCodes HttpResultCode = csp::web::EResponseCodes::ResponseContinue;
    ERequestFailureReason FailureReason = ERequestFailureReason::SpacePublicNameDuplicate;
    NullResult ExpectedFailureResult(ResultCode, HttpResultCode, FailureReason);

    ::testing::MockFunction<void(const NullResult& Result)> MockResultCallback;

    // When the result is a failure, we expect the error callback to be called and an error message logged. //
    {
        // When we pass empty optionals, we expect the values from the Result to be used //
        // Expect that the callback is called with the result constructed as expected.
        EXPECT_CALL(MockResultCallback, Call(ExpectedFailureResult)).Times(1);
        // Expect that we log the error message
        EXPECT_CALL(MockLogger.MockLogCallback, Call(ErrorMsg)).Times(1);

        ASSERT_ANY_THROW(csp::systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(
            MockResultCallback.AsStdFunction(), SuccessMsg.c_str(), ErrorMsg.c_str(), {}, {}, {}, csp::common::LogLevel::Log)(ExpectedFailureResult));

        ::testing::Mock::VerifyAndClearExpectations(&MockResultCallback);
        ::testing::Mock::VerifyAndClearExpectations(&MockLogger.MockLogCallback);
    }
    {
        // When we pass full optionals, we expect the values from the optionals to be used to construct the result passed in the callback //

        EResultCode ResultCodeExplicit = EResultCode::InProgress;
        csp::web::EResponseCodes HttpResultCodeExplicit = csp::web::EResponseCodes::ResponseProcessing;
        ERequestFailureReason FailureReasonExplicit = ERequestFailureReason::AssetAudioVideoLimitReached;
        NullResult ExpectedFailureResultExplicit(ResultCodeExplicit, HttpResultCodeExplicit,
            FailureReasonExplicit); // Note not passed to function invocation, to check that the optionals are used.

        // Expect that the callback is called with the result constructed as expected.
        EXPECT_CALL(MockResultCallback, Call(ExpectedFailureResultExplicit)).Times(1);
        // Expect that we log the error message
        EXPECT_CALL(MockLogger.MockLogCallback, Call(ErrorMsg)).Times(1);

        ASSERT_ANY_THROW(csp::systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(MockResultCallback.AsStdFunction(),
            SuccessMsg.c_str(), ErrorMsg.c_str(), std::make_optional(ResultCodeExplicit), std::make_optional(HttpResultCodeExplicit),
            std::make_optional(FailureReasonExplicit), csp::common::LogLevel::Log)(ExpectedFailureResult));
    }
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestAssertRequestSuccessOrErrorFromErrorCodeWhenSuccess)
{
    RAIIMockLogger MockLogger {};
    csp::common::String SuccessMsg = "Mock Success Msg";

    ::testing::MockFunction<void(const NullResult& Result)> MockResultCallback;

    // When we don't provide an error code, we expect to just log a success message, no return or exception //
    EXPECT_CALL(MockLogger.MockLogCallback, Call(SuccessMsg)).Times(1);

    csp::common::continuations::AssertRequestSuccessOrErrorFromMultiplayerErrorCode(MockResultCallback.AsStdFunction(), SuccessMsg.c_str(),
        MakeInvalid<NullResult>(), *csp::systems::SystemsManager::Get().GetLogSystem(), csp::common::LogLevel::Log)({});
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestAssertRequestSuccessOrErrorFromErrorCodeWhenError)
{
    RAIIMockLogger MockLogger {};

    /* Specific values are irrelevent */
    csp::common::String SuccessMsg = "Mock Success Msg";

    ::testing::MockFunction<void(const NullResult& Result)> MockResultCallback;

    // When we provide an error code, we expect the error callback to be called and an error message logged.
    csp::multiplayer::ErrorCode ErrorCode = csp::multiplayer::ErrorCode::NotConnected;
    std::string ExpectedErrorMsg = std::string("Operation errored with error code: ") + csp::multiplayer::ErrorCodeToString(ErrorCode);

    {
        // Expect that we log the error message
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::String(ExpectedErrorMsg.c_str()))).Times(1);

        ASSERT_ANY_THROW(
            csp::common::continuations::AssertRequestSuccessOrErrorFromMultiplayerErrorCode(MockResultCallback.AsStdFunction(), SuccessMsg.c_str(),
                MakeInvalid<NullResult>(), *csp::systems::SystemsManager::Get().GetLogSystem(), csp::common::LogLevel::Log)(ErrorCode));

        ::testing::Mock::VerifyAndClearExpectations(&MockResultCallback);
        ::testing::Mock::VerifyAndClearExpectations(&MockLogger.MockLogCallback);
    }
    {
        // When we pass empty optionals, we expect default values to be used to construct the result passed in the callback //
        NullResult DefaultFailureResult = MakeInvalid<NullResult>();

        // Expect that the callback is called with the result constructed as expected.
        EXPECT_CALL(MockResultCallback, Call(DefaultFailureResult)).Times(1);
        // Expect that we log the error message
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::String(ExpectedErrorMsg.c_str()))).Times(1);

        ASSERT_ANY_THROW(
            csp::common::continuations::AssertRequestSuccessOrErrorFromMultiplayerErrorCode(MockResultCallback.AsStdFunction(), SuccessMsg.c_str(),
                MakeInvalid<NullResult>(), *csp::systems::SystemsManager::Get().GetLogSystem(), csp::common::LogLevel::Log)(ErrorCode));
    }
}

// See `Continuation.h::detail::testing` for specifics of how these tests run
CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestWhenNoExceptionThrownInContinuationChainThenHandlerNotCalled)
{
    // No exception, expect exception handler callable not called.
    ::testing::MockFunction<void(const std::exception&)> MockExceptionHandlerCallable;
    EXPECT_CALL(MockExceptionHandlerCallable, Call(::testing::_)).Times(0);
    csp::systems::continuations::detail::testing::SpawnChainThatThrowsNoExceptionWithHandlerAtEnd(MockExceptionHandlerCallable.AsStdFunction());
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestWhenExceptionThrownInContinuationChainThenHandlerCalled)
{
    // Exception thrown, expect exception handler callable called
    ::testing::MockFunction<void(const std::exception&)> MockExceptionHandlerCallable;
    EXPECT_CALL(MockExceptionHandlerCallable, Call(::testing::_)).Times(1);
    csp::systems::continuations::detail::testing::SpawnChainThatThrowsGeneralExceptionWithHandlerAtEnd(MockExceptionHandlerCallable.AsStdFunction());
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestWhenContinuationChainCancelledThenHandlerAndResultCallbackCalled)
{
    // Exception thrown, expect exception handler callable called, as well as result callback called. (Just testing our specific way of throwing here)
    ::testing::MockFunction<void(const std::exception&)> MockExceptionHandlerCallable;
    ::testing::MockFunction<void(const NullResult& Result)> MockResultCallback;
    EXPECT_CALL(MockExceptionHandlerCallable, Call(::testing::_)).Times(1);
    EXPECT_CALL(MockResultCallback, Call(::testing::_)).Times(1);
    csp::systems::continuations::detail::testing::SpawnChainThatCallsLogHTTPErrorAndCancelContinuationWithHandlerAtEnd(
        MockExceptionHandlerCallable.AsStdFunction(), MockResultCallback.AsStdFunction());
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestCallableCalledAndIntermediateNotWhenExceptionThrownHigherInChain)
{
    // Exception thrown higher in chain, expect exception handler callable intermediate method not called, but exception handler callable called.
    ::testing::MockFunction<void()> MockIntermediateStepCallable;
    ::testing::MockFunction<void(const std::exception&)> MockExceptionHandlerCallable;
    ::testing::MockFunction<void(const NullResult& Result)> MockResultCallback;
    EXPECT_CALL(MockIntermediateStepCallable, Call()).Times(0);
    EXPECT_CALL(MockExceptionHandlerCallable, Call(::testing::_)).Times(1);
    EXPECT_CALL(MockResultCallback, Call(::testing::_)).Times(1);
    csp::systems::continuations::detail::testing::SpawnChainThatCallsLogHTTPErrorAndCancelContinuationWithIntermediateStepAndHandlerAtEnd(
        MockIntermediateStepCallable.AsStdFunction(), MockExceptionHandlerCallable.AsStdFunction(), MockResultCallback.AsStdFunction());
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestCallableCalledOnSameThreadInContinuationChain)
{
    // Since we're exposing callbacks to chains anyway, might as well verify the WASM requirement that
    // callbacks occur on the same thread as invocation, (ie, inline_scheduler() works ... this is _definately_
    // testing the library now...).

    const auto ThisThreadId = std::this_thread::get_id();
    auto VerifyThread = [ThisThreadId](const std::exception& /*Except*/) { ASSERT_EQ(ThisThreadId, std::this_thread::get_id()); };
    // Just use the exception handler to serve as a general purpose way to call a callable in tests.
    // Could simply have made another detail::testing function ... but why bother when this already exists.
    csp::systems::continuations::detail::testing::SpawnChainThatThrowsGeneralExceptionWithHandlerAtEnd(VerifyThread);
}