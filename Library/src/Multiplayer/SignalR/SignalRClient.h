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
#include "Web/WebClient.h"

#include <atomic>
#include <signalrclient/http_client.h>
#include <signalrclient/signalr_client_config.h>
#include <signalrclient/websocket_client.h>

namespace csp::systems
{

class LoginState;
class UserSystem;

}

namespace csp::multiplayer
{

class CSPWebsocketClient : public signalr::websocket_client
{
public:
    CSPWebsocketClient() noexcept;

    void start(const std::string& url, std::function<void(std::exception_ptr)> callback) override;
    void stop(std::function<void(std::exception_ptr)> callback) override;
    void send(const std::string& payload, signalr::transfer_format format, std::function<void(std::exception_ptr)> callback) override;
    void receive(std::function<void(const std::string&, std::exception_ptr)> callback) override;

private:
    csp::systems::UserSystem* UserSystem;
    const csp::systems::LoginState* LoginState;
    std::atomic_bool RefreshInitialised;
};

class CSPHttpClient : public signalr::http_client
{
public:
    CSPHttpClient();

    void send(const std::string& url, const signalr::http_request& request,
        std::function<void(const signalr::http_response&, std::exception_ptr)> callback) override;

private:
    csp::web::WebClient* WebClientHttps;
};

} // namespace csp::multiplayer
