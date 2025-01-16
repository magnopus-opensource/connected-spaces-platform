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

#define DECLARE_SCRIPT_PROPERTY(TYPE, NAME)                                                                                                          \
    TYPE Get##NAME() const;                                                                                                                          \
    void Set##NAME(TYPE Value);

#define DEFINE_SCRIPT_PROPERTY_TYPE(COMP, CSPTYPE, SCRIPTTYPE, NAME)                                                                                 \
    void COMP##ScriptInterface::Set##NAME(SCRIPTTYPE Value)                                                                                          \
    {                                                                                                                                                \
        ((COMP*)Component)->Set##NAME((CSPTYPE)Value);                                                                                               \
        SendPropertyUpdate();                                                                                                                        \
    }                                                                                                                                                \
                                                                                                                                                     \
    SCRIPTTYPE COMP##ScriptInterface::Get##NAME() const { return (SCRIPTTYPE)((COMP*)Component)->Get##NAME(); }

#define DEFINE_SCRIPT_PROPERTY_STRING(COMP, NAME)                                                                                                    \
    void COMP##ScriptInterface::Set##NAME(std::string Value)                                                                                         \
    {                                                                                                                                                \
        ((COMP*)Component)->Set##NAME((csp::common::String)Value.c_str());                                                                           \
        SendPropertyUpdate();                                                                                                                        \
    }                                                                                                                                                \
                                                                                                                                                     \
    std::string COMP##ScriptInterface::Get##NAME() const { return (std::string)((COMP*)Component)->Get##NAME().c_str(); }

#define DEFINE_SCRIPT_PROPERTY_VEC2(COMP, NAME)                                                                                                      \
    ComponentScriptInterface::Vector2 COMP##ScriptInterface::Get##NAME() const                                                                       \
    {                                                                                                                                                \
        ComponentScriptInterface::Vector2 Vec = { 0, 0 };                                                                                            \
                                                                                                                                                     \
        if (Component)                                                                                                                               \
        {                                                                                                                                            \
            csp::common::Vector2 Value = ((COMP*)Component)->Get##NAME();                                                                            \
                                                                                                                                                     \
            Vec[0] = Value.X;                                                                                                                        \
            Vec[1] = Value.Y;                                                                                                                        \
        }                                                                                                                                            \
                                                                                                                                                     \
        return Vec;                                                                                                                                  \
    }                                                                                                                                                \
                                                                                                                                                     \
    void COMP##ScriptInterface::Set##NAME(ComponentScriptInterface::Vector2 Vec)                                                                     \
    {                                                                                                                                                \
        csp::common::Vector2 Value(Vec[0], Vec[1]);                                                                                                  \
        ((COMP*)Component)->Set##NAME(Value);                                                                                                        \
                                                                                                                                                     \
        SendPropertyUpdate();                                                                                                                        \
    }

#define DEFINE_SCRIPT_PROPERTY_VEC3(COMP, NAME)                                                                                                      \
    ComponentScriptInterface::Vector3 COMP##ScriptInterface::Get##NAME() const                                                                       \
    {                                                                                                                                                \
        ComponentScriptInterface::Vector3 Vec = { 0, 0, 0 };                                                                                         \
                                                                                                                                                     \
        if (Component)                                                                                                                               \
        {                                                                                                                                            \
            csp::common::Vector3 Value = ((COMP*)Component)->Get##NAME();                                                                            \
                                                                                                                                                     \
            Vec[0] = Value.X;                                                                                                                        \
            Vec[1] = Value.Y;                                                                                                                        \
            Vec[2] = Value.Z;                                                                                                                        \
        }                                                                                                                                            \
                                                                                                                                                     \
        return Vec;                                                                                                                                  \
    }                                                                                                                                                \
                                                                                                                                                     \
    void COMP##ScriptInterface::Set##NAME(ComponentScriptInterface::Vector3 Vec)                                                                     \
    {                                                                                                                                                \
        csp::common::Vector3 Value(Vec[0], Vec[1], Vec[2]);                                                                                          \
        ((COMP*)Component)->Set##NAME(Value);                                                                                                        \
                                                                                                                                                     \
        SendPropertyUpdate();                                                                                                                        \
    }

#define DEFINE_SCRIPT_PROPERTY_VEC4(COMP, NAME)                                                                                                      \
    ComponentScriptInterface::Vector4 COMP##ScriptInterface::Get##NAME() const                                                                       \
    {                                                                                                                                                \
        ComponentScriptInterface::Vector4 Vec = { 0, 0, 0, 0 };                                                                                      \
                                                                                                                                                     \
        if (Component)                                                                                                                               \
        {                                                                                                                                            \
            csp::common::Vector4 Value = ((COMP*)Component)->Get##NAME();                                                                            \
                                                                                                                                                     \
            Vec[0] = Value.X;                                                                                                                        \
            Vec[1] = Value.Y;                                                                                                                        \
            Vec[2] = Value.Z;                                                                                                                        \
            Vec[3] = Value.W;                                                                                                                        \
        }                                                                                                                                            \
                                                                                                                                                     \
        return Vec;                                                                                                                                  \
    }                                                                                                                                                \
                                                                                                                                                     \
    void COMP##ScriptInterface::Set##NAME(ComponentScriptInterface::Vector4 Vec)                                                                     \
    {                                                                                                                                                \
        csp::common::Vector4 Value(Vec[0], Vec[1], Vec[2], Vec[3]);                                                                                  \
        ((COMP*)Component)->Set##NAME(Value);                                                                                                        \
                                                                                                                                                     \
        SendPropertyUpdate();                                                                                                                        \
    }
