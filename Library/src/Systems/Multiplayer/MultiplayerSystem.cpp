#include "CSP/Systems/Multiplayer/MultiplayerSystem.h"
#include "CSP/Systems/Spaces/SpaceSystem.h"
#include "Common/Convert.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/MultiplayerService/Api.h"
#include "Services/MultiplayerService/Dto.h"
#include "Systems/ResultHelpers.h"

namespace chs = csp::services::generated;

namespace csp::systems
{
MultiplayerSystem::MultiplayerSystem(csp::web::WebClient* InWebClient, csp::systems::SpaceSystem& SpaceSystem, csp::common::LogSystem& LogSystem)
    : SystemBase(InWebClient, nullptr, &LogSystem)
    , SpaceSystem { &SpaceSystem }
{
    ScopeLeaderApi = std::make_unique<chs::multiplayerservice::ScopeLeaderApi>(InWebClient);
    ScopesApi = std::make_unique<chs::multiplayerservice::ScopesApi>(InWebClient);
}

MultiplayerSystem::MultiplayerSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , SpaceSystem { nullptr }
{
}

MultiplayerSystem::~MultiplayerSystem() { }

void MultiplayerSystem::GetScopesBySpace(const csp::common::String& SpaceId, ScopesResultCallback Callback)
{
    // This specifies that the scope relates to a group (space) using the ReferenceId. Additional values may be possible in the future.
    // We can then allow clients to specify this value.
    constexpr const char* ReferenceType = "GroupId";

    // Ensure we're in the space we want to get scopes for.
    if (SpaceSystem->GetCurrentSpace().Id != SpaceId)
    {
        LogSystem->LogMsg(csp::common::LogLevel::Error, "GetScopesBySpace: You must have entered the space you want to get scopes for");
        Callback(MakeInvalid<ScopesResult>());
        return;
    }

    csp::services::ResponseHandlerPtr ResponseHandler
        = ScopeLeaderApi->CreateHandler<ScopesResultCallback, ScopesResult, void, services::DtoArray<chs::multiplayerservice::ScopeDto>>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs::multiplayerservice::ScopesApi*>(ScopesApi.get())
        ->scopesReferenceTypeReferenceTypeReferenceIdReferenceIdGet({ ReferenceType, SpaceId }, ResponseHandler);
}

void MultiplayerSystem::UpdateScopeById(const csp::common::String& ScopeId, const csp::systems::Scope& Scope, ScopeResultCallback Callback)
{
    auto Dto = std::make_shared<chs::multiplayerservice::ScopeDto>();

    Dto->SetReferenceId(Scope.ReferenceId);
    Dto->SetReferenceType(Scope.ReferenceId);
    Dto->SetName(Scope.Name);

    auto PubSubModelDto = std::make_shared<chs::multiplayerservice::PubSubModel>();

    switch (Scope.PubSubType)
    {
    case PubSubModelType::Global:
        PubSubModelDto->SetValue(chs::multiplayerservice::PubSubModel::ePubSubModel::GLOBAL);
        break;
    case PubSubModelType::Object:
        PubSubModelDto->SetValue(chs::multiplayerservice::PubSubModel::ePubSubModel::OBJECT);
        break;
    default:
        LogSystem->LogMsg(csp::common::LogLevel::Error, "UpdateScopeById: Invalid PubSubModel type specified");
        Callback(MakeInvalid<ScopeResult>());
        return;
    }

    Dto->SetPubSubModel(PubSubModelDto);
    Dto->SetSolveRadius(Scope.SolveRadius);
    Dto->SetManagedLeaderElection(Scope.ManagedLeaderElection);

    csp::services::ResponseHandlerPtr ResponseHandler
        = ScopeLeaderApi->CreateHandler<ScopeResultCallback, ScopeResult, void, chs::multiplayerservice::ScopeDto>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs::multiplayerservice::ScopesApi*>(ScopesApi.get())->scopesIdPut({ ScopeId, Dto }, ResponseHandler);
}

void MultiplayerSystem::GetScopeLeader(const csp::common::String& ScopeId, ScopeLeaderResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = ScopeLeaderApi->CreateHandler<ScopeLeaderResultCallback, ScopeLeaderResult, void, chs::multiplayerservice::ScopeLeaderDto>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs::multiplayerservice::ScopeLeaderApi*>(ScopeLeaderApi.get())->scopesScopeIdLeaderGet({ ScopeId }, ResponseHandler);
}

void MultiplayerSystem::__PerformLeaderElectionInScope(const csp::common::String& ScopeId,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& UserIdsToExclude, NullResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = ScopeLeaderApi->CreateHandler<NullResultCallback, NullResult, void, chs::multiplayerservice::ScopeLeaderDto>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs::multiplayerservice::ScopeLeaderApi*>(ScopeLeaderApi.get())
        ->scopesScopeIdLeader_electionPost({ ScopeId, csp::common::Convert(UserIdsToExclude) }, ResponseHandler);
}

}
