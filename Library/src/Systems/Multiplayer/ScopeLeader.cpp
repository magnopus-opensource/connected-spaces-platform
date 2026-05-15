#include "CSP/Systems/Multiplayer/ScopeLeader.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/MultiplayerService/Dto.h"

namespace chs = csp::services::generated;

namespace csp::systems
{
const ScopeLeader& ScopeLeaderResult::GetScopeLeader() const { return m_leader; }

void ScopeLeaderResult::OnResponse(const csp::services::ApiResponseBase* apiResponse)
{
    ResultBase::OnResponse(apiResponse);

    auto* scopeLeaderResponse = static_cast<chs::multiplayerservice::ScopeLeaderDto*>(apiResponse->GetDto());
    const csp::web::HttpResponse* response = apiResponse->GetResponse();

    if (apiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        scopeLeaderResponse->FromJson(response->GetPayload().GetContent());
        DtoToScopeLeader(*scopeLeaderResponse, m_leader);
    }
}
void DtoToScopeLeader(const csp::services::generated::multiplayerservice::ScopeLeaderDto& dto, csp::systems::ScopeLeader& scopeLeader)
{
    if (dto.HasScopeId())
    {
        scopeLeader.ScopeId = dto.GetScopeId();
    }
    if (dto.HasLeaderClientId())
    {
        scopeLeader.ScopeClientId = dto.GetLeaderClientId();
    }
    if (dto.HasLeaderUserId())
    {
        scopeLeader.ScopeLeaderUserId = dto.GetLeaderUserId();
    }
    if (dto.HasElectionInProgress())
    {
        scopeLeader.ElectionInProgress = dto.GetElectionInProgress();
    }
}
}
