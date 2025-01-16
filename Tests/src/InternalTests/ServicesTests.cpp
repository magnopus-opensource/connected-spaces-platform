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

#include "Services/ApiBase/ApiBase.h"

#include "TestHelpers.h"

using namespace csp::services;

CSP_INTERNAL_TEST(CSPEngine, ServicesTests, IsValidResponseCodeTest)
{
    DtoBase* InDto = nullptr;
    ApiResponseBase ResponseBase = ApiResponseBase(InDto);

    // 201 SUCCESS
    ResponseBase.SetResponseCode(csp::web::EResponseCodes::ResponseCreated, csp::web::EResponseCodes::ResponseOK);
    EXPECT_TRUE(ResponseBase.GetResponseCode() == EResponseCode::ResponseSuccess);

    // 226 SUCCESS
    ResponseBase.SetResponseCode(csp::web::EResponseCodes::ResponseImUsed, csp::web::EResponseCodes::ResponseOK);
    EXPECT_TRUE(ResponseBase.GetResponseCode() == EResponseCode::ResponseSuccess);

    // 100 FAILURE
    ResponseBase.SetResponseCode(csp::web::EResponseCodes::ResponseContinue, csp::web::EResponseCodes::ResponseOK);
    EXPECT_TRUE(ResponseBase.GetResponseCode() == EResponseCode::ResponseFailed);

    // 300 FAILURE
    ResponseBase.SetResponseCode(csp::web::EResponseCodes::ResponseMultipleChoices, csp::web::EResponseCodes::ResponseOK);
    EXPECT_TRUE(ResponseBase.GetResponseCode() == EResponseCode::ResponseFailed);

    // 400 FAILURE
    ResponseBase.SetResponseCode(csp::web::EResponseCodes::ResponseBadRequest, csp::web::EResponseCodes::ResponseOK);
    EXPECT_TRUE(ResponseBase.GetResponseCode() == EResponseCode::ResponseFailed);

    // 500 FAILURE
    ResponseBase.SetResponseCode(csp::web::EResponseCodes::ResponseInternalServerError, csp::web::EResponseCodes::ResponseOK);
    EXPECT_TRUE(ResponseBase.GetResponseCode() == EResponseCode::ResponseFailed);
}

#endif