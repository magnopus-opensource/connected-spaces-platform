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
#pragma once

#include "ReplicatedValue.h"

#include <stdexcept>
#include <fmt/format.h>

namespace csp::common
{
inline csp::common::String ReplicatedValueTypeToString(csp::common::ReplicatedValueType Type)
{
    switch (Type)
    {
    case csp::common::ReplicatedValueType::InvalidType:
        return "InvalidType";
    case csp::common::ReplicatedValueType::Boolean:
        return "Boolean";
    case csp::common::ReplicatedValueType::Integer:
        return "Integer";
    case csp::common::ReplicatedValueType::Float:
        return "Float";
    case csp::common::ReplicatedValueType::String:
        return "String";
    case csp::common::ReplicatedValueType::Vector3:
        return "Vector3";
    case csp::common::ReplicatedValueType::Vector4:
        return "Vector4";
    case csp::common::ReplicatedValueType::Vector2:
        return "Vector2";
    case csp::common::ReplicatedValueType::StringMap:
        return "StringMap";
    default:
        return "UnknownType";
    }
}

class ReplicatedValueException : public std::runtime_error
{
public:
    ReplicatedValueException(ReplicatedValueType Expected, ReplicatedValueType Actual)
        : runtime_error("ReplicatedValue type mismatch")
        , ExpectedType(Expected)
        , ActualType(Actual)
    {
        ExceptionMsg = fmt::format(
            "Expected - {} but found {}.", ReplicatedValueTypeToString(ExpectedType).c_str(), ReplicatedValueTypeToString(ActualType).c_str());
    }

    const char* what() const noexcept override { return ExceptionMsg.c_str(); }

    ReplicatedValueType GetExpectedType() const { return ExpectedType; }
    ReplicatedValueType GetActualType() const { return ActualType; }

private:
    ReplicatedValueType ExpectedType;
    ReplicatedValueType ActualType;
    std::string ExceptionMsg;
};
}

