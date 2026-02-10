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

/*
 * Enums common between all the libraries that make up CSP.
 * These tend to merely be types for data exchange between libraries.
 * Don't get too hung up on namespaces
 */

#pragma once
#include <cstdint>
#include <string>

namespace csp::common
{
/// @brief Enum which represents possible login states of a CSP client.
enum class ELoginState : uint8_t
{
    LoginThirdPartyProviderDetailsRequested,
    LoginRequested,
    LoggedIn,
    LogoutRequested,
    LoggedOut,
    Error,
};

}

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

namespace csp::multiplayer
{

/// @brief Enumerates the supported states for an avatar.
///        These are used to establish the related animation that the avatar will use on its state machine.
enum class AvatarState
{
    Idle = 0,
    Walking,
    Running,
    Flying,
    Jumping,
    Falling,
    Num
};

/// @brief Enumerates the supported play mode for the avatar.
enum class AvatarPlayMode
{
    /// Viewer mode, with desktop or mobile viewer
    Default = 0,
    /// Intended for use with augmented reality viewers (e.g. mobile AR)
    AR,
    /// Intended for use with virtual reality viewers (e.g. Quest 2)
    VR,
    /// Intended for use with creator privileges (e.g. designers and editors customizing a space)
    Creator,
    Num
};

/// @brief Enum used to indicate the failure state of a multiplayer request.
enum class ErrorCode
{
    None,
    Unknown,
    NotConnected,
    AlreadyConnected,
    SpaceUserLimitExceeded
};

inline std::string ErrorCodeToString(csp::multiplayer::ErrorCode ErrorCode)
{
    std::string ErrorCodeString;
    switch (ErrorCode)
    {
    case csp::multiplayer::ErrorCode::None:
    {
        ErrorCodeString = "None";
        break;
    }
    case csp::multiplayer::ErrorCode::Unknown:
    {
        ErrorCodeString = "Unknown";
        break;
    }
    case csp::multiplayer::ErrorCode::NotConnected:
    {
        ErrorCodeString = "NotConnected";
        break;
    }
    case csp::multiplayer::ErrorCode::AlreadyConnected:
    {
        ErrorCodeString = "AlreadyConnected";
        break;
    }
    case csp::multiplayer::ErrorCode::SpaceUserLimitExceeded:
    {
        ErrorCodeString = "SpaceUserLimitExceeded";
        break;
    }
    default:
    {
        ErrorCodeString = std::string("Unknown error code. Value") + std::to_string(static_cast<unsigned int>(ErrorCode));
        break;
    }
    }

    return ErrorCodeString;
}

/// @brief Enum representing the failure reason of an entity modification operation returned from IRealtimeEngine::IsEntityModifiable.
enum class ModifiableStatus
{
    /// The operation succeeded.
    Modifiable,
    /// The entity is locked (LockType is not None).
    EntityLocked,
    /// Entity doesn't belong to this client and is not transferable.
    EntityNotOwnedAndUntransferable
};

/// @brief Enumerates the type of stereo the video player and texture material supports.
enum class StereoVideoType
{
    None = 0,
    SideBySide,
    TopBottom
};
} // namespace csp::multiplayer

namespace csp::systems
{

/// @brief Indicates special handling for any thirdparty platform
/// @note We may remove this soon, as it's deceptive implying these are the only platforms we support.
enum class EThirdPartyPlatform
{
    NONE,
    UNREAL,
    UNITY
};

/// @brief Code to indicate the result of a request.
/// Request results should be checked for a success by clients before using any other accessors.
enum class EResultCode : uint8_t
{
    Init,
    InProgress,
    Success,
    Failed
};

enum class ERequestFailureReason
{
    Unknown = -1,
    None = 0,
    AddUserToSpaceDenied,
    UserSpaceAccessDenied,
    UserSpaceBannedAccessDenied,
    UserSpaceFullAccessDenied,
    UserSpaceInviteExpired,
    SpacePublicNameDuplicate,
    UserMaxSpaceLimitReached,
    UserAccountLocked,
    UserMissingPassword,
    UserUnverifiedEmail,
    UserBannedFromSpace,
    UserInvalidEmailDomain,
    UserInvalidThirdPartyAuth,
    UserAgeNotVerified,
    UserGuestLoginDisallowed,
    UserAgoraLimitReached,
    UserOpenAILimitReached,
    UserTicketedSpacesLimitReached,
    UserSpaceConcurrentUsersLimitReached,
    PrototypeReservedKeysNotAllowed,
    AssetInvalidFileContents,
    AssetInvalidFileType,
    AssetAudioVideoLimitReached,
    AssetObjectCaptureLimitReached,
    AssetTotalUploadSizeLimitReached,
    TicketUnknownNumber,
    TicketEmailMismatch,
    TicketVendorOAuthFailure,
    TicketOAuthTokenInvalid,
    TicketAlreadyApplied,
    ShopifyConnectionBroken,
    ShopifyInvalidStoreName,
    UserShopifyLimitReached,
    UserTokenRefreshFailed,
    InvalidSequenceKey,
};
} // namespace csp::systems
