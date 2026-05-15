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

#include "CSP/Systems/Spaces/SpaceSystem.h"

#include "CSP/CSPFoundation.h"
#include "CSP/Common/SharedConstants.h"
#include "CSP/Common/SharedEnums.h"
#include "CSP/Common/StringFormat.h"
#include "CSP/Multiplayer/ContinuationUtils.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/NetworkEventBus.h"
#include "CSP/Multiplayer/OnlineRealtimeEngine.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Multiplayer/MultiplayerSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "CallHelpers.h"
#include "Common/Convert.h"
#include "Debug/Logging.h"
#include "Events/EventSystem.h"
#include "Services/AggregationService/Api.h"
#include "Services/AggregationService/Dto.h"
#include "Services/UserService/Api.h"
#include "Services/UserService/Dto.h"
#include "Systems/ResultHelpers.h"
#include "Systems/Spaces/SpaceSystemHelpers.h"
#include "Systems/Spatial/PointOfInterestInternalSystem.h"

#include <exception>
#include <fmt/format.h>
#include <memory>
#include <optional>
#include <rapidjson/rapidjson.h>
#include <thread>

#include "CSP/Systems/ContinuationUtils.h"

using namespace csp;
using namespace csp::common;

namespace chs = csp::services::generated::userservice;
namespace chsaggregation = csp::services::generated::aggregationservice;

namespace
{

constexpr const int MAX_SPACES_RESULTS = 100;

// Construct a new DuplicateSpaceOptions dto request object. This function is called by both DuplicateSpace and DuplicateSpaceAsync methods.
// The only difference is in the value they pass for the AsyncCall parameter.
std::shared_ptr<chsaggregation::DuplicateSpaceOptions> ConstructDuplicateSpaceOptions(csp::systems::UserSystem* userSystem, const String& spaceId,
    const String& newName, csp::systems::SpaceAttributes newAttributes, const Optional<Array<String>>& memberGroupIds, bool shallowCopy,
    bool asyncCall)
{
    auto request = std::make_shared<chsaggregation::DuplicateSpaceOptions>();
    request->SetSpaceId(spaceId);
    request->SetNewGroupOwnerId(userSystem->GetLoginState().UserId);
    request->SetNewUniqueName(newName);
    request->SetDiscoverable(HasFlag(newAttributes, csp::systems::SpaceAttributes::IsDiscoverable));
    request->SetRequiresInvite(HasFlag(newAttributes, csp::systems::SpaceAttributes::RequiresInvite));
    request->SetShallowCopy(shallowCopy);
    request->SetAsyncCall(asyncCall);

    if (memberGroupIds.HasValue())
    {
        auto memberGroupIdsVec = Convert(memberGroupIds);

        request->SetMemberGroupIds(*memberGroupIdsVec);
    }

    return request;
}

} // namespace

namespace csp::systems
{
SpaceSystem::SpaceSystem()
    : SystemBase { nullptr, nullptr, nullptr }
    , m_userSystem(nullptr)
    , m_groupApi { nullptr }
    , m_spaceApi { nullptr }
    , m_multiplayerSystem { nullptr }
{
}

SpaceSystem::SpaceSystem(
    csp::web::WebClient* webClient, multiplayer::NetworkEventBus& eventBus, csp::systems::UserSystem* userSystem, csp::common::LogSystem& logSystem)
    : SystemBase(webClient, &eventBus, &logSystem)
    , m_userSystem(userSystem)
    , m_currentSpace()
{
    m_groupApi = new chs::GroupApi(webClient);
    m_spaceApi = new chsaggregation::SpaceApi(webClient);
}

SpaceSystem::~SpaceSystem()
{
    delete (m_groupApi);
    delete (m_spaceApi);
}

/* CreateSpace Continuations */
async::task<SpaceResult> SpaceSystem::CreateSpaceGroupInfo(
    const String& name, const String& description, SpaceAttributes attributes, const Optional<Array<String>>& tags)
{
    auto onCompleteEvent = std::make_shared<async::event_task<SpaceResult>>();
    async::task<SpaceResult> onCompleteTask = onCompleteEvent->get_task();

    auto groupInfo = systems::SpaceSystemHelpers::DefaultGroupInfo();
    groupInfo->SetName(name);
    groupInfo->SetDescription(description);
    groupInfo->SetDiscoverable(HasFlag(attributes, csp::systems::SpaceAttributes::IsDiscoverable));
    groupInfo->SetRequiresInvite(HasFlag(attributes, csp::systems::SpaceAttributes::RequiresInvite));
    groupInfo->SetGroupType("space");

    if (tags.HasValue())
    {
        groupInfo->SetTags(csp::common::Convert(tags).value());
    }

    csp::services::ResponseHandlerPtr responseHandler = m_groupApi->CreateHandler<SpaceResultCallback, SpaceResult, void, chs::GroupDto>(
        [](const SpaceResult&) {}, nullptr, csp::web::EResponseCodes::ResponseOK, std::move(*onCompleteEvent.get()));

    static_cast<chs::GroupApi*>(m_groupApi)->groupsPost({ groupInfo }, responseHandler);

    return onCompleteTask;
}

std::function<async::task<AssetCollectionResult>()> SpaceSystem::CreateSpaceMetadataAssetCollection(
    const std::shared_ptr<SpaceResult>& space, const csp::common::Map<csp::common::String, csp::common::String>& metadata)
{
    return [space, metadata]() -> async::task<AssetCollectionResult>
    {
        const auto id = space->GetSpace().Id;
        const auto name = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(id);
        auto* assetSystem = SystemsManager::Get().GetAssetSystem();

        // Don't assign this AssetCollection to a space so any user can retrieve the metadata without joining the space
        return assetSystem->CreateAssetCollection(id, nullptr, name, metadata, EAssetCollectionType::FOUNDATION_INTERNAL, nullptr);
    };
}

async::task<AssetCollectionResult> SpaceSystem::CreateSpaceThumbnailAssetCollection(const std::shared_ptr<SpaceResult>& space)
{
    const auto spaceId = space->GetSpace().Id;
    const auto name = SpaceSystemHelpers::GetSpaceThumbnailAssetCollectionName(spaceId);
    auto* assetSystem = SystemsManager::Get().GetAssetSystem();

    // don't associate this asset collection with a particular space so that it can be retrieved by guest users without joining the space
    return assetSystem->CreateAssetCollection(spaceId, nullptr, name, nullptr, EAssetCollectionType::SPACE_THUMBNAIL, Array<String>({ spaceId }));
}

std::function<async::task<AssetResult>()> SpaceSystem::CreateSpaceThumbnailAsset(
    const std::shared_ptr<SpaceResult>& space, const std::shared_ptr<AssetCollectionResult>& assetCollectionResult)
{
    return [space, assetCollectionResult]() -> async::task<AssetResult>
    {
        const auto spaceId = space->GetSpace().Id;
        const auto name = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(spaceId);
        auto* assetSystem = SystemsManager::Get().GetAssetSystem();

        return assetSystem->CreateAsset(assetCollectionResult->GetAssetCollection(), name, nullptr, nullptr, EAssetType::IMAGE);
    };
}

std::function<async::task<UriResult>(const AssetResult& result)> SpaceSystem::UploadSpaceThumbnailAsset(
    const std::shared_ptr<AssetCollectionResult>& assetCollectionResult, FileAssetDataSource& data)
{
    return [assetCollectionResult, data](const AssetResult& result) -> async::task<UriResult>
    {
        Asset uploadAsset = result.GetAsset();
        uploadAsset.FileName = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(SpaceSystemHelpers::GetAssetFileExtension(data.GetMimeType()));
        uploadAsset.MimeType = data.GetMimeType();
        auto* assetSystem = SystemsManager::Get().GetAssetSystem();

        return assetSystem->UploadAssetDataEx(
            assetCollectionResult->GetAssetCollection(), uploadAsset, data, csp::common::CancellationToken::Dummy());
    };
}

std::function<async::task<UriResult>(const AssetResult& result)> SpaceSystem::UploadSpaceThumbnailAssetWithBuffer(
    const std::shared_ptr<AssetCollectionResult>& assetCollectionResult, const csp::systems::BufferAssetDataSource& data)
{
    return [assetCollectionResult, data](const AssetResult& result) -> async::task<UriResult>
    {
        Asset uploadAsset = result.GetAsset();
        uploadAsset.FileName = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(SpaceSystemHelpers::GetAssetFileExtension(data.GetMimeType()));
        uploadAsset.MimeType = data.GetMimeType();
        auto* assetSystem = SystemsManager::Get().GetAssetSystem();

        return assetSystem->UploadAssetDataEx(
            assetCollectionResult->GetAssetCollection(), uploadAsset, data, csp::common::CancellationToken::Dummy());
    };
}

std::function<async::task<UriResult>()> SpaceSystem::CreateAndUploadSpaceThumbnailToSpace(
    const std::shared_ptr<SpaceResult>& space, const csp::common::Optional<csp::systems::FileAssetDataSource>& data)
{
    return [space, data, this]() -> async::task<UriResult>
    {
        if (!data.HasValue())
        {
            // In the event the optional is null we still want to return success to continue the chain.
            return async::make_task(UriResult(EResultCode::Success, 200));
        }

        const auto spaceId = space->GetSpace().Id;
        auto thumbnailAssetCollection = std::make_shared<AssetCollectionResult>();

        // This task is designed to be a link in another continuation chain
        // It is not responsible for handling its own callbacks (success or failure). Instead,
        // it returns the task that allows the next task in the chain to be executed.
        return CreateSpaceThumbnailAssetCollection(space)
            .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(
                "SpaceSystem::CreateAndUploadSpaceThumbnailToSpace, successfully created space thumbnail asset collection.",
                "Failed to create space thumbnail asset collection.", {}, {}, {}))
            .then(systems::continuations::GetResultFromContinuation<AssetCollectionResult>(thumbnailAssetCollection))
            .then(CreateSpaceThumbnailAsset(space, thumbnailAssetCollection))
            .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetResult>(
                "SpaceSystem::CreateAndUploadSpaceThumbnailToSpace, successfully created space thumbnail asset.",
                "Failed to create space thumbnail asset.", {}, {}, {}))
            .then(UploadSpaceThumbnailAsset(thumbnailAssetCollection, *data))
            .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(
                "SpaceSystem::CreateAndUploadSpaceThumbnailToSpace, successfully upload space thumbnail asset.",
                "Failed to upload space thumbnail asset.", {}, {}, {}));
    };
}

std::function<async::task<UriResult>()> SpaceSystem::CreateAndUploadSpaceThumbnailWithBufferToSpace(
    const std::shared_ptr<SpaceResult>& space, const csp::systems::BufferAssetDataSource& data)
{
    return [space, data, this]() -> async::task<UriResult>
    {
        const auto spaceId = space->GetSpace().Id;
        auto thumbnailAssetCollection = std::make_shared<AssetCollectionResult>();

        // This task is designed to be a link in another continuation chain
        // It is not responsible for handling its own callbacks (success or failure). Instead,
        // it returns the task that allows the next task in the chain to be executed.
        return CreateSpaceThumbnailAssetCollection(space)
            .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(
                "SpaceSystem::CreateAndUploadSpaceThumbnailWithBufferToSpace, successfully created space thumbnail asset collection.",
                "Failed to create space thumbnail asset collection.", {}, {}, {}))
            .then(systems::continuations::GetResultFromContinuation<AssetCollectionResult>(thumbnailAssetCollection))
            .then(CreateSpaceThumbnailAsset(space, thumbnailAssetCollection))
            .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetResult>(
                "SpaceSystem::CreateAndUploadSpaceThumbnailWithBufferToSpace, successfully created space thumbnail asset.",
                "Failed to create space thumbnail asset.", {}, {}, {}))
            .then(UploadSpaceThumbnailAssetWithBuffer(thumbnailAssetCollection, data))
            .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(
                "SpaceSystem::CreateAndUploadSpaceThumbnailWithBufferToSpace, successfully upload space thumbnail asset.",
                "Failed to upload space thumbnail asset.", {}, {}, {}));
    };
}

std::function<async::task<NullResult>()> SpaceSystem::BulkInviteUsersToSpaceIfNeccesary(
    SpaceSystem* spaceSystem, const std::shared_ptr<SpaceResult>& space, const Optional<InviteUserRoleInfoCollection>& inviteUsers)
{
    return [spaceSystem, space, inviteUsers]() -> async::task<NullResult>
    {
        if (!inviteUsers.HasValue() || inviteUsers->InviteUserRoleInfos.IsEmpty())
        {
            // In the event the optional is null we still want to return success to continue the chain.
            return async::make_task(NullResult(EResultCode::Success, 200));
        }

        const auto spaceId = space->GetSpace().Id;

        // This task is designed to be a link in another continuation chain
        // It is not responsible for handling its own callbacks (success or failure). Instead,
        // it returns the task that allows the next task in the chain to be executed.
        return spaceSystem->BulkInviteToSpace(spaceId, *inviteUsers)
            .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(
                "SpaceSystem::BulkInviteUsersToSpace, successfully invited users to space.", "Failed to invite users to space.", {}, {}, {}));
    };
}

std::function<async::task<SpaceResult>(const SpaceResult& spaceResult)> SpaceSystem::RegisterScopesInSpace(
    csp::common::IRealtimeEngine* realtimeEngine)
{
    // Fow now, we only want to register the default scope to run scripts.
    // When we fully enable scope support, this will need to change.
    return [this, realtimeEngine](const SpaceResult& spaceResult)
    {
        // 1. Find all the scopes associated with the space.
        auto finishedGetScopeEvent = std::make_shared<async::event_task<csp::systems::SpaceResult>>();
        auto finishedGetScopeContinuation = finishedGetScopeEvent->get_task();

        this->m_multiplayerSystem->GetScopesBySpace(spaceResult.GetSpace().Id)
            .then(async::inline_scheduler(),
                [this, finishedGetScopeEvent, spaceResult, realtimeEngine](const csp::systems::ScopesResult& scopesResult)
                {
                    // 2. Ensure we have single default, auto-generated scope.
                    // This is done by checking for the global scope in the space, and that we only have 1 of these scopes, which is global.
                    // Right now, clients can't create scopes, so there should be a 0 chance of the above conditions failing.
                    const auto& scopes = scopesResult.GetScopes();

                    if (scopes.Size() < 1)
                    {
                        this->m_logSystem->LogMsg(
                            csp::common::LogLevel::Error, "SpaceSystem::RegisterScopesInSpace: Space doesn't have a scope.");
                        finishedGetScopeEvent->set_exception(std::make_exception_ptr(csp::common::continuations::ResultException(
                            "SpaceSystem::RegisterScopesInSpace: Space doesn't have a scope", MakeInvalid<csp::systems::SpaceResult>())));
                        return;
                    }

                    if (scopes.Size() > 1)
                    {
                        this->m_logSystem->LogMsg(
                            csp::common::LogLevel::Error, "SpaceSystem::RegisterScopesInSpace: Multiple scopes found. This version of CSP only supports spaces that have only a single global scope.");
                        finishedGetScopeEvent->set_exception(std::make_exception_ptr(csp::common::continuations::ResultException(
                            "SpaceSystem::RegisterScopesInSpace: Space has multiple scopes", MakeInvalid<csp::systems::SpaceResult>())));
                        return;
                    }

                    if (scopes[0].PubSubType != PubSubModelType::Global)
                    {
                        this->m_logSystem->LogMsg(csp::common::LogLevel::Error,
                            "SpaceSystem::RegisterScopesInSpace: Space doesn't contain a global scope.");
                        finishedGetScopeEvent->set_exception(std::make_exception_ptr(csp::common::continuations::ResultException(
                            "SpaceSystem::RegisterScopesInSpace: Space doesn't contain a global scope.", MakeInvalid<csp::systems::SpaceResult>())));
                        return;
                    }

                    // No need to check validity as the above conditions guarantee a valid iterator.
                    auto defaultScopeIt
                        = std::find_if(scopes.begin(), scopes.end(),
                        [](const Scope& scope) { return scope.PubSubType == PubSubModelType::Global; });

                    const bool managedLeaderElection = defaultScopeIt->ManagedLeaderElection;

                    // This will set server-side election to true if the scope has ManagedLeaderElection enabled. Otherwise it will default to client
                    // election.
                    static_cast<csp::multiplayer::OnlineRealtimeEngine*>(realtimeEngine)->SetServerSideElectionEnabled(managedLeaderElection);
                    // Start leader election
                    static_cast<csp::multiplayer::OnlineRealtimeEngine*>(realtimeEngine)->EnableLeaderElection();

                    // We don't want to register the scope and activate the server-side leader election system,
                    // if the scope doesn't have ManagedLeaderElection set to true.
                    if (managedLeaderElection == false)
                    {
                        finishedGetScopeEvent->set(spaceResult);
                        return;
                    }

                    // 3. We want to find the scope leader, and register this scope to the RealtimeEngine for use in leader election.
                    this->m_multiplayerSystem->GetScopeLeader(defaultScopeIt->Id,
                        [finishedGetScopeEvent, spaceResult, realtimeEngine](const ScopeLeaderResult& leaderResult)
                        {
                            if (leaderResult.GetResultCode() == EResultCode::InProgress)
                            {
                                return;
                            }

                             const ScopeLeader& leader = leaderResult.GetScopeLeader();

                            std::optional<uint64_t> leaderUserId
                                = (leader.ScopeClientId != 0) ? std::make_optional<uint64_t>(leader.ScopeClientId) : std::nullopt;

                            static_cast<csp::multiplayer::OnlineRealtimeEngine*>(realtimeEngine)
                                ->RegisterDefaultScope(leader.ScopeId.c_str(), leaderUserId);

                            finishedGetScopeEvent->set(spaceResult);
                        });
                });
        return finishedGetScopeContinuation;
    };
}

/* EnterSpace Continuations */
auto SpaceSystem::AddUserToSpaceIfNecessary(SpaceResultCallback callback, SpaceSystem& spaceSystem)
{
    return [callback, &spaceSystem](const SpaceResult& getSpaceResult)
    {
        CSP_LOG_MSG(csp::common::LogLevel::Log, "SpaceSystem::AddUserToSpaceIfNecessary");

        /* Once we have permissions to discover a space, attempt to enter it */
        const auto& spaceToJoin = getSpaceResult.GetSpace();

        const String userId = SystemsManager::Get().GetUserSystem()->GetLoginState().UserId;
        const bool joiningSpaceRequiresInvite = HasFlag(spaceToJoin.Attributes, SpaceAttributes::RequiresInvite);

        // The user is known to the space if they are a user, moderator or creator. This is important if the space requires an invite.
        const bool userIsRecognizedBySpace = spaceToJoin.UserIsKnownToSpace(userId);

        /* If we need permissions, check that the user has permission to enter this specific space */
        if (joiningSpaceRequiresInvite && !userIsRecognizedBySpace)
        {
            SpaceResult result(EResultCode::Failed, csp::web::EResponseCodes::ResponseForbidden, ERequestFailureReason::UserSpaceAccessDenied);
            systems::continuations::LogHTTPErrorAndCancelContinuation<SpaceResult>(
                "Logged in user does not have permission to join this space. Failed to add to space.", result);
        }

        /* By this point,you should be allowed to join the space
               Add the user to the space even if they are already added */

        auto userAddedToSpaceChainStartEvent = std::make_shared<async::event_task<SpaceResult>>();
        auto userAddedToSpaceChainContinuation = userAddedToSpaceChainStartEvent->get_task();
        // AddUserToSpace does not give a callback (feels like it should...) if the user is already added to the space.
        // Branch so we always continue, using an event so we can forward the continuation no matter what branch.
        if (!userIsRecognizedBySpace)
        {
            CSP_LOG_MSG(csp::common::LogLevel::Log, "Adding user to space.");

            // Use the request continuation to set the event ... to fire another continuation to allow continued chaining.
            spaceSystem.AddUserToSpace(getSpaceResult, userId)
                .then(async::inline_scheduler(),
                    [userAddedToSpaceChainStartEvent](const SpaceResult& addedToSpaceResult)
                    { userAddedToSpaceChainStartEvent->set(addedToSpaceResult); });
        }
        else
        {
            CSP_LOG_MSG(csp::common::LogLevel::Log, "No need to add user to space.");

            // Just pass along the previous result
            userAddedToSpaceChainStartEvent->set(getSpaceResult);
        }

        return userAddedToSpaceChainContinuation;
    };
}

auto SpaceSystem::FireEnterSpaceEvent(Space& outCurrentSpace)
{
    return [&outCurrentSpace](const SpaceResult& spaceResult)
    {
        CSP_LOG_MSG(csp::common::LogLevel::Log, "SpaceSystem::FireEnterSpaceEvent");

        /* We're here. The space knows about us. We're definately in the allowed users. Let's join! */
        csp::events::Event* enterSpaceEvent = csp::events::EventSystem::Get().AllocateEvent(csp::events::SPACESYSTEM_ENTER_SPACE_EVENT_ID);
        enterSpaceEvent->AddString("SpaceId", spaceResult.GetSpace().Id);
        csp::events::EventSystem::Get().EnqueueEvent(enterSpaceEvent);
        outCurrentSpace = spaceResult.GetSpace();
        return spaceResult;
    };
}

/*
 * ** EnterSpace Flow **
 * GetSpace
 * AssertRequestSuccessOrError (GetSpace Validation)
 * AddUserToSpaceIfNecessary
 * AssertRequestSuccessOrError (AddUserToSpace Validation)
 * FireEnterSpaceEvent
 * RefreshMultiplayerScopes
 * RegisterScopesInSpace
 * FetchAllEntitiesAndPopulateBuffers
 * AssertRequestSuccessOrErrorFromMultiplayerErrorCode (RefreshMultiplayerScopes Validation)
 * ReportSuccess
 * InvokeIfExceptionInChain (Handle any errors from the above Assert methods in chain, resets state)
 */
