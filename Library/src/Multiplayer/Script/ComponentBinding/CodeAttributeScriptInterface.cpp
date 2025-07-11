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

#include "Multiplayer/Script/ComponentBinding/CodeAttributeScriptInterface.h"
#include "CSP/Multiplayer/Components/CodeAttribute.h"
#include "Multiplayer/Script/CodeAttributeScriptMacros.h" // Include the new header

namespace csp::multiplayer
{

// Initialize the static js_class_id
int CodeAttributeScriptInterface::js_class_id = 0;

CodeAttributeScriptInterface::CodeAttributeScriptInterface()
    : mCodeAttribute()
{
}

CodeAttributeScriptInterface::CodeAttributeScriptInterface(const CodeAttribute* attribute)
    : mCodeAttribute(*attribute)
{
}

// Special implementation for Type property to handle PropertyType enum
void CodeAttributeScriptInterface::SetType(int32_t Value)
{
    mCodeAttribute.SetType(static_cast<CodePropertyType>(Value));
}

int32_t CodeAttributeScriptInterface::GetType() const
{
    return static_cast<int32_t>(mCodeAttribute.GetType());
}

// Use macros for other properties
DEFINE_ATTRIBUTE_PROPERTY_STRING(CodeAttribute, StringValue);
DEFINE_ATTRIBUTE_PROPERTY_STRING(CodeAttribute, AssetCollectionValue);
DEFINE_ATTRIBUTE_PROPERTY_STRING(CodeAttribute, AssetValue);
DEFINE_ATTRIBUTE_PROPERTY_TYPE(CodeAttribute, float, float, FloatValue);
DEFINE_ATTRIBUTE_PROPERTY_TYPE(CodeAttribute, int32_t, int32_t, IntValue);
DEFINE_ATTRIBUTE_PROPERTY_TYPE(CodeAttribute, bool, bool, BoolValue);
DEFINE_ATTRIBUTE_PROPERTY_VEC2(CodeAttribute, Vector2Value);
DEFINE_ATTRIBUTE_PROPERTY_VEC3(CodeAttribute, Vector3Value);
DEFINE_ATTRIBUTE_PROPERTY_VEC4(CodeAttribute, Vector4Value);
DEFINE_ATTRIBUTE_PROPERTY_TYPE(CodeAttribute, float, float, Min);
DEFINE_ATTRIBUTE_PROPERTY_TYPE(CodeAttribute, float, float, Max);

} // namespace csp::multiplayer
