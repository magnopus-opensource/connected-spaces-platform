#pragma once
#include "Olympus/Common/Array.h"
#include "Olympus/Common/String.h"
#include "Olympus/Services/WebService.h"

namespace oly_systems
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
class OLY_API ThirdPartyProviderDetails
{
public:
    oly_common::String ProviderName;
    oly_common::String ProviderClientId;
    oly_common::Array<oly_common::String> ProviderAuthScopes;
    oly_common::String AuthoriseURL;
};

class OLY_API ProviderDetailsResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    friend class UserSystem;
    OLY_END_IGNORE
    /** @endcond */

public:
    ProviderDetailsResult() = default;
    ProviderDetailsResult(void*) {};

    [[nodiscard]] ThirdPartyProviderDetails& GetDetails();
    [[nodiscard]] const ThirdPartyProviderDetails& GetDetails() const;

private:
    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    ThirdPartyProviderDetails ProviderDetails;
};

typedef std::function<void(const ProviderDetailsResult& Result)> ProviderDetailsResultCallback;

}; // namespace oly_systems
