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

#include "Multiplayer/Script/ComponentScriptInterface.h"

#include <string>
#include <vector>

namespace csp::multiplayer
{

class TextSpaceComponent;

class TextSpaceComponentScriptInterface : public ComponentScriptInterface
{
public:
	TextSpaceComponentScriptInterface(TextSpaceComponent* InComponent = nullptr);
	DECLARE_SCRIPT_PROPERTY(std::string, Text);
	DECLARE_SCRIPT_PROPERTY(Vector3, Position);
	DECLARE_SCRIPT_PROPERTY(Vector3, Scale);
	DECLARE_SCRIPT_PROPERTY(Vector4, Rotation);
	DECLARE_SCRIPT_PROPERTY(Vector3, TextColor);
	DECLARE_SCRIPT_PROPERTY(Vector3, BackgroundColor);
	DECLARE_SCRIPT_PROPERTY(bool, IsBackgroundVisible);
	DECLARE_SCRIPT_PROPERTY(uint32_t, Width);
	DECLARE_SCRIPT_PROPERTY(uint32_t, Height);
	DECLARE_SCRIPT_PROPERTY(int64_t, BillboardMode);
	DECLARE_SCRIPT_PROPERTY(bool, IsVisible);
	DECLARE_SCRIPT_PROPERTY(bool, IsARVisible);
};

} // namespace csp::multiplayer