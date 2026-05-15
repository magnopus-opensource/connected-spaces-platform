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
#include "Multiplayer/SignalR/ISignalRConnection.h"
#include "Multiplayer/SignalR/SignalRClient.h"

#include <iostream>
#include <limits>
#include <signalrclient/signalr_value.h>

namespace csp::multiplayer
{

constexpr const uint64_t ALL_CLIENTS_ID = std::numeric_limits<uint64_t>::max();

NetworkEventManagerImpl::NetworkEventManagerImpl(MultiplayerConnection* inMultiplayerConnection)
    : m_multiplayerConnectionInst(inMultiplayerConnection)
    , m_connection(nullptr)
{
}

void NetworkEventManagerImpl::SetConnection(csp::multiplayer::ISignalRConnection& inConnection) { m_connection = &inConnection; }

void NetworkEventManagerImpl::SendNetworkEvent(const csp::common::String& eventName,
    const csp::common::Array<csp::common::ReplicatedValue>& arguments, uint64_t targetClientId, ErrorCodeCallbackHandler callback)
{
    if (m_connection == nullptr)
    {
        if (callback)
        {
            callback(ErrorCode::NotConnected);
        }

        return;
    }

    csp::multiplayer::ISignalRConnection* iSignalRConnectionPtr = static_cast<csp::multiplayer::ISignalRConnection*>(m_connection);

    std::function<void(signalr::value, std::exception_ptr)> localCallback = [callback](signalr::value /*Result*/, std::exception_ptr except)
    {
        if (except != nullptr)
        {
            auto [Error, ExceptionErrorMsg] = MultiplayerConnection::ParseMultiplayerErrorFromExceptionPtr(except);
            if (callback)
            {
                callback(Error);
            }

            return;
        }

        if (callback)
        {
            callback(ErrorCode::None);
        }
    };

    std::map<uint64_t, signalr::value> components;

    for (size_t i = 0; i < arguments.Size(); ++i)
    {
        switch (arguments[i].GetReplicatedValueType())
        {
        case csp::common::ReplicatedValueType::Boolean:
        {
            std::vector<signalr::value> fields;
            fields.push_back(arguments[i].GetBool());

            std::vector<signalr::value> component { static_cast<uint64_t>(mcs::ItemComponentDataType::NULLABLE_BOOL), fields };
            components.insert({ i, component });

            break;
        }
        case csp::common::ReplicatedValueType::Integer:
        {
            std::vector<signalr::value> fields;
            fields.push_back(arguments[i].GetInt());

            std::vector<signalr::value> component { static_cast<uint64_t>(mcs::ItemComponentDataType::NULLABLE_INT64), fields };
            components.insert({ i, component });

            break;
        }
        case csp::common::ReplicatedValueType::Float:
        {
            std::vector<signalr::value> fields;
            fields.push_back(arguments[i].GetFloat());

            std::vector<signalr::value> component { static_cast<uint64_t>(mcs::ItemComponentDataType::NULLABLE_DOUBLE), fields };
            components.insert({ i, component });

            break;
        }
        case csp::common::ReplicatedValueType::String:
        {
            std::vector<signalr::value> fields;
            fields.push_back(arguments[i].GetString().c_str());

            std::vector<signalr::value> component { static_cast<uint64_t>(mcs::ItemComponentDataType::STRING), fields };
            components.insert({ i, component });

            break;
        }
        case csp::common::ReplicatedValueType::Vector2:
        {
            std::vector<signalr::value> fields;
            auto vector = arguments[i].GetVector2();
            fields.push_back(std::vector<signalr::value> { vector.X, vector.Y });

            std::vector<signalr::value> component { static_cast<uint64_t>(mcs::ItemComponentDataType::FLOAT_ARRAY), fields };
            components.insert({ i, component });

            break;
        }
        case csp::common::ReplicatedValueType::Vector3:
        {
            std::vector<signalr::value> fields;
            auto vector = arguments[i].GetVector3();
            fields.push_back(std::vector<signalr::value> { vector.X, vector.Y, vector.Z });

            std::vector<signalr::value> component { static_cast<uint64_t>(mcs::ItemComponentDataType::FLOAT_ARRAY), fields };
            components.insert({ i, component });

            break;
        }
        case csp::common::ReplicatedValueType::Vector4:
        {
            std::vector<signalr::value> fields;
            auto vector = arguments[i].GetVector4();
            fields.push_back(std::vector<signalr::value> { vector.X, vector.Y, vector.Z, vector.W });

            std::vector<signalr::value> component { static_cast<uint64_t>(mcs::ItemComponentDataType::FLOAT_ARRAY), fields };
            components.insert({ i, component });

            break;
        }
        default:
            assert(false && "Argument csp::common::ReplicatedValueType is unsupported.");
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
    std::vector<signalr::value> eventMessage { eventName.c_str(), (uint64_t)m_multiplayerConnectionInst->GetClientId(),
        (targetClientId == ALL_CLIENTS_ID) ? signalr::value_type::null : signalr::value(targetClientId), components };

    std::vector<signalr::value> invokeArguments;
    invokeArguments.push_back(eventMessage);

    iSignalRConnectionPtr->Invoke(
        m_multiplayerConnectionInst->GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_EVENT_MESSAGE), invokeArguments, localCallback);
}

} // namespace csp::multiplayer
