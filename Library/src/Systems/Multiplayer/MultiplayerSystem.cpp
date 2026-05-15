#include "CSP/Systems/Multiplayer/MultiplayerSystem.h"
#include "CSP/Systems/ContinuationUtils.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "Common/Convert.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/MultiplayerService/Api.h"
#include "Services/MultiplayerService/Dto.h"
#include "Systems/ResultHelpers.h"

namespace chs = csp::services::generated;

namespace csp::systems
{
MultiplayerSystem::MultiplayerSystem(csp::web::WebClient* inWebClient, csp::systems::SpaceSystem& spaceSystem, csp::common::LogSystem& logSystem)
    : SystemBase(inWebClient, nullptr, &logSystem)
    , m_spaceSystem { &spaceSystem }
{
    m_scopeLeaderApi = std::make_unique<chs::multiplayerservice::ScopeLeaderApi>(inWebClient);
    m_scopesApi = std::make_unique<chs::multiplayerservice::ScopesApi>(inWebClient);
}

MultiplayerSystem::MultiplayerSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , m_spaceSystem { nullptr }
{
}

MultiplayerSystem::~MultiplayerSystem() { }

void MultiplayerSystem::GetScopesBySpace(const csp::common::String& spaceId, ScopesResultCallback callback)
{
    // This specifies that the scope relates to a group (space) using the ReferenceId. Additional values may be possible in the future.
    // We can then allow clients to specify this value.
    // This is currently populated automatically by chs for the default scope that's created with the space.
    constexpr const char* referenceType = "GroupId";

    // Ensure we're in the space we want to get scopes for.
    if (m_spaceSystem->GetCurrentSpace().Id != spaceId)
    {
        m_logSystem->LogMsg(csp::common::LogLevel::Error, "GetScopesBySpace: You must have entered the space you want to get scopes for");
        callback(MakeInvalid<ScopesResult>());
        return;
    }

    csp::services::ResponseHandlerPtr responseHandler
        = m_scopeLeaderApi->CreateHandler<ScopesResultCallback, ScopesResult, void, services::DtoArray<chs::multiplayerservice::ScopeDto>>(
            callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs::multiplayerservice::ScopesApi*>(m_scopesApi.get())
        ->scopesReferenceTypeReferenceTypeReferenceIdReferenceIdGet({ referenceType, spaceId }, responseHandler);
}

async::task<ScopesResult> MultiplayerSystem::GetScopesBySpace(const csp::common::String& spaceId)
{
    auto event = std::make_shared<async::event_task<csp::systems::ScopesResult>>();
    auto task = event->get_task();

    GetScopesBySpace(spaceId,
        [event](const ScopesResult& result)
        {
            if (result.GetResultCode() != EResultCode::InProgress)
            {
                event->set(result);
            }
        });

    return task;
}

void MultiplayerSystem::UpdateScopeById(const csp::common::String& scopeId, const csp::systems::Scope& scope, ScopeResultCallback callback)
{
    auto dto = std::make_shared<chs::multiplayerservice::ScopeDto>();

    dto->SetReferenceId(scope.ReferenceId);
    dto->SetReferenceType(scope.ReferenceId);
    dto->SetName(scope.Name);

    auto pubSubModelDto = std::make_shared<chs::multiplayerservice::PubSubModel>();

    switch (scope.PubSubType)
    {
    case PubSubModelType::Global:
        pubSubModelDto->SetValue(chs::multiplayerservice::PubSubModel::ePubSubModel::GLOBAL);
        break;
    case PubSubModelType::Object:
        pubSubModelDto->SetValue(chs::multiplayerservice::PubSubModel::ePubSubModel::OBJECT);
        break;
    default:
        m_logSystem->LogMsg(csp::common::LogLevel::Error, "UpdateScopeById: Invalid PubSubModel type specified");
        callback(MakeInvalid<ScopeResult>());
        return;
    }

    dto->SetPubSubModel(pubSubModelDto);
    dto->SetSolveRadius(scope.SolveRadius);
    dto->SetManagedLeaderElection(scope.ManagedLeaderElection);

    csp::services::ResponseHandlerPtr responseHandler
        = m_scopeLeaderApi->CreateHandler<ScopeResultCallback, ScopeResult, void, chs::multiplayerservice::ScopeDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs::multiplayerservice::ScopesApi*>(m_scopesApi.get())->scopesIdPut({ scopeId, dto }, responseHandler);
}

void MultiplayerSystem::GetScopeLeader(const csp::common::String& scopeId, ScopeLeaderResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_scopeLeaderApi->CreateHandler<ScopeLeaderResultCallback, ScopeLeaderResult, void, chs::multiplayerservice::ScopeLeaderDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs::multiplayerservice::ScopeLeaderApi*>(m_scopeLeaderApi.get())->scopesScopeIdLeaderGet({ scopeId }, responseHandler);
}

async::task<ScopeLeaderResult> MultiplayerSystem::GetScopeLeader(const csp::common::String& scopeId)
{
    auto event = std::make_shared<async::event_task<csp::systems::ScopeLeaderResult>>();
    auto task = event->get_task();

    GetScopeLeader(scopeId,
        [event](const ScopeLeaderResult& result)
        {
            if (result.GetResultCode() != EResultCode::InProgress)
            {
                event->set(result);
            }
        });

    return task;
}

void MultiplayerSystem::__PerformLeaderElectionInScope(const csp::common::String& scopeId,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& userIdsToExclude, NullResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_scopeLeaderApi->CreateHandler<NullResultCallback, NullResult, void, chs::multiplayerservice::ScopeLeaderDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs::multiplayerservice::ScopeLeaderApi*>(m_scopeLeaderApi.get())
        ->scopesScopeIdLeader_electionPost({ scopeId, csp::common::Convert(userIdsToExclude) }, responseHandler);
}

}
