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

#include <atomic>

namespace csp::web
{

/// Upload and download progress for large files
class HttpProgress
{
public:
    HttpProgress();
    ~HttpProgress();

    void SetProgressPercentage(float Progress);
    float GetProgressPercentage() const;

    // Explicit assignment operator because of atomic member
    HttpProgress& operator=(const HttpProgress& other) noexcept
    {
        Progress = other.Progress.load();
        return *this;
    }

private:
    std::atomic<uint32_t> Progress;
};

} // namespace csp::web