void SpaceSystem::EnterSpace(const String& spaceId, csp::common::IRealtimeEngine* realtimeEngine, SpaceResultCallback callback)
{
    if (realtimeEngine == nullptr)
    {
        CSP_LOG_MSG(csp::common::LogLevel::Error, "RealtimeEngine pointer passed to EnterSpace cannot be null");
        callback(SpaceResult(EResultCode::Failed, csp::web::EResponseCodes::ResponseBadRequest, ERequestFailureReason::None));
        return;
    }

    if (!realtimeEngine->GetEntityFetchCompleteCallback())
    {
        // Would be better if this were a type-invariant on RealtimeEngine, rather than a function precondition, but wrapper gen dosen't let us
        // pass callbacks in constructors.
        CSP_LOG_MSG(csp::common::LogLevel::Error,
            "Provided RealtimeEngine has a null EntityFetchCompleteCallback. Set one prior to calling EnterSpace by calling "
            "`SetSetEntityFetchCompleteCallback`");
        callback(SpaceResult(EResultCode::Failed, csp::web::EResponseCodes::ResponseBadRequest, ERequestFailureReason::None));
        return;
    }

    // It's invalid to enter an online space without a multiplayer connection
    if ((realtimeEngine->GetRealtimeEngineType() == csp::common::RealtimeEngineType::Online)
        && (!csp::systems::SystemsManager::Get().GetMultiplayerConnection()->IsConnected()))
    {
        CSP_LOG_MSG(csp::common::LogLevel::Error,
            "Cannot enter an online space without an established multiplayer connection. Did you create one when logging in?");
        callback(SpaceResult(EResultCode::Failed, csp::web::EResponseCodes::ResponseBadRequest, ERequestFailureReason::None));
        return;
    }

    // Hack alert. Not the best place to be doing this, but don't want to force the client to do this right this second, the api isn't strong
    // enough and it'll be too easy to get wrong. Will need to break this dependency. We do the opposite in ExitSpace because we need to null
    // the pointer, shared_ptrs and weak_ptrs would solve this entirely if they could be passed across the interface.
    if (realtimeEngine->GetRealtimeEngineType() == csp::common::RealtimeEngineType::Online)
    {
        csp::systems::SystemsManager::Get().GetMultiplayerConnection()->SetOnlineRealtimeEngine(
            static_cast<csp::multiplayer::OnlineRealtimeEngine*>(realtimeEngine));

        // This should start at false until we validate that the spaces default scope has ManagedLeaderElection enabled
        static_cast<csp::multiplayer::OnlineRealtimeEngine*>(realtimeEngine)->SetServerSideElectionEnabled(false);
    }

    CSP_LOG_MSG(csp::common::LogLevel::Log, "SpaceSystem::EnterSpace");

    // If online, get the space, add the user to it. If offline, create a local space and forward a local result through.
    auto upstreamConnectionTask = (realtimeEngine->GetRealtimeEngineType() == csp::common::RealtimeEngineType::Online)
        ? GetSpace(spaceId)
              .then(async::inline_scheduler(),
                  systems::continuations::AssertRequestSuccessOrErrorFromResult<SpaceResult>(
                      "SpaceSystem::EnterSpace, successfully discovered space.",
                      "Logged in user does not have permission to discover this space. Failed to enter space.", {}, {}, {}))
              .then(async::inline_scheduler(), AddUserToSpaceIfNecessary(callback, *this))
              .then(async::inline_scheduler(),
                  systems::continuations::AssertRequestSuccessOrErrorFromResult<SpaceResult>(
                      "SpaceSystem::EnterSpace, successfully added user to space (if not already added).",
                      "Failed to Enter Space. AddUserToSpace returned unexpected failure.", {}, {}, {}))
              .then(async::inline_scheduler(),
                  [realtimeEngine, spaceId](const SpaceResult& spaceResult)
                  {
                      /* Refresh the multiplayer connection to force the scopes to change
                      This is done in a nested continuation to prevent passing SpaceResult through all of the internal calls */
                      auto refreshMultiplayerConnectionEvent = std::make_shared<async::event_task<csp::systems::SpaceResult>>();
                      auto refreshMultiplayerConnectionContinuation = refreshMultiplayerConnectionEvent->get_task();

                      /* Investigate whether this needs to happen at all, it 's overwhelmingly complex... If you' re doing anything AOI,
                       * this probably wants rewritten or removed along with your work. */
                      static_cast<csp::multiplayer::OnlineRealtimeEngine*>(realtimeEngine)
                          ->RefreshMultiplayerConnectionToEnactScopeChange(spaceId)
                          .then(async::inline_scheduler(),
                              [refreshMultiplayerConnectionEvent, spaceResult]() { refreshMultiplayerConnectionEvent->set(spaceResult); });

                      return refreshMultiplayerConnectionContinuation;
                  })
              .then(async::inline_scheduler(), FireEnterSpaceEvent(m_currentSpace)) // Neccesary?
              .then(async::inline_scheduler(), this->RegisterScopesInSpace(realtimeEngine))
        : async::spawn(async::inline_scheduler(),
            [spaceId]()
            {
                // Offline, build a local space result
                CSP_LOG_MSG(csp::common::LogLevel::Log, "Entering Offline Space");

                Space localSpace {};

                /* Depending on how you think about this, you might think this is a bit of a bug.
                   Consider, you still need to login to use the API, and logging in generates you a user-id from MCS.
                   One might think we should be using that. The reason we don't is simply because we don't
                   store that ID in the system currently, and it dosen't really matter currently.
                   However this may be an improvement we want to make, although consider that it would get in the way
                   of any fully-offline flows we might to add. */
                csp::common::String localUser = std::to_string(csp::common::LocalClientID).c_str();

                localSpace.CreatedAt = DateTime::TimeNow().GetUtcString();
                localSpace.Name = "Offline Space";
                localSpace.Id = spaceId;
                localSpace.CreatedBy = localUser;
                localSpace.OwnerId = localUser;
                localSpace.UserIds = { localUser };
                localSpace.ModeratorIds = { localUser };

                SpaceResult localSpaceResult {};
                localSpaceResult.SetSpace(localSpace);
                localSpaceResult.SetResult(EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseOK));
                return localSpaceResult;
            }).then(async::inline_scheduler(), FireEnterSpaceEvent(m_currentSpace));

    // Whether we've done an upstream online connection or just a local one, finish entering the space
    upstreamConnectionTask
        .then(async::inline_scheduler(), FireEnterSpaceEvent(m_currentSpace)) // Neccesary?
        .then(async::inline_scheduler(),
            [realtimeEngine](const SpaceResult& spaceResult)
            {
                /* Because this is external api (RealtimeEngine) we use the callback for chaining, rather than a nicer interface.
                 * Need to make sure we've finished fetching all the entities before we move on
                 *
                 * The way this works is, in OnlineRealtimeEngine we do a bunch of nested callbacks to refresh scopes,
                 * they can't be synchronous because we need to wait until one callback finishes until we can call the next one.
                 * Once that's done, we're free to request entities from the backend. It is at this point, once we have fired the
                 * entity request, that the login is free to continue.
                 * However, more generally, there are two callback points, when we've made the request, AND, when the request is finished and
                 * all the entities are fetched. We internally care about the first in order to progress the EnterSpace flow, as you
                 * can enter a space before all the entities are fetched. Clients need to know about the latter however.
                 * Via this mechanism, a realtime engine can abstractly decide if its data fetch is synchronous or not, and whether to yield
                 * back to clients before or after all entities have been fetched.
                 *
                 * If we can get rid of this refreshScopes behaviour, this all gets extremely simpler.
                 */
                auto finishedFetchEntitySetupEvent = std::make_shared<async::event_task<csp::systems::SpaceResult>>();
                auto finishedFetchEntitySetupContinuation = finishedFetchEntitySetupEvent->get_task();

                // This is what fetches the data for the space, all the assets and whatnot. Creates the space entities in the realtime engine.
                realtimeEngine->FetchAllEntitiesAndPopulateBuffers(spaceResult.GetSpace().Id,
                    [finishedFetchEntitySetupEvent, resultCopy = spaceResult]()
                    { finishedFetchEntitySetupEvent->set(resultCopy); }); // Forward through the SpaceResult

                return finishedFetchEntitySetupContinuation;
            })
        .then(async::inline_scheduler(), systems::continuations::ReportSuccess(callback, "Successfully entered space."))
        .then(async::inline_scheduler(),
            csp::common::continuations::InvokeIfExceptionInChain(*csp::systems::SystemsManager::Get().GetLogSystem(),
                [callback, &currentSpace = m_currentSpace]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& except)
                {
                    currentSpace = {};
                    callback(MakeInvalid<SpaceResult>());
                }));
}

void SpaceSystem::ExitSpace(NullResultCallback callback)
{
    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "Exiting Space %s", m_currentSpace.Name.c_str());

    // As the user is exiting the space, we now clear all scopes that they are registered to.
    auto& systemsManager = systems::SystemsManager::Get();
    auto* multiplayerConnection = systemsManager.GetMultiplayerConnection();

    // If not connected, do not attempt to disconnect
    if ((multiplayerConnection != nullptr) && (multiplayerConnection->IsConnected()))
    {
        multiplayerConnection->StopListening()
            .then(multiplayer::continuations::UnwrapSignalRResultOrThrow<false>())
            .then(async::inline_scheduler(), [multiplayerConnection]() { return multiplayerConnection->ResetScopes(); })
            .then(multiplayer::continuations::UnwrapSignalRResultOrThrow<false>())
            .then(async::inline_scheduler(), [multiplayerConnection]() { multiplayerConnection->SetOnlineRealtimeEngine(nullptr); })
            .then(systems::continuations::ReportSuccess(callback, "Successfully exited space."))
            .then(common::continuations::InvokeIfExceptionInChain(*m_logSystem,
                [callback, multiplayerConnection]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
                {
                    csp::systems::SystemsManager::Get().GetLogSystem()->LogMsg(
                        csp::common::LogLevel::Fatal, fmt::format("Error on exiting spaces: {}", exception.what()).c_str());

                    // Null the realtime engine pointer in the multiplayer connection such that it stops dispatching signalR updates.
                    // (Error paths are messy, what does failing to leave a space mean memory wise? This is why owned types with RAII work so
                    // much better).
                    multiplayerConnection->SetOnlineRealtimeEngine(nullptr);
                    callback(MakeInvalid<NullResult>());
                }));
    }
    else
    {
        csp::systems::SystemsManager::Get().GetLogSystem()->LogMsg(
            csp::common::LogLevel::Verbose, "Multiplayer connection not connected when exiting space, skipping disconnect.");

        const NullResult result(EResultCode::Success, 200);
        INVOKE_IF_NOT_NULL(callback, result);
    }

    events::Event* exitSpaceEvent = events::EventSystem::Get().AllocateEvent(events::SPACESYSTEM_EXIT_SPACE_EVENT_ID);
    exitSpaceEvent->AddString("SpaceId", m_currentSpace.Id);

    events::EventSystem::Get().EnqueueEvent(exitSpaceEvent);

    m_currentSpace = Space();
}

bool SpaceSystem::IsInSpace() { return !m_currentSpace.Id.IsEmpty(); }

const Space& SpaceSystem::GetCurrentSpace() const { return m_currentSpace; }

/*
 * ** CreateSpace Flow **
 * CreateSpace
 * AssertRequestSuccessOrError (CreateSpace Validation)
 * CreateSpaceMetadataAssetCollection
 * AssertRequestSuccessOrError (CreateSpaceMetadataAssetCollection Validation)
 * CreateAndUploadSpaceThumbnailToSpace
 * AssertRequestSuccessOrError (CreateAndUploadSpaceThumbnailToSpace Validation)
 * BulkInviteUsersToSpaceIfNeccesary
 * AssertRequestSuccessOrErrorFromResult (BulkInviteUsersToSpaceIfNeccesary Validation)
 * Promotes the created space through the SpaceResultCallback
 * InvokeIfExceptionInChain (Handle any errors from the above Assert methods in chain, rolls back partial state)
 */
