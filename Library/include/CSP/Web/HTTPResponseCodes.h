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
#pragma once
#include <cstdint>

namespace csp::web
{

/// @brief Enum which represents all possible HTTP responses.
enum class EResponseCodes : uint16_t
{
    ResponseInit = 0,
    ResponseContinue = 100,
    ResponseSwitchingProtocols = 101,
    ResponseProcessing = 102,
    ResponseOK = 200,
    ResponseCreated = 201,
    ResponseAccepted = 202,
    ResponseNonauthoritative = 203,
    ResponseNoContent = 204,
    ResponseResetContent = 205,
    ResponsePartialContent = 206,
    ResponseMultiStatus = 207,
    ResponseAlreadyReported = 208,
    ResponseImUsed = 226,
    ResponseMultipleChoices = 300,
    ResponseMovedPermanently = 301,
    ResponseFound = 302,
    ResponseSeeOther = 303,
    ResponseNotModified = 304,
    ResponseUseProxy = 305,
    ResponseTemporaryRedirect = 307,
    ResponsePermanentRedirect = 308,
    ResponseBadRequest = 400,
    ResponseUnauthorized = 401,
    ResponsePaymentRequired = 402,
    ResponseForbidden = 403,
    ResponseNotFound = 404,
    ResponseMethodNotAllowed = 405,
    ResponseNotAcceptable = 406,
    ResponseProxyAuthenticationRequired = 407,
    ResponseRequestTimeout = 408,
    ResponseConflict = 409,
    ResponseGone = 410,
    ResponseLengthRequired = 411,
    ResponsePreconditionFailed = 412,
    ResponseRequestEntityTooLarge = 413,
    ResponseRequestUriTooLong = 414,
    ResponseUnsupportedMediaType = 415,
    ResponseRequestedRangeNotSatisfiable = 416,
    ResponseExpectationFailed = 417,
    ResponseImATeapot = 418,
    ResponseEnchanceYourCalm = 420,
    ResponseMisdirectedRequest = 421,
    ResponseUnprocessableEntity = 422,
    ResponseLocked = 423,
    ResponseFailedDependency = 424,
    ResponseUpgradeRequired = 426,
    ResponsePreconditionRequired = 428,
    ResponseTooManyRequests = 429,
    ResponseRequestHeaderFieldsTooLarge = 431,
    ResponseUnavailableForLegalReasons = 451,
    ResponseInternalServerError = 500,
    ResponseNotImplemented = 501,
    ResponseBadGateway = 502,
    ResponseServiceUnavailable = 503,
    ResponseGatewayTimeout = 504,
    ResponseVersionNotSupported = 505,
    ResponseVariantAlsoNegotiates = 506,
    ResponseInsufficientStorage = 507,
    ResponseLoopDetected = 508,
    ResponseNotExtended = 510,
    ResponseNetworkAuthenticationRequired = 511
};

} // namespace csp::web
