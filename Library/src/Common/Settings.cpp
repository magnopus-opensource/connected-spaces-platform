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

#include "CSP/Common/Settings.h"
#include "CSP/Common/fmt_Formatters.h"
#include "Json/JsonSerializer.h"

namespace csp::common
{
bool ApplicationSettings::operator==(const ApplicationSettings& other) const
{
    return (ApplicationName == other.ApplicationName) && (Context == other.Context) && (AllowAnonymous == other.AllowAnonymous)
        && (Settings == other.Settings);
}

bool ApplicationSettings::operator!=(const ApplicationSettings& other) const { return !(*this == other); }

bool SettingsCollection::operator==(const SettingsCollection& other) const
{
    return (UserId == other.UserId) && (Context == other.Context) && (Settings == other.Settings);
}

bool SettingsCollection::operator!=(const SettingsCollection& other) const { return !(*this == other); }
} // namespace csp::systems

void ToJson(csp::json::JsonSerializer& serializer, const csp::common::ApplicationSettings& obj)
{
    serializer.SerializeMember("applicationName", obj.ApplicationName);
    serializer.SerializeMember("context", obj.Context);
    serializer.SerializeMember("allowAnonymous", obj.AllowAnonymous);
    serializer.SerializeMember("settings", obj.Settings);
}
