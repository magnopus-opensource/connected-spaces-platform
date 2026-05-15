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
#include "Common/Web/HttpAuth.h"

#include <assert.h>

namespace csp::web
{

// @RossB: TODO: Temporarily hanged from static to pointers due to iOS runtime issues
HttpAuth::AccessToken* HttpAuth::Token = nullptr;
HttpAuth::AccessToken* HttpAuth::RefreshToken = nullptr;
csp::common::String* HttpAuth::TokenExpiry = nullptr;
csp::common::String* HttpAuth::RefreshTokenExpiry = nullptr;

HttpAuth::HttpAuth() { }

void HttpAuth::SetAccessToken(const AccessToken& inToken, const csp::common::String& inTokenExpiry, const AccessToken& inRefreshToken,
    const csp::common::String& inRefreshTokenExpiry)
{
    if (Token == nullptr)
    {
        Token = new HttpAuth::AccessToken();
    }

    if (RefreshToken == nullptr)
    {
        RefreshToken = new HttpAuth::AccessToken();
    }

    if (TokenExpiry == nullptr)
    {
        TokenExpiry = new csp::common::String();
    }

    if (RefreshTokenExpiry == nullptr)
    {
        RefreshTokenExpiry = new csp::common::String();
    }

    // @Todo Need to be cautious about multi-threading here
    *Token = inToken;
    *RefreshToken = inRefreshToken;
    *TokenExpiry = inTokenExpiry;
    *RefreshTokenExpiry = inRefreshTokenExpiry;
}

const HttpAuth::AccessToken& HttpAuth::GetAccessToken()
{
    if (Token == nullptr)
    {
        Token = new HttpAuth::AccessToken();
    }

    return *Token;
}

const csp::common::String& HttpAuth::GetTokenExpiry()
{
    if (TokenExpiry == nullptr)
    {
        TokenExpiry = new csp::common::String();
    }

    return *TokenExpiry;
}

const csp::common::String& HttpAuth::GetRefreshTokenExpiry()
{
    if (RefreshTokenExpiry == nullptr)
    {
        RefreshTokenExpiry = new csp::common::String();
    }

    return *RefreshTokenExpiry;
}

const HttpAuth::AccessToken& HttpAuth::GetRefreshToken()
{
    if (RefreshToken == nullptr)
    {
        RefreshToken = new HttpAuth::AccessToken();
    }

    return *RefreshToken;
}

bool HttpAuth::HasTokenExpired()
{
    csp::common::DateTime expiry(*TokenExpiry);
    bool expired = expiry >= csp::common::DateTime::TimeNow();
    return expired;
}

} // namespace csp::web
