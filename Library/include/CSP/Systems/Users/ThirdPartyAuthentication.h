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
    // @brief The name of the provider, e.g. "Google"
    csp::common::String ProviderName;

    // @brief The application client id with the provider
    csp::common::String ProviderClientId;

    // @brief The scopes required to authenticate with this provider
    csp::common::Array<csp::common::String> ProviderAuthScopes;

    // @brief URL of the provider's authentication endpoint
    csp::common::String ProviderAuthURL;

    // @brief A unique state Id to correlate the authorization request and response from the social provider.
    csp::common::String ThirdPartyAuthStateId;

    // @brief URL of the provider's token endpoint used to initiate the authorization flow
    csp::common::String ProviderRedirectURL;
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
    ProviderDetailsResult(void*) {};

    [[nodiscard]] ThirdPartyProviderDetails& GetDetails();
    [[nodiscard]] const ThirdPartyProviderDetails& GetDetails() const;

private:
    CSP_NO_EXPORT void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

    ThirdPartyProviderDetails ProviderDetails;
};

typedef std::function<void(const ProviderDetailsResult& Result)> ProviderDetailsResultCallback;

}; // namespace csp::systems
