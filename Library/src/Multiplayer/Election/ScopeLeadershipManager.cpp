#include "ScopeLeadershipManager.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "Multiplayer/SignalR/SignalRConnection.h"

#include <fmt/format.h>
#include <signalrclient/signalr_value.h>

namespace csp::multiplayer
{
ScopeLeadershipManager::ScopeLeadershipManager(MultiplayerConnection& connection, csp::common::LogSystem& logSystem)
    : m_connection { connection }
    , m_logSystem { logSystem }
{
}

void ScopeLeadershipManager::RegisterScope(const std::string& scopeId, const std::optional<uint64_t>& leaderId)
{
    if (leaderId.has_value())
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Log,
            fmt::format("ScopeLeadershipManager::RegisterScope Called for scope {0} with leader: {1}.", scopeId, *leaderId).c_str());

        m_scopes[scopeId] = ScopeLeaderData {};
        m_scopes[scopeId]->LeaderClientId = *leaderId;
    }
    else
    {
        m_logSystem.LogMsg(
            csp::common::LogLevel::Log, fmt::format("ScopeLeadershipManager::RegisterScope Called for scope {0} with no leader.", scopeId).c_str());

        m_scopes[scopeId] = std::nullopt;
    }
}

void ScopeLeadershipManager::DeregisterScope(const std::string& scopeId) { m_scopes.erase(scopeId); }

void ScopeLeadershipManager::OnElectedScopeLeader(const std::string& scopeId, uint64_t clientId)
{
    auto scopeIt = m_scopes.find(scopeId);

    if (scopeIt == m_scopes.end())
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("ScopeLeadershipManager::OnElectedScopeLeader Event called for scope: {0} that isn't registered, for new "
                        "leader: {1}.",
                scopeId, clientId)
                .c_str());
        return;
    }

    std::optional<ScopeLeaderData>& data = scopeIt->second;

    if (data.has_value())
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Warning,
            fmt::format("ScopeLeadershipManager::OnElectedScopeLeader Event called for scope: {0}, that already has the leader: {1}, for new leader: "
                        "{2}. Overwriting old value.",
                scopeId, data->LeaderClientId, clientId)
                .c_str());
    }

    m_logSystem.LogMsg(csp::common::LogLevel::Log,
        fmt::format("ScopeLeadershipManager::OnElectedScopeLeader New leader: {0}, for scope: {1}.", clientId, scopeId).c_str());

    m_scopes[scopeId] = { clientId, std::chrono::steady_clock::time_point {} };
}

void ScopeLeadershipManager::OnVacatedAsScopeLeader(const std::string& scopeId)
{
    auto scopeIt = m_scopes.find(scopeId);

    if (scopeIt == m_scopes.end())
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for scope: {0} that isn't registered.", scopeId).c_str());
        return;
    }

    std::optional<ScopeLeaderData>& data = scopeIt->second;

    if (data.has_value())
    {
        // Leader has been vacated, so null the data.
        data = std::nullopt;
    }
    else
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Warning,
            fmt::format("ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for the scope: {0} that doesn't have a leader.", scopeId)
                .c_str());
    }

    m_logSystem.LogMsg(
        csp::common::LogLevel::Log, fmt::format("ScopeLeadershipManager::OnVacatedAsScopeLeader Event called for scope: {}.", scopeId).c_str());
}

void ScopeLeadershipManager::SendHeartbeatIfElectedScopeLeader()
{
    const auto currentTime = std::chrono::steady_clock::now();

    for (auto& leaderDataPair : m_scopes)
    {
        const std::string& scopeId = leaderDataPair.first;
        std::optional<ScopeLeaderData>& leaderData = leaderDataPair.second;

        // We should only send a heartbeat if the local client is the leader of the scope.
        if (IsLocalClientLeader(scopeId))
        {
            // Ensure the correct amount of time has passed.
            if (currentTime - leaderData->LastHeartbeatTime > LeaderElectionHeartbeatInterval)
            {
                // Send heartbeat.
                leaderData->LastHeartbeatTime = currentTime;
                SendLeaderHeartbeat(scopeId);
            }
        }
    }
}

std::optional<uint64_t> ScopeLeadershipManager::GetLeaderClientId(const std::string& scopeId) const
{
    auto scopeIt = m_scopes.find(scopeId);

    if (scopeIt == m_scopes.end())
    {
        m_logSystem.LogMsg(csp::common::LogLevel::Error,
            fmt::format("ScopeLeadershipManager::GetLeaderClientId Event called for the scope: {} that isn't registered.", scopeId).c_str());
        return std::nullopt;
    }

    const std::optional<ScopeLeaderData>& leaderData = scopeIt->second;

    if (leaderData.has_value() == false)
    {
        // Scope doesn't have a leader.
        return std::nullopt;
    }

    return leaderData->LeaderClientId;
}

bool ScopeLeadershipManager::IsLocalClientLeader(const std::string& scopeId) const
{
    auto leaderClientId = GetLeaderClientId(scopeId);

    if (leaderClientId.has_value() == false)
    {
        return false;
    }

    return *leaderClientId == m_connection.GetClientId();
}

void ScopeLeadershipManager::SendLeaderHeartbeat(const std::string& scopeId)
{
    const signalr::value params { std::vector { signalr::value { scopeId } } };

    // We will use this to check that our object is still valid when the callback is executed, as the callback could be executed after this object is
    // destroyed. If the weak pointer can't be accessed via `lock`, then we know the object has been destroyed and we won't execute the body of the callback.
    std::weak_ptr<int> weakToken = m_lifetimeToken;

    m_connection.GetSignalRConnection()->Invoke(m_connection.GetMultiplayerHubMethods().Get(MultiplayerHubMethod::SEND_SCOPE_LEADER_HEARTBEAT), params,
        [this, weakToken, scopeId](signalr::value, std::exception_ptr exception)
        {
            // Bail if the weak token is not valid. Indicates that the ScopeLeadershipManager has been destroyed.
            std::shared_ptr<int> sharedToken = weakToken.lock();
            if (!sharedToken)
            {
                return;
            }

            if (exception)
            {
                // An exception was thrown. In this case, we just log an error to notify clients.
                // There isn't anything else we can do, as this all happens server-side.
                try
                {
                    std::rethrow_exception(exception);
                }
                catch (const std::exception& exception)
                {
                    m_logSystem.LogMsg(csp::common::LogLevel::Error,
                        fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Failed to send heartbeat for scope: {0} with error: {1}", scopeId,
                            exception.what())
                            .c_str());
                }
                catch (...)
                {
                    m_logSystem.LogMsg(csp::common::LogLevel::Error,
                        fmt::format(
                            "ScopeLeadershipManager::SendLeaderHeartbeat Failed to send heartbeat for scope: {} with an unknown error.", scopeId)
                            .c_str());
                }
            }
            else
            {
                // Successfuly sent the heartbeat.
                m_logSystem.LogMsg(csp::common::LogLevel::VeryVerbose,
                    fmt::format("ScopeLeadershipManager::SendLeaderHeartbeat Heartbeat was successfuly sent for scope: {}", scopeId).c_str());
            }
        });
    }
}
