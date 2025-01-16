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

#include <cassert>
#include <regex>
#include <string>
#include <vector>

namespace csp::web
{

class Uri
{
public:
    Uri();
    explicit Uri(const char* InUri);
    Uri(csp::common::String& InUri);

    const char* GetAsString() const;
    const std::string GetAsStdString() const;

    void SetWithParams(const char* InUri, std::initializer_list<csp::common::String> Params);

    template <class T> void AddQueryParams(const char* ParamName, T Param);

    static csp::common::String Encode(const csp::common::String& InUri);

private:
    csp::common::String UriPath;
    uint32_t NumParams;
};

template <class T>
[[deprecated("Unsupported type for URI param, please add support for it")]] inline void Uri::AddQueryParams(const char* ParamName, T Param)
{
    assert(false && "Unknown param type in Api binding");
}

template <> inline void Uri::AddQueryParams(const char* ParamName, csp::common::String Param)
{
    if (Param.Length() > 0)
    {
        std::string Path = UriPath.c_str();

        const std::string Separator = (NumParams == 0) ? std::string("?") : std::string("&");
        Path = Path + Separator + std::string(ParamName) + std::string("=") + std::string(Encode(Param));
        ++NumParams;

        UriPath = csp::common::String(Path.c_str());
    }
}

template <> inline void Uri::AddQueryParams(const char* ParamName, std::vector<csp::common::String> Param)
{
    if (Param.size() == 0)
    {
        return;
    }

    std::string Path = UriPath.c_str();

    for (size_t i = 0; i < Param.size(); ++i)
    {
        std::string Separator = (NumParams == 0) ? std::string("?") : std::string("&");
        Path = Path + Separator + std::string(ParamName) + std::string("=") + std::string(Encode(Param[i]));
        ++NumParams;
    }

    UriPath = csp::common::String(Path.c_str());
}

template <> inline void Uri::AddQueryParams(const char* ParamName, double Param)
{
    std::string Path = UriPath.c_str();

    std::string Separator = (NumParams == 0) ? std::string("?") : std::string("&");
    Path = Path + Separator + std::string(ParamName) + std::string("=") + std::to_string(Param);
    ++NumParams;

    UriPath = csp::common::String(Path.c_str());
}

template <> inline void Uri::AddQueryParams(const char* ParamName, int32_t Param)
{
    std::string Path = UriPath.c_str();

    std::string Separator = (NumParams == 0) ? std::string("?") : std::string("&");
    Path = Path + Separator + std::string(ParamName) + std::string("=") + std::to_string(Param);
    ++NumParams;

    UriPath = csp::common::String(Path.c_str());
}

template <> inline void Uri::AddQueryParams(const char* ParamName, bool Param)
{
    std::string Path = UriPath.c_str();

    std::string Separator = (NumParams == 0) ? std::string("?") : std::string("&");
    Path = Path + Separator + std::string(ParamName) + std::string("=") + (Param ? "true" : "false");
    ++NumParams;

    UriPath = csp::common::String(Path.c_str());
}

} // namespace csp::web
