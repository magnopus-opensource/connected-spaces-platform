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
#include "Multiplayer/MCS/MCSTypes.h"
#include "Multiplayer/MultiplayerConstants.h"
#include "Multiplayer/SignalR/SignalRClient.h"
#include <Multiplayer/SignalR/ISignalRConnection.h>

#include <iostream>
#include <limits>
#include <signalrclient/signalr_value.h>

namespace csp::multiplayer
{

constexpr const uint64_t ALL_CLIENTS_ID = std::numeric_limits<uint64_t>::max();

NetworkEventManagerImpl::NetworkEventManagerImpl(MultiplayerConnection* InMultiplayerConnection)
    : MultiplayerConnectionInst(InMultiplayerConnection)
    , Connection(nullptr)
{
}

void NetworkEventManagerImpl::SetConnection(csp::multiplayer::ISignalRConnection* InConnection) { Connection = InConnection; }

void NetworkEventManagerImpl::SendNetworkEvent(const csp::common::String& EventName,
    const csp::common::Array<csp::common::ReplicatedValue>& Arguments, uint64_t TargetClientId, ErrorCodeCallbackHandler Callback)
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

    std::map<uint64_t, signalr::value> Components;

    for (size_t i = 0; i < Arguments.Size(); ++i)
    {
        switch (Arguments[i].GetReplicatedValueType())
        {
        case csp::common::ReplicatedValueType::Boolean:
        {
            std::vector<signalr::value> Fields;
            Fields.push_back(Arguments[i].GetBool());

            std::vector<signalr::value> Component { static_cast<uint64_t>(mcs::ItemComponentDataType::NULLABLE_BOOL), Fields };
            Components.insert({ i, Component });

            break;
        }
        case csp::common::ReplicatedValueType::Integer:
        {
            std::vector<signalr::value> Fields;
            Fields.push_back(Arguments[i].GetInt());

            std::vector<signalr::value> Component { static_cast<uint64_t>(mcs::ItemComponentDataType::NULLABLE_INT64), Fields };
            Components.insert({ i, Component });

            break;
        }
        case csp::common::ReplicatedValueType::Float:
        {
            std::vector<signalr::value> Fields;
            Fields.push_back(Arguments[i].GetFloat());

            std::vector<signalr::value> Component { static_cast<uint64_t>(mcs::ItemComponentDataType::NULLABLE_DOUBLE), Fields };
            Components.insert({ i, Component });

            break;
        }
        case csp::common::ReplicatedValueType::String:
        {
            std::vector<signalr::value> Fields;
            Fields.push_back(Arguments[i].GetString().c_str());

            std::vector<signalr::value> Component { static_cast<uint64_t>(mcs::ItemComponentDataType::STRING), Fields };
            Components.insert({ i, Component });

            break;
        }
        case csp::common::ReplicatedValueType::Vector2:
        {
            std::vector<signalr::value> Fields;
            auto Vector = Arguments[i].GetVector2();
            Fields.push_back(std::vector<signalr::value> { Vector.X, Vector.Y });

            std::vector<signalr::value> Component { static_cast<uint64_t>(mcs::ItemComponentDataType::FLOAT_ARRAY), Fields };
            Components.insert({ i, Component });

            break;
        }
        case csp::common::ReplicatedValueType::Vector3:
        {
            std::vector<signalr::value> Fields;
            auto Vector = Arguments[i].GetVector3();
            Fields.push_back(std::vector<signalr::value> { Vector.X, Vector.Y, Vector.Z });

            std::vector<signalr::value> Component { static_cast<uint64_t>(mcs::ItemComponentDataType::FLOAT_ARRAY), Fields };
            Components.insert({ i, Component });

            break;
        }
        case csp::common::ReplicatedValueType::Vector4:
        {
            std::vector<signalr::value> Fields;
            auto Vector = Arguments[i].GetVector4();
            Fields.push_back(std::vector<signalr::value> { Vector.X, Vector.Y, Vector.Z, Vector.W });

            std::vector<signalr::value> Component { static_cast<uint64_t>(mcs::ItemComponentDataType::FLOAT_ARRAY), Fields };
            Components.insert({ i, Component });

            break;
        }
        default:
            assert(false && "Argument ReplicatedValueType is unsupported.");
            break;
        }
    }

    /*
     * class EventMessage
     * [0] string EventType
     * [1] uint SenderClientId
     * [2] uint? RecipientClientId
     * [3] map<uint, vec> Components
     */
    std::vector<signalr::value> EventMessage { EventName.c_str(), (uint64_t)MultiplayerConnectionInst->GetClientId(),
        (TargetClientId == ALL_CLIENTS_ID) ? signalr::value_type::null : signalr::value(TargetClientId), Components };

    std::vector<signalr::value> InvokeArguments;
    InvokeArguments.push_back(EventMessage);

    ISignalRConnectionPtr->Invoke(
        MultiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_EVENT_MESSAGE), InvokeArguments, LocalCallback);
}

} // namespace csp::multiplayer
