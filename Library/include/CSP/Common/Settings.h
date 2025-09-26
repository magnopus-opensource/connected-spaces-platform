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

#pragma once

#include "CSP/CSPCommon.h"

#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"

namespace csp::common
{

/// @ingroup Application Settings System
/// @brief Represents configuration settings for an application context.
class CSP_API ApplicationSettings
{
public:
    /// @brief The name of the application.
    /// Used to identify the application this settings instance is associated with.
    csp::common::String ApplicationName;

    /// @brief A context identifier for the settings.
    /// This can be used to scope or separate settings across different environments or modules.
    csp::common::String Context;

    /// @brief Flag indicating whether anonymous access is allowed.
    /// If true, an anonymous user can access these settings.
    bool AllowAnonymous = false;

    /// @brief A key-value store of arbitrary application settings.
    /// The keys and values are both strings and represent additional configurable options.
    csp::common::Map<csp::common::String, csp::common::String> Settings;

    bool operator==(const csp::common::ApplicationSettings& Other);
    bool operator!=(const csp::common::ApplicationSettings& Other);
};

/// @brief Represents configuration settings related to a user in a specific context
class CSP_API SettingsCollection
{
public:
    SettingsCollection() = default;

    /// @brief ID of the user these settings relate to
    csp::common::String UserId;

    /// @brief A context identifier for the settings.
    /// This can be used to scope or separate settings across different environments or modules.
    csp::common::String Context;

    /// @brief A key-value store of arbitrary user settings.
    csp::common::Map<csp::common::String, csp::common::String> Settings;

    bool operator==(const csp::common::SettingsCollection& Other);
    bool operator!=(const csp::common::SettingsCollection& Other);
};
} // namespace csp::systems

namespace csp::json
{
class JsonSerializer;
}

void ToJson(csp::json::JsonSerializer& Serializer, const csp::common::ApplicationSettings& Obj);
