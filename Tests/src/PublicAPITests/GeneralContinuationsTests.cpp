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

#include "Common/Continuations.h"

#include "CSP/Systems/SystemsResult.h"
#include "CSP/Web/HTTPResponseCodes.h"
#include "Services/ApiBase/ApiBase.h"
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

namespace
{
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

#if RUN_ALL_UNIT_TESTS || RUN_GENERAL_CONTINUATIONS_TESTS || RUN_REPORT_SUCCESS_TEST
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

    csp::common::continuations::ReportSuccess(MockResultCallback.AsStdFunction(), SuccessMsg.c_str())();
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_GENERAL_CONTINUATIONS_TESTS || RUN_LOG_ERROR_AND_CANCEL_TEST
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
    ASSERT_ANY_THROW(csp::common::continuations::LogErrorAndCancelContinuation(
        MockResultCallback.AsStdFunction(), ErrorMsg.c_str(), ResultCode, HttpResultCode, FailureReason));
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_GENERAL_CONTINUATIONS_TESTS || RUN_ASSERT_REQUEST_SUCCESS_OR_ERROR_FROM_RESULT_WHEN_SUCCESS_TEST
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
    ASSERT_EQ(csp::common::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(
                  MockResultCallback.AsStdFunction(), SuccessMsg.c_str(), ErrorMsg.c_str(), {}, {}, {}, LogLevel::Log)(SuccessResult),
        SuccessResult);
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_GENERAL_CONTINUATIONS_TESTS || RUN_ASSERT_REQUEST_SUCCESS_OR_ERROR_FROM_RESULT_WHEN_ERROR_TEST
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

        ASSERT_ANY_THROW(csp::common::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(
            MockResultCallback.AsStdFunction(), SuccessMsg.c_str(), ErrorMsg.c_str(), {}, {}, {}, LogLevel::Log)(ExpectedFailureResult));

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

        ASSERT_ANY_THROW(csp::common::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(MockResultCallback.AsStdFunction(),
            SuccessMsg.c_str(), ErrorMsg.c_str(), std::make_optional(ResultCodeExplicit), std::make_optional(HttpResultCodeExplicit),
            std::make_optional(FailureReasonExplicit), LogLevel::Log)(ExpectedFailureResult));
    }
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_GENERAL_CONTINUATIONS_TESTS || RUN_ASSERT_REQUEST_SUCCESS_OR_ERROR_FROM_ERRORCODE_WHEN_SUCCESS_TEST
CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestAssertRequestSuccessOrErrorFromErrorCodeWhenSuccess)
{
    RAIIMockLogger MockLogger {};
    csp::common::String SuccessMsg = "Mock Success Msg";

    ::testing::MockFunction<void(const NullResult& Result)> MockResultCallback;

    // When we don't provide an error code, we expect to just log a success message, no return or exception //
    EXPECT_CALL(MockLogger.MockLogCallback, Call(SuccessMsg)).Times(1);

    csp::common::continuations::AssertRequestSuccessOrErrorFromErrorCode(
        MockResultCallback.AsStdFunction(), SuccessMsg.c_str(), {}, {}, {}, LogLevel::Log)({});
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_GENERAL_CONTINUATIONS_TESTS || RUN_ASSERT_REQUEST_SUCCESS_OR_ERROR_FROM_ERRORCODE_WHEN_ERROR_TEST
CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestAssertRequestSuccessOrErrorFromErrorCodeWhenError)
{
    RAIIMockLogger MockLogger {};

    /* Specific values are irrelevent */
    csp::common::String SuccessMsg = "Mock Success Msg";
    EResultCode ResultCode = EResultCode::Failed;
    csp::web::EResponseCodes HttpResultCode = csp::web::EResponseCodes::ResponseContinue;
    ERequestFailureReason FailureReason = ERequestFailureReason::SpacePublicNameDuplicate;
    NullResult ExpectedFailureResult(ResultCode, HttpResultCode, FailureReason);

    ::testing::MockFunction<void(const NullResult& Result)> MockResultCallback;
    ::testing::MockFunction<void(NullResultCallback Callback, std::string ErrorMsg, EResultCode ResultCode, uint16_t HttpResultCode,
        ERequestFailureReason FailureReason, LogLevel LogLevel)>
        MockLogErrorAndCancelContinuation;

    // When we provide an error code, we expect the error callback to be called and an error message logged.
    csp::multiplayer::ErrorCode ErrorCode = csp::multiplayer::ErrorCode::NotConnected;
    std::string ExpectedErrorMsg = std::string("Operation errored with error code: ") + csp::multiplayer::ErrorCodeToString(ErrorCode);

    {
        // When we pass full optionals, we expect the values from the optionals to be used to construct the result passed in the callback //
        // Expect that the callback is called with the result constructed as expected.
        EXPECT_CALL(MockResultCallback, Call(ExpectedFailureResult)).Times(1);
        // Expect that we log the error message
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::String(ExpectedErrorMsg.c_str()))).Times(1);

        ASSERT_ANY_THROW(csp::common::continuations::AssertRequestSuccessOrErrorFromErrorCode(MockResultCallback.AsStdFunction(), SuccessMsg.c_str(),
            std::make_optional(ResultCode), std::make_optional(HttpResultCode), std::make_optional(FailureReason), LogLevel::Log)(ErrorCode));

        ::testing::Mock::VerifyAndClearExpectations(&MockResultCallback);
        ::testing::Mock::VerifyAndClearExpectations(&MockLogger.MockLogCallback);
    }
    {
        // When we pass empty optionals, we expect default values to be used to construct the result passed in the callback //
        NullResult DefaultFailureResult(EResultCode::Failed, 500, ERequestFailureReason::Unknown);

        // Expect that the callback is called with the result constructed as expected.
        EXPECT_CALL(MockResultCallback, Call(DefaultFailureResult)).Times(1);
        // Expect that we log the error message
        EXPECT_CALL(MockLogger.MockLogCallback, Call(csp::common::String(ExpectedErrorMsg.c_str()))).Times(1);

        ASSERT_ANY_THROW(csp::common::continuations::AssertRequestSuccessOrErrorFromErrorCode(
            MockResultCallback.AsStdFunction(), SuccessMsg.c_str(), {}, {}, {}, LogLevel::Log)(ErrorCode));
    }
}
#endif

