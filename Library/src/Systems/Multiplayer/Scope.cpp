#include "CSP/Systems/Multiplayer/Scope.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/MultiplayerService/Dto.h"

namespace chs = csp::services::generated;

namespace csp::systems
{
namespace
{
    void DtoToPubSubModelType(const chs::multiplayerservice::PubSubModel& Model, PubSubModelType& Type)
    {
        if (Model.GetValue() == chs::multiplayerservice::PubSubModel::ePubSubModel::GLOBAL)
        {
            Type = PubSubModelType::Global;
        }
        else if (Model.GetValue() == chs::multiplayerservice::PubSubModel::ePubSubModel::OBJECT)
        {
            Type = PubSubModelType::Object;
        }
    }
}

bool Scope::operator==(const Scope& Other) const
{
    return Id == Other.Id && ReferenceId == Other.ReferenceId && ReferenceType == Other.ReferenceType && Name == Other.Name
        && PubSubType == Other.PubSubType && SolveRadius == Other.SolveRadius && ManagedLeaderElection == Other.ManagedLeaderElection;
}

bool Scope::operator!=(const Scope& Other) const { return !(*this == Other); }

void DtoToScope(const chs::multiplayerservice::ScopeDto& Dto, csp::systems::Scope& ScopeLeader)
{
    if (Dto.HasId())
    {
        ScopeLeader.Id = Dto.GetId();
    }
    if (Dto.HasReferenceId())
    {
        ScopeLeader.ReferenceId = Dto.GetReferenceId();
    }
    if (Dto.HasReferenceType())
    {
        ScopeLeader.ReferenceType = Dto.GetReferenceType();
    }
    if (Dto.HasName())
    {
        ScopeLeader.Name = Dto.GetName();
    }
    if (Dto.HasPubSubModel())
    {
        DtoToPubSubModelType(*Dto.GetPubSubModel(), ScopeLeader.PubSubType);
    }
    if (Dto.HasSolveRadius())
    {
        ScopeLeader.SolveRadius = Dto.GetSolveRadius();
    }
    if (Dto.HasManagedLeaderElection())
    {
        ScopeLeader.ManagedLeaderElection = Dto.GetManagedLeaderElection();
    }
}

const Scope& ScopeResult::GetScope() const { return Scope; }

ScopeResult::ScopeResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
    : ResultBase { ResCode, HttpResCode }
{
}

void ScopeResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* ScopeResponse = static_cast<chs::multiplayerservice::ScopeDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        ScopeResponse->FromJson(Response->GetPayload().GetContent());
        DtoToScope(*ScopeResponse, Scope);
    }
}
ScopesResult::ScopesResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
    : ResultBase { ResCode, HttpResCode }
{
}

const csp::common::Array<Scope>& ScopesResult::GetScopes() const { return Scopes; }

void ScopesResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        auto* ScopesResponse = static_cast<csp::services::DtoArray<chs::multiplayerservice::ScopeDto>*>(ApiResponse->GetDto());
        ScopesResponse->FromJson(Response->GetPayload().GetContent());

        std::vector<chs::multiplayerservice::ScopeDto>& ScopesArray = ScopesResponse->GetArray();
        Scopes = csp::common::Array<Scope>(ScopesArray.size());

        for (size_t i = 0; i < ScopesArray.size(); ++i)
        {
            DtoToScope(ScopesArray[i], Scopes[i]);
        }
    }
}

}
