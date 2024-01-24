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
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"

#include <functional>


namespace csp::multiplayer
{

class MultiplayerConnection;
class ReplicatedValue;
class SignalRConnection;
enum class ErrorCode;


class NetworkEventManagerImpl
{
public:
	NetworkEventManagerImpl(MultiplayerConnection* InMultiplayerConnection);

	typedef std::function<void(ErrorCode)> ErrorCodeCallbackHandler;

	void SetConnection(csp::multiplayer::SignalRConnection* InConnection);

	CSP_NO_EXPORT void SendNetworkEvent(const csp::common::String& EventName,
										const csp::common::Array<ReplicatedValue>& Arguments,
										uint64_t TargetClientId,
										ErrorCodeCallbackHandler Callback);

private:
	MultiplayerConnection* MultiplayerConnectionInst;
	csp::multiplayer::SignalRConnection* Connection;
};

} // namespace csp::multiplayer