// See `Continuation.h::detail::testing` for specifics of how these tests run
#if RUN_ALL_UNIT_TESTS || RUN_GENERAL_CONTINUATIONS_TESTS || RUN_WHEN_NO_EXCEPTION_THROWN_IN_CONTINUATION_CHAIN_THEN_HANDLER_NOT_CALLED_TEST
CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestWhenNoExceptionThrownInContinuationChainThenHandlerNotCalled)
{
    // No exception, expect exception handler callable not called.
    ::testing::MockFunction<void()> MockExceptionHandlerCallable;
    EXPECT_CALL(MockExceptionHandlerCallable, Call()).Times(0);
    csp::common::continuations::detail::testing::SpawnChainThatThrowsNoExceptionWithHandlerAtEnd(MockExceptionHandlerCallable.AsStdFunction());
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_GENERAL_CONTINUATIONS_TESTS || RUN_WHEN_EXCEPTION_THROWN_IN_CONTINUATION_CHAIN_THEN_HANDLER_CALLED_TEST
CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestWhenExceptionThrownInContinuationChainThenHandlerCalled)
{
    // Exception thrown, expect exception handler callable called
    ::testing::MockFunction<void()> MockExceptionHandlerCallable;
    EXPECT_CALL(MockExceptionHandlerCallable, Call()).Times(1);
    csp::common::continuations::detail::testing::SpawnChainThatThrowsGeneralExceptionWithHandlerAtEnd(MockExceptionHandlerCallable.AsStdFunction());
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_GENERAL_CONTINUATIONS_TESTS || RUN_WHEN_CONTINUATION_CHAIN_CANCELLED_THEN_HANDLER_AND_RESULTCALLBACK_CALLED_TEST
CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestWhenContinuationChainCancelledThenHandlerAndResultCallbackCalled)
{
    // Exception thrown, expect exception handler callable called, as well as result callback called. (Just testing our specific way of throwing here)
    ::testing::MockFunction<void()> MockExceptionHandlerCallable;
    ::testing::MockFunction<void(const NullResult& Result)> MockResultCallback;
    EXPECT_CALL(MockExceptionHandlerCallable, Call()).Times(1);
    EXPECT_CALL(MockResultCallback, Call(::testing::_)).Times(1);
    csp::common::continuations::detail::testing::SpawnChainThatCallsLogErrorAndCancelContinuationWithHandlerAtEnd(
        MockExceptionHandlerCallable.AsStdFunction(), MockResultCallback.AsStdFunction());
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_GENERAL_CONTINUATIONS_TESTS || RUN_CALLABLE_CALLED_AND_INTERMEDIATE_NOT_WHEN_EXCEPTION_THROWN_HIGHER_IN_CHAIN_TEST
CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestCallableCalledAndIntermediateNotWhenExceptionThrownHigherInChain)
{
    // Exception thrown higher in chain, expect exception handler callable intermediate method not called, but exception handler callable called.
    ::testing::MockFunction<void()> MockIntermediateStepCallable;
    ::testing::MockFunction<void()> MockExceptionHandlerCallable;
    ::testing::MockFunction<void(const NullResult& Result)> MockResultCallback;
    EXPECT_CALL(MockIntermediateStepCallable, Call()).Times(0);
    EXPECT_CALL(MockExceptionHandlerCallable, Call()).Times(1);
    EXPECT_CALL(MockResultCallback, Call(::testing::_)).Times(1);
    csp::common::continuations::detail::testing::SpawnChainThatCallsLogErrorAndCancelContinuationWithIntermediateStepAndHandlerAtEnd(
        MockIntermediateStepCallable.AsStdFunction(), MockExceptionHandlerCallable.AsStdFunction(), MockResultCallback.AsStdFunction());
}
#endif

#if RUN_ALL_UNIT_TESTS || RUN_GENERAL_CONTINUATIONS_TESTS || RUN_CALLABLE_CALLED_ON_SAME_THREAD_IN_CONTINUATION_CHAIN
CSP_PUBLIC_TEST(CSPEngine, GeneralContinuationsTests, TestCallableCalledOnSameThreadInContinuationChain)
{
    // Since we're exposing callbacks to chains anyway, might as well verify the WASM requirement that
    // callbacks occur on the same thread as invocation, (ie, inline_scheduler() works ... this is _definately_
    // testing the library now...).

    const auto ThisThreadId = std::this_thread::get_id();
    auto VerifyThread = [ThisThreadId]() { ASSERT_EQ(ThisThreadId, std::this_thread::get_id()); };
    // Just use the exception handler to serve as a general purpose way to call a callable in tests.
    // Could simply have made another detail::testing function ... but why bother when this already exists.
    csp::common::continuations::detail::testing::SpawnChainThatThrowsGeneralExceptionWithHandlerAtEnd(VerifyThread);
}
#endif