#include "CSP/Systems/Multiplayer/Scope.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/MultiplayerService/Dto.h"

namespace chs = csp::services::generated;

namespace csp::systems
{
namespace
{
    void DtoToPubSubModelType(const chs::multiplayerservice::PubSubModel& model, PubSubModelType& type)
    {
        if (model.GetValue() == chs::multiplayerservice::PubSubModel::ePubSubModel::GLOBAL)
        {
            type = PubSubModelType::Global;
        }
        else if (model.GetValue() == chs::multiplayerservice::PubSubModel::ePubSubModel::OBJECT)
        {
            type = PubSubModelType::Object;
        }
    }
}

bool Scope::operator==(const Scope& other) const
{
    return Id == other.Id && ReferenceId == other.ReferenceId && ReferenceType == other.ReferenceType && Name == other.Name
        && PubSubType == other.PubSubType && SolveRadius == other.SolveRadius && ManagedLeaderElection == other.ManagedLeaderElection;
}

bool Scope::operator!=(const Scope& other) const { return !(*this == other); }

void DtoToScope(const chs::multiplayerservice::ScopeDto& dto, csp::systems::Scope& scopeLeader)
{
    if (dto.HasId())
    {
        scopeLeader.Id = dto.GetId();
    }
    if (dto.HasReferenceId())
    {
        scopeLeader.ReferenceId = dto.GetReferenceId();
    }
    if (dto.HasReferenceType())
    {
        scopeLeader.ReferenceType = dto.GetReferenceType();
    }
    if (dto.HasName())
    {
        scopeLeader.Name = dto.GetName();
    }
    if (dto.HasPubSubModel())
    {
        DtoToPubSubModelType(*dto.GetPubSubModel(), scopeLeader.PubSubType);
    }
    if (dto.HasSolveRadius())
    {
        scopeLeader.SolveRadius = dto.GetSolveRadius();
    }
    if (dto.HasManagedLeaderElection())
    {
        scopeLeader.ManagedLeaderElection = dto.GetManagedLeaderElection();
    }
}

const Scope& ScopeResult::GetScope() const { return m_scope; }

ScopeResult::ScopeResult(csp::systems::EResultCode resCode, uint16_t httpResCode)
    : ResultBase { resCode, httpResCode }
{
}

void ScopeResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* scopeResponse = static_cast<chs::multiplayerservice::ScopeDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        scopeResponse->FromJson(response->GetPayload().GetContent());
        DtoToScope(*scopeResponse, m_scope);
    }
}
ScopesResult::ScopesResult(csp::systems::EResultCode resCode, uint16_t httpResCode)
    : ResultBase { resCode, httpResCode }
{
}

const csp::common::Array<Scope>& ScopesResult::GetScopes() const { return m_scopes; }

void ScopesResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        auto* scopesResponse = static_cast<csp::services::DtoArray<chs::multiplayerservice::ScopeDto>*>(apiResponse->GetDto());
        scopesResponse->FromJson(response->GetPayload().GetContent());

        std::vector<chs::multiplayerservice::ScopeDto>& scopesArray = scopesResponse->GetArray();
        m_scopes = csp::common::Array<Scope>(scopesArray.size());

        for (size_t i = 0; i < scopesArray.size(); ++i)
        {
            DtoToScope(scopesArray[i], m_scopes[i]);
        }
    }
}

}
