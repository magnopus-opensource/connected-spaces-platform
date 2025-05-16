#include "CSP/Multiplayer/LocalScript/LocalScriptResult.h"

namespace csp::systems {

const csp::common::Map<csp::common::String, csp::common::String>& LocalScriptResult::GetLocalScripts() const {
    return LocalScripts;
}

void LocalScriptResult::SetLocalScripts(const csp::common::Map<csp::common::String, csp::common::String>& LocalScripts) {
    this->LocalScripts = LocalScripts;
}

void LocalScriptResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse) {
    (void)ApiResponse;
}

} // namespace csp::systems