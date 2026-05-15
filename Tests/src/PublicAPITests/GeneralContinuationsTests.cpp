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
    RAIIMockLogger mockLogger {};

    /* Specific values are irrelevent */
    const csp::common::String successMsg = "Mock Success Msg";
    const csp::common::LogLevel logLevel = csp::common::LogLevel::Log;
    const EResultCode resultCode = EResultCode::Success;
    const csp::web::EResponseCodes httpResultCode = csp::web::EResponseCodes::ResponseOK;
    const ERequestFailureReason failureReason = ERequestFailureReason::None;

    NullResult expectedResult(resultCode, httpResultCode, failureReason);

    ::testing::MockFunction<void(const NullResult& result)> mockResultCallback;
    // Expect that the callback is called with the result constructed as expected.
    EXPECT_CALL(mockResultCallback, Call(expectedResult)).Times(1);
    // Expect that we log the success message
    EXPECT_CALL(mockLogger.MockLogCallback, Call(logLevel, successMsg)).Times(1);

    csp::systems::continuations::ReportSuccess(mockResultCallback.AsStdFunction(), successMsg.c_str())();
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestLogErrorAndCancel)
{
    RAIIMockLogger mockLogger {};

    /* Specific values are irrelevent */
    const csp::common::String errorMsg = "Mock Error Msg";
    const EResultCode resultCode = EResultCode::Failed;
    const csp::web::EResponseCodes httpResultCode = csp::web::EResponseCodes::ResponseContinue;
    const ERequestFailureReason failureReason = ERequestFailureReason::SpacePublicNameDuplicate;

    NullResult expectedResult(resultCode, httpResultCode, failureReason);

    // Expect that we log the error message
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, errorMsg)).Times(1);

    // This throws a async++::task_cancelled exception, but we don't want to link that lib in the tests, so just expect any exception.
    ASSERT_ANY_THROW(csp::systems::continuations::LogHTTPErrorAndCancelContinuation<NullResult>(errorMsg.c_str(), expectedResult));
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestAssertRequestSuccessOrErrorFromResultWhenSuccess)
{
    RAIIMockLogger mockLogger {};

    const csp::common::String errorMsg = "Mock Error Msg";
    const csp::common::String successMsg = "Mock Success Msg";

    // When we succeed, we should just log, and forward the result
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, successMsg)).Times(1);

    NullResult successResult(EResultCode::Success, 200, ERequestFailureReason::None);
    ASSERT_EQ(csp::systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(
                  successMsg.c_str(), errorMsg.c_str(), {}, {}, {}, csp::common::LogLevel::Log)(successResult),
        successResult);
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestAssertRequestSuccessOrErrorFromResultWhenError)
{
    RAIIMockLogger mockLogger {};

    /* Specific values are irrelevent */
    const csp::common::String errorMsg = "Mock Error Msg";
    const csp::common::String successMsg = "Mock Success Msg";

    const EResultCode resultCode = EResultCode::Failed;
    const csp::web::EResponseCodes httpResultCode = csp::web::EResponseCodes::ResponseContinue;
    const ERequestFailureReason failureReason = ERequestFailureReason::SpacePublicNameDuplicate;

    NullResult expectedFailureResult(resultCode, httpResultCode, failureReason);

    // When the result is a failure, we expect an error message logged and the callback not to be triggered from the intermediate function.
    {
        // Expect that we log the error message
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, errorMsg)).Times(1);

        ASSERT_ANY_THROW(csp::systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(
            successMsg.c_str(), errorMsg.c_str(), {}, {}, {}, csp::common::LogLevel::Log)(expectedFailureResult));

        ::testing::Mock::VerifyAndClearExpectations(&mockLogger.MockLogCallback);
    }
    {
        // When we pass full optionals, we expect the values from the optionals to be used to construct the result passed in the callback
        EResultCode resultCodeExplicit = EResultCode::InProgress;
        csp::web::EResponseCodes httpResultCodeExplicit = csp::web::EResponseCodes::ResponseProcessing;
        ERequestFailureReason failureReasonExplicit = ERequestFailureReason::AssetAudioVideoLimitReached;
        NullResult expectedFailureResultExplicit(resultCodeExplicit, httpResultCodeExplicit,
            failureReasonExplicit); // Note not passed to function invocation, to check that the optionals are used.
        // Expect that we log the error message
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, errorMsg)).Times(1);

        ASSERT_ANY_THROW(csp::systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(successMsg.c_str(), errorMsg.c_str(),
            std::make_optional(resultCodeExplicit), std::make_optional(httpResultCodeExplicit), std::make_optional(failureReasonExplicit),
            csp::common::LogLevel::Log)(expectedFailureResult));
    }
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestAssertRequestSuccessOrErrorFromErrorCodeWhenSuccess)
{
    RAIIMockLogger mockLogger {};
    const csp::common::String successMsg = "Mock Success Msg";

    // When we don't provide an error code, we expect to just log a success message, no return or exception
    EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Log, successMsg)).Times(1);

    csp::common::continuations::AssertRequestSuccessOrErrorFromMultiplayerErrorCode(
        successMsg.c_str(), MakeInvalid<NullResult>(), *csp::systems::SystemsManager::Get().GetLogSystem())({});
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestAssertRequestSuccessOrErrorFromErrorCodeWhenError)
{
    RAIIMockLogger mockLogger {};

    /* Specific values are irrelevent */
    const csp::common::String successMsg = "Mock Success Msg";

    // When we provide an error code, we expect an error message logged and the callback not to be triggered from the intermediate function.
    const csp::multiplayer::ErrorCode errorCode = csp::multiplayer::ErrorCode::NotConnected;
    const std::string expectedErrorMsg = std::string("Operation errored with error code: ") + csp::multiplayer::ErrorCodeToString(errorCode);

    {
        // Expect that we log the error message
        EXPECT_CALL(mockLogger.MockLogCallback, Call(csp::common::LogLevel::Error, csp::common::String(expectedErrorMsg.c_str()))).Times(1);

        ASSERT_ANY_THROW(csp::common::continuations::AssertRequestSuccessOrErrorFromMultiplayerErrorCode(
            successMsg.c_str(), MakeInvalid<NullResult>(), *csp::systems::SystemsManager::Get().GetLogSystem())(errorCode));
    }
}

// See `Continuation.h::detail::testing` for specifics of how these tests run
CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestWhenNoExceptionThrownInContinuationChainThenHandlerNotCalled)
{
    // No exception, expect exception handler callable not called.
    ::testing::MockFunction<void(const std::exception&)> mockExceptionHandlerCallable;
    EXPECT_CALL(mockExceptionHandlerCallable, Call(::testing::_)).Times(0);
    csp::systems::continuations::detail::testing::SpawnChainThatThrowsNoExceptionWithHandlerAtEnd(mockExceptionHandlerCallable.AsStdFunction());
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestWhenExpectedExceptionThrownInContinuationChainThenHandlerCalled)
{
    // Exception thrown, expect exception handler callable called for expected exception
    ::testing::MockFunction<void(const csp::common::continuations::ExpectedExceptionBase&)> mockExpectedHandlerCallable;
    EXPECT_CALL(mockExpectedHandlerCallable, Call(::testing::_)).Times(1);

    ::testing::MockFunction<void(const std::exception&)> mockUnexpectedHandlerCallable;
    EXPECT_CALL(mockUnexpectedHandlerCallable, Call(::testing::_)).Times(0);

    csp::systems::continuations::detail::testing::SpawnChainThatThrowsGeneralExceptionWithHandlerAtEnd(mockExpectedHandlerCallable.AsStdFunction(),
        mockUnexpectedHandlerCallable.AsStdFunction(), csp::common::continuations::ResultException("", MakeInvalid<NullResult>()));
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestWhenUnexpectedExceptionThrownInContinuationChainThenHandlerCalled)
{
    // Exception thrown, expect exception handler callable called for unexpected exception
    ::testing::MockFunction<void(const csp::common::continuations::ExpectedExceptionBase&)> mockExpectedHandlerCallable;
    EXPECT_CALL(mockExpectedHandlerCallable, Call(::testing::_)).Times(0);

    ::testing::MockFunction<void(const std::exception&)> mockUnexpectedHandlerCallable;
    EXPECT_CALL(mockUnexpectedHandlerCallable, Call(::testing::_)).Times(1);

    csp::systems::continuations::detail::testing::SpawnChainThatThrowsGeneralExceptionWithHandlerAtEnd(
        mockExpectedHandlerCallable.AsStdFunction(), mockUnexpectedHandlerCallable.AsStdFunction(), std::runtime_error(""));
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestWhenContinuationChainCancelledThenHandlerAndResultCallbackCalled)
{
    // Exception thrown, expect exception handler callable called, and the callback not to be triggered from the intermediate function. (Just testing
    // our specific way of throwing here)
    ::testing::MockFunction<void(const std::exception&)> mockExceptionHandlerCallable;
    EXPECT_CALL(mockExceptionHandlerCallable, Call(::testing::_)).Times(1);
    csp::systems::continuations::detail::testing::SpawnChainThatCallsLogHTTPErrorAndCancelContinuationWithHandlerAtEnd(
        mockExceptionHandlerCallable.AsStdFunction());
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestCallableCalledAndIntermediateNotWhenExceptionThrownHigherInChain)
{
    // Exception thrown higher in chain, expect exception handler callable intermediate method not called, but exception handler callable called.
    ::testing::MockFunction<void()> mockIntermediateStepCallable;
    ::testing::MockFunction<void(const std::exception&)> mockExceptionHandlerCallable;
    EXPECT_CALL(mockIntermediateStepCallable, Call()).Times(0);
    EXPECT_CALL(mockExceptionHandlerCallable, Call(::testing::_)).Times(1);
    csp::systems::continuations::detail::testing::SpawnChainThatCallsLogHTTPErrorAndCancelContinuationWithIntermediateStepAndHandlerAtEnd(
        mockIntermediateStepCallable.AsStdFunction(), mockExceptionHandlerCallable.AsStdFunction());
}

CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestCallableCalledOnSameThreadInContinuationChain)
{
    // Since we're exposing callbacks to chains anyway, might as well verify the WASM requirement that
    // callbacks occur on the same thread as invocation, (ie, inline_scheduler() works ... this is _definately_
    // testing the library now...).

    const auto thisThreadId = std::this_thread::get_id();
    auto verifyThread = [thisThreadId](const std::exception& /*Except*/) { ASSERT_EQ(thisThreadId, std::this_thread::get_id()); };

    ::testing::MockFunction<void(const std::exception&)> mockUnexpectedHandlerCallable;
    EXPECT_CALL(mockUnexpectedHandlerCallable, Call(::testing::_)).Times(0);

    // Just use the exception handler to serve as a general purpose way to call a callable in tests.
    // Could simply have made another detail::testing function ... but why bother when this already exists.
    csp::systems::continuations::detail::testing::SpawnChainThatThrowsGeneralExceptionWithHandlerAtEnd(
        verifyThread, mockUnexpectedHandlerCallable.AsStdFunction(), csp::common::continuations::ResultException("", MakeInvalid<NullResult>()));
}
