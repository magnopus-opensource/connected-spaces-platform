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

#include "CSP/Common/String.h"
#include "Web/Json.h"

#include <map>
#include <memory>
#include <vector>

namespace csp::services
{

namespace utility
{
    using string_t = csp::common::String;
    using datetime = csp::common::String;
} // namespace utility

/// @brief Abstract base class for Dto objects
///
/// Data Transfer objects represent the data being send with each web service call
/// and this base class defines the functions that convert this data to and from
/// Json to be sent or recieved as http content
class DtoBase
{
public:
    DtoBase() { }
    virtual ~DtoBase() { }

    virtual utility::string_t ToJson() const;
    virtual void FromJson(const utility::string_t& Json) { }
};

class EnumBase
{
public:
    EnumBase() { }
    virtual ~EnumBase() { }

    virtual utility::string_t ToJson() const;
    virtual void FromJson(const utility::string_t& Json) { }
};

/// @brief Null Dto object
///
/// Used in templated functions when we don't need a Dto object for a CHS method
/// or the method does not return and content
class NullDto : public DtoBase
{
public:
    NullDto() { }
    virtual ~NullDto() { }

    utility::string_t ToJson() const override;
    void FromJson(const utility::string_t& Json) override { }
};

} // namespace csp::services
