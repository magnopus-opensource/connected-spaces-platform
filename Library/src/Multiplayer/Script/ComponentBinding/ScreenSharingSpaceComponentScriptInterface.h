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

#include "Multiplayer/Script/ComponentScriptInterface.h"

#include <string>
#include <vector>

namespace csp::multiplayer
{

class ScreenSharingSpaceComponent;

class ScreenSharingSpaceComponentScriptInterface : public ComponentScriptInterface
{
public:
    ScreenSharingSpaceComponentScriptInterface(ScreenSharingSpaceComponent* InComponent = nullptr);

    DECLARE_SCRIPT_PROPERTY(std::string, UserId);
    DECLARE_SCRIPT_PROPERTY(std::string, DefaultImageCollectionId);
    DECLARE_SCRIPT_PROPERTY(std::string, DefaultImageAssetId);
    DECLARE_SCRIPT_PROPERTY(float, AttenuationRadius);

    DECLARE_SCRIPT_PROPERTY(Vector3, Position);
    DECLARE_SCRIPT_PROPERTY(Vector4, Rotation);
    DECLARE_SCRIPT_PROPERTY(Vector3, Scale);

    DECLARE_SCRIPT_PROPERTY(bool, IsVisible);
    DECLARE_SCRIPT_PROPERTY(bool, IsARVisible);
    DECLARE_SCRIPT_PROPERTY(bool, IsVirtualVisible);
    DECLARE_SCRIPT_PROPERTY(bool, IsShadowCaster);
};

} // namespace csp::multiplayer
