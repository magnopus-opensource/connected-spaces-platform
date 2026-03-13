/*
 * Copyright 2026 Magnopus LLC

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

#include "CSP/Multiplayer/Components/NgxScriptAttribute.h"

namespace csp::multiplayer
{

NgxScriptAttribute::NgxScriptAttribute()
    : Name("")
    , Type(ScriptAttributeType::Invalid)
    , Min(0.0f)
    , Max(0.0f)
    , Step(0.0f)
    , HasMin(false)
    , HasMax(false)
    , HasStep(false)
    , Required(false)
    , Value()
{
}

bool NgxScriptAttribute::operator==(const NgxScriptAttribute& Other) const
{
    return Name == Other.Name && Type == Other.Type && Min == Other.Min && Max == Other.Max && Step == Other.Step && HasMin == Other.HasMin
        && HasMax == Other.HasMax && HasStep == Other.HasStep && Required == Other.Required;
}

bool NgxScriptAttribute::operator!=(const NgxScriptAttribute& Other) const { return !(*this == Other); }

} // namespace csp::multiplayer
