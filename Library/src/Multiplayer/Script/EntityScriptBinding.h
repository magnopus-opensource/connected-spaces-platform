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

namespace csp::common
{
class LogSystem;
class IJSScriptRunner;
class IRealtimeEngine;
}

namespace csp::multiplayer
{

class EntityScriptBinding : public csp::common::IScriptBinding
{
public:
    EntityScriptBinding(csp::common::IRealtimeEngine* inEntitySystem, csp::common::LogSystem& logSystem);
    ~EntityScriptBinding();

    void Bind(int64_t contextId, csp::common::IJSScriptRunner& scriptRunner) override;

    static EntityScriptBinding* BindEntitySystem(
        csp::common::IRealtimeEngine* inEntitySystem, csp::common::LogSystem& logSystem, csp::common::IJSScriptRunner& scriptRunner);
    static void RemoveBinding(EntityScriptBinding* inEntityBinding, csp::common::IJSScriptRunner& scriptRunner);

private:
    class SchemaCacheImpl;
    std::unique_ptr<SchemaCacheImpl> m_schemaCache;

    csp::common::IRealtimeEngine* m_entitySystem;
    csp::common::LogSystem& m_logSystem;
};

} // namespace csp::multiplayer
