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
#include <vector>

using Vector2Std = std::vector<float>;
using Vector3Std = std::vector<float>;
using Vector4Std = std::vector<float>;

// Same declaration macro as in ComponentScriptMacros.h
#define DECLARE_SCRIPT_PROPERTY(TYPE, NAME)                                                                                                          \
    TYPE Get##NAME() const;                                                                                                                          \
    void Set##NAME(TYPE Value);

// CodeAttribute-specific macros without Component references
#define DEFINE_ATTRIBUTE_PROPERTY_TYPE(COMP, CSPTYPE, SCRIPTTYPE, NAME)                                                                              \
    void COMP##ScriptInterface::Set##NAME(SCRIPTTYPE Value)                                                                                          \
    {                                                                                                                                                \
        m##COMP.Set##NAME((CSPTYPE)Value);                                                                                                           \
    }                                                                                                                                                \
                                                                                                                                                     \
    SCRIPTTYPE COMP##ScriptInterface::Get##NAME() const { return (SCRIPTTYPE)m##COMP.Get##NAME(); }

#define DEFINE_ATTRIBUTE_PROPERTY_STRING(COMP, NAME)                                                                                                 \
    void COMP##ScriptInterface::Set##NAME(std::string Value)                                                                                         \
    {                                                                                                                                                \
        m##COMP.Set##NAME((csp::common::String)Value.c_str());                                                                                       \
    }                                                                                                                                                \
                                                                                                                                                     \
    std::string COMP##ScriptInterface::Get##NAME() const { return (std::string)m##COMP.Get##NAME().c_str(); }

#define DEFINE_ATTRIBUTE_PROPERTY_VEC2(COMP, NAME)                                                                                                   \
    Vector2Std COMP##ScriptInterface::Get##NAME() const                                                                       \
    {                                                                                                                                                \
        Vector2Std Vec = { 0, 0 };                                                                                            \
                                                                                                                                                     \
        csp::common::Vector2 Value = m##COMP.Get##NAME();                                                                                            \
        Vec[0] = Value.X;                                                                                                                            \
        Vec[1] = Value.Y;                                                                                                                            \
                                                                                                                                                     \
        return Vec;                                                                                                                                  \
    }                                                                                                                                                \
                                                                                                                                                     \
    void COMP##ScriptInterface::Set##NAME(Vector2Std Vec)                                                                     \
    {                                                                                                                                                \
        csp::common::Vector2 Value(Vec[0], Vec[1]);                                                                                                  \
        m##COMP.Set##NAME(Value);                                                                                                                    \
    }

#define DEFINE_ATTRIBUTE_PROPERTY_VEC3(COMP, NAME)                                                                                                   \
    Vector3Std COMP##ScriptInterface::Get##NAME() const                                                                       \
    {                                                                                                                                                \
        Vector3Std Vec = { 0, 0, 0 };                                                                                         \
                                                                                                                                                     \
        csp::common::Vector3 Value = m##COMP.Get##NAME();                                                                                            \
        Vec[0] = Value.X;                                                                                                                            \
        Vec[1] = Value.Y;                                                                                                                            \
        Vec[2] = Value.Z;                                                                                                                            \
                                                                                                                                                     \
        return Vec;                                                                                                                                  \
    }                                                                                                                                                \
                                                                                                                                                     \
    void COMP##ScriptInterface::Set##NAME(Vector3Std Vec)                                                                     \
    {                                                                                                                                                \
        csp::common::Vector3 Value(Vec[0], Vec[1], Vec[2]);                                                                                          \
        m##COMP.Set##NAME(Value);                                                                                                                    \
    }

#define DEFINE_ATTRIBUTE_PROPERTY_VEC4(COMP, NAME)                                                                                                   \
    Vector4Std COMP##ScriptInterface::Get##NAME() const                                                                       \
    {                                                                                                                                                \
        Vector4Std Vec = { 0, 0, 0, 0 };                                                                                      \
                                                                                                                                                     \
        csp::common::Vector4 Value = m##COMP.Get##NAME();                                                                                            \
        Vec[0] = Value.X;                                                                                                                            \
        Vec[1] = Value.Y;                                                                                                                            \
        Vec[2] = Value.Z;                                                                                                                            \
        Vec[3] = Value.W;                                                                                                                            \
                                                                                                                                                     \
        return Vec;                                                                                                                                  \
    }                                                                                                                                                \
                                                                                                                                                     \
    void COMP##ScriptInterface::Set##NAME(Vector4Std Vec)                                                                     \
    {                                                                                                                                                \
        csp::common::Vector4 Value(Vec[0], Vec[1], Vec[2], Vec[3]);                                                                                  \
        m##COMP.Set##NAME(Value);                                                                                                                    \
    }
