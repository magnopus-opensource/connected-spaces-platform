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
#include "Multiplayer/NgxScript/NgxAssetScriptBinding.h"

#include "CSP/Common/Array.h"
#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Systems/Assets/Asset.h"
#include "CSP/Systems/Assets/AssetCollection.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/WebService.h"
#include "Multiplayer/AssetQueryUtils.h"

#include <quickjs.h>
#include <quickjspp.hpp>

#include <mutex>
#include <string>
#include <vector>

namespace csp::multiplayer
{

// --- PendingPromise (full definition, only visible in this TU) ----------------

struct NgxAssetScriptBinding::PendingPromise
{
    JSValue Resolve;
    JSValue Reject;
    bool Invalidated = false;
    std::mutex Mutex;
};

// --- helpers ------------------------------------------------------------------

namespace
{

void RejectWithMessage(JSContext* Ctx, JSValue RejectFn, const char* Message)
{
    JSValue ErrorMsg = JS_NewString(Ctx, Message);
    JSValueConst Args[1] = { ErrorMsg };
    JS_Call(Ctx, RejectFn, JS_UNDEFINED, 1, Args);
    JS_FreeValue(Ctx, ErrorMsg);
}

// Converts csp::common::Array<T> to std::vector<T> (used for snapshot before async chain).
std::vector<csp::systems::AssetCollection> ToVector(const csp::common::Array<csp::systems::AssetCollection>& In)
{
    std::vector<csp::systems::AssetCollection> Out;
    Out.reserve(In.Size());
    for (size_t I = 0; I < In.Size(); ++I)
    {
        Out.push_back(In[I]);
    }
    return Out;
}

std::vector<csp::systems::Asset> ToVector(const csp::common::Array<csp::systems::Asset>& In)
{
    std::vector<csp::systems::Asset> Out;
    Out.reserve(In.Size());
    for (size_t I = 0; I < In.Size(); ++I)
    {
        Out.push_back(In[I]);
    }
    return Out;
}

constexpr const char* ASSET_SYSTEM_SETUP_SCRIPT = R"JS(
globalThis.__cspAssetSystem = {
    query(queryObj) {
        return globalThis.__ngxAssetQuery(JSON.stringify(queryObj ?? {}));
    },
};
)JS";

} // anonymous namespace

// --- NgxAssetScriptBinding ----------------------------------------------------

NgxAssetScriptBinding::NgxAssetScriptBinding(csp::systems::AssetSystem* InAssetSystem, csp::common::LogSystem& InLogSystem)
    : AssetSystem(InAssetSystem)
    , LogSystem(InLogSystem)
{
    LogSystem.LogMsg(csp::common::LogLevel::Verbose, "NgxAssetScriptBinding initialised");
}

NgxAssetScriptBinding::~NgxAssetScriptBinding() = default;

