#include "CSP/Systems/Multiplayer/ScopeLeader.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/MultiplayerService/Dto.h"

namespace chs = csp::services::generated;

namespace csp::systems
{
const ScopeLeader& ScopeLeaderResult::GetScopeLeader() const { return Leader; }

void ScopeLeaderResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
    ResultBase::OnResponse(ApiResponse);

    auto* ScopeLeaderResponse = static_cast<chs::multiplayerservice::ScopeLeaderDto*>(ApiResponse->GetDto());
    const csp::web::HttpResponse* Response = ApiResponse->GetResponse();

    if (ApiResponse->GetResponseCode() == csp::services::EResponseCode::ResponseSuccess)
    {
        // Build the Dto from the response Json
        ScopeLeaderResponse->FromJson(Response->GetPayload().GetContent());
        DtoToScopeLeader(*ScopeLeaderResponse, Leader);
    }
}
void DtoToScopeLeader(const csp::services::generated::multiplayerservice::ScopeLeaderDto& Dto, csp::systems::ScopeLeader& ScopeLeader)
{
    if (Dto.HasScopeId())
    {
        ScopeLeader.ScopeId = Dto.GetScopeId();
    }
    if (Dto.HasLeaderUserId())
    {
        ScopeLeader.ScopeLeaderUserId = Dto.GetLeaderUserId();
    }
    if (Dto.HasElectionInProgress())
    {
        ScopeLeader.ElectionInProgress = Dto.GetElectionInProgress();
    }
}
}
