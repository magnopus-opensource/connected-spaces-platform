/*
 * Copyright 2023 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "CSP/Common/Interfaces/IScriptBinding.h"

#include <memory>

namespace qjs
{
class Context;
} // namespace qjs

namespace csp::common
{
class LogSystem;
class IJSScriptRunner;
class IRealtimeEngine;
}

namespace csp::multiplayer
{

class EntityScriptBinding;

/// @brief NGX-specific script binding wrapper.
/// This keeps NGX scripting customizations isolated from the legacy EntityScriptBinding implementation.
class NgxEntityScriptBinding : public csp::common::IScriptBinding
{
public:
    NgxEntityScriptBinding(csp::common::IRealtimeEngine* InEntitySystem, csp::common::LogSystem& LogSystem);
    ~NgxEntityScriptBinding() override;

    void Bind(int64_t ContextId, csp::common::IJSScriptRunner& ScriptRunner) override;
    void BindToContext(qjs::Context& Context, int64_t ContextId = 0);

    static NgxEntityScriptBinding* BindEntitySystem(
        csp::common::IRealtimeEngine* InEntitySystem, csp::common::LogSystem& LogSystem, csp::common::IJSScriptRunner& ScriptRunner);
    static void RemoveBinding(NgxEntityScriptBinding* InEntityBinding, csp::common::IJSScriptRunner& ScriptRunner);

private:
    csp::common::IRealtimeEngine* EntitySystem;
    std::unique_ptr<EntityScriptBinding> LegacyBinding;
};

} // namespace csp::multiplayer
