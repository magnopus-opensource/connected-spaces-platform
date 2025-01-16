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

namespace csp::systems
{

/// @brief Enum for setting csp::systems::Asset third party platform type.
/// NONE indicates that the asset will work on any platform.
/// Any other value indicates it will only work with this platform.
enum class EThirdPartyPlatform
{
    NONE,
    UNREAL,
    UNITY
};
} // namespace csp::systems