void SpaceSystem::CreateSpace(const String& name, const String& description, SpaceAttributes attributes,
    const Optional<InviteUserRoleInfoCollection>& inviteUsers, const Map<String, String>& metadata, const Optional<FileAssetDataSource>& thumbnail,
    const Optional<Array<String>>& tags, SpaceResultCallback callback)
{
    auto currentSpaceResult = std::make_shared<SpaceResult>();

    CreateSpaceGroupInfo(name, description, attributes, tags)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<SpaceResult>(
            "SpaceSystem::CreateSpace, successfully created space.", "Failed to create space.", {}, {}, {}))
        .then(systems::continuations::GetResultFromContinuation<SpaceResult>(currentSpaceResult))
        .then(CreateSpaceMetadataAssetCollection(currentSpaceResult, metadata))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(
            "SpaceSystem::CreateSpace, successfully created space metadata asset collection.", "Failed to create space metadata asset collection.",
            {}, {}, {}))
        .then(CreateAndUploadSpaceThumbnailToSpace(currentSpaceResult, thumbnail))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(
            "SpaceSystem::CreateSpace, successfully created thumbnail.", "Failed to create thumbnail.", {}, {}, {}))
        .then(BulkInviteUsersToSpaceIfNeccesary(this, currentSpaceResult, inviteUsers))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(
            "SpaceSystem::CreateSpace, successfully invited users to space.", "Failed to invite users to space.", {}, {}, {}))
        .then(
            [currentSpaceResult, callback]()
            {
                CSP_LOG_MSG(csp::common::LogLevel::Log,
                    csp::common::StringFormat("Successfully created space: %s", static_cast<const char*>(currentSpaceResult->GetSpace().Name)));

                INVOKE_IF_NOT_NULL(callback, *currentSpaceResult);
            })
        .then(csp::common::continuations::InvokeIfExceptionInChain(
            *csp::systems::SystemsManager::Get().GetLogSystem(),
            [this, currentSpaceResult, callback]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
            {
                auto nullResultCallback
                    = [](const csp::systems::NullResult& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; };

                this->DeleteSpace(currentSpaceResult->GetSpace().Id, nullResultCallback);

                callback(csp::common::continuations::GetResultExceptionOrInvalid<SpaceResult>(exception));
            },
            [this, currentSpaceResult, callback]([[maybe_unused]] const std::exception& exception)
            {
                auto nullResultCallback
                    = [](const csp::systems::NullResult& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; };

                this->DeleteSpace(currentSpaceResult->GetSpace().Id, nullResultCallback);

                callback(MakeInvalid<SpaceResult>());
            }));
}

/*
 * ** CreateSpaceWithBuffer Flow **
 * CreateSpace
 * AssertRequestSuccessOrError (CreateSpace Validation)
 * CreateSpaceMetadataAssetCollection
 * AssertRequestSuccessOrError (CreateSpaceMetadataAssetCollection Validation)
 * CreateAndUploadSpaceThumbnailWithBufferToSpace
 * AssertRequestSuccessOrError (CreateAndUploadSpaceThumbnailWithBufferToSpace Validation)
 * BulkInviteUsersToSpaceIfNeccesary
 * AssertRequestSuccessOrErrorFromResult (BulkInviteUsersToSpaceIfNeccesary Validation)
 * Promotes the created space through the SpaceResultCallback
 * InvokeIfExceptionInChain (Handle any errors from the above Assert methods in chain, rolls back partial state)
 */
void SpaceSystem::CreateSpaceWithBuffer(const String& name, const String& description, SpaceAttributes attributes,
    const Optional<InviteUserRoleInfoCollection>& inviteUsers, const Map<String, String>& metadata, const BufferAssetDataSource& thumbnail,
    const Optional<Array<String>>& tags, SpaceResultCallback callback)
{
    auto currentSpaceResult = std::make_shared<SpaceResult>();

    CreateSpaceGroupInfo(name, description, attributes, tags)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<SpaceResult>(
            "SpaceSystem::CreateSpaceWithBuffer, successfully created space.", "Failed to create space.", {}, {}, {}))
        .then(systems::continuations::GetResultFromContinuation<SpaceResult>(currentSpaceResult))
        .then(CreateSpaceMetadataAssetCollection(currentSpaceResult, metadata))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(
            "SpaceSystem::CreateSpaceWithBuffer, successfully created space metadata asset collection.",
            "Failed to create space metadata asset collection.", {}, {}, {}))
        .then(CreateAndUploadSpaceThumbnailWithBufferToSpace(currentSpaceResult, thumbnail))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(
            "SpaceSystem::CreateSpaceWithBuffer, successfully created thumbnail.", "Failed to create thumbnail.", {}, {}, {}))
        .then(BulkInviteUsersToSpaceIfNeccesary(this, currentSpaceResult, inviteUsers))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(
            "SpaceSystem::CreateSpaceWithBuffer, successfully invited users to space.", "Failed to invite users to space.", {}, {}, {}))
        .then(
            [currentSpaceResult, callback]()
            {
                CSP_LOG_MSG(csp::common::LogLevel::Log,
                    csp::common::StringFormat("Successfully created space: %s", static_cast<const char*>(currentSpaceResult->GetSpace().Name)));

                INVOKE_IF_NOT_NULL(callback, *currentSpaceResult);
            })
        .then(csp::common::continuations::InvokeIfExceptionInChain(
            *csp::systems::SystemsManager::Get().GetLogSystem(),
            [this, currentSpaceResult, callback]([[maybe_unused]] const csp::common::continuations::ExpectedExceptionBase& exception)
            {
                auto nullResultCallback
                    = [](const csp::systems::NullResult& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; };

                this->DeleteSpace(currentSpaceResult->GetSpace().Id, nullResultCallback);

                callback(csp::common::continuations::GetResultExceptionOrInvalid<SpaceResult>(exception));
            },
            [this, currentSpaceResult, callback]([[maybe_unused]] const std::exception& exception)
            {
                auto nullResultCallback
                    = [](const csp::systems::NullResult& result) { return result.GetResultCode() != csp::systems::EResultCode::InProgress; };

                this->DeleteSpace(currentSpaceResult->GetSpace().Id, nullResultCallback);

                callback(MakeInvalid<SpaceResult>());
            }));
}

void SpaceSystem::UpdateSpace(const String& spaceId, const Optional<String>& name, const Optional<String>& description,
    const Optional<SpaceAttributes>& attributes, const csp::common::Optional<csp::common::Array<csp::common::String>>& tags,
    BasicSpaceResultCallback callback)
{
    CSP_PROFILE_SCOPED();

    auto liteGroupInfo = std::make_shared<chs::GroupLiteDto>();

    if (name.HasValue())
    {
        liteGroupInfo->SetName(*name);
    }

    if (description.HasValue())
    {
        liteGroupInfo->SetDescription(*description);
    }

    bool isDiscoverable = false;
    bool requiresInvite = true;

    if (attributes.HasValue())
    {
        isDiscoverable = HasFlag(*attributes, SpaceAttributes::IsDiscoverable);
        requiresInvite = HasFlag(*attributes, SpaceAttributes::RequiresInvite);
    }

    if (tags.HasValue())
    {
        liteGroupInfo->SetTags(csp::common::Convert(tags).value());
    }

    // Note that these are required fields from a services point of view.
    liteGroupInfo->SetDiscoverable(isDiscoverable);
    liteGroupInfo->SetRequiresInvite(requiresInvite);
    liteGroupInfo->SetAutoModerator(false);

    csp::services::ResponseHandlerPtr responseHandler
        = m_groupApi->CreateHandler<BasicSpaceResultCallback, BasicSpaceResult, void, chs::GroupLiteDto>(callback, nullptr);

    static_cast<chs::GroupApi*>(m_groupApi)->groupsGroupIdLitePut({ spaceId, liteGroupInfo }, responseHandler);
}

void SpaceSystem::DeleteSpace(const csp::common::String& spaceId, NullResultCallback callback)
{
    CSP_PROFILE_SCOPED();

    csp::services::ResponseHandlerPtr responseHandler = m_groupApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chsaggregation::SpaceApi*>(m_spaceApi)->spacesSpaceIdDelete({ spaceId }, responseHandler);
}

void SpaceSystem::GetSpaces(SpacesResultCallback callback)
{
    const String inUserId = m_userSystem->GetLoginState().UserId;

    csp::services::ResponseHandlerPtr responseHandler
        = m_groupApi->CreateHandler<SpacesResultCallback, SpacesResult, void, csp::services::DtoArray<chs::GroupDto>>(callback, nullptr);

    static_cast<chs::GroupApi*>(m_groupApi)->usersUserIdGroupsGet({ inUserId }, responseHandler);
}

void SpaceSystem::GetSpacesByAttributes(const Optional<bool>& inIsDiscoverable, const Optional<bool>& inIsArchived,
    const Optional<bool>& inRequiresInvite, const Optional<int>& inResultsSkip, const Optional<int>& inResultsMax,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& mustContainTags,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& mustExcludeTags, const csp::common::Optional<bool>& inMustIncludeAllTags,
    BasicSpacesResultCallback callback)
{
    auto isDiscoverable = inIsDiscoverable.HasValue() ? *inIsDiscoverable : std::optional<bool>(std::nullopt);
    auto isArchived = inIsArchived.HasValue() ? *inIsArchived : std::optional<bool>(std::nullopt);
    auto requiresInvite = inRequiresInvite.HasValue() ? *inRequiresInvite : std::optional<bool>(std::nullopt);
    auto resultsSkip = inResultsSkip.HasValue() ? *inResultsSkip : std::optional<int32_t>(std::nullopt);
    std::optional<int32_t> resultsMax = inResultsMax.HasValue() ? std::min(MAX_SPACES_RESULTS, *inResultsMax) : MAX_SPACES_RESULTS;

    if (inResultsMax.HasValue() && *inResultsMax > MAX_SPACES_RESULTS)
    {
        CSP_LOG_WARN_FORMAT("Provided value `%i` for ResultsMax exceeded max value and was reduced to `%i`.", *inResultsMax, MAX_SPACES_RESULTS);
    }

    auto tags
        = mustContainTags.HasValue() ? csp::common::Convert(mustContainTags).value() : std::optional<std::vector<csp::common::String>>(std::nullopt);
    auto excludedTags
        = mustExcludeTags.HasValue() ? csp::common::Convert(mustExcludeTags).value() : std::optional<std::vector<csp::common::String>>(std::nullopt);
    auto mustIncludeAllTags = inMustIncludeAllTags.HasValue() ? *inMustIncludeAllTags : std::optional<bool>(std::nullopt);

    csp::services::ResponseHandlerPtr responseHandler
        = m_groupApi->CreateHandler<BasicSpacesResultCallback, BasicSpacesResult, void, csp::services::DtoArray<chs::GroupLiteDto>>(callback, nullptr);

    static_cast<chs::GroupApi*>(m_groupApi)->groupsLiteGet(
        {
            std::nullopt, // Ids
            std::nullopt, // GroupTypes
            std::nullopt, // Names
            std::nullopt, // PartialName
            std::nullopt, // GroupOwnerIds
            std::nullopt, // ExcludeGroupOwnerIds
            std::nullopt, // ExcludeIds
            std::nullopt, // Users
            isDiscoverable, // Discoverable
            std::nullopt, // AutoModerator
            requiresInvite, // RequiresInvite
            isArchived, // Archived
            std::nullopt, // OrganizationIds (no longer used)
            tags, // Tags
            excludedTags, // ExcludedTags
            mustIncludeAllTags, // TagsAll
            resultsSkip, // Skip
            resultsMax // Limit
        },
        responseHandler);
}

void SpaceSystem::GetSpacesByIds(const Array<String>& requestedSpaceIDs, SpacesResultCallback callback)
{
    if (requestedSpaceIDs.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("No space ids given");

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<SpacesResult>());

        return;
    }

    std::vector<String> spaceIds;
    spaceIds.reserve(requestedSpaceIDs.Size());

    for (size_t idx = 0; idx < requestedSpaceIDs.Size(); ++idx)
    {
        spaceIds.push_back(requestedSpaceIDs[idx]);
    }

    csp::services::ResponseHandlerPtr responseHandler
        = m_groupApi->CreateHandler<SpacesResultCallback, SpacesResult, void, csp::services::DtoArray<chs::GroupDto>>(callback, nullptr);

    static_cast<chs::GroupApi*>(m_groupApi)->groupsGet({ spaceIds }, responseHandler);
}

