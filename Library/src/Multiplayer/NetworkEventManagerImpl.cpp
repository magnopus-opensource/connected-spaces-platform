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

#include "NetworkEventManagerImpl.h"

#include "CSP/Common/ReplicatedValue.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "MCS/MCSTypes.h"
#include "Multiplayer/MCSComponentPacker.h"
#include "Multiplayer/MultiplayerConstants.h"
#include "Multiplayer/SignalR/ISignalRConnection.h"
#include "Multiplayer/SignalR/SignalRClient.h"

#include <iostream>
#include <limits>
#include <signalrclient/signalr_value.h>

namespace csp::multiplayer
{

constexpr const uint32_t ALL_CLIENTS_ID = std::numeric_limits<uint32_t>::max();

NetworkEventManagerImpl::NetworkEventManagerImpl(MultiplayerConnection* InMultiplayerConnection)
    : MultiplayerConnectionInst(InMultiplayerConnection)
    , Connection(nullptr)
{
}

void NetworkEventManagerImpl::SetConnection(csp::multiplayer::ISignalRConnection& InConnection) { Connection = &InConnection; }

void NetworkEventManagerImpl::SendNetworkEvent(const csp::common::String& EventName,
    const csp::common::Array<csp::common::ReplicatedValue>& Arguments, uint32_t TargetClientId, ErrorCodeCallbackHandler Callback)
{
    if (Connection == nullptr)
    {
        if (Callback)
        {
            Callback(ErrorCode::NotConnected);
        }

        return;
    }

    csp::multiplayer::ISignalRConnection* ISignalRConnectionPtr = static_cast<csp::multiplayer::ISignalRConnection*>(Connection);

    std::function<void(signalr::value, std::exception_ptr)> LocalCallback = [Callback](signalr::value /*Result*/, std::exception_ptr Except)
    {
        if (Except != nullptr)
        {
            auto [Error, ExceptionErrorMsg] = MultiplayerConnection::ParseMultiplayerErrorFromExceptionPtr(Except);
            if (Callback)
            {
                Callback(Error);
            }

            return;
        }

        if (Callback)
        {
            Callback(ErrorCode::None);
        }
    };

    MCSComponentPacker ComponentPacker;

    for (size_t i = 0; i < Arguments.Size(); ++i)
    {
        ComponentPacker.WriteValue(static_cast<uint16_t>(i), Arguments[i]);
    }

    std::optional<uint32_t> EventTargetClientId;

    if ((TargetClientId != ALL_CLIENTS_ID))
    {
        EventTargetClientId = TargetClientId;
    }

    mcs::EventMessage Event { EventName.c_str(), MultiplayerConnectionInst->GetClientId(), EventTargetClientId,
        ComponentPacker.GetComponents() };

    SignalRSerializer Serializer;
    Event.Serialize(Serializer);

    std::vector<signalr::value> InvokeArguments;
    InvokeArguments.push_back(Serializer.Get());

    ISignalRConnectionPtr->Invoke(
        MultiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_EVENT_MESSAGE), InvokeArguments, LocalCallback);
}

} // namespace csp::multiplayer
