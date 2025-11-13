#include "ScopeLeadershipManager.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "Multiplayer/SignalR/SignalRConnection.h"

#include <fmt/format.h>
#include <signalrclient/signalr_value.h>

namespace csp::multiplayer
{
ScopeLeadershipManager::ScopeLeadershipManager(MultiplayerConnection& Connection, csp::common::LogSystem& LogSystem)
    : Connection { Connection }
    , LogSystem { LogSystem }
{
}

void ScopeLeadershipManager::RegisterScope(const std::string& ScopeId, const std::optional<uint64_t>& LeaderId)
{
    if (LeaderId.has_value())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Log,
            fmt::format("ScopeLeadershipManager::RegisterScope Called for scope {0} with leader: {1}.", ScopeId, *LeaderId).c_str());

        Scopes[ScopeId] = ScopeLeaderData {};
        Scopes[ScopeId]->LeaderClientId = *LeaderId;
    }
    else
    {
        LogSystem.LogMsg(
            csp::common::LogLevel::Log, fmt::format("ScopeLeadershipManager::RegisterScope Called for scope {0} with no leader.", ScopeId).c_str());

        Scopes[ScopeId] = std::nullopt;
    }
}

void ScopeLeadershipManager::DeregisterScope(const std::string& ScopeId) { Scopes.erase(ScopeId); }

void ScopeLeadershipManager::OnElectedScopeLeader(const std::string& ScopeId, uint64_t ClientId)
{
    auto ScopeIt = Scopes.find(ScopeId);

    if (ScopeIt == Scopes.end())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("ScopeLeadershipManager::OnElectedScopeLeader Event called for scope: {0} that isn't registered, for new "
                        "leader: {1}.",
                ScopeId, ClientId)
                .c_str());
        return;
    }

    std::optional<ScopeLeaderData>& Data = ScopeIt->second;

    if (Data.has_value())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning,
            fmt::format(
                "ScopeLeadershipManager::OnElectedScopeLeader Event called for scope: {0}, that already has the leader: {1}, for new leader: {2}.",
                ScopeId, Data->LeaderClientId, ClientId)
                .c_str());
    }

    LogSystem.LogMsg(csp::common::LogLevel::Log,
        fmt::format("ScopeLeadershipManager::OnElectedScopeLeader New leader: {0}, for scope: {1}.", ClientId, ScopeId).c_str());

    Scopes[ScopeId] = { ClientId, std::chrono::steady_clock::time_point {} };
}

void ScopeLeadershipManager::OnVacatedAsScopeLeader(const std::string& ScopeId)
{
    auto ScopeIt = Scopes.find(ScopeId);

    if (ScopeIt == Scopes.end())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for scope: {0} that isn't registered.", ScopeId).c_str());
        return;
    }

    std::optional<ScopeLeaderData>& Data = ScopeIt->second;

    if (Data.has_value())
    {
        // Leader has been vacated, so null the data.
        Data = std::nullopt;
    }
    else
    {
        LogSystem.LogMsg(csp::common::LogLevel::Warning,
            fmt::format("ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for the scope: {0} that doesn't have a leader.", ScopeId)
                .c_str());
    }

    LogSystem.LogMsg(
        csp::common::LogLevel::Log, fmt::format("ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for scope: {}.", ScopeId).c_str());
}

void ScopeLeadershipManager::Update()
{
    const auto CurrentTime = std::chrono::steady_clock::now();

    for (auto& LeaderDataPair : Scopes)
    {
        const std::string& ScopeId = LeaderDataPair.first;
        std::optional<ScopeLeaderData>& LeaderData = LeaderDataPair.second;

        // We should only send a heartbeat if the local client is the leader of the scope.
        if (IsLocalClientLeader(ScopeId))
        {
            // Ensure the correct amount of time has passed.
            if (CurrentTime - LeaderData->LastHeartbeatTime > LeaderElectionHeartbeatInterval)
            {
                // Send heartbeat.
                LeaderData->LastHeartbeatTime = CurrentTime;
                SendLeaderHeartbeat(ScopeId);
            }
        }
    }
}

std::optional<uint64_t> ScopeLeadershipManager::GetLeaderClientId(const std::string& ScopeId) const
{
    auto ScopeIt = Scopes.find(ScopeId);

    if (ScopeIt == Scopes.end())
    {
        LogSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("ScopeLeadershipManager::GetLeaderClientId Event called for the scope: {} that isn't registered.", ScopeId).c_str());
        return std::nullopt;
    }

    const std::optional<ScopeLeaderData>& LeaderData = ScopeIt->second;

    if (ScopeIt->second.has_value() == false)
    {
        // Scope doesn't have a leader.
        return std::nullopt;
    }

    return LeaderData->LeaderClientId;
}

bool ScopeLeadershipManager::IsLocalClientLeader(const std::string& ScopeId) const
{
    auto LeaderClientId = GetLeaderClientId(ScopeId);

    if (LeaderClientId.has_value() == false)
    {
        return false;
    }

    return *LeaderClientId == Connection.GetClientId();
}

void ScopeLeadershipManager::SendLeaderHeartbeat(const std::string& ScopeId)
{
    const signalr::value Params { std::vector { signalr::value { ScopeId } } };

    Connection.GetSignalRConnection()->Invoke(Connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_SCOPE_LEADER_HEARTBEAT), Params,
        [this, ScopeId](signalr::value, std::exception_ptr Exception)
        {
            if (Exception)
            {
                // An exception was thrown. In this case, we just log an error to notify clients.
                // There isn't anything else we can do, as this all happens server-side.
                try
                {
                    std::rethrow_exception(Exception);
                }
                catch (const std::exception& Exception)
                {
                    LogSystem.LogMsg(csp::common::LogLevel::Error,
                        fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Failed to send heartbeat for scope: {0} with error: {1}", ScopeId,
                            Exception.what())
                            .c_str());
                }
                catch (...)
                {
                    LogSystem.LogMsg(csp::common::LogLevel::Error,
                        fmt::format(
                            "ScopeLeadershipManager::SendLeaderHeartbeat Failed to send heartbeat for scope: {} with an unknown error.", ScopeId)
                            .c_str());
                }
            }
            else
            {
                // Successfuly sent the heartbeat.
                LogSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
                    fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Heartbeat was successfuly sent for scope: {}", ScopeId).c_str());
            }
        });
}
}