void SpaceSystem::GetSpacesForUserId(const String& userId, SpacesResultCallback callback)
{
    const String inUserId = userId;

    csp::services::ResponseHandlerPtr responseHandler
        = m_groupApi->CreateHandler<SpacesResultCallback, SpacesResult, void, csp::services::DtoArray<chs::GroupDto>>(callback, nullptr);

    static_cast<chs::GroupApi*>(m_groupApi)->usersUserIdGroupsGet({ userId }, responseHandler);
}

void SpaceSystem::GetSpace(const String& spaceId, SpaceResultCallback callback)
{
    if (spaceId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("No space id given");
        INVOKE_IF_NOT_NULL(callback, MakeInvalid<SpaceResult>());
        return;
    }

    csp::services::ResponseHandlerPtr responseHandler
        = m_groupApi->CreateHandler<SpaceResultCallback, SpaceResult, void, chs::GroupDto>(callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs::GroupApi*>(m_groupApi)->groupsGroupIdGet({ spaceId }, responseHandler);
}

async::task<SpaceResult> SpaceSystem::GetSpace(const String& spaceId)
{
    async::event_task<SpaceResult> onCompleteEvent;
    async::task<SpaceResult> onCompleteTask = onCompleteEvent.get_task();

    if (spaceId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("No space id given");
        onCompleteEvent.set_exception(std::make_exception_ptr(std::exception()));
        return onCompleteTask;
    }

    csp::services::ResponseHandlerPtr responseHandler = m_groupApi->CreateHandler<SpaceResultCallback, SpaceResult, void, chs::GroupDto>(
        [](const SpaceResult&) {}, nullptr, csp::web::EResponseCodes::ResponseOK, std::move(onCompleteEvent));

    static_cast<chs::GroupApi*>(m_groupApi)->groupsGroupIdGet({ spaceId }, responseHandler);

    return onCompleteTask;
}

void SpaceSystem::InviteToSpace(const csp::common::String& spaceId, const String& email, const Optional<bool>& isModeratorRole,
    const Optional<String>& emailLinkUrl, const Optional<String>& signupUrl, NullResultCallback callback)
{
    auto groupInviteInfo = std::make_shared<chs::GroupInviteDto>();
    groupInviteInfo->SetEmail(email);

    if (isModeratorRole.HasValue())
    {
        groupInviteInfo->SetAsModerator(*isModeratorRole);
    }

    auto emailLinkUrlParam = emailLinkUrl.HasValue() && !emailLinkUrl->IsEmpty() ? (*emailLinkUrl) : std::optional<String>(std::nullopt);
    auto signupUrlParam = signupUrl.HasValue() && !signupUrl->IsEmpty() ? (*signupUrl) : std::optional<String>(std::nullopt);

    csp::services::ResponseHandlerPtr responseHandler = m_groupApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs::GroupApi*>(m_groupApi)->groupsGroupIdEmail_invitesPost(
        { spaceId, std::nullopt, emailLinkUrlParam, signupUrlParam, groupInviteInfo }, responseHandler);
}

void SpaceSystem::BulkInviteToSpace(const String& spaceId, const InviteUserRoleInfoCollection& inviteUsers, NullResultCallback callback)
{
    std::vector<std::shared_ptr<chs::GroupInviteDto>> groupInvites
        = systems::SpaceSystemHelpers::GenerateGroupInvites(inviteUsers.InviteUserRoleInfos);

    auto emailLinkUrlParam = !inviteUsers.EmailLinkUrl.IsEmpty() ? (inviteUsers.EmailLinkUrl) : std::optional<String>(std::nullopt);
    auto signupUrlParam = !inviteUsers.SignupUrl.IsEmpty() ? (inviteUsers.SignupUrl) : std::optional<String>(std::nullopt);

    csp::services::ResponseHandlerPtr responseHandler = m_groupApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs::GroupApi*>(m_groupApi)->groupsGroupIdEmail_invitesBulkPost(
        { spaceId, std::nullopt, emailLinkUrlParam, signupUrlParam, groupInvites }, responseHandler);
}

async::task<NullResult> SpaceSystem::BulkInviteToSpace(const csp::common::String& spaceId, const InviteUserRoleInfoCollection& inviteUsers)
{
    async::event_task<NullResult> onCompleteEvent;
    async::task<NullResult> onCompleteTask = onCompleteEvent.get_task();

    std::vector<std::shared_ptr<chs::GroupInviteDto>> groupInvites
        = systems::SpaceSystemHelpers::GenerateGroupInvites(inviteUsers.InviteUserRoleInfos);

    auto emailLinkUrlParam = !inviteUsers.EmailLinkUrl.IsEmpty() ? (inviteUsers.EmailLinkUrl) : std::optional<String>(std::nullopt);
    auto signupUrlParam = !inviteUsers.SignupUrl.IsEmpty() ? (inviteUsers.SignupUrl) : std::optional<String>(std::nullopt);

    csp::services::ResponseHandlerPtr responseHandler = m_groupApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        [](const NullResult&) {}, nullptr, csp::web::EResponseCodes::ResponseNoContent, std::move(onCompleteEvent));

    static_cast<chs::GroupApi*>(m_groupApi)->groupsGroupIdEmail_invitesBulkPost(
        { spaceId, std::nullopt, emailLinkUrlParam, signupUrlParam, groupInvites }, responseHandler);

    return onCompleteTask;
}

void SpaceSystem::GetPendingUserInvites(const String& spaceId, PendingInvitesResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_groupApi->CreateHandler<PendingInvitesResultCallback, PendingInvitesResult, void, csp::services::DtoArray<chs::GroupInviteDto>>(
            callback, nullptr);

    static_cast<chs::GroupApi*>(m_groupApi)->groupsGroupIdEmail_invitesGet({ spaceId }, responseHandler);
}

void SpaceSystem::GetAcceptedUserInvites(const String& spaceId, AcceptedInvitesResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_groupApi->CreateHandler<AcceptedInvitesResultCallback, AcceptedInvitesResult, void, csp::services::DtoArray<chs::GroupInviteDto>>(
            callback, nullptr);

    static_cast<chs::GroupApi*>(m_groupApi)->groupsGroupIdEmail_invitesAcceptedGet({ spaceId }, responseHandler);
}

void SpaceSystem::AddUserToSpace(const csp::common::String& spaceId, const String& userId, SpaceResultCallback callback)
{
    // This function right here is the only place in the whole of CSP that needs to use group code.
    // So, rather than bloat our `Space` class with the property, and give clients something that they have zero use for,
    // we prefer to pay the cost of an additional call to the cloud in the one place we need it, in order to retrieve it.
    // This function is not expected to be on any hot code path, so the perf cost is expected to be low. It's worth it for the api quality.

    GetSpace(spaceId,
        [userId, callback, this](const SpaceResult& result)
        {
            if (result.GetResultCode() == EResultCode::InProgress)
            {
                return;
            }

            if (result.GetResultCode() == EResultCode::Failed)
            {
                INVOKE_IF_NOT_NULL(callback, result);

                return;
            }

            const csp::common::String& spaceCode = result.GetSpaceCode();

            csp::services::ResponseHandlerPtr responseHandler
                = m_groupApi->CreateHandler<SpaceResultCallback, SpaceResult, void, chs::GroupDto>(callback, nullptr);

            static_cast<chs::GroupApi*>(m_groupApi)->group_codesGroupCodeUsersUserIdPut({ spaceCode, userId }, responseHandler);
        });
}

async::task<SpaceResult> SpaceSystem::AddUserToSpace(const SpaceResult& result, const String& userId)
{
    // Because we react in a continuation, we need to keep the event alive, hence shared ptr.
    std::shared_ptr<async::event_task<SpaceResult>> onCompleteEvent = std::make_shared<async::event_task<SpaceResult>>();
    async::task<SpaceResult> onCompleteTask = onCompleteEvent->get_task();

    const csp::common::String& spaceCode = result.GetSpaceCode();

    csp::services::ResponseHandlerPtr responseHandler = m_groupApi->CreateHandler<SpaceResultCallback, SpaceResult, void, chs::GroupDto>(
        [](const SpaceResult&) {}, nullptr, csp::web::EResponseCodes::ResponseOK, std::move(*onCompleteEvent.get()));

    static_cast<chs::GroupApi*>(m_groupApi)->group_codesGroupCodeUsersUserIdPut({ spaceCode, userId }, responseHandler);

    return onCompleteTask;
}

void SpaceSystem::RemoveUserFromSpace(const String& spaceId, const String& userId, NullResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_groupApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(callback, nullptr);

    static_cast<chs::GroupApi*>(m_groupApi)->groupsGroupIdUsersUserIdDelete({ spaceId, userId }, responseHandler);
}

void SpaceSystem::AddSiteInfo(const String& spaceId, Site& siteInfo, SiteResultCallback callback)
{
    auto& systemsManager = SystemsManager::Get();
    auto* poiInternalSystem = static_cast<PointOfInterestInternalSystem*>(systemsManager.GetPointOfInterestSystem());

    siteInfo.SpaceId = spaceId;
    poiInternalSystem->CreateSite(siteInfo, callback);
}

void SpaceSystem::RemoveSiteInfo(const String& spaceId, Site& siteInfo, NullResultCallback callback)
{
    auto& systemsManager = SystemsManager::Get();
    auto* poiInternalSystem = static_cast<PointOfInterestInternalSystem*>(systemsManager.GetPointOfInterestSystem());

    siteInfo.SpaceId = spaceId;
    poiInternalSystem->DeleteSite(siteInfo, callback);
}

void SpaceSystem::GetSitesInfo(const String& spaceId, SitesCollectionResultCallback callback)
{
    auto& systemsManager = SystemsManager::Get();
    auto* poiInternalSystem = static_cast<PointOfInterestInternalSystem*>(systemsManager.GetPointOfInterestSystem());

    poiInternalSystem->GetSites(spaceId, callback);
}

void SpaceSystem::UpdateUserRole(const String& spaceId, const UserRoleInfo& newUserRoleInfo, NullResultCallback callback)
{
    const auto newUserRole = newUserRoleInfo.UserRole;
    const auto& userId = newUserRoleInfo.UserId;

    if (newUserRole == SpaceUserRole::Owner)
    {
        csp::services::ResponseHandlerPtr responseHandler
            = m_groupApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(callback, nullptr);

        static_cast<chs::GroupApi*>(m_groupApi)->groupsGroupIdOwnerNewGroupOwnerIdPut({ spaceId, userId }, responseHandler);
    }
    else if (newUserRole == SpaceUserRole::Moderator)
    {
        csp::services::ResponseHandlerPtr responseHandler = m_groupApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

        static_cast<chs::GroupApi*>(m_groupApi)->groupsGroupIdModeratorsUserIdPut({ spaceId, userId }, responseHandler);
    }
    else if (newUserRole == SpaceUserRole::User)
    {
        // TODO: When the Client will be able to change the space owner role get a fresh Space object to see if the NewUserRoleInfo.UserId is
        // still a space owner
        if (spaceId == newUserRoleInfo.UserId)
        {
            // An owner must firstly pass the space ownership to someone else before it can become a user
            INVOKE_IF_NOT_NULL(callback, MakeInvalid<NullResult>());

            return;
        }

        csp::services::ResponseHandlerPtr responseHandler = m_groupApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
            callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

        static_cast<chs::GroupApi*>(m_groupApi)->groupsGroupIdModeratorsUserIdDelete({ spaceId, userId }, responseHandler);
    }
    else
    {
        CSP_LOG_ERROR_MSG("SpaceSystem::UpdateUserRole failed: Unsupported User Role!");

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<NullResult>());
    }
}

