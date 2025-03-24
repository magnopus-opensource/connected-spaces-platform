#include "CSP/Multiplayer/LocalScript/LocalScriptResult.h"

namespace csp::systems {

const csp::common::Map<csp::common::String, csp::common::String>& LocalScriptsResult::GetLocalScripts() const {
    return LocalScripts;
}

void LocalScriptsResult::SetLocalScripts(const csp::common::Map<csp::common::String, csp::common::String>& LocalScripts) {
    this->LocalScripts = LocalScripts;
}

void LocalScriptsResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse) {}

} // namespace csp::systems