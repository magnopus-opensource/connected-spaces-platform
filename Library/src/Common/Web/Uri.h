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
    explicit Uri(const char* inUri);
    Uri(csp::common::String& inUri);

    const char* GetAsString() const;
    const std::string GetAsStdString() const;

    void SetWithParams(const char* inUri, std::initializer_list<csp::common::String> params);

    template <class T> void AddQueryParams(const char* paramName, T param);

    static csp::common::String Encode(const csp::common::String& inUri);

private:
    csp::common::String m_uriPath;
    uint32_t m_numParams;
};

template <class T>
[[deprecated("Unsupported type for URI param, please add support for it")]] inline void Uri::AddQueryParams(const char* /*ParamName*/, T /*Param*/)
{
    assert(false && "Unknown param type in Api binding");
}

template <> inline void Uri::AddQueryParams(const char* paramName, csp::common::String param)
{
    if (param.Length() > 0)
    {
        std::string path = m_uriPath.c_str();

        const std::string separator = (m_numParams == 0) ? std::string("?") : std::string("&");
        path = path + separator + std::string(paramName) + std::string("=") + std::string(Encode(param));
        ++m_numParams;

        m_uriPath = csp::common::String(path.c_str());
    }
}

template <> inline void Uri::AddQueryParams(const char* paramName, std::vector<csp::common::String> param)
{
    if (param.size() == 0)
    {
        return;
    }

    std::string path = m_uriPath.c_str();

    for (size_t i = 0; i < param.size(); ++i)
    {
        std::string separator = (m_numParams == 0) ? std::string("?") : std::string("&");
        path = path + separator + std::string(paramName) + std::string("=") + std::string(Encode(param[i]));
        ++m_numParams;
    }

    m_uriPath = csp::common::String(path.c_str());
}

template <> inline void Uri::AddQueryParams(const char* paramName, double param)
{
    std::string path = m_uriPath.c_str();

    std::string separator = (m_numParams == 0) ? std::string("?") : std::string("&");
    path = path + separator + std::string(paramName) + std::string("=") + std::to_string(param);
    ++m_numParams;

    m_uriPath = csp::common::String(path.c_str());
}

template <> inline void Uri::AddQueryParams(const char* paramName, int32_t param)
{
    std::string path = m_uriPath.c_str();

    std::string separator = (m_numParams == 0) ? std::string("?") : std::string("&");
    path = path + separator + std::string(paramName) + std::string("=") + std::to_string(param);
    ++m_numParams;

    m_uriPath = csp::common::String(path.c_str());
}

template <> inline void Uri::AddQueryParams(const char* paramName, bool param)
{
    std::string path = m_uriPath.c_str();

    std::string separator = (m_numParams == 0) ? std::string("?") : std::string("&");
    path = path + separator + std::string(paramName) + std::string("=") + (param ? "true" : "false");
    ++m_numParams;

    m_uriPath = csp::common::String(path.c_str());
}

} // namespace csp::web
