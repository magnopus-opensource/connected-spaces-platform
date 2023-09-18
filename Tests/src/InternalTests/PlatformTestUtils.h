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

#include "../PublicAPITests/UserSystemTestHelpers.h"
#include "Multiplayer/WebSocketClient.h"
#include "Web/WebClient.h"

csp::multiplayer::IWebSocketClient* WebSocketStart(const csp::common::String& Uri);

void WebSocketStop(csp::multiplayer::IWebSocketClient* WebSocketClient);
void WebSocketSend(csp::multiplayer::IWebSocketClient* WebSocketClient, const csp::common::String& Data);
void WebSocketSendReceive(csp::multiplayer::IWebSocketClient* WebSocketClient);

void InitialiseFoundation();