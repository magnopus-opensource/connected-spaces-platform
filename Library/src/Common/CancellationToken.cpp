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

#include "Memory/Memory.h"

#include <atomic>

namespace csp::common
{

class CancellationToken::Impl
{
public:
    Impl()
        : IsCancelled(false)
    {
    }

    ~Impl() = default;

    void Cancel() { IsCancelled = true; }

    bool Cancelled() const { return IsCancelled; }

private:
    std::atomic_bool IsCancelled;
};

CancellationToken::CancellationToken()
    : ImplPtr(CSP_NEW Impl())
{
}

CancellationToken::~CancellationToken() { CSP_DELETE(ImplPtr); }

void CancellationToken::Cancel() { ImplPtr->Cancel(); }

bool CancellationToken::Cancelled() const { return ImplPtr->Cancelled(); }

CancellationToken& CancellationToken::Dummy()
{
    static CancellationToken Token;
    return Token;
}

} // namespace csp::common
