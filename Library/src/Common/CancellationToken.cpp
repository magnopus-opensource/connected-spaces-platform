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

#include "CSP/Common/CancellationToken.h"

#include <atomic>

namespace csp::common
{

class CancellationToken::Impl
{
public:
    Impl()
        : m_isCancelled(false)
    {
    }

    ~Impl() = default;

    void Cancel() { m_isCancelled = true; }

    bool Cancelled() const { return m_isCancelled; }

private:
    std::atomic_bool m_isCancelled;
};

CancellationToken::CancellationToken()
    : m_implPtr(new Impl())
{
}

CancellationToken::~CancellationToken() { delete (m_implPtr); }

void CancellationToken::Cancel() { m_implPtr->Cancel(); }

bool CancellationToken::Cancelled() const { return m_implPtr->Cancelled(); }

CancellationToken& CancellationToken::Dummy()
{
    static CancellationToken token;
    return token;
}

} // namespace csp::common