void SpaceSystem::GetUsersRoles(const String& spaceId, const Array<String>& requestedUserIds, UserRoleCollectionCallback callback)
{
    SpaceResultCallback getSpaceCallback = [callback, requestedUserIds](const SpaceResult& spaceResult)
    {
        if (spaceResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        UserRoleCollectionResult internalResult(spaceResult.GetResultCode(), spaceResult.GetHttpResultCode());

        if (spaceResult.GetResultCode() == EResultCode::Success)
        {
            auto& space = spaceResult.GetSpace();
            internalResult.FillUsersRoles(space, requestedUserIds);
        }

        INVOKE_IF_NOT_NULL(callback, internalResult);
    };

    GetSpace(spaceId, getSpaceCallback);
}

void SpaceSystem::UpdateSpaceMetadata(const String& spaceId, const Map<String, String>& newMetadata, NullResultCallback callback)
{
    if (spaceId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UpdateSpaceMetadata called with empty SpaceId. Aborting call.");

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<NullResult>());

        return;
    }

    AssetCollectionResultCallback metadataAssetCollCallback = [callback, newMetadata](const AssetCollectionResult& result)
    {
        if (result.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        auto assetSystem = SystemsManager::Get().GetAssetSystem();

        if (result.GetResultCode() == EResultCode::Failed)
        {
            NullResult internalResult(result);
            INVOKE_IF_NOT_NULL(callback, internalResult);

            return;
        }

        AssetCollectionResultCallback updateAssetCollCallback = [callback](const AssetCollectionResult& result)
        {
            NullResult internalResult(result);
            INVOKE_IF_NOT_NULL(callback, internalResult);
        };

        const auto& assetCollection = result.GetAssetCollection();
        assetSystem->UpdateAssetCollectionMetadata(assetCollection, newMetadata, {}, updateAssetCollCallback);
    };

    GetMetadataAssetCollection(spaceId, metadataAssetCollCallback);
}

void SpaceSystem::GetSpacesMetadata(const Array<String>& spaceIds, SpacesMetadataResultCallback callback)
{
    AssetCollectionsResultCallback metadataAssetCollCallback = [callback](const AssetCollectionsResult& result)
    {
        SpacesMetadataResult internalResult(result.GetResultCode(), result.GetHttpResultCode());

        if (result.GetResultCode() == EResultCode::Success)
        {
            Map<String, Map<String, String>> spacesMetadata;
            Map<String, Array<String>> spacesTags;
            const auto& assetCollections = result.GetAssetCollections();

            for (size_t i = 0; i < assetCollections.Size(); ++i)
            {
                const auto& assetCollection = assetCollections[i];

                auto spaceId = SpaceSystemHelpers::GetSpaceIdFromMetadataAssetCollectionName(assetCollection.Name);

                spacesMetadata[spaceId] = systems::SpaceSystemHelpers::LegacyAssetConversion(assetCollection);
            }

            internalResult.SetMetadata(spacesMetadata);
            internalResult.SetTags(spacesTags);
        }

        INVOKE_IF_NOT_NULL(callback, internalResult);
    };

    GetMetadataAssetCollections(spaceIds, metadataAssetCollCallback);
}

void SpaceSystem::GetSpaceMetadata(const String& spaceId, SpaceMetadataResultCallback callback)
{
    if (spaceId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("GetSpaceMetadata called with empty SpaceId. Aborting call.");

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<SpaceMetadataResult>());

        return;
    }

    AssetCollectionResultCallback metadataAssetCollCallback = [callback](const AssetCollectionResult& result)
    {
        SpaceMetadataResult internalResult(result.GetResultCode(), result.GetHttpResultCode());

        if (result.GetResultCode() == EResultCode::Success)
        {
            const auto& assetCollection = result.GetAssetCollection();

            internalResult.SetMetadata(systems::SpaceSystemHelpers::LegacyAssetConversion(assetCollection));
        }

        INVOKE_IF_NOT_NULL(callback, internalResult);
    };

    GetMetadataAssetCollection(spaceId, metadataAssetCollCallback);
}

void SpaceSystem::UpdateSpaceThumbnail(const String& spaceId, const FileAssetDataSource& newThumbnail, NullResultCallback callback)
{
    AssetCollectionsResultCallback thumbnailAssetCollCallback = [callback, spaceId, newThumbnail, this](const AssetCollectionsResult& assetCollResult)
    {
        if (assetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (assetCollResult.GetResultCode() == EResultCode::Failed)
        {
            NullResult internalResult(assetCollResult);
            INVOKE_IF_NOT_NULL(callback, internalResult);

            return;
        }

        const auto& assetCollections = assetCollResult.GetAssetCollections();

        if (assetCollections.IsEmpty())
        {
            // space without a thumbnail
            AddSpaceThumbnail(spaceId, newThumbnail, callback);

            return;
        }

        const auto& thumbnailAssetCollection = assetCollections[0];

        AssetsResultCallback thumbnailAssetCallback = [callback, newThumbnail, thumbnailAssetCollection](const AssetsResult& assetsResult)
        {
            if (assetsResult.GetResultCode() == EResultCode::InProgress)
            {
                return;
            }

            if (assetsResult.GetResultCode() == EResultCode::Failed)
            {
                NullResult internalResult(assetsResult);
                INVOKE_IF_NOT_NULL(callback, internalResult);

                return;
            }

            UriResultCallback uploadCallback = [callback](const UriResult& uploadResult)
            {
                if (uploadResult.GetResultCode() == EResultCode::Failed)
                {
                    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
                        (int)uploadResult.GetResultCode(), uploadResult.GetHttpResultCode());
                }

                NullResult internalResult(uploadResult);
                INVOKE_IF_NOT_NULL(callback, internalResult);
            };

            auto& thumbnailAsset = ((csp::systems::AssetsResult&)assetsResult).GetAssets()[0];
            thumbnailAsset.MimeType = newThumbnail.GetMimeType();

            auto* assetSystem = SystemsManager::Get().GetAssetSystem();
            assetSystem->UploadAssetData(thumbnailAssetCollection, thumbnailAsset, newThumbnail, uploadCallback);
        };

        GetSpaceThumbnailAsset(thumbnailAssetCollection, thumbnailAssetCallback);
    };

    GetSpaceThumbnailAssetCollection(spaceId, thumbnailAssetCollCallback);
}

void SpaceSystem::UpdateSpaceThumbnailWithBuffer(const String& spaceId, const BufferAssetDataSource& newThumbnail, NullResultCallback callback)
{
    AssetCollectionsResultCallback thumbnailAssetCollCallback = [callback, spaceId, newThumbnail, this](const AssetCollectionsResult& assetCollResult)
    {
        if (assetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (assetCollResult.GetResultCode() == EResultCode::Failed)
        {
            const NullResult internalResult(assetCollResult);
            INVOKE_IF_NOT_NULL(callback, internalResult);

            return;
        }

        const auto& assetCollections = assetCollResult.GetAssetCollections();

        if (assetCollections.IsEmpty())
        {
            // Space without a thumbnail
            AddSpaceThumbnailWithBuffer(spaceId, newThumbnail, callback);

            return;
        }

        const auto& thumbnailAssetCollection = assetCollections[0];

        AssetsResultCallback thumbnailAssetCallback = [callback, newThumbnail, thumbnailAssetCollection](const AssetsResult& assetsResult)
        {
            if (assetsResult.GetResultCode() == EResultCode::InProgress)
            {
                return;
            }

            if (assetsResult.GetResultCode() == EResultCode::Failed)
            {
                NullResult internalResult(assetsResult);
                INVOKE_IF_NOT_NULL(callback, internalResult);

                return;
            }

            UriResultCallback uploadCallback = [callback](const UriResult& uploadResult)
            {
                if (uploadResult.GetResultCode() == EResultCode::Failed)
                {
                    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
                        (int)uploadResult.GetResultCode(), uploadResult.GetHttpResultCode());
                }

                NullResult internalResult(uploadResult);
                INVOKE_IF_NOT_NULL(callback, internalResult);
            };

            auto& thumbnailAsset = ((csp::systems::AssetsResult&)assetsResult).GetAssets()[0];
            thumbnailAsset.FileName
                = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(SpaceSystemHelpers::GetAssetFileExtension(newThumbnail.GetMimeType()));
            thumbnailAsset.MimeType = newThumbnail.GetMimeType();
            const auto assetSystem = SystemsManager::Get().GetAssetSystem();
            assetSystem->UploadAssetData(thumbnailAssetCollection, thumbnailAsset, newThumbnail, uploadCallback);
        };

        GetSpaceThumbnailAsset(thumbnailAssetCollection, thumbnailAssetCallback);
    };

    GetSpaceThumbnailAssetCollection(spaceId, thumbnailAssetCollCallback);
}