void NgxAssetScriptBinding::BindToContext(qjs::Context& Context, const csp::common::String& SpaceId)
{
    JSContext* Ctx = Context.ctx;
    const std::string SpaceIdStr(SpaceId.c_str());

    Context.global()["__ngxAssetQuery"] =
        [this, Ctx, SpaceIdStr](const std::string& QueryJson) -> qjs::Value
    {
        // Create a native Promise.
        JSValue ResolvingFuncs[2];
        JSValue Promise = JS_NewPromiseCapability(Ctx, ResolvingFuncs);

        // Move ownership into a PendingPromise on the heap.
        auto Handle = std::make_shared<PendingPromise>();
        Handle->Resolve = ResolvingFuncs[0]; // ownership transferred
        Handle->Reject  = ResolvingFuncs[1]; // ownership transferred

        {
            std::lock_guard<std::mutex> Lock(PendingMutex);
            PendingPromises.push_back(Handle);
        }

        if (!AssetSystem)
        {
            std::lock_guard<std::mutex> Lock(Handle->Mutex);
            if (!Handle->Invalidated)
            {
                RejectWithMessage(Ctx, Handle->Reject, "AssetSystem unavailable");
                JS_FreeValue(Ctx, Handle->Resolve);
                JS_FreeValue(Ctx, Handle->Reject);
                Handle->Invalidated = true;
            }
            RemovePendingPromise(Handle);
            return qjs::Value(Ctx, std::move(Promise));
        }

        // Step 1: fetch all collections for the current space.
        AssetSystem->FindAssetCollections(
            csp::common::Optional<csp::common::Array<csp::common::String>> {},
            csp::common::Optional<csp::common::String> {},
            csp::common::Optional<csp::common::Array<csp::common::String>> {},
            csp::common::Optional<csp::common::Array<csp::systems::EAssetCollectionType>> {},
            csp::common::Optional<csp::common::Array<csp::common::String>> {},
            csp::common::Array<csp::common::String> { SpaceIdStr.c_str() },
            csp::common::Optional<int> {},
            csp::common::Optional<int> {},
            [this, Ctx, Handle, QueryJson](const csp::systems::AssetCollectionsResult& CollectionsResult)
            {
                {
                    std::lock_guard<std::mutex> Lock(Handle->Mutex);
                    if (Handle->Invalidated)
                    {
                        return;
                    }
                }

                if (CollectionsResult.GetResultCode() != csp::systems::EResultCode::Success)
                {
                    std::lock_guard<std::mutex> Lock(Handle->Mutex);
                    if (!Handle->Invalidated)
                    {
                        RejectWithMessage(Ctx, Handle->Reject, "TheAssetSystem.query: failed to find asset collections");
                        JS_FreeValue(Ctx, Handle->Resolve);
                        JS_FreeValue(Ctx, Handle->Reject);
                        Handle->Invalidated = true;
                    }
                    RemovePendingPromise(Handle);
                    return;
                }

                const auto& RawCollections = CollectionsResult.GetAssetCollections();
                if (RawCollections.Size() == 0)
                {
                    // No collections → resolve with empty array.
                    std::lock_guard<std::mutex> Lock(Handle->Mutex);
                    if (!Handle->Invalidated)
                    {
                        JSValue EmptyArray = JS_ParseJSON(Ctx, "[]", 2, "<asset-query>");
                        JSValueConst Args[1] = { EmptyArray };
                        JS_Call(Ctx, Handle->Resolve, JS_UNDEFINED, 1, Args);
                        JS_FreeValue(Ctx, EmptyArray);
                        JS_FreeValue(Ctx, Handle->Resolve);
                        JS_FreeValue(Ctx, Handle->Reject);
                        Handle->Invalidated = true;
                    }
                    RemovePendingPromise(Handle);
                    return;
                }

                // Build collection ID array for GetAssetsByCriteria.
                csp::common::Array<csp::common::String> CollectionIds(RawCollections.Size());
                for (size_t I = 0; I < RawCollections.Size(); ++I)
                {
                    CollectionIds[I] = RawCollections[I].Id;
                }

                // Snapshot collections for client-side evaluation.
                auto Collections = std::make_shared<std::vector<csp::systems::AssetCollection>>(ToVector(RawCollections));

                // Extract server-side type filter from query.
                const AssetQueryCriteria Criteria = ExtractAssetQueryCriteria(QueryJson);

                // Step 2: fetch assets (optionally filtered by type server-side).
                AssetSystem->GetAssetsByCriteria(
                    CollectionIds,
                    csp::common::Optional<csp::common::Array<csp::common::String>> {},
                    csp::common::Optional<csp::common::Array<csp::common::String>> {},
                    Criteria.AssetTypes,
                    [this, Ctx, Handle, QueryJson, Collections](const csp::systems::AssetsResult& AssetsResult)
                    {
                        std::lock_guard<std::mutex> Lock(Handle->Mutex);
                        if (Handle->Invalidated)
                        {
                            return;
                        }

                        Handle->Invalidated = true;

                        if (AssetsResult.GetResultCode() != csp::systems::EResultCode::Success)
                        {
                            RejectWithMessage(Ctx, Handle->Reject, "TheAssetSystem.query: failed to get assets");
                            JS_FreeValue(Ctx, Handle->Resolve);
                            JS_FreeValue(Ctx, Handle->Reject);
                            RemovePendingPromise(Handle);
                            return;
                        }

                        // Client-side evaluation for and/or/not predicates.
                        const auto RawAssets = ToVector(AssetsResult.GetAssets());
                        const auto FilteredAssets = EvaluateAssetQuery(QueryJson, RawAssets, *Collections);
                        const std::string Json = SerializeAssetsToJson(FilteredAssets);

                        JSValue ResultArray = JS_ParseJSON(Ctx, Json.c_str(), Json.size(), "<asset-query>");
                        JSValueConst Args[1] = { ResultArray };
                        JS_Call(Ctx, Handle->Resolve, JS_UNDEFINED, 1, Args);
                        JS_FreeValue(Ctx, ResultArray);
                        JS_FreeValue(Ctx, Handle->Resolve);
                        JS_FreeValue(Ctx, Handle->Reject);
                        RemovePendingPromise(Handle);
                    });
            });

        return qjs::Value(Ctx, std::move(Promise));
    };

    // Inject the JS-side __cspAssetSystem wrapper.
    Context.eval(ASSET_SYSTEM_SETUP_SCRIPT, "<ngx-asset-script-binding>", JS_EVAL_TYPE_GLOBAL);
}

void NgxAssetScriptBinding::RejectAndCleanupPendingPromises(JSContext* Ctx)
{
    std::lock_guard<std::mutex> Lock(PendingMutex);
    for (auto& Handle : PendingPromises)
    {
        std::lock_guard<std::mutex> HandleLock(Handle->Mutex);
        if (!Handle->Invalidated)
        {
            RejectWithMessage(Ctx, Handle->Reject, "Space session ended");
            JS_FreeValue(Ctx, Handle->Resolve);
            JS_FreeValue(Ctx, Handle->Reject);
            Handle->Invalidated = true;
        }
    }
    PendingPromises.clear();
}

void NgxAssetScriptBinding::RemovePendingPromise(const std::shared_ptr<PendingPromise>& Handle)
{
    std::lock_guard<std::mutex> Lock(PendingMutex);
    PendingPromises.erase(std::remove(PendingPromises.begin(), PendingPromises.end(), Handle), PendingPromises.end());
}

} // namespace csp::multiplayer
