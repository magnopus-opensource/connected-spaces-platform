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

#include "CSP/CSPCommon.h"

namespace csp::common
{

/// @brief Used with Http requests to managed cancellation states atomically.
/// Allows a request to be cancelled within the web client if connection is shutdown.
class CSP_API CancellationToken
{
public:
    /// Constructs a CancellationToken.
    CancellationToken();
    /// @brief Copy constructor.
    /// @param rhs const CancellationToken&
    CSP_NO_EXPORT CancellationToken(const CancellationToken& rhs) = delete;
    /// @brief Move constructor.
    /// @param rhs CancellationToken&&
    CSP_NO_EXPORT CancellationToken(CancellationToken&& rhs) = delete;

    /// @brief Destructor.
    ~CancellationToken();

    /// @brief Copy assignment.
    /// @param rhs const CancellationToken&
    /// @return CancellationToken&
    CancellationToken& operator=(const CancellationToken& rhs) = delete;

    /// @brief Move assignment.
    /// @param rhs CancellationToken&&
    /// @return CancellationToken&
    CancellationToken& operator=(CancellationToken&& rhs) = delete;

    /// @brief Sets the cancellation state to cancelled.
    // This will stop the request being sent by the web client.
    void Cancel();

    /// @brief Check if a request has been cancelled.
    /// @return bool
    bool Cancelled() const;

    /// @brief Constructs a blank token.
    /// @return CancellationToken&
    static CancellationToken& Dummy();

private:
    class Impl;
    Impl* ImplPtr;
};

} // namespace csp::common
