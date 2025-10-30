#include "CSP/Systems/Multiplayer/MultiplayerSystem.h"
#include "Common/Convert.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/MultiplayerService/Api.h"
#include "Services/MultiplayerService/Dto.h"
#include "Systems/ResultHelpers.h"

namespace chs = csp::services::generated;

namespace csp::systems
{
MultiplayerSystem::MultiplayerSystem(csp::web::WebClient* InWebClient, csp::common::LogSystem& LogSystem)
    : SystemBase(InWebClient, nullptr, &LogSystem)
{
    ScopeLeaderApi = std::make_unique<chs::multiplayerservice::ScopeLeaderApi>(InWebClient);
    ScopesApi = std::make_unique<chs::multiplayerservice::ScopesApi>(InWebClient);
}

MultiplayerSystem::MultiplayerSystem()
    : SystemBase(nullptr, nullptr, nullptr)
{
}

MultiplayerSystem::~MultiplayerSystem() { }

void MultiplayerSystem::GetScopesBySpace(const csp::common::String& SpaceId, ScopesResultCallback Callback)
{
    constexpr const char* ReferenceType = "GroupId";

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
        LogSystem->LogMsg(csp::common::LogLevel::Error, "Invalid PubSubModel type specified");
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

void MultiplayerSystem::PerformLeaderElectionInScope(const csp::common::String& ScopeId,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& UserIdsToExclude, NullResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = ScopeLeaderApi->CreateHandler<NullResultCallback, NullResult, void, chs::multiplayerservice::ScopeLeaderDto>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs::multiplayerservice::ScopeLeaderApi*>(ScopeLeaderApi.get())
        ->scopesScopeIdLeader_electionPost({ ScopeId, csp::common::Convert(UserIdsToExclude) }, ResponseHandler);
}

}
