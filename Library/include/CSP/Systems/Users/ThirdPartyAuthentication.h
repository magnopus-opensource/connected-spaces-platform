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
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/WebService.h"

namespace csp::systems
{

/// @brief FDN supported Authentication Providers, the ones that can be used are the ones above Num
/// Note: it's important for this enum to start with 0
/// Note2: make sure to keep all enum values *above* Num
enum EThirdPartyAuthenticationProviders
{
    Google = 0,
    Discord,
    Apple,
    Num,
    Invalid = Num
};

/// @ingroup User System
/// @brief Data class used in the GetThirdPartyProviderAuthoriseURL authentication step
class CSP_API ThirdPartyProviderDetails
{
public:
    csp::common::String ProviderName;
    csp::common::String ProviderClientId;
    csp::common::Array<csp::common::String> ProviderAuthScopes;
    csp::common::String AuthoriseURL;
};

/// @brief Result structure for a third party auth provider details request
class CSP_API ProviderDetailsResult : public csp::systems::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    CSP_START_IGNORE
    friend class UserSystem;
    CSP_END_IGNORE
    /** @endcond */

public:
    ProviderDetailsResult() = default;
    ProviderDetailsResult(void*) { };

    [[nodiscard]] ThirdPartyProviderDetails& GetDetails();
    [[nodiscard]] const ThirdPartyProviderDetails& GetDetails() const;

private:
    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    ThirdPartyProviderDetails ProviderDetails;
};

/// @brief Data requred to establish third party authentication.
class CSP_API ThirdPartyAuthDetails
{
public:
    ThirdPartyAuthDetails()
        : ThirdPartyRequestedAuthProvider(EThirdPartyAuthenticationProviders::Invalid) { };

    csp::common::String ThirdPartyAuthStateId;
    EThirdPartyAuthenticationProviders ThirdPartyRequestedAuthProvider;
    csp::common::String ThirdPartyAuthRedirectURL;
    csp::common::String ThirdPartyAuthURL;
};

/// @ingroup User System
/// @brief Data class used to contain third party auth details provided by MCS for the specified provider
class CSP_API ThirdPartyAuthDetailsResult : public ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    friend class UserSystem;
    /** @endcond */

public:
    [[nodiscard]] const ThirdPartyAuthDetails& GetThirdPartyAuthDetails() const;
    CSP_NO_EXPORT ThirdPartyAuthDetailsResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
        : csp::systems::ResultBase(ResCode, HttpResCode)
    {
    }

private:
    ThirdPartyAuthDetailsResult() = default;
    ThirdPartyAuthDetailsResult(csp::common::String ThirdPartyAuthStateId, EThirdPartyAuthenticationProviders ThirdPartyRequestedAuthProvider,
        csp::common::String ThirdPartyAuthRedirectURL, csp::common::String ThirdPartyAuthURL);

    ThirdPartyAuthDetails AuthDetails;
};

typedef std::function<void(const ProviderDetailsResult& Result)> ProviderDetailsResultCallback;
typedef std::function<void(const ThirdPartyAuthDetailsResult& Result)> ThirdPartyAuthDetailsResultCallback;

}; // namespace csp::systems
