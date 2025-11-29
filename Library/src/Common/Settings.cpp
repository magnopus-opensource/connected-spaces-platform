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
bool ApplicationSettings::operator==(const ApplicationSettings& Other) const
{
    return (ApplicationName == Other.ApplicationName) && (Context == Other.Context) && (AllowAnonymous == Other.AllowAnonymous)
        && (Settings == Other.Settings);
}

bool ApplicationSettings::operator!=(const ApplicationSettings& Other) const { return !(*this == Other); }

bool SettingsCollection::operator==(const SettingsCollection& Other) const
{
    return (UserId == Other.UserId) && (Context == Other.Context) && (Settings == Other.Settings);
}

bool SettingsCollection::operator!=(const SettingsCollection& Other) const { return !(*this == Other); }
} // namespace csp::systems

void ToJson(csp::json::JsonSerializer& Serializer, const csp::common::ApplicationSettings& Obj)
{
    Serializer.SerializeMember("applicationName", Obj.ApplicationName);
    Serializer.SerializeMember("context", Obj.Context);
    Serializer.SerializeMember("allowAnonymous", Obj.AllowAnonymous);
    Serializer.SerializeMember("settings", Obj.Settings);
}
