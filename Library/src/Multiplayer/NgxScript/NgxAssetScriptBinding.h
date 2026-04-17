/*
 * Copyright 2026 Magnopus LLC

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

#include "CSP/Common/String.h"

#include <memory>
#include <mutex>
#include <vector>

struct JSContext;

namespace qjs
{
class Context;
} // namespace qjs

namespace csp::common
{
class LogSystem;
} // namespace csp::common

namespace csp::systems
{
class AssetSystem;
} // namespace csp::systems

namespace csp::multiplayer
{

/// @brief Binds TheAssetSystem JS globals to a QuickJS context.
/// Exposes a single `__ngxAssetQuery(queryJson)` function that returns a native
/// Promise resolving to an array of asset objects.
class NgxAssetScriptBinding
{
public:
    NgxAssetScriptBinding(csp::systems::AssetSystem* InAssetSystem, csp::common::LogSystem& InLogSystem);
    ~NgxAssetScriptBinding();

    /// @brief Register C++ globals and inject the __cspAssetSystem JS setup into Context.
    /// Must be called after the context is (re)created.
    void BindToContext(qjs::Context& Context, const csp::common::String& SpaceId);

    /// @brief Reject and free all in-flight Promises. Call BEFORE JS_FreeContext.
    void RejectAndCleanupPendingPromises(JSContext* Ctx);

private:
    // Forward-declared so that JSValue is only included in the .cpp.
    struct PendingPromise;

    csp::systems::AssetSystem* AssetSystem;
    csp::common::LogSystem& LogSystem;

    std::mutex PendingMutex;
    std::vector<std::shared_ptr<PendingPromise>> PendingPromises;

    void RemovePendingPromise(const std::shared_ptr<PendingPromise>& Handle);
};

} // namespace csp::multiplayer