void SpaceSystem::GetSpaceThumbnail(const String& spaceId, UriResultCallback callback)
{
    AssetCollectionsResultCallback thumbnailAssetCollCallback = [callback, this](const AssetCollectionsResult& assetCollResult)
    {
        if (assetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (assetCollResult.GetResultCode() == EResultCode::Failed)
        {
            const UriResult internalResult(assetCollResult.GetResultCode(), assetCollResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(callback, internalResult);

            return;
        }

        const auto& assetCollections = assetCollResult.GetAssetCollections();

        if (assetCollections.IsEmpty())
        {
            // Space doesn't have a thumbnail
            UriResult internalResult(EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNotFound));
            INVOKE_IF_NOT_NULL(callback, internalResult);

            return;
        }

        const auto& thumbnailAssetCollection = assetCollections[0];

        AssetsResultCallback thumbnailAssetCallback = [callback](const AssetsResult& assetsResult)
        {
            if (assetsResult.GetResultCode() == EResultCode::InProgress)
            {
                return;
            }

            UriResult internalResult(assetsResult.GetResultCode(), assetsResult.GetHttpResultCode());

            if (assetsResult.GetResultCode() == EResultCode::Success)
            {
                internalResult.SetUri(assetsResult.GetAssets()[0].Uri);
            }

            INVOKE_IF_NOT_NULL(callback, internalResult);
        };

        GetSpaceThumbnailAsset(thumbnailAssetCollection, thumbnailAssetCallback);
    };

    GetSpaceThumbnailAssetCollection(spaceId, thumbnailAssetCollCallback);
}

void SpaceSystem::AddUserToSpaceBanList(const String& spaceId, const String& requestedUserId, NullResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_groupApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(callback, nullptr);
    static_cast<chs::GroupApi*>(m_groupApi)->groupsGroupIdBanned_usersUserIdPut({ spaceId, requestedUserId }, responseHandler);
}

void SpaceSystem::DeleteUserFromSpaceBanList(const String& spaceId, const String& requestedUserId, NullResultCallback callback)
{
    csp::services::ResponseHandlerPtr responseHandler
        = m_groupApi->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(callback, nullptr);
    static_cast<chs::GroupApi*>(m_groupApi)->groupsGroupIdBanned_usersUserIdDelete({ spaceId, requestedUserId }, responseHandler);
}

void SpaceSystem::GetMetadataAssetCollection(const String& spaceId, AssetCollectionResultCallback callback)
{
    auto* assetSystem = SystemsManager::Get().GetAssetSystem();
    auto metadataAssetCollectionName = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(spaceId);

    assetSystem->GetAssetCollectionByName(metadataAssetCollectionName, callback);
}

void SpaceSystem::GetMetadataAssetCollections(const Array<csp::common::String>& spaceIds, AssetCollectionsResultCallback callback)
{
    auto* assetSystem = SystemsManager::Get().GetAssetSystem();
    Array<String> prototypeNames(spaceIds.Size());

    for (size_t item = 0; item < spaceIds.Size(); ++item)
    {
        prototypeNames[item] = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(spaceIds[item]);
    }

    assetSystem->FindAssetCollections(nullptr, nullptr, prototypeNames, nullptr, nullptr, nullptr, nullptr, nullptr, callback);
}

void SpaceSystem::RemoveMetadata(const String& spaceId, NullResultCallback callback)
{
    if (spaceId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("RemoveMetadata called with empty SpaceId. Aborting call.");

        INVOKE_IF_NOT_NULL(callback, MakeInvalid<NullResult>());

        return;
    }

    AssetCollectionResultCallback getAssetCollCallback = [callback](const AssetCollectionResult& assetCollResult)
    {
        if (assetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (assetCollResult.GetResultCode() == EResultCode::Failed)
        {
            NullResult internalResult(assetCollResult);
            INVOKE_IF_NOT_NULL(callback, internalResult);

            return;
        }

        NullResultCallback deleteAssetCollCallback = [callback](const NullResult& result)
        {
            NullResult internalResult(result);
            INVOKE_IF_NOT_NULL(callback, result);
        };

        auto* assetSystem = SystemsManager::Get().GetAssetSystem();
        assetSystem->DeleteAssetCollection(assetCollResult.GetAssetCollection(), deleteAssetCollCallback);
    };

    GetMetadataAssetCollection(spaceId, getAssetCollCallback);
}

void SpaceSystem::AddSpaceThumbnail(const csp::common::String& spaceId, const FileAssetDataSource& imageDataSource, NullResultCallback callback)
{
    auto* assetSystem = SystemsManager::Get().GetAssetSystem();

    AssetCollectionResultCallback createAssetCollCallback
        = [callback, assetSystem, spaceId, imageDataSource](const AssetCollectionResult& assetCollResult)
    {
        if (assetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (assetCollResult.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Log,
                "The Space thumbnail asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
                (int)assetCollResult.GetResultCode(), assetCollResult.GetHttpResultCode());

            NullResult internalResult(assetCollResult);
            INVOKE_IF_NOT_NULL(callback, internalResult);

            return;
        }

        const auto& thumbnailAssetColl = assetCollResult.GetAssetCollection();

        AssetResultCallback createAssetCallback = [callback, assetSystem, thumbnailAssetColl, imageDataSource](const AssetResult& createAssetResult)
        {
            if (createAssetResult.GetResultCode() == EResultCode::InProgress)
            {
                return;
            }

            if (createAssetResult.GetResultCode() == EResultCode::Failed)
            {
                CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail asset creation was not successful. ResCode: %d, HttpResCode: %d",
                    (int)createAssetResult.GetResultCode(), createAssetResult.GetHttpResultCode());

                NullResult internalResult(createAssetResult);
                INVOKE_IF_NOT_NULL(callback, internalResult);

                return;
            }

            UriResultCallback uploadCallback = [callback](const UriResult& uploadResult)
            {
                if (uploadResult.GetResultCode() == EResultCode::Failed)
                {
                    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
                        (int)uploadResult.GetResultCode(), uploadResult.GetHttpResultCode());
                }

                const NullResult internalResult(uploadResult);
                INVOKE_IF_NOT_NULL(callback, internalResult);
            };

            assetSystem->UploadAssetData(thumbnailAssetColl, createAssetResult.GetAsset(), imageDataSource, uploadCallback);
        };

        const auto uniqueAssetName = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(spaceId);
        assetSystem->CreateAsset(thumbnailAssetColl, uniqueAssetName, nullptr, nullptr, EAssetType::IMAGE, createAssetCallback);
    };

    const String spaceThumbnailAssetCollectionName = SpaceSystemHelpers::GetSpaceThumbnailAssetCollectionName(spaceId);
    const auto tag = Array<String>({ spaceId });

    // don't associate this asset collection with a particular space so that it can be retrieved by guest users that have not joined this
    // space
    assetSystem->CreateAssetCollection(
        spaceId, nullptr, spaceThumbnailAssetCollectionName, nullptr, EAssetCollectionType::SPACE_THUMBNAIL, tag, createAssetCollCallback);
}

void SpaceSystem::AddSpaceThumbnailWithBuffer(
    const csp::common::String& spaceId, const BufferAssetDataSource& imageDataSource, NullResultCallback callback)
{
    auto* assetSystem = SystemsManager::Get().GetAssetSystem();

    AssetCollectionResultCallback createAssetCollCallback
        = [callback, spaceId, imageDataSource, assetSystem](const AssetCollectionResult& assetCollResult)
    {
        if (assetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (assetCollResult.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Log,
                "The Space thumbnail asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
                (int)assetCollResult.GetResultCode(), assetCollResult.GetHttpResultCode());

            NullResult internalResult(assetCollResult);
            INVOKE_IF_NOT_NULL(callback, internalResult);

            return;
        }

        const auto& thumbnailAssetColl = assetCollResult.GetAssetCollection();

        AssetResultCallback createAssetCallback = [callback, imageDataSource, thumbnailAssetColl, assetSystem](const AssetResult& createAssetResult)
        {
            if (createAssetResult.GetResultCode() == EResultCode::InProgress)
            {
                return;
            }

            if (createAssetResult.GetResultCode() == EResultCode::Failed)
            {
                CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail asset creation was not successful. ResCode: %d, HttpResCode: %d",
                    (int)createAssetResult.GetResultCode(), createAssetResult.GetHttpResultCode());

                NullResult internalResult(createAssetResult);
                INVOKE_IF_NOT_NULL(callback, internalResult);

                return;
            }

            UriResultCallback uploadCallback = [callback](const UriResult& uploadResult)
            {
                if (uploadResult.GetResultCode() == EResultCode::Failed)
                {
                    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
                        (int)uploadResult.GetResultCode(), uploadResult.GetHttpResultCode());
                }

                NullResult internalResult(uploadResult);
                INVOKE_IF_NOT_NULL(callback, internalResult);
            };

            Asset thumbnailAsset = createAssetResult.GetAsset();

            thumbnailAsset.FileName
                = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(SpaceSystemHelpers::GetAssetFileExtension(imageDataSource.GetMimeType()));
            thumbnailAsset.MimeType = imageDataSource.GetMimeType();
            assetSystem->UploadAssetData(thumbnailAssetColl, thumbnailAsset, imageDataSource, uploadCallback);
        };

        const auto uniqueAssetName = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(spaceId);
        assetSystem->CreateAsset(thumbnailAssetColl, uniqueAssetName, nullptr, nullptr, EAssetType::IMAGE, createAssetCallback);
    };

    const String spaceThumbnailAssetCollectionName = SpaceSystemHelpers::GetSpaceThumbnailAssetCollectionName(spaceId);
    const Array<String> tag({ spaceId });

    assetSystem->CreateAssetCollection(
        spaceId, nullptr, spaceThumbnailAssetCollectionName, nullptr, EAssetCollectionType::SPACE_THUMBNAIL, tag, createAssetCollCallback);
}

void SpaceSystem::GetSpaceThumbnailAssetCollection(const csp::common::String& spaceId, AssetCollectionsResultCallback callback)
{
    AssetCollectionsResultCallback getAssetCollCallback = [callback](const AssetCollectionsResult& assetCollResult)
    {
        if (assetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (assetCollResult.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail asset collection retrieval has failed. ResCode: %d, HttpResCode: %d",
                (int)assetCollResult.GetResultCode(), assetCollResult.GetHttpResultCode());
        }

        INVOKE_IF_NOT_NULL(callback, assetCollResult);
    };

    auto* assetSystem = SystemsManager::Get().GetAssetSystem();
    auto metadataAssetCollectionName = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(spaceId);

    Array<csp::systems::EAssetCollectionType> prototypeTypes = { EAssetCollectionType::SPACE_THUMBNAIL };
    Array<String> prototypeTags = { spaceId };
    Array<String> groupIds = { spaceId };

    assetSystem->FindAssetCollections(nullptr, nullptr, nullptr, prototypeTypes, prototypeTags, groupIds, nullptr, nullptr, getAssetCollCallback);
}

void SpaceSystem::GetSpaceThumbnailAsset(const AssetCollection& thumbnailAssetCollection, AssetsResultCallback callback)
{
    AssetsResultCallback thumbnailAssetCallback = [callback](const AssetsResult& assetsResult)
    {
        if (assetsResult.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail asset retrieval has failed. ResCode: %d, HttpResCode: %d",
                (int)assetsResult.GetResultCode(), assetsResult.GetHttpResultCode());
        }
        else if (assetsResult.GetResultCode() == EResultCode::Success)
        {
            assert(!assetsResult.GetAssets().IsEmpty() && "Space thumbnail asset should exist");
            assert((assetsResult.GetAssets().Size() == 1) && "There should be only one Space thumbnail asset");
        }

        INVOKE_IF_NOT_NULL(callback, assetsResult);
    };

    auto* assetSystem = SystemsManager::Get().GetAssetSystem();
    assetSystem->GetAssetsInCollection(thumbnailAssetCollection, thumbnailAssetCallback);
}

void SpaceSystem::RemoveSpaceThumbnail(const csp::common::String& spaceId, NullResultCallback callback)
{
    auto* assetSystem = SystemsManager::Get().GetAssetSystem();

    AssetCollectionsResultCallback thumbnailAssetCollCallback = [callback, assetSystem, this](const AssetCollectionsResult& assetCollResult)
    {
        if (assetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (assetCollResult.GetResultCode() == EResultCode::Failed)
        {
            NullResult internalResult(assetCollResult);
            INVOKE_IF_NOT_NULL(callback, internalResult);

            return;
        }

        const auto& assetCollections = assetCollResult.GetAssetCollections();

        if (assetCollections.IsEmpty())
        {
            // Space doesn't have a thumbnail so we're done
            NullResult internalResult(assetCollResult);
            INVOKE_IF_NOT_NULL(callback, internalResult);

            return;
        }

        const auto& thumbnailAssetCollection = assetCollections[0];

        AssetsResultCallback thumbnailAssetCallback = [callback, assetSystem, thumbnailAssetCollection](const AssetsResult& assetsResult)
        {
            if (assetsResult.GetResultCode() == EResultCode::InProgress)
            {
                return;
            }

            if (assetsResult.GetResultCode() == EResultCode::Failed)
            {
                NullResult internalResult(assetsResult);
                INVOKE_IF_NOT_NULL(callback, internalResult);

                return;
            }

            NullResultCallback deleteAssetCallback = [callback, assetSystem, thumbnailAssetCollection](const NullResult& deleteAssetResult)
            {
                if (deleteAssetResult.GetResultCode() == EResultCode::InProgress)
                {
                    return;
                }

                if (deleteAssetResult.GetResultCode() == EResultCode::Failed)
                {
                    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail asset deletion was not successful. ResCode: %d, HttpResCode: %d",
                        (int)deleteAssetResult.GetResultCode(), deleteAssetResult.GetHttpResultCode());

                    NullResult internalResult(deleteAssetResult);
                    INVOKE_IF_NOT_NULL(callback, deleteAssetResult);

                    return;
                }

                NullResultCallback deleteAssetCollCallback = [callback, deleteAssetResult](const NullResult& deleteAssetCollResult)
                {
                    if (deleteAssetCollResult.GetResultCode() == EResultCode::InProgress)
                    {
                        return;
                    }

                    if (deleteAssetCollResult.GetResultCode() == EResultCode::Failed)
                    {
                        CSP_LOG_FORMAT(csp::common::LogLevel::Log,
                            "The Space thumbnail asset collection deletion has failed. ResCode: %d, HttpResCode: %d",
                            (int)deleteAssetResult.GetResultCode(), deleteAssetResult.GetHttpResultCode());
                    }

                    NullResult internalResult(deleteAssetCollResult);
                    INVOKE_IF_NOT_NULL(callback, deleteAssetCollResult);
                };

                assetSystem->DeleteAssetCollection(thumbnailAssetCollection, deleteAssetCollCallback);
            };

            const auto& thumbnailAsset = assetsResult.GetAssets()[0];
            assetSystem->DeleteAsset(thumbnailAssetCollection, thumbnailAsset, deleteAssetCallback);
        };

        GetSpaceThumbnailAsset(thumbnailAssetCollection, thumbnailAssetCallback);
    };

    GetSpaceThumbnailAssetCollection(spaceId, thumbnailAssetCollCallback);
}

