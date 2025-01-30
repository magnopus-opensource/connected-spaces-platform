#pragma once

#include "Olympus/Common/Array.h"
#include "Olympus/Common/Map.h"
#include "Olympus/Common/String.h"
#include "Olympus/OlympusCommon.h"
#include "Olympus/Services/WebService.h"
#include "Olympus/Web/HTTPResponseCodes.h"

#include <functional>

namespace oly_services
{

class ApiResponseBase;

OLY_START_IGNORE
template <typename T, typename U, typename V, typename W> class ApiResponseHandler;
OLY_END_IGNORE

} // namespace oly_services

namespace oly_systems
{

/// @ingroup Settings System
/// @brief Data representation of a Settings collection which maps to a UserService::Settings.
class OLY_API SettingsCollection
{
public:
    SettingsCollection() = default;

    oly_common::String UserId;
    oly_common::String Context;
    oly_common::Map<oly_common::String, oly_common::String> Settings;
};

/// @ingroup Settings System
/// @brief Data class used to contain information when creating a Settings collection.
class OLY_API SettingsCollectionResult : public oly_services::ResultBase
{
    /** @cond DO_NOT_DOCUMENT */
    OLY_START_IGNORE
    template <typename T, typename U, typename V, typename W> friend class oly_services::ApiResponseHandler;
    OLY_END_IGNORE
    /** @endcond */

public:
    /// @brief Retrieves the settings collection result.
    /// @return SettingsCollection : const ref of settings collection class
    const SettingsCollection& GetSettingsCollection() const;

private:
    SettingsCollectionResult(void*) {};

    void OnResponse(const oly_services::ApiResponseBase* ApiResponse) override;

    SettingsCollection SettingsCollection;
};

/// @brief Callback containing Settings collection.
/// @param Result SettingsCollectionResult : result class
typedef std::function<void(const SettingsCollectionResult& Result)> SettingsResultCallback;

} // namespace oly_systems
