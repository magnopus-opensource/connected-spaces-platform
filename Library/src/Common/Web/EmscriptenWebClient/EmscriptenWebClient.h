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

#include "Common/Web/WebClient.h"

namespace csp::common
{
class LogSystem;
}
namespace csp::web
{

class EmscriptenWebClient : public WebClient
{
public:
    virtual ~EmscriptenWebClient() {};

    std::string MD5Hash(const void* Data, const size_t Size) override;
    void SetFileUploadContentFromFile(HttpPayload* Payload, const char* FilePath, const char* Version, const csp::common::String& MediaType) override;
    void SetFileUploadContentFromString(HttpPayload* Payload, const csp::common::String& StringSource, const csp::common::String& FileName,
        const char* Version, const csp::common::String& MediaType) override;
    void SetFileUploadContentFromBuffer(HttpPayload* Payload, const char* Buffer, size_t BufferLength, const csp::common::String& FileName,
        const char* Version, const csp::common::String& MediaType) override;

    // Instances of EmscriptenWebClient should not be created. You should instead rely on the instance that `csp::systems::SystemsManager` holds.
    EmscriptenWebClient(const Port InPort, const ETransferProtocol Tp, csp::common::IAuthContext& AuthContext, csp::common::LogSystem* LogSystem, bool AutoRefresh = true);
    EmscriptenWebClient(const Port InPort, const ETransferProtocol Tp, csp::common::LogSystem* LogSystem, bool AutoRefresh = true);

private:
    void Send(HttpRequest& Request) override;
};

} // namespace csp::web
