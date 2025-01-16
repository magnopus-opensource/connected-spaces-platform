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
#include "Common/DateTime.h"

namespace csp::web
{

class HttpAuth
{
public:
    HttpAuth();

    using AccessToken = csp::common::String;

    static void SetAccessToken(const AccessToken& InToken, const csp::common::String& InTokenExpiry, const AccessToken& InRefreshToken,
        const csp::common::String& InRefreshTokenExpiry);

    static const AccessToken& GetAccessToken();
    static const AccessToken& GetRefreshToken();
    static const csp::common::String& GetTokenExpiry();
    static const csp::common::String& GetRefreshTokenExpiry();

    static bool HasTokenExpired();

private:
    // @RossB: TODO: Temporarily changed from statics to pointers due to iOS runtime issues
    static AccessToken* Token;
    static AccessToken* RefreshToken;
    static csp::common::String* TokenExpiry;
    static csp::common::String* RefreshTokenExpiry;
};

} // namespace csp::web