void SpaceSystem::GetSpaceGeoLocationInternal(const csp::common::String& spaceId, SpaceGeoLocationResultCallback callback)
{
    auto& systemsManager = SystemsManager::Get();
    auto* poiInternalSystem = static_cast<PointOfInterestInternalSystem*>(systemsManager.GetPointOfInterestSystem());
    poiInternalSystem->GetSpaceGeoLocation(spaceId, callback);
}

void SpaceSystem::GetSpaceGeoLocation(const csp::common::String& spaceId, SpaceGeoLocationResultCallback callback)
{
    // First refresh the space to ensure the user has access to the space
    SpaceResultCallback getSpaceCallback = [callback, this](const SpaceResult& getSpaceResult)
    {
        if (getSpaceResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (getSpaceResult.GetResultCode() == EResultCode::Failed)
        {
            SpaceGeoLocationResult result(getSpaceResult.GetResultCode(), getSpaceResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(callback, result);

            return;
        }

        const auto& refreshedSpace = getSpaceResult.GetSpace();
        const auto& userId = SystemsManager::Get().GetUserSystem()->GetLoginState().UserId;

        // First check if the user is the owner
        bool userCanAccessSpaceDetails = !(bool)(refreshedSpace.Attributes & SpaceAttributes::RequiresInvite) || refreshedSpace.OwnerId == userId;

        // If the user is not the owner check are they a moderator
        if (!userCanAccessSpaceDetails)
        {
            userCanAccessSpaceDetails = systems::SpaceSystemHelpers::IdCheck(userId, refreshedSpace.ModeratorIds);
        }

        // If the user is not the owner or a moderator check are the full user list
        if (!userCanAccessSpaceDetails)
        {
            userCanAccessSpaceDetails = systems::SpaceSystemHelpers::IdCheck(userId, refreshedSpace.UserIds);
        }

        if (!userCanAccessSpaceDetails)
        {
            SpaceGeoLocationResult result(EResultCode::Failed, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));
            INVOKE_IF_NOT_NULL(callback, result);

            return;
        }

        GetSpaceGeoLocationInternal(refreshedSpace.Id, callback);
    };

    GetSpace(spaceId, getSpaceCallback);
}

void SpaceSystem::UpdateSpaceGeoLocation(const csp::common::String& spaceId, const csp::common::Optional<GeoLocation>& location,
    const csp::common::Optional<float>& orientation, const csp::common::Optional<csp::common::Array<GeoLocation>>& geoFence,
    SpaceGeoLocationResultCallback callback)
{
    SpaceGeoLocationResultCallback getSpaceGeoLocationCallback
        = [callback, spaceId, location, orientation, geoFence](const SpaceGeoLocationResult& getGeoLocationResult)
    {
        if (getGeoLocationResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (getGeoLocationResult.GetResultCode() == EResultCode::Failed)
        {
            INVOKE_IF_NOT_NULL(callback, getGeoLocationResult);

            return;
        }

        auto& systemsManager = SystemsManager::Get();
        auto* poiInternalSystem = static_cast<PointOfInterestInternalSystem*>(systemsManager.GetPointOfInterestSystem());

        if (getGeoLocationResult.m_hasGeoLocation)
        {
            poiInternalSystem->UpdateSpaceGeoLocation(
                spaceId, getGeoLocationResult.GetSpaceGeoLocation().m_id, location, orientation, geoFence, callback);
        }
        else
        {
            poiInternalSystem->AddSpaceGeoLocation(spaceId, location, orientation, geoFence, callback);
        }
    };

    // First refresh the space to ensure the user has access to the space
    SpaceResultCallback getSpaceCallback = [callback, getSpaceGeoLocationCallback, this](const SpaceResult& getSpaceResult)
    {
        if (getSpaceResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (getSpaceResult.GetResultCode() == EResultCode::Failed)
        {
            SpaceGeoLocationResult result(getSpaceResult.GetResultCode(), getSpaceResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(callback, result);

            return;
        }

        const auto& refreshedSpace = getSpaceResult.GetSpace();
        const auto& userId = SystemsManager::Get().GetUserSystem()->GetLoginState().UserId;

        // First check if the user is the owner
        bool userCanModifySpace = refreshedSpace.OwnerId == userId;

        // If the user is not the owner check are they a moderator
        if (!userCanModifySpace)
        {
            userCanModifySpace = systems::SpaceSystemHelpers::IdCheck(userId, refreshedSpace.ModeratorIds);
        }

        if (!userCanModifySpace)
        {
            SpaceGeoLocationResult result(EResultCode::Failed, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));
            INVOKE_IF_NOT_NULL(callback, result);

            return;
        }

        GetSpaceGeoLocationInternal(refreshedSpace.Id, getSpaceGeoLocationCallback);
    };

    GetSpace(spaceId, getSpaceCallback);
}

void SpaceSystem::DeleteSpaceGeoLocation(const csp::common::String& spaceId, NullResultCallback callback)
{
    SpaceGeoLocationResultCallback getSpaceGeoLocationCallback = [callback](const SpaceGeoLocationResult& getGeoLocationResult)
    {
        if (getGeoLocationResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        auto& systemsManager = SystemsManager::Get();
        auto* poiInternalSystem = static_cast<PointOfInterestInternalSystem*>(systemsManager.GetPointOfInterestSystem());

        if (!getGeoLocationResult.m_hasGeoLocation)
        {
            INVOKE_IF_NOT_NULL(callback, MakeInvalid<NullResult>());

            return;
        }

        poiInternalSystem->DeleteSpaceGeoLocation(getGeoLocationResult.GetSpaceGeoLocation().m_id, callback);
    };

    // First refresh the space to ensure the user has access to the space
    SpaceResultCallback getSpaceCallback = [callback, getSpaceGeoLocationCallback, this](const SpaceResult& getSpaceResult)
    {
        if (getSpaceResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (getSpaceResult.GetResultCode() == EResultCode::Failed)
        {
            NullResult result(getSpaceResult.GetResultCode(), getSpaceResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(callback, result);

            return;
        }

        const auto& refreshedSpace = getSpaceResult.GetSpace();
        const auto& userId = SystemsManager::Get().GetUserSystem()->GetLoginState().UserId;

        // First check if the user is the owner
        bool userCanModifySpace = refreshedSpace.OwnerId == userId;

        // If the user is not the owner check are they a moderator
        if (!userCanModifySpace)
        {
            userCanModifySpace = systems::SpaceSystemHelpers::IdCheck(userId, refreshedSpace.ModeratorIds);
        }

        if (!userCanModifySpace)
        {
            NullResult result(EResultCode::Failed, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));
            INVOKE_IF_NOT_NULL(callback, result);

            return;
        }

        GetSpaceGeoLocationInternal(refreshedSpace.Id, getSpaceGeoLocationCallback);
    };

    GetSpace(spaceId, getSpaceCallback);
}

void SpaceSystem::DuplicateSpace(const String& spaceId, const String& newName, SpaceAttributes newAttributes,
    const Optional<Array<String>>& memberGroupIds, bool shallowCopy, SpaceResultCallback callback)
{
    auto request = ConstructDuplicateSpaceOptions(m_userSystem, spaceId, newName, newAttributes, memberGroupIds, shallowCopy, false);

    csp::services::ResponseHandlerPtr responseHandler
        = m_spaceApi->CreateHandler<csp::systems::SpaceResultCallback, csp::systems::SpaceResult, void, chs::GroupDto>(callback, nullptr);

    static_cast<chsaggregation::SpaceApi*>(m_spaceApi)->spacesSpaceIdDuplicatePost(
        {
            spaceId, // spaceId
            false, // asyncCall
            request // RequestBody
        },
        responseHandler // ResponseHandler
    );
}

void SpaceSystem::DuplicateSpaceAsync(const String& spaceId, const String& newName, SpaceAttributes newAttributes,
    const Optional<Array<String>>& memberGroupIds, bool shallowCopy, NullResultCallback callback)
{
    auto request = ConstructDuplicateSpaceOptions(m_userSystem, spaceId, newName, newAttributes, memberGroupIds, shallowCopy, true);

    csp::services::ResponseHandlerPtr responseHandler
        = m_spaceApi->CreateHandler<csp::systems::NullResultCallback, csp::systems::NullResult, void, chs::GroupDto>(callback, nullptr);

    static_cast<chsaggregation::SpaceApi*>(m_spaceApi)->spacesSpaceIdDuplicatePost(
        {
            spaceId, // spaceId
            true, // asyncCall
            request // RequestBody
        },
        responseHandler // ResponseHandler
    );
}

void SpaceSystem::SetAsyncCallCompletedCallback(AsyncCallCompletedCallbackHandler callback)
{
    m_asyncCallCompletedCallback = std::move(callback);

    if (!m_asyncCallCompletedCallback)
    {
        CSP_LOG_ERROR_MSG("Error: The AsyncCallCompletedCallback handler has not been set and the SpaceSystem has not been registered with the "
                          "AsyncCallCompleted event. Please call 'SetAsyncCallCompletedCallback()' with a valid AsyncCallCompletedCallbackHandler.");
        return;
    }

    m_eventBusPtr->ListenNetworkEvent(
        csp::multiplayer::NetworkEventRegistration("CSPInternal::SpaceSystem",
            csp::multiplayer::NetworkEventBus::StringFromNetworkEvent(csp::multiplayer::NetworkEventBus::NetworkEvent::AsyncCallCompleted)),
        [this](const csp::common::NetworkEventData& networkEventData) { this->OnAsyncCallCompletedEvent(networkEventData); });
}

void SpaceSystem::OnAsyncCallCompletedEvent(const csp::common::NetworkEventData& networkEventData)
{
    if (!m_asyncCallCompletedCallback)
    {
        return;
    }

    const csp::common::AsyncCallCompletedEventData& asyncCallCompletedEventData
        = static_cast<const csp::common::AsyncCallCompletedEventData&>(networkEventData);

    m_asyncCallCompletedCallback(asyncCallCompletedEventData);
}

void SpaceSystem::SetMultiplayerSystem(csp::systems::MultiplayerSystem& inMultiplayerSystem) { m_multiplayerSystem = &inMultiplayerSystem; }
} // namespace csp::systems
