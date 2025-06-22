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

#include "CSP/Multiplayer/Components/CodeAttribute.h"
#include "Multiplayer/Script/CodeAttributeScriptMacros.h" // Include for Vector types
#include <string>

namespace csp::multiplayer
{

class CodeAttributeScriptInterface
{
public:
    CodeAttributeScriptInterface();
    CodeAttributeScriptInterface(const CodeAttribute* attribute);

    // Add static class id for QuickJS binding
    static int js_class_id;

    // Update property declarations to use std vector types
    DECLARE_SCRIPT_PROPERTY(int32_t, Type);
    DECLARE_SCRIPT_PROPERTY(std::string, StringValue);
    DECLARE_SCRIPT_PROPERTY(float, FloatValue);
    DECLARE_SCRIPT_PROPERTY(int32_t, IntValue);
    DECLARE_SCRIPT_PROPERTY(bool, BoolValue);
    DECLARE_SCRIPT_PROPERTY(Vector2Std, Vector2Value);
    DECLARE_SCRIPT_PROPERTY(Vector3Std, Vector3Value);
    DECLARE_SCRIPT_PROPERTY(Vector4Std, Vector4Value);
    DECLARE_SCRIPT_PROPERTY(float, Min);
    DECLARE_SCRIPT_PROPERTY(float, Max);

private:
    CodeAttribute mCodeAttribute;
};

} // namespace csp::multiplayer
