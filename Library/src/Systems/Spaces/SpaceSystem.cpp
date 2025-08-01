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
#include "CSP/Common/SharedEnums.h"
#include "CSP/Common/StringFormat.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Multiplayer/NetworkEventBus.h"
#include "CSP/Multiplayer/SpaceEntitySystem.h"
#include "CSP/Systems/Assets/AssetSystem.h"
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

} // namespace

namespace csp::systems
{

SpaceSystem::SpaceSystem()
    : SystemBase(nullptr, nullptr, nullptr)
    , GroupAPI(nullptr)
    , SpaceAPI(nullptr)
{
}

SpaceSystem::SpaceSystem(csp::web::WebClient* InWebClient, csp::common::LogSystem& LogSystem)
    : SystemBase(InWebClient, nullptr, &LogSystem)
    , CurrentSpace()
{
    GroupAPI = new chs::GroupApi(InWebClient);
    SpaceAPI = new chsaggregation::SpaceApi(InWebClient);
}

SpaceSystem::~SpaceSystem() { delete (GroupAPI); }

/* CreateSpace Continuations */
async::task<SpaceResult> SpaceSystem::CreateSpaceGroupInfo(
    const String& Name, const String& Description, SpaceAttributes Attributes, const Optional<Array<String>>& Tags)
{
    auto OnCompleteEvent = std::make_shared<async::event_task<SpaceResult>>();
    async::task<SpaceResult> OnCompleteTask = OnCompleteEvent->get_task();

    auto GroupInfo = systems::SpaceSystemHelpers::DefaultGroupInfo();
    GroupInfo->SetName(Name);
    GroupInfo->SetDescription(Description);
    GroupInfo->SetDiscoverable(HasFlag(Attributes, csp::systems::SpaceAttributes::IsDiscoverable));
    GroupInfo->SetRequiresInvite(HasFlag(Attributes, csp::systems::SpaceAttributes::RequiresInvite));
    GroupInfo->SetGroupType("space");

    if (Tags.HasValue())
    {
        GroupInfo->SetTags(csp::common::Convert(Tags).value());
    }

    csp::services::ResponseHandlerPtr ResponseHandler = GroupAPI->CreateHandler<SpaceResultCallback, SpaceResult, void, chs::GroupDto>(
        [](const SpaceResult&) {}, nullptr, csp::web::EResponseCodes::ResponseOK, std::move(*OnCompleteEvent.get()));

    static_cast<chs::GroupApi*>(GroupAPI)->groupsPost(GroupInfo, ResponseHandler);

    return OnCompleteTask;
}

std::function<async::task<AssetCollectionResult>()> SpaceSystem::CreateSpaceMetadataAssetCollection(
    const std::shared_ptr<SpaceResult>& Space, const csp::common::Map<csp::common::String, csp::common::String>& Metadata)
{
    return [Space, Metadata]() -> async::task<AssetCollectionResult>
    {
        const auto Id = Space->GetSpace().Id;
        const auto Name = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(Id);
        auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

        // Don't assign this AssetCollection to a space so any user can retrieve the metadata without joining the space
        return AssetSystem->CreateAssetCollection(Id, nullptr, Name, Metadata, EAssetCollectionType::FOUNDATION_INTERNAL, nullptr);
    };
}

async::task<AssetCollectionResult> SpaceSystem::CreateSpaceThumbnailAssetCollection(const std::shared_ptr<SpaceResult>& Space)
{
    const auto SpaceId = Space->GetSpace().Id;
    const auto Name = SpaceSystemHelpers::GetSpaceThumbnailAssetCollectionName(SpaceId);
    auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

    // don't associate this asset collection with a particular space so that it can be retrieved by guest users without joining the space
    return AssetSystem->CreateAssetCollection(SpaceId, nullptr, Name, nullptr, EAssetCollectionType::SPACE_THUMBNAIL, Array<String>({ SpaceId }));
}

std::function<async::task<AssetResult>()> SpaceSystem::CreateSpaceThumbnailAsset(
    const std::shared_ptr<SpaceResult>& Space, const std::shared_ptr<AssetCollectionResult>& AssetCollectionResult)
{
    return [Space, AssetCollectionResult]() -> async::task<AssetResult>
    {
        const auto SpaceId = Space->GetSpace().Id;
        const auto Name = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(SpaceId);
        auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

        return AssetSystem->CreateAsset(AssetCollectionResult->GetAssetCollection(), Name, nullptr, nullptr, EAssetType::IMAGE);
    };
}

std::function<async::task<UriResult>(const AssetResult& Result)> SpaceSystem::UploadSpaceThumbnailAsset(
    const std::shared_ptr<AssetCollectionResult>& AssetCollectionResult, FileAssetDataSource& Data)
{
    return [AssetCollectionResult, Data](const AssetResult& Result) -> async::task<UriResult>
    {
        Asset UploadAsset = Result.GetAsset();
        UploadAsset.FileName = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(SpaceSystemHelpers::GetAssetFileExtension(Data.GetMimeType()));
        UploadAsset.MimeType = Data.GetMimeType();
        auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

        return AssetSystem->UploadAssetDataEx(
            AssetCollectionResult->GetAssetCollection(), UploadAsset, Data, csp::common::CancellationToken::Dummy());
    };
}

std::function<async::task<UriResult>(const AssetResult& Result)> SpaceSystem::UploadSpaceThumbnailAssetWithBuffer(
    const std::shared_ptr<AssetCollectionResult>& AssetCollectionResult, const csp::systems::BufferAssetDataSource& Data)
{
    return [AssetCollectionResult, Data](const AssetResult& Result) -> async::task<UriResult>
    {
        Asset UploadAsset = Result.GetAsset();
        UploadAsset.FileName = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(SpaceSystemHelpers::GetAssetFileExtension(Data.GetMimeType()));
        UploadAsset.MimeType = Data.GetMimeType();
        auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

        return AssetSystem->UploadAssetDataEx(
            AssetCollectionResult->GetAssetCollection(), UploadAsset, Data, csp::common::CancellationToken::Dummy());
    };
}

std::function<async::task<UriResult>()> SpaceSystem::CreateAndUploadSpaceThumbnailToSpace(
    const std::shared_ptr<SpaceResult>& Space, const csp::common::Optional<csp::systems::FileAssetDataSource>& Data)
{
    return [Space, Data, this]() -> async::task<UriResult>
    {
        if (!Data.HasValue())
        {
            // In the event the optional is null we still want to return success to continue the chain.
            return async::make_task(UriResult(EResultCode::Success, 200));
        }

        const auto SpaceId = Space->GetSpace().Id;
        auto ThumbnailAssetCollection = std::make_shared<AssetCollectionResult>();

        NullResultCallback Callback;

        return CreateSpaceThumbnailAssetCollection(Space)
            .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
                "SpaceSystem::CreateAndUploadSpaceThumbnailToSpace, successfully created space thumbnail asset collection.",
                "Failed to create space thumbnail asset collection.", {}, {}, {}))
            .then(systems::continuations::GetResultFromContinuation<AssetCollectionResult>(ThumbnailAssetCollection))
            .then(CreateSpaceThumbnailAsset(Space, ThumbnailAssetCollection))
            .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetResult>(Callback,
                "SpaceSystem::CreateAndUploadSpaceThumbnailToSpace, successfully created space thumbnail asset.",
                "Failed to create space thumbnail asset.", {}, {}, {}))
            .then(UploadSpaceThumbnailAsset(ThumbnailAssetCollection, *Data))
            .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(Callback,
                "SpaceSystem::CreateAndUploadSpaceThumbnailToSpace, successfully upload space thumbnail asset.",
                "Failed to upload space thumbnail asset.", {}, {}, {}));
    };
}

std::function<async::task<UriResult>()> SpaceSystem::CreateAndUploadSpaceThumbnailWithBufferToSpace(
    const std::shared_ptr<SpaceResult>& Space, const csp::systems::BufferAssetDataSource& Data)
{
    return [Space, Data, this]() -> async::task<UriResult>
    {
        const auto SpaceId = Space->GetSpace().Id;
        auto ThumbnailAssetCollection = std::make_shared<AssetCollectionResult>();

        NullResultCallback Callback;

        return CreateSpaceThumbnailAssetCollection(Space)
            .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
                "SpaceSystem::CreateAndUploadSpaceThumbnailWithBufferToSpace, successfully created space thumbnail asset collection.",
                "Failed to create space thumbnail asset collection.", {}, {}, {}))
            .then(systems::continuations::GetResultFromContinuation<AssetCollectionResult>(ThumbnailAssetCollection))
            .then(CreateSpaceThumbnailAsset(Space, ThumbnailAssetCollection))
            .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetResult>(Callback,
                "SpaceSystem::CreateAndUploadSpaceThumbnailWithBufferToSpace, successfully created space thumbnail asset.",
                "Failed to create space thumbnail asset.", {}, {}, {}))
            .then(UploadSpaceThumbnailAssetWithBuffer(ThumbnailAssetCollection, Data))
            .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(Callback,
                "SpaceSystem::CreateAndUploadSpaceThumbnailWithBufferToSpace, successfully upload space thumbnail asset.",
                "Failed to upload space thumbnail asset.", {}, {}, {}));
    };
}

std::function<async::task<NullResult>()> SpaceSystem::BulkInviteUsersToSpaceIfNeccesary(
    SpaceSystem* SpaceSystem, const std::shared_ptr<SpaceResult>& Space, const Optional<InviteUserRoleInfoCollection>& InviteUsers)
{
    return [SpaceSystem, Space, InviteUsers]() -> async::task<NullResult>
    {
        if (!InviteUsers.HasValue() || InviteUsers->InviteUserRoleInfos.IsEmpty())
        {
            // In the event the optional is null we still want to return success to continue the chain.
            return async::make_task(NullResult(EResultCode::Success, 200));
        }

        const auto SpaceId = Space->GetSpace().Id;

        NullResultCallback Callback;

        return SpaceSystem->BulkInviteToSpace(SpaceId, *InviteUsers)
            .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(Callback,
                "SpaceSystem::BulkInviteUsersToSpace, successfully invited users to space.", "Failed to invited users to space.", {}, {}, {}));
    };
}

/* EnterSpace Continuations */
auto SpaceSystem::AddUserToSpaceIfNecessary(NullResultCallback Callback, SpaceSystem& SpaceSystem)
{
    return [Callback, &SpaceSystem](const SpaceResult& GetSpaceResult)
    {
        CSP_LOG_MSG(csp::common::LogLevel::Log, "SpaceSystem::AddUserToSpaceIfNecessary");

        /* Once we have permissions to discover a space, attempt to enter it */
        const auto& SpaceToJoin = GetSpaceResult.GetSpace();

        const String UserId = SystemsManager::Get().GetUserSystem()->GetLoginState().UserId;
        const bool JoiningSpaceRequiresInvite = HasFlag(SpaceToJoin.Attributes, SpaceAttributes::RequiresInvite);

        // The user is known to the space if they are a user, moderator or creator. This is important if the space requires an invite.
        const bool UserIsRecognizedBySpace = SpaceToJoin.UserIsKnownToSpace(UserId);

        /* If we need permissions, check that the user has permission to enter this specific space */
        if (JoiningSpaceRequiresInvite && !UserIsRecognizedBySpace)
        {
            systems::continuations::LogHTTPErrorAndCancelContinuation(Callback,
                "Logged in user does not have permission to join this space. Failed to add to space.", EResultCode::Failed,
                csp::web::EResponseCodes::ResponseForbidden, ERequestFailureReason::UserSpaceAccessDenied);
        }

        /* By this point,you should be allowed to join the space
               Add the user to the space even if they are already added */

        auto UserAddedToSpaceChainStartEvent = std::make_shared<async::event_task<SpaceResult>>();
        auto UserAddedToSpaceChainContinuation = UserAddedToSpaceChainStartEvent->get_task();
        // AddUserToSpace does not give a callback (feels like it should...) if the user is already added to the space.
        // Branch so we always continue, using an event so we can forward the continuation no matter what branch.
        if (!UserIsRecognizedBySpace)
        {
            CSP_LOG_MSG(csp::common::LogLevel::Log, "Adding user to space.");

            // Use the request continuation to set the event ... to fire another continuation to allow continued chaining.
            SpaceSystem.AddUserToSpace(SpaceToJoin.Id, UserId)
                .then(async::inline_scheduler(),
                    [UserAddedToSpaceChainStartEvent](const SpaceResult& AddedToSpaceResult)
                    { UserAddedToSpaceChainStartEvent->set(AddedToSpaceResult); });
        }
        else
        {
            CSP_LOG_MSG(csp::common::LogLevel::Log, "No need to add user to space.");

            // Just pass along the previous result
            UserAddedToSpaceChainStartEvent->set(GetSpaceResult);
        }

        return UserAddedToSpaceChainContinuation;
    };
}

auto SpaceSystem::FireEnterSpaceEvent(Space& OutCurrentSpace)
{
    return [&OutCurrentSpace](const SpaceResult& SpaceResult)
    {
        CSP_LOG_MSG(csp::common::LogLevel::Log, "SpaceSystem::FireEnterSpaceEvent");

        /* We're here. The space knows about us. We're definately in the allowed users. Let's join! */
        csp::events::Event* EnterSpaceEvent = csp::events::EventSystem::Get().AllocateEvent(csp::events::SPACESYSTEM_ENTER_SPACE_EVENT_ID);
        EnterSpaceEvent->AddString("SpaceId", SpaceResult.GetSpace().Id);
        csp::events::EventSystem::Get().EnqueueEvent(EnterSpaceEvent);
        OutCurrentSpace = SpaceResult.GetSpace();
        return SpaceResult;
    };
}

auto SpaceSystem::RefreshMultiplayerScopes()
{
    return [](const SpaceResult& SpaceResult)
    {
        CSP_LOG_MSG(csp::common::LogLevel::Log, "SpaceSystem::RefreshMultiplayerScopes");

        /* Refresh the multiplayer connection to force the scopes to change */
        /* This is wrapping a yet-to-be refactored method that uses nested callbacks, hence the event, and shared pointer for lifetime */
        auto RefreshMultiplayerConnectionEvent = std::make_shared<async::event_task<std::optional<csp::multiplayer::ErrorCode>>>();
        auto RefreshMultiplayerConnectionContinuation = RefreshMultiplayerConnectionEvent->get_task();

        csp::systems::SystemsManager::Get().GetSpaceEntitySystem()->RefreshMultiplayerConnectionToEnactScopeChange(
            SpaceResult.GetSpace().Id, RefreshMultiplayerConnectionEvent);

        return RefreshMultiplayerConnectionContinuation;
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
 * AssertRequestSuccessOrErrorFromMultiplayerErrorCode (RefreshMultiplayerScopes Validation)
 * ReportSuccess
 * InvokeIfExceptionInChain (Handle any errors from the above Assert methods in chain, resets state)
 */
void SpaceSystem::EnterSpace(const String& SpaceId, NullResultCallback Callback)
{
    CSP_LOG_MSG(csp::common::LogLevel::Log, "SpaceSystem::EnterSpace");

    GetSpace(SpaceId)
        .then(async::inline_scheduler(),
            systems::continuations::AssertRequestSuccessOrErrorFromResult<SpaceResult>(Callback,
                "SpaceSystem::EnterSpace, successfully discovered space.",
                "Logged in user does not have permission to discover this space. Failed to enter space.", {}, {}, {}))
        .then(async::inline_scheduler(), AddUserToSpaceIfNecessary(Callback, *this))
        .then(async::inline_scheduler(),
            systems::continuations::AssertRequestSuccessOrErrorFromResult<SpaceResult>(Callback,
                "SpaceSystem::EnterSpace, successfully added user to space (if not already added).",
                "Failed to Enter Space. AddUserToSpace returned unexpected failure.", {}, {}, {}))
        .then(async::inline_scheduler(),
            [](const SpaceResult& SpaceResult)
            {
                // Initialize (or ensure initialization) prior to use (In RefreshMultiplayerScopes)
                csp::systems::SystemsManager::Get().GetSpaceEntitySystem()->Initialise();
                return SpaceResult;
            })
        .then(async::inline_scheduler(), FireEnterSpaceEvent(CurrentSpace))
        .then(async::inline_scheduler(), RefreshMultiplayerScopes())
        .then(async::inline_scheduler(),
            common::continuations::AssertRequestSuccessOrErrorFromMultiplayerErrorCode(Callback,
                "SpaceSystem: EnterSpace, successfully refreshed multiplayer scopes", MakeInvalid<NullResult>(),
                *csp::systems::SystemsManager::Get().GetLogSystem(), csp::common::LogLevel::Error))
        .then(async::inline_scheduler(), systems::continuations::ReportSuccess(Callback, "Successfully entered space."))
        .then(async::inline_scheduler(),
            csp::common::continuations::InvokeIfExceptionInChain([&CurrentSpace = CurrentSpace](const std::exception& /*Except*/)
                { CurrentSpace = {}; },
                *csp::systems::SystemsManager::Get().GetLogSystem()));
}

void SpaceSystem::ExitSpace(NullResultCallback Callback)
{
    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "Exiting Space %s", CurrentSpace.Name.c_str());

    // As the user is exiting the space, we now clear all scopes that they are registered to.
    auto& SystemsManager = systems::SystemsManager::Get();
    auto* MultiplayerConnection = SystemsManager.GetMultiplayerConnection();

    if (MultiplayerConnection != nullptr)
    {
        csp::systems::SystemsManager::Get().GetSpaceEntitySystem()->Shutdown();

        MultiplayerConnection->StopListening(
            [MultiplayerConnection, Callback](multiplayer::ErrorCode Error)
            {
                if (Error != multiplayer::ErrorCode::None)
                {
                    CSP_LOG_ERROR_FORMAT("Error on exiting spaces, whilst stopping listening in order to clear scopes, ErrorCode: %s",
                        multiplayer::ErrorCodeToString(Error).c_str());
                    INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

                    return;
                }

                MultiplayerConnection->ResetScopes(
                    [Callback](multiplayer::ErrorCode Error)
                    {
                        if (Error != multiplayer::ErrorCode::None)
                        {
                            CSP_LOG_ERROR_FORMAT(
                                "Error on exiting spaces whilst clearing scopes, ErrorCode: %s", multiplayer::ErrorCodeToString(Error).c_str());
                            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

                            return;
                        }

                        const NullResult Result(EResultCode::Success, 200);
                        INVOKE_IF_NOT_NULL(Callback, Result);
                    });
            });
    }

    events::Event* ExitSpaceEvent = events::EventSystem::Get().AllocateEvent(events::SPACESYSTEM_EXIT_SPACE_EVENT_ID);
    ExitSpaceEvent->AddString("SpaceId", CurrentSpace.Id);

    events::EventSystem::Get().EnqueueEvent(ExitSpaceEvent);

    CurrentSpace = Space();
}

bool SpaceSystem::IsInSpace() { return !CurrentSpace.Id.IsEmpty(); }

const Space& SpaceSystem::GetCurrentSpace() const { return CurrentSpace; }

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
void SpaceSystem::CreateSpace(const String& Name, const String& Description, SpaceAttributes Attributes,
    const Optional<InviteUserRoleInfoCollection>& InviteUsers, const Map<String, String>& Metadata, const Optional<FileAssetDataSource>& Thumbnail,
    const Optional<Array<String>>& Tags, SpaceResultCallback Callback)
{
    auto CurrentSpaceResult = std::make_shared<SpaceResult>();

    CreateSpaceGroupInfo(Name, Description, Attributes, Tags)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<SpaceResult>(
            Callback, "SpaceSystem::CreateSpace, successfully created space.", "Failed to create space.", {}, {}, {}))
        .then(systems::continuations::GetResultFromContinuation<SpaceResult>(CurrentSpaceResult))
        .then(CreateSpaceMetadataAssetCollection(CurrentSpaceResult, Metadata))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
            "SpaceSystem::CreateSpace, successfully created space metadata asset collection.", "Failed to create space metadata asset collection.",
            {}, {}, {}))
        .then(CreateAndUploadSpaceThumbnailToSpace(CurrentSpaceResult, Thumbnail))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(
            Callback, "SpaceSystem::CreateSpace, successfully created thumbnail.", "Failed to create thumbnail.", {}, {}, {}))
        .then(BulkInviteUsersToSpaceIfNeccesary(this, CurrentSpaceResult, InviteUsers))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(
            Callback, "SpaceSystem::CreateSpace, successfully invited users to space.", "Failed to invited users to space.", {}, {}, {}))
        .then(
            [CurrentSpaceResult, Callback]()
            {
                CSP_LOG_MSG(csp::common::LogLevel::Log,
                    csp::common::StringFormat("Successfully created space: %s", static_cast<const char*>(CurrentSpaceResult->GetSpace().Name)));

                INVOKE_IF_NOT_NULL(Callback, *CurrentSpaceResult);
            })
        .then(csp::common::continuations::InvokeIfExceptionInChain(
            [this, CurrentSpaceResult, Callback](const std::exception& /*Except*/)
            {
                auto NullResultCallback
                    = [](const csp::systems::NullResult& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; };

                this->DeleteSpace(CurrentSpaceResult->GetSpace().Id, NullResultCallback);

                Callback(MakeInvalid<SpaceResult>());
            },
            *csp::systems::SystemsManager::Get().GetLogSystem()));
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
void SpaceSystem::CreateSpaceWithBuffer(const String& Name, const String& Description, SpaceAttributes Attributes,
    const Optional<InviteUserRoleInfoCollection>& InviteUsers, const Map<String, String>& Metadata, const BufferAssetDataSource& Thumbnail,
    const Optional<Array<String>>& Tags, SpaceResultCallback Callback)
{
    auto CurrentSpaceResult = std::make_shared<SpaceResult>();

    CreateSpaceGroupInfo(Name, Description, Attributes, Tags)
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<SpaceResult>(
            Callback, "SpaceSystem::CreateSpaceWithBuffer, successfully created space.", "Failed to create space.", {}, {}, {}))
        .then(systems::continuations::GetResultFromContinuation<SpaceResult>(CurrentSpaceResult))
        .then(CreateSpaceMetadataAssetCollection(CurrentSpaceResult, Metadata))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<AssetCollectionResult>(Callback,
            "SpaceSystem::CreateSpaceWithBuffer, successfully created space metadata asset collection.",
            "Failed to create space metadata asset collection.", {}, {}, {}))
        .then(CreateAndUploadSpaceThumbnailWithBufferToSpace(CurrentSpaceResult, Thumbnail))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<UriResult>(
            Callback, "SpaceSystem::CreateSpaceWithBuffer, successfully created thumbnail.", "Failed to create thumbnail.", {}, {}, {}))
        .then(BulkInviteUsersToSpaceIfNeccesary(this, CurrentSpaceResult, InviteUsers))
        .then(systems::continuations::AssertRequestSuccessOrErrorFromResult<NullResult>(
            Callback, "SpaceSystem::CreateSpaceWithBuffer, successfully invited users to space.", "Failed to invited users to space.", {}, {}, {}))
        .then(
            [CurrentSpaceResult, Callback]()
            {
                CSP_LOG_MSG(csp::common::LogLevel::Log,
                    csp::common::StringFormat("Successfully created space: %s", static_cast<const char*>(CurrentSpaceResult->GetSpace().Name)));

                INVOKE_IF_NOT_NULL(Callback, *CurrentSpaceResult);
            })
        .then(csp::common::continuations::InvokeIfExceptionInChain(
            [this, CurrentSpaceResult, Callback](const std::exception& /*Except*/)
            {
                auto NullResultCallback
                    = [](const csp::systems::NullResult& Result) { return Result.GetResultCode() != csp::systems::EResultCode::InProgress; };

                this->DeleteSpace(CurrentSpaceResult->GetSpace().Id, NullResultCallback);

                Callback(MakeInvalid<SpaceResult>());
            },
            *csp::systems::SystemsManager::Get().GetLogSystem()));
}

void SpaceSystem::UpdateSpace(const String& SpaceId, const Optional<String>& Name, const Optional<String>& Description,
    const Optional<SpaceAttributes>& Attributes, const csp::common::Optional<csp::common::Array<csp::common::String>>& Tags,
    BasicSpaceResultCallback Callback)
{
    CSP_PROFILE_SCOPED();

    auto LiteGroupInfo = std::make_shared<chs::GroupLiteDto>();

    if (Name.HasValue())
    {
        LiteGroupInfo->SetName(*Name);
    }

    if (Description.HasValue())
    {
        LiteGroupInfo->SetDescription(*Description);
    }

    bool IsDiscoverable = false;
    bool RequiresInvite = true;

    if (Attributes.HasValue())
    {
        IsDiscoverable = HasFlag(*Attributes, SpaceAttributes::IsDiscoverable);
        RequiresInvite = HasFlag(*Attributes, SpaceAttributes::RequiresInvite);
    }

    if (Tags.HasValue())
    {
        LiteGroupInfo->SetTags(csp::common::Convert(Tags).value());
    }

    // Note that these are required fields from a services point of view.
    LiteGroupInfo->SetDiscoverable(IsDiscoverable);
    LiteGroupInfo->SetRequiresInvite(RequiresInvite);
    LiteGroupInfo->SetAutoModerator(false);

    csp::services::ResponseHandlerPtr ResponseHandler
        = GroupAPI->CreateHandler<BasicSpaceResultCallback, BasicSpaceResult, void, chs::GroupLiteDto>(Callback, nullptr);

    static_cast<chs::GroupApi*>(GroupAPI)->groupsGroupIdLitePut(SpaceId, LiteGroupInfo, ResponseHandler);
}

void SpaceSystem::DeleteSpace(const csp::common::String& SpaceId, NullResultCallback Callback)
{
    CSP_PROFILE_SCOPED();

    csp::services::ResponseHandlerPtr ResponseHandler = GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chsaggregation::SpaceApi*>(SpaceAPI)->spacesSpaceIdDelete(SpaceId, ResponseHandler);
}

void SpaceSystem::GetSpaces(SpacesResultCallback Callback)
{
    const auto* UserSystem = SystemsManager::Get().GetUserSystem();
    const String InUserId = UserSystem->GetLoginState().UserId;

    csp::services::ResponseHandlerPtr ResponseHandler
        = GroupAPI->CreateHandler<SpacesResultCallback, SpacesResult, void, csp::services::DtoArray<chs::GroupDto>>(Callback, nullptr);

    static_cast<chs::GroupApi*>(GroupAPI)->usersUserIdGroupsGet(InUserId, ResponseHandler);
}

void SpaceSystem::GetSpacesByAttributes(const Optional<bool>& InIsDiscoverable, const Optional<bool>& InIsArchived,
    const Optional<bool>& InRequiresInvite, const Optional<int>& InResultsSkip, const Optional<int>& InResultsMax,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& MustContainTags,
    const csp::common::Optional<csp::common::Array<csp::common::String>>& MustExcludeTags, const csp::common::Optional<bool>& InMustIncludeAllTags,
    BasicSpacesResultCallback Callback)
{
    auto IsDiscoverable = InIsDiscoverable.HasValue() ? *InIsDiscoverable : std::optional<bool>(std::nullopt);
    auto IsArchived = InIsArchived.HasValue() ? *InIsArchived : std::optional<bool>(std::nullopt);
    auto RequiresInvite = InRequiresInvite.HasValue() ? *InRequiresInvite : std::optional<bool>(std::nullopt);
    auto ResultsSkip = InResultsSkip.HasValue() ? *InResultsSkip : std::optional<int32_t>(std::nullopt);
    std::optional<int32_t> ResultsMax = InResultsMax.HasValue() ? std::min(MAX_SPACES_RESULTS, *InResultsMax) : MAX_SPACES_RESULTS;

    if (InResultsMax.HasValue() && *InResultsMax > MAX_SPACES_RESULTS)
    {
        CSP_LOG_WARN_FORMAT("Provided value `%i` for ResultsMax exceeded max value and was reduced to `%i`.", *InResultsMax, MAX_SPACES_RESULTS);
    }

    auto Tags
        = MustContainTags.HasValue() ? csp::common::Convert(MustContainTags).value() : std::optional<std::vector<csp::common::String>>(std::nullopt);
    auto ExcludedTags
        = MustExcludeTags.HasValue() ? csp::common::Convert(MustExcludeTags).value() : std::optional<std::vector<csp::common::String>>(std::nullopt);
    auto MustIncludeAllTags = InMustIncludeAllTags.HasValue() ? *InMustIncludeAllTags : std::optional<bool>(std::nullopt);

    csp::services::ResponseHandlerPtr ResponseHandler
        = GroupAPI->CreateHandler<BasicSpacesResultCallback, BasicSpacesResult, void, csp::services::DtoArray<chs::GroupLiteDto>>(Callback, nullptr);

    static_cast<chs::GroupApi*>(GroupAPI)->groupsLiteGet(std::nullopt, // Ids
        std::nullopt, // GroupTypes
        std::nullopt, // Names
        std::nullopt, // PartialName
        std::nullopt, // GroupOwnerIds
        std::nullopt, // ExcludeGroupOwnerIds
        std::nullopt, // ExcludeIds
        std::nullopt, // Users
        IsDiscoverable, // Discoverable
        std::nullopt, // AutoModerator
        RequiresInvite, // RequiresInvite
        IsArchived, // Archived
        std::nullopt, // OrganizationIds (no longer used)
        Tags, // Tags
        ExcludedTags, // ExcludedTags
        MustIncludeAllTags, // TagsAll
        ResultsSkip, // Skip
        ResultsMax, // Limit
        ResponseHandler);
}

void SpaceSystem::GetSpacesByIds(const Array<String>& RequestedSpaceIDs, SpacesResultCallback Callback)
{
    if (RequestedSpaceIDs.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("No space ids given");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SpacesResult>());

        return;
    }

    std::vector<String> SpaceIds;
    SpaceIds.reserve(RequestedSpaceIDs.Size());

    for (size_t idx = 0; idx < RequestedSpaceIDs.Size(); ++idx)
    {
        SpaceIds.push_back(RequestedSpaceIDs[idx]);
    }

    csp::services::ResponseHandlerPtr ResponseHandler
        = GroupAPI->CreateHandler<SpacesResultCallback, SpacesResult, void, csp::services::DtoArray<chs::GroupDto>>(Callback, nullptr);

    static_cast<chs::GroupApi*>(GroupAPI)->groupsGet(SpaceIds, ResponseHandler);
}

void SpaceSystem::GetSpacesForUserId(const String& UserId, SpacesResultCallback Callback)
{
    const String InUserId = UserId;

    csp::services::ResponseHandlerPtr ResponseHandler
        = GroupAPI->CreateHandler<SpacesResultCallback, SpacesResult, void, csp::services::DtoArray<chs::GroupDto>>(Callback, nullptr);

    static_cast<chs::GroupApi*>(GroupAPI)->usersUserIdGroupsGet(UserId, ResponseHandler);
}

void SpaceSystem::GetSpace(const String& SpaceId, SpaceResultCallback Callback)
{
    if (SpaceId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("No space id given");
        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SpaceResult>());
        return;
    }

    csp::services::ResponseHandlerPtr ResponseHandler
        = GroupAPI->CreateHandler<SpaceResultCallback, SpaceResult, void, chs::GroupDto>(Callback, nullptr, csp::web::EResponseCodes::ResponseOK);

    static_cast<chs::GroupApi*>(GroupAPI)->groupsGroupIdGet(SpaceId, ResponseHandler);
}

async::task<SpaceResult> SpaceSystem::GetSpace(const String& SpaceId)
{
    async::event_task<SpaceResult> OnCompleteEvent;
    async::task<SpaceResult> OnCompleteTask = OnCompleteEvent.get_task();

    if (SpaceId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("No space id given");
        OnCompleteEvent.set_exception(std::make_exception_ptr(std::exception()));
        return OnCompleteTask;
    }

    csp::services::ResponseHandlerPtr ResponseHandler = GroupAPI->CreateHandler<SpaceResultCallback, SpaceResult, void, chs::GroupDto>(
        [](const SpaceResult&) {}, nullptr, csp::web::EResponseCodes::ResponseOK, std::move(OnCompleteEvent));

    static_cast<chs::GroupApi*>(GroupAPI)->groupsGroupIdGet(SpaceId, ResponseHandler);

    return OnCompleteTask;
}

void SpaceSystem::InviteToSpace(const csp::common::String& SpaceId, const String& Email, const Optional<bool>& IsModeratorRole,
    const Optional<String>& EmailLinkUrl, const Optional<String>& SignupUrl, NullResultCallback Callback)
{
    auto GroupInviteInfo = std::make_shared<chs::GroupInviteDto>();
    GroupInviteInfo->SetEmail(Email);

    if (IsModeratorRole.HasValue())
    {
        GroupInviteInfo->SetAsModerator(*IsModeratorRole);
    }

    auto EmailLinkUrlParam = EmailLinkUrl.HasValue() && !EmailLinkUrl->IsEmpty() ? (*EmailLinkUrl) : std::optional<String>(std::nullopt);
    auto SignupUrlParam = SignupUrl.HasValue() && !SignupUrl->IsEmpty() ? (*SignupUrl) : std::optional<String>(std::nullopt);

    csp::services::ResponseHandlerPtr ResponseHandler = GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs::GroupApi*>(GroupAPI)->groupsGroupIdEmailInvitesPost(
        SpaceId, std::nullopt, EmailLinkUrlParam, SignupUrlParam, GroupInviteInfo, ResponseHandler);
}

void SpaceSystem::BulkInviteToSpace(const String& SpaceId, const InviteUserRoleInfoCollection& InviteUsers, NullResultCallback Callback)
{
    std::vector<std::shared_ptr<chs::GroupInviteDto>> GroupInvites
        = systems::SpaceSystemHelpers::GenerateGroupInvites(InviteUsers.InviteUserRoleInfos);

    auto EmailLinkUrlParam = !InviteUsers.EmailLinkUrl.IsEmpty() ? (InviteUsers.EmailLinkUrl) : std::optional<String>(std::nullopt);
    auto SignupUrlParam = !InviteUsers.SignupUrl.IsEmpty() ? (InviteUsers.SignupUrl) : std::optional<String>(std::nullopt);

    csp::services::ResponseHandlerPtr ResponseHandler = GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

    static_cast<chs::GroupApi*>(GroupAPI)->groupsGroupIdEmailInvitesBulkPost(
        SpaceId, std::nullopt, EmailLinkUrlParam, SignupUrlParam, GroupInvites, ResponseHandler);
}

async::task<NullResult> SpaceSystem::BulkInviteToSpace(const csp::common::String& SpaceId, const InviteUserRoleInfoCollection& InviteUsers)
{
    async::event_task<NullResult> OnCompleteEvent;
    async::task<NullResult> OnCompleteTask = OnCompleteEvent.get_task();

    std::vector<std::shared_ptr<chs::GroupInviteDto>> GroupInvites
        = systems::SpaceSystemHelpers::GenerateGroupInvites(InviteUsers.InviteUserRoleInfos);

    auto EmailLinkUrlParam = !InviteUsers.EmailLinkUrl.IsEmpty() ? (InviteUsers.EmailLinkUrl) : std::optional<String>(std::nullopt);
    auto SignupUrlParam = !InviteUsers.SignupUrl.IsEmpty() ? (InviteUsers.SignupUrl) : std::optional<String>(std::nullopt);

    csp::services::ResponseHandlerPtr ResponseHandler = GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
        [](const NullResult&) {}, nullptr, csp::web::EResponseCodes::ResponseNoContent, std::move(OnCompleteEvent));

    static_cast<chs::GroupApi*>(GroupAPI)->groupsGroupIdEmailInvitesBulkPost(
        SpaceId, std::nullopt, EmailLinkUrlParam, SignupUrlParam, GroupInvites, ResponseHandler);

    return OnCompleteTask;
}

void SpaceSystem::GetPendingUserInvites(const String& SpaceId, PendingInvitesResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = GroupAPI->CreateHandler<PendingInvitesResultCallback, PendingInvitesResult, void, csp::services::DtoArray<chs::GroupInviteDto>>(
            Callback, nullptr);

    static_cast<chs::GroupApi*>(GroupAPI)->groupsGroupIdEmailInvitesGet(SpaceId, ResponseHandler);
}

void SpaceSystem::GetAcceptedUserInvites(const String& SpaceId, AcceptedInvitesResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = GroupAPI->CreateHandler<AcceptedInvitesResultCallback, AcceptedInvitesResult, void, csp::services::DtoArray<chs::GroupInviteDto>>(
            Callback, nullptr);

    static_cast<chs::GroupApi*>(GroupAPI)->groupsGroupIdEmailInvitesAcceptedGet(SpaceId, ResponseHandler);
}

void SpaceSystem::AddUserToSpace(const csp::common::String& SpaceId, const String& UserId, SpaceResultCallback Callback)
{
    // This function right here is the only place in the whole of CSP that needs to use group code.
    // So, rather than bloat our `Space` class with the property, and give clients something that they have zero use for,
    // we prefer to pay the cost of an additional call to the cloud in the one place we need it, in order to retrieve it.
    // This function is not expected to be on any hot code path, so the perf cost is expected to be low. It's worth it for the api quality.

    GetSpace(SpaceId,
        [UserId, Callback, this](const SpaceResult& Result)
        {
            if (Result.GetResultCode() == EResultCode::InProgress)
            {
                return;
            }

            if (Result.GetResultCode() == EResultCode::Failed)
            {
                INVOKE_IF_NOT_NULL(Callback, Result);

                return;
            }

            const csp::common::String& SpaceCode = Result.GetSpaceCode();

            csp::services::ResponseHandlerPtr ResponseHandler
                = GroupAPI->CreateHandler<SpaceResultCallback, SpaceResult, void, chs::GroupDto>(Callback, nullptr);

            static_cast<chs::GroupApi*>(GroupAPI)->groupCodesGroupCodeUsersUserIdPut(SpaceCode, UserId, ResponseHandler);
        });
}

async::task<SpaceResult> SpaceSystem::AddUserToSpace(const csp::common::String& SpaceId, const String& UserId)
{
    // Because we react in a continuation, we need to keep the event alive, hence shared ptr.
    std::shared_ptr<async::event_task<SpaceResult>> OnCompleteEvent = std::make_shared<async::event_task<SpaceResult>>();
    async::task<SpaceResult> OnCompleteTask = OnCompleteEvent->get_task();

    GetSpace(SpaceId).then(async::inline_scheduler(),
        [UserId, OnCompleteEvent, this](const SpaceResult& Result) mutable
        {
            // .then continuations are only called for success of failure (completion), no need to handle inprogress
            if (Result.GetResultCode() == EResultCode::Failed)
            {
                OnCompleteEvent->set(Result); // Set the failure result for error handling
                return;
            }

            const csp::common::String& SpaceCode = Result.GetSpaceCode();

            csp::services::ResponseHandlerPtr ResponseHandler = GroupAPI->CreateHandler<SpaceResultCallback, SpaceResult, void, chs::GroupDto>(
                [](const SpaceResult&) {}, nullptr, csp::web::EResponseCodes::ResponseOK, std::move(*OnCompleteEvent.get()));

            static_cast<chs::GroupApi*>(GroupAPI)->groupCodesGroupCodeUsersUserIdPut(SpaceCode, UserId, ResponseHandler);
        });

    return OnCompleteTask;
}

void SpaceSystem::RemoveUserFromSpace(const String& SpaceId, const String& UserId, NullResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback, nullptr);

    static_cast<chs::GroupApi*>(GroupAPI)->groupsGroupIdUsersUserIdDelete(SpaceId, UserId, ResponseHandler);
}

void SpaceSystem::AddSiteInfo(const String& SpaceId, Site& SiteInfo, SiteResultCallback Callback)
{
    auto& SystemsManager = SystemsManager::Get();
    auto* POIInternalSystem = static_cast<PointOfInterestInternalSystem*>(SystemsManager.GetPointOfInterestSystem());

    SiteInfo.SpaceId = SpaceId;
    POIInternalSystem->CreateSite(SiteInfo, Callback);
}

void SpaceSystem::RemoveSiteInfo(const String& SpaceId, Site& SiteInfo, NullResultCallback Callback)
{
    auto& SystemsManager = SystemsManager::Get();
    auto* POIInternalSystem = static_cast<PointOfInterestInternalSystem*>(SystemsManager.GetPointOfInterestSystem());

    SiteInfo.SpaceId = SpaceId;
    POIInternalSystem->DeleteSite(SiteInfo, Callback);
}

void SpaceSystem::GetSitesInfo(const String& SpaceId, SitesCollectionResultCallback Callback)
{
    auto& SystemsManager = SystemsManager::Get();
    auto* POIInternalSystem = static_cast<PointOfInterestInternalSystem*>(SystemsManager.GetPointOfInterestSystem());

    POIInternalSystem->GetSites(SpaceId, Callback);
}

void SpaceSystem::UpdateUserRole(const String& SpaceId, const UserRoleInfo& NewUserRoleInfo, NullResultCallback Callback)
{
    const auto NewUserRole = NewUserRoleInfo.UserRole;
    const auto& UserId = NewUserRoleInfo.UserId;

    if (NewUserRole == SpaceUserRole::Owner)
    {
        csp::services::ResponseHandlerPtr ResponseHandler
            = GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback, nullptr);

        static_cast<chs::GroupApi*>(GroupAPI)->groupsGroupIdOwnerNewGroupOwnerIdPut(SpaceId, UserId, ResponseHandler);
    }
    else if (NewUserRole == SpaceUserRole::Moderator)
    {
        csp::services::ResponseHandlerPtr ResponseHandler = GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

        static_cast<chs::GroupApi*>(GroupAPI)->groupsGroupIdModeratorsUserIdPut(SpaceId, UserId, ResponseHandler);
    }
    else if (NewUserRole == SpaceUserRole::User)
    {
        // TODO: When the Client will be able to change the space owner role get a fresh Space object to see if the NewUserRoleInfo.UserId is
        // still a space owner
        if (SpaceId == NewUserRoleInfo.UserId)
        {
            // An owner must firstly pass the space ownership to someone else before it can become a user
            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

            return;
        }

        csp::services::ResponseHandlerPtr ResponseHandler = GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
            Callback, nullptr, csp::web::EResponseCodes::ResponseNoContent);

        static_cast<chs::GroupApi*>(GroupAPI)->groupsGroupIdModeratorsUserIdDelete(SpaceId, UserId, ResponseHandler);
    }
    else
    {
        CSP_LOG_ERROR_MSG("SpaceSystem::UpdateUserRole failed: Unsupported User Role!");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());
    }
}

void SpaceSystem::GetUsersRoles(const String& SpaceId, const Array<String>& RequestedUserIds, UserRoleCollectionCallback Callback)
{
    SpaceResultCallback GetSpaceCallback = [Callback, RequestedUserIds](const SpaceResult& SpaceResult)
    {
        if (SpaceResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        UserRoleCollectionResult InternalResult(SpaceResult.GetResultCode(), SpaceResult.GetHttpResultCode());

        if (SpaceResult.GetResultCode() == EResultCode::Success)
        {
            auto& Space = SpaceResult.GetSpace();
            InternalResult.FillUsersRoles(Space, RequestedUserIds);
        }

        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    };

    GetSpace(SpaceId, GetSpaceCallback);
}

void SpaceSystem::UpdateSpaceMetadata(const String& SpaceId, const Map<String, String>& NewMetadata, NullResultCallback Callback)
{
    if (SpaceId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("UpdateSpaceMetadata called with empty SpaceId. Aborting call.");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

        return;
    }

    AssetCollectionResultCallback MetadataAssetCollCallback = [Callback, NewMetadata](const AssetCollectionResult& Result)
    {
        if (Result.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        auto AssetSystem = SystemsManager::Get().GetAssetSystem();

        if (Result.GetResultCode() == EResultCode::Failed)
        {
            NullResult InternalResult(Result);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        AssetCollectionResultCallback UpdateAssetCollCallback = [Callback](const AssetCollectionResult& _Result)
        {
            NullResult InternalResult(_Result);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);
        };

        const auto& AssetCollection = Result.GetAssetCollection();
        AssetSystem->UpdateAssetCollectionMetadata(AssetCollection, NewMetadata, {}, UpdateAssetCollCallback);
    };

    GetMetadataAssetCollection(SpaceId, MetadataAssetCollCallback);
}

void SpaceSystem::GetSpacesMetadata(const Array<String>& SpaceIds, SpacesMetadataResultCallback Callback)
{
    AssetCollectionsResultCallback MetadataAssetCollCallback = [Callback](const AssetCollectionsResult& Result)
    {
        SpacesMetadataResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

        if (Result.GetResultCode() == EResultCode::Success)
        {
            Map<String, Map<String, String>> SpacesMetadata;
            Map<String, Array<String>> SpacesTags;
            const auto& AssetCollections = Result.GetAssetCollections();

            for (size_t i = 0; i < AssetCollections.Size(); ++i)
            {
                const auto& AssetCollection = AssetCollections[i];

                auto SpaceId = SpaceSystemHelpers::GetSpaceIdFromMetadataAssetCollectionName(AssetCollection.Name);

                SpacesMetadata[SpaceId] = systems::SpaceSystemHelpers::LegacyAssetConversion(AssetCollection);
            }

            InternalResult.SetMetadata(SpacesMetadata);
            InternalResult.SetTags(SpacesTags);
        }

        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    };

    GetMetadataAssetCollections(SpaceIds, MetadataAssetCollCallback);
}

void SpaceSystem::GetSpaceMetadata(const String& SpaceId, SpaceMetadataResultCallback Callback)
{
    if (SpaceId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("GetSpaceMetadata called with empty SpaceId. Aborting call.");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SpaceMetadataResult>());

        return;
    }

    AssetCollectionResultCallback MetadataAssetCollCallback = [Callback](const AssetCollectionResult& Result)
    {
        SpaceMetadataResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

        if (Result.GetResultCode() == EResultCode::Success)
        {
            const auto& AssetCollection = Result.GetAssetCollection();

            InternalResult.SetMetadata(systems::SpaceSystemHelpers::LegacyAssetConversion(AssetCollection));
        }

        INVOKE_IF_NOT_NULL(Callback, InternalResult);
    };

    GetMetadataAssetCollection(SpaceId, MetadataAssetCollCallback);
}

void SpaceSystem::UpdateSpaceThumbnail(const String& SpaceId, const FileAssetDataSource& NewThumbnail, NullResultCallback Callback)
{
    AssetCollectionsResultCallback ThumbnailAssetCollCallback = [Callback, SpaceId, NewThumbnail, this](const AssetCollectionsResult& AssetCollResult)
    {
        if (AssetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (AssetCollResult.GetResultCode() == EResultCode::Failed)
        {
            NullResult InternalResult(AssetCollResult);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        const auto& AssetCollections = AssetCollResult.GetAssetCollections();

        if (AssetCollections.IsEmpty())
        {
            // space without a thumbnail
            AddSpaceThumbnail(SpaceId, NewThumbnail, Callback);

            return;
        }

        const auto& ThumbnailAssetCollection = AssetCollections[0];

        AssetsResultCallback ThumbnailAssetCallback = [Callback, NewThumbnail, ThumbnailAssetCollection](const AssetsResult& AssetsResult)
        {
            if (AssetsResult.GetResultCode() == EResultCode::InProgress)
            {
                return;
            }

            if (AssetsResult.GetResultCode() == EResultCode::Failed)
            {
                NullResult InternalResult(AssetsResult);
                INVOKE_IF_NOT_NULL(Callback, InternalResult);

                return;
            }

            UriResultCallback UploadCallback = [Callback](const UriResult& UploadResult)
            {
                if (UploadResult.GetResultCode() == EResultCode::Failed)
                {
                    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
                        (int)UploadResult.GetResultCode(), UploadResult.GetHttpResultCode());
                }

                NullResult InternalResult(UploadResult);
                INVOKE_IF_NOT_NULL(Callback, InternalResult);
            };

            auto& ThumbnailAsset = ((csp::systems::AssetsResult&)AssetsResult).GetAssets()[0];
            ThumbnailAsset.MimeType = NewThumbnail.GetMimeType();

            auto* AssetSystem = SystemsManager::Get().GetAssetSystem();
            AssetSystem->UploadAssetData(ThumbnailAssetCollection, ThumbnailAsset, NewThumbnail, UploadCallback);
        };

        GetSpaceThumbnailAsset(ThumbnailAssetCollection, ThumbnailAssetCallback);
    };

    GetSpaceThumbnailAssetCollection(SpaceId, ThumbnailAssetCollCallback);
}

void SpaceSystem::UpdateSpaceThumbnailWithBuffer(const String& SpaceId, const BufferAssetDataSource& NewThumbnail, NullResultCallback Callback)
{
    AssetCollectionsResultCallback ThumbnailAssetCollCallback = [Callback, SpaceId, NewThumbnail, this](const AssetCollectionsResult& AssetCollResult)
    {
        if (AssetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (AssetCollResult.GetResultCode() == EResultCode::Failed)
        {
            const NullResult InternalResult(AssetCollResult);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        const auto& AssetCollections = AssetCollResult.GetAssetCollections();

        if (AssetCollections.IsEmpty())
        {
            // Space without a thumbnail
            AddSpaceThumbnailWithBuffer(SpaceId, NewThumbnail, Callback);

            return;
        }

        const auto& ThumbnailAssetCollection = AssetCollections[0];

        AssetsResultCallback ThumbnailAssetCallback = [Callback, NewThumbnail, ThumbnailAssetCollection](const AssetsResult& AssetsResult)
        {
            if (AssetsResult.GetResultCode() == EResultCode::InProgress)
            {
                return;
            }

            if (AssetsResult.GetResultCode() == EResultCode::Failed)
            {
                NullResult InternalResult(AssetsResult);
                INVOKE_IF_NOT_NULL(Callback, InternalResult);

                return;
            }

            UriResultCallback UploadCallback = [Callback](const UriResult& UploadResult)
            {
                if (UploadResult.GetResultCode() == EResultCode::Failed)
                {
                    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
                        (int)UploadResult.GetResultCode(), UploadResult.GetHttpResultCode());
                }

                NullResult InternalResult(UploadResult);
                INVOKE_IF_NOT_NULL(Callback, InternalResult);
            };

            auto& ThumbnailAsset = ((csp::systems::AssetsResult&)AssetsResult).GetAssets()[0];
            ThumbnailAsset.FileName
                = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(SpaceSystemHelpers::GetAssetFileExtension(NewThumbnail.GetMimeType()));
            ThumbnailAsset.MimeType = NewThumbnail.GetMimeType();
            const auto AssetSystem = SystemsManager::Get().GetAssetSystem();
            AssetSystem->UploadAssetData(ThumbnailAssetCollection, ThumbnailAsset, NewThumbnail, UploadCallback);
        };

        GetSpaceThumbnailAsset(ThumbnailAssetCollection, ThumbnailAssetCallback);
    };

    GetSpaceThumbnailAssetCollection(SpaceId, ThumbnailAssetCollCallback);
}

void SpaceSystem::GetSpaceThumbnail(const String& SpaceId, UriResultCallback Callback)
{
    AssetCollectionsResultCallback ThumbnailAssetCollCallback = [Callback, this](const AssetCollectionsResult& AssetCollResult)
    {
        if (AssetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (AssetCollResult.GetResultCode() == EResultCode::Failed)
        {
            const UriResult InternalResult(AssetCollResult.GetResultCode(), AssetCollResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        const auto& AssetCollections = AssetCollResult.GetAssetCollections();

        if (AssetCollections.IsEmpty())
        {
            // Space doesn't have a thumbnail
            UriResult InternalResult(EResultCode::Success, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNotFound));
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        const auto& ThumbnailAssetCollection = AssetCollections[0];

        AssetsResultCallback ThumbnailAssetCallback = [Callback](const AssetsResult& AssetsResult)
        {
            if (AssetsResult.GetResultCode() == EResultCode::InProgress)
            {
                return;
            }

            UriResult InternalResult(AssetsResult.GetResultCode(), AssetsResult.GetHttpResultCode());

            if (AssetsResult.GetResultCode() == EResultCode::Success)
            {
                InternalResult.SetUri(AssetsResult.GetAssets()[0].Uri);
            }

            INVOKE_IF_NOT_NULL(Callback, InternalResult);
        };

        GetSpaceThumbnailAsset(ThumbnailAssetCollection, ThumbnailAssetCallback);
    };

    GetSpaceThumbnailAssetCollection(SpaceId, ThumbnailAssetCollCallback);
}

void SpaceSystem::AddUserToSpaceBanList(const String& SpaceId, const String& RequestedUserId, NullResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback, nullptr);
    static_cast<chs::GroupApi*>(GroupAPI)->groupsGroupIdBannedUsersUserIdPut(SpaceId, RequestedUserId, ResponseHandler);
}

void SpaceSystem::DeleteUserFromSpaceBanList(const String& SpaceId, const String& RequestedUserId, NullResultCallback Callback)
{
    csp::services::ResponseHandlerPtr ResponseHandler
        = GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback, nullptr);
    static_cast<chs::GroupApi*>(GroupAPI)->groupsGroupIdBannedUsersUserIdDelete(SpaceId, RequestedUserId, ResponseHandler);
}

void SpaceSystem::GetMetadataAssetCollection(const String& SpaceId, AssetCollectionResultCallback Callback)
{
    auto* AssetSystem = SystemsManager::Get().GetAssetSystem();
    auto MetadataAssetCollectionName = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(SpaceId);

    AssetSystem->GetAssetCollectionByName(MetadataAssetCollectionName, Callback);
}

void SpaceSystem::GetMetadataAssetCollections(const Array<csp::common::String>& SpaceIds, AssetCollectionsResultCallback Callback)
{
    auto* AssetSystem = SystemsManager::Get().GetAssetSystem();
    Array<String> PrototypeNames(SpaceIds.Size());

    for (size_t item = 0; item < SpaceIds.Size(); ++item)
    {
        PrototypeNames[item] = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(SpaceIds[item]);
    }

    AssetSystem->FindAssetCollections(nullptr, nullptr, PrototypeNames, nullptr, nullptr, nullptr, nullptr, nullptr, Callback);
}

void SpaceSystem::RemoveMetadata(const String& SpaceId, NullResultCallback Callback)
{
    if (SpaceId.IsEmpty())
    {
        CSP_LOG_ERROR_MSG("RemoveMetadata called with empty SpaceId. Aborting call.");

        INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

        return;
    }

    AssetCollectionResultCallback GetAssetCollCallback = [Callback](const AssetCollectionResult& AssetCollResult)
    {
        if (AssetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (AssetCollResult.GetResultCode() == EResultCode::Failed)
        {
            NullResult InternalResult(AssetCollResult);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        NullResultCallback DeleteAssetCollCallback = [Callback](const NullResult& Result)
        {
            NullResult InternalResult(Result);
            INVOKE_IF_NOT_NULL(Callback, Result);
        };

        auto* AssetSystem = SystemsManager::Get().GetAssetSystem();
        AssetSystem->DeleteAssetCollection(AssetCollResult.GetAssetCollection(), DeleteAssetCollCallback);
    };

    GetMetadataAssetCollection(SpaceId, GetAssetCollCallback);
}

void SpaceSystem::AddSpaceThumbnail(const csp::common::String& SpaceId, const FileAssetDataSource& ImageDataSource, NullResultCallback Callback)
{
    auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

    AssetCollectionResultCallback CreateAssetCollCallback
        = [Callback, AssetSystem, SpaceId, ImageDataSource](const AssetCollectionResult& AssetCollResult)
    {
        if (AssetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (AssetCollResult.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Log,
                "The Space thumbnail asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
                (int)AssetCollResult.GetResultCode(), AssetCollResult.GetHttpResultCode());

            NullResult InternalResult(AssetCollResult);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        const auto& ThumbnailAssetColl = AssetCollResult.GetAssetCollection();

        AssetResultCallback CreateAssetCallback = [Callback, AssetSystem, ThumbnailAssetColl, ImageDataSource](const AssetResult& CreateAssetResult)
        {
            if (CreateAssetResult.GetResultCode() == EResultCode::InProgress)
            {
                return;
            }

            if (CreateAssetResult.GetResultCode() == EResultCode::Failed)
            {
                CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail asset creation was not successful. ResCode: %d, HttpResCode: %d",
                    (int)CreateAssetResult.GetResultCode(), CreateAssetResult.GetHttpResultCode());

                NullResult InternalResult(CreateAssetResult);
                INVOKE_IF_NOT_NULL(Callback, InternalResult);

                return;
            }

            UriResultCallback UploadCallback = [Callback](const UriResult& UploadResult)
            {
                if (UploadResult.GetResultCode() == EResultCode::Failed)
                {
                    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
                        (int)UploadResult.GetResultCode(), UploadResult.GetHttpResultCode());
                }

                const NullResult InternalResult(UploadResult);
                INVOKE_IF_NOT_NULL(Callback, InternalResult);
            };

            AssetSystem->UploadAssetData(ThumbnailAssetColl, CreateAssetResult.GetAsset(), ImageDataSource, UploadCallback);
        };

        const auto UniqueAssetName = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(SpaceId);
        AssetSystem->CreateAsset(ThumbnailAssetColl, UniqueAssetName, nullptr, nullptr, EAssetType::IMAGE, CreateAssetCallback);
    };

    const String SpaceThumbnailAssetCollectionName = SpaceSystemHelpers::GetSpaceThumbnailAssetCollectionName(SpaceId);
    const auto Tag = Array<String>({ SpaceId });

    // don't associate this asset collection with a particular space so that it can be retrieved by guest users that have not joined this
    // space
    AssetSystem->CreateAssetCollection(
        SpaceId, nullptr, SpaceThumbnailAssetCollectionName, nullptr, EAssetCollectionType::SPACE_THUMBNAIL, Tag, CreateAssetCollCallback);
}

void SpaceSystem::AddSpaceThumbnailWithBuffer(
    const csp::common::String& SpaceId, const BufferAssetDataSource& ImageDataSource, NullResultCallback Callback)
{
    auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

    AssetCollectionResultCallback CreateAssetCollCallback
        = [Callback, SpaceId, ImageDataSource, AssetSystem](const AssetCollectionResult& AssetCollResult)
    {
        if (AssetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (AssetCollResult.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Log,
                "The Space thumbnail asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
                (int)AssetCollResult.GetResultCode(), AssetCollResult.GetHttpResultCode());

            NullResult InternalResult(AssetCollResult);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        const auto& ThumbnailAssetColl = AssetCollResult.GetAssetCollection();

        AssetResultCallback CreateAssetCallback = [Callback, ImageDataSource, ThumbnailAssetColl, AssetSystem](const AssetResult& CreateAssetResult)
        {
            if (CreateAssetResult.GetResultCode() == EResultCode::InProgress)
            {
                return;
            }

            if (CreateAssetResult.GetResultCode() == EResultCode::Failed)
            {
                CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail asset creation was not successful. ResCode: %d, HttpResCode: %d",
                    (int)CreateAssetResult.GetResultCode(), CreateAssetResult.GetHttpResultCode());

                NullResult InternalResult(CreateAssetResult);
                INVOKE_IF_NOT_NULL(Callback, InternalResult);

                return;
            }

            UriResultCallback UploadCallback = [Callback](const UriResult& UploadResult)
            {
                if (UploadResult.GetResultCode() == EResultCode::Failed)
                {
                    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
                        (int)UploadResult.GetResultCode(), UploadResult.GetHttpResultCode());
                }

                NullResult InternalResult(UploadResult);
                INVOKE_IF_NOT_NULL(Callback, InternalResult);
            };

            Asset ThumbnailAsset = CreateAssetResult.GetAsset();

            ThumbnailAsset.FileName
                = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(SpaceSystemHelpers::GetAssetFileExtension(ImageDataSource.GetMimeType()));
            ThumbnailAsset.MimeType = ImageDataSource.GetMimeType();
            AssetSystem->UploadAssetData(ThumbnailAssetColl, ThumbnailAsset, ImageDataSource, UploadCallback);
        };

        const auto UniqueAssetName = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(SpaceId);
        AssetSystem->CreateAsset(ThumbnailAssetColl, UniqueAssetName, nullptr, nullptr, EAssetType::IMAGE, CreateAssetCallback);
    };

    const String SpaceThumbnailAssetCollectionName = SpaceSystemHelpers::GetSpaceThumbnailAssetCollectionName(SpaceId);
    const Array<String> Tag({ SpaceId });

    AssetSystem->CreateAssetCollection(
        SpaceId, nullptr, SpaceThumbnailAssetCollectionName, nullptr, EAssetCollectionType::SPACE_THUMBNAIL, Tag, CreateAssetCollCallback);
}

void SpaceSystem::GetSpaceThumbnailAssetCollection(const csp::common::String& SpaceId, AssetCollectionsResultCallback Callback)
{
    AssetCollectionsResultCallback GetAssetCollCallback = [Callback](const AssetCollectionsResult& AssetCollResult)
    {
        if (AssetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (AssetCollResult.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail asset collection retrieval has failed. ResCode: %d, HttpResCode: %d",
                (int)AssetCollResult.GetResultCode(), AssetCollResult.GetHttpResultCode());
        }

        INVOKE_IF_NOT_NULL(Callback, AssetCollResult);
    };

    auto* AssetSystem = SystemsManager::Get().GetAssetSystem();
    auto MetadataAssetCollectionName = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(SpaceId);

    Array<csp::systems::EAssetCollectionType> PrototypeTypes = { EAssetCollectionType::SPACE_THUMBNAIL };
    Array<String> PrototypeTags = { SpaceId };
    Array<String> GroupIds = { SpaceId };

    AssetSystem->FindAssetCollections(nullptr, nullptr, nullptr, PrototypeTypes, PrototypeTags, GroupIds, nullptr, nullptr, GetAssetCollCallback);
}

void SpaceSystem::GetSpaceThumbnailAsset(const AssetCollection& ThumbnailAssetCollection, AssetsResultCallback Callback)
{
    AssetsResultCallback ThumbnailAssetCallback = [Callback](const AssetsResult& AssetsResult)
    {
        if (AssetsResult.GetResultCode() == EResultCode::Failed)
        {
            CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail asset retrieval has failed. ResCode: %d, HttpResCode: %d",
                (int)AssetsResult.GetResultCode(), AssetsResult.GetHttpResultCode());
        }
        else if (AssetsResult.GetResultCode() == EResultCode::Success)
        {
            assert(!AssetsResult.GetAssets().IsEmpty() && "Space thumbnail asset should exist");
            assert((AssetsResult.GetAssets().Size() == 1) && "There should be only one Space thumbnail asset");
        }

        INVOKE_IF_NOT_NULL(Callback, AssetsResult);
    };

    auto* AssetSystem = SystemsManager::Get().GetAssetSystem();
    AssetSystem->GetAssetsInCollection(ThumbnailAssetCollection, ThumbnailAssetCallback);
}

void SpaceSystem::RemoveSpaceThumbnail(const csp::common::String& SpaceId, NullResultCallback Callback)
{
    auto* AssetSystem = SystemsManager::Get().GetAssetSystem();

    AssetCollectionsResultCallback ThumbnailAssetCollCallback = [Callback, AssetSystem, this](const AssetCollectionsResult& AssetCollResult)
    {
        if (AssetCollResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (AssetCollResult.GetResultCode() == EResultCode::Failed)
        {
            NullResult InternalResult(AssetCollResult);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        const auto& AssetCollections = AssetCollResult.GetAssetCollections();

        if (AssetCollections.IsEmpty())
        {
            // Space doesn't have a thumbnail so we're done
            NullResult InternalResult(AssetCollResult);
            INVOKE_IF_NOT_NULL(Callback, InternalResult);

            return;
        }

        const auto& ThumbnailAssetCollection = AssetCollections[0];

        AssetsResultCallback ThumbnailAssetCallback = [Callback, AssetSystem, ThumbnailAssetCollection](const AssetsResult& AssetsResult)
        {
            if (AssetsResult.GetResultCode() == EResultCode::InProgress)
            {
                return;
            }

            if (AssetsResult.GetResultCode() == EResultCode::Failed)
            {
                NullResult InternalResult(AssetsResult);
                INVOKE_IF_NOT_NULL(Callback, InternalResult);

                return;
            }

            NullResultCallback DeleteAssetCallback = [Callback, AssetSystem, ThumbnailAssetCollection](const NullResult& DeleteAssetResult)
            {
                if (DeleteAssetResult.GetResultCode() == EResultCode::InProgress)
                {
                    return;
                }

                if (DeleteAssetResult.GetResultCode() == EResultCode::Failed)
                {
                    CSP_LOG_FORMAT(csp::common::LogLevel::Log, "The Space thumbnail asset deletion was not successful. ResCode: %d, HttpResCode: %d",
                        (int)DeleteAssetResult.GetResultCode(), DeleteAssetResult.GetHttpResultCode());

                    NullResult InternalResult(DeleteAssetResult);
                    INVOKE_IF_NOT_NULL(Callback, DeleteAssetResult);

                    return;
                }

                NullResultCallback DeleteAssetCollCallback = [Callback, DeleteAssetResult](const NullResult& DeleteAssetCollResult)
                {
                    if (DeleteAssetCollResult.GetResultCode() == EResultCode::InProgress)
                    {
                        return;
                    }

                    if (DeleteAssetCollResult.GetResultCode() == EResultCode::Failed)
                    {
                        CSP_LOG_FORMAT(csp::common::LogLevel::Log,
                            "The Space thumbnail asset collection deletion has failed. ResCode: %d, HttpResCode: %d",
                            (int)DeleteAssetResult.GetResultCode(), DeleteAssetResult.GetHttpResultCode());
                    }

                    NullResult InternalResult(DeleteAssetCollResult);
                    INVOKE_IF_NOT_NULL(Callback, DeleteAssetCollResult);
                };

                AssetSystem->DeleteAssetCollection(ThumbnailAssetCollection, DeleteAssetCollCallback);
            };

            const auto& ThumbnailAsset = AssetsResult.GetAssets()[0];
            AssetSystem->DeleteAsset(ThumbnailAssetCollection, ThumbnailAsset, DeleteAssetCallback);
        };

        GetSpaceThumbnailAsset(ThumbnailAssetCollection, ThumbnailAssetCallback);
    };

    GetSpaceThumbnailAssetCollection(SpaceId, ThumbnailAssetCollCallback);
}

void SpaceSystem::GetSpaceGeoLocationInternal(const csp::common::String& SpaceId, SpaceGeoLocationResultCallback Callback)
{
    auto& SystemsManager = SystemsManager::Get();
    auto* POIInternalSystem = static_cast<PointOfInterestInternalSystem*>(SystemsManager.GetPointOfInterestSystem());
    POIInternalSystem->GetSpaceGeoLocation(SpaceId, Callback);
}

void SpaceSystem::GetSpaceGeoLocation(const csp::common::String& SpaceId, SpaceGeoLocationResultCallback Callback)
{
    // First refresh the space to ensure the user has access to the space
    SpaceResultCallback GetSpaceCallback = [Callback, this](const SpaceResult& GetSpaceResult)
    {
        if (GetSpaceResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (GetSpaceResult.GetResultCode() == EResultCode::Failed)
        {
            SpaceGeoLocationResult Result(GetSpaceResult.GetResultCode(), GetSpaceResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, Result);

            return;
        }

        const auto& RefreshedSpace = GetSpaceResult.GetSpace();
        const auto& UserId = SystemsManager::Get().GetUserSystem()->GetLoginState().UserId;

        // First check if the user is the owner
        bool UserCanAccessSpaceDetails = !(bool)(RefreshedSpace.Attributes & SpaceAttributes::RequiresInvite) || RefreshedSpace.OwnerId == UserId;

        // If the user is not the owner check are they a moderator
        if (!UserCanAccessSpaceDetails)
        {
            UserCanAccessSpaceDetails = systems::SpaceSystemHelpers::IdCheck(UserId, RefreshedSpace.ModeratorIds);
        }

        // If the user is not the owner or a moderator check are the full user list
        if (!UserCanAccessSpaceDetails)
        {
            UserCanAccessSpaceDetails = systems::SpaceSystemHelpers::IdCheck(UserId, RefreshedSpace.UserIds);
        }

        if (!UserCanAccessSpaceDetails)
        {
            SpaceGeoLocationResult Result(EResultCode::Failed, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));
            INVOKE_IF_NOT_NULL(Callback, Result);

            return;
        }

        GetSpaceGeoLocationInternal(RefreshedSpace.Id, Callback);
    };

    GetSpace(SpaceId, GetSpaceCallback);
}

void SpaceSystem::UpdateSpaceGeoLocation(const csp::common::String& SpaceId, const csp::common::Optional<GeoLocation>& Location,
    const csp::common::Optional<float>& Orientation, const csp::common::Optional<csp::common::Array<GeoLocation>>& GeoFence,
    SpaceGeoLocationResultCallback Callback)
{
    SpaceGeoLocationResultCallback GetSpaceGeoLocationCallback
        = [Callback, SpaceId, Location, Orientation, GeoFence](const SpaceGeoLocationResult& GetGeoLocationResult)
    {
        if (GetGeoLocationResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (GetGeoLocationResult.GetResultCode() == EResultCode::Failed)
        {
            INVOKE_IF_NOT_NULL(Callback, GetGeoLocationResult);

            return;
        }

        auto& SystemsManager = SystemsManager::Get();
        auto* POIInternalSystem = static_cast<PointOfInterestInternalSystem*>(SystemsManager.GetPointOfInterestSystem());

        if (GetGeoLocationResult.HasGeoLocation)
        {
            POIInternalSystem->UpdateSpaceGeoLocation(
                SpaceId, GetGeoLocationResult.GetSpaceGeoLocation().Id, Location, Orientation, GeoFence, Callback);
        }
        else
        {
            POIInternalSystem->AddSpaceGeoLocation(SpaceId, Location, Orientation, GeoFence, Callback);
        }
    };

    // First refresh the space to ensure the user has access to the space
    SpaceResultCallback GetSpaceCallback = [Callback, GetSpaceGeoLocationCallback, this](const SpaceResult& GetSpaceResult)
    {
        if (GetSpaceResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (GetSpaceResult.GetResultCode() == EResultCode::Failed)
        {
            SpaceGeoLocationResult Result(GetSpaceResult.GetResultCode(), GetSpaceResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, Result);

            return;
        }

        const auto& RefreshedSpace = GetSpaceResult.GetSpace();
        const auto& UserId = SystemsManager::Get().GetUserSystem()->GetLoginState().UserId;

        // First check if the user is the owner
        bool UserCanModifySpace = RefreshedSpace.OwnerId == UserId;

        // If the user is not the owner check are they a moderator
        if (!UserCanModifySpace)
        {
            UserCanModifySpace = systems::SpaceSystemHelpers::IdCheck(UserId, RefreshedSpace.ModeratorIds);
        }

        if (!UserCanModifySpace)
        {
            SpaceGeoLocationResult Result(EResultCode::Failed, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));
            INVOKE_IF_NOT_NULL(Callback, Result);

            return;
        }

        GetSpaceGeoLocationInternal(RefreshedSpace.Id, GetSpaceGeoLocationCallback);
    };

    GetSpace(SpaceId, GetSpaceCallback);
}

void SpaceSystem::DeleteSpaceGeoLocation(const csp::common::String& SpaceId, NullResultCallback Callback)
{
    SpaceGeoLocationResultCallback GetSpaceGeoLocationCallback = [Callback](const SpaceGeoLocationResult& GetGeoLocationResult)
    {
        if (GetGeoLocationResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        auto& SystemsManager = SystemsManager::Get();
        auto* POIInternalSystem = static_cast<PointOfInterestInternalSystem*>(SystemsManager.GetPointOfInterestSystem());

        if (!GetGeoLocationResult.HasGeoLocation)
        {
            INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

            return;
        }

        POIInternalSystem->DeleteSpaceGeoLocation(GetGeoLocationResult.GetSpaceGeoLocation().Id, Callback);
    };

    // First refresh the space to ensure the user has access to the space
    SpaceResultCallback GetSpaceCallback = [Callback, GetSpaceGeoLocationCallback, this](const SpaceResult& GetSpaceResult)
    {
        if (GetSpaceResult.GetResultCode() == EResultCode::InProgress)
        {
            return;
        }

        if (GetSpaceResult.GetResultCode() == EResultCode::Failed)
        {
            NullResult Result(GetSpaceResult.GetResultCode(), GetSpaceResult.GetHttpResultCode());
            INVOKE_IF_NOT_NULL(Callback, Result);

            return;
        }

        const auto& RefreshedSpace = GetSpaceResult.GetSpace();
        const auto& UserId = SystemsManager::Get().GetUserSystem()->GetLoginState().UserId;

        // First check if the user is the owner
        bool UserCanModifySpace = RefreshedSpace.OwnerId == UserId;

        // If the user is not the owner check are they a moderator
        if (!UserCanModifySpace)
        {
            UserCanModifySpace = systems::SpaceSystemHelpers::IdCheck(UserId, RefreshedSpace.ModeratorIds);
        }

        if (!UserCanModifySpace)
        {
            NullResult Result(EResultCode::Failed, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));
            INVOKE_IF_NOT_NULL(Callback, Result);

            return;
        }

        GetSpaceGeoLocationInternal(RefreshedSpace.Id, GetSpaceGeoLocationCallback);
    };

    GetSpace(SpaceId, GetSpaceCallback);
}

void SpaceSystem::DuplicateSpace(const String& SpaceId, const String& NewName, SpaceAttributes NewAttributes,
    const Optional<Array<String>>& MemberGroupIds, bool ShallowCopy, SpaceResultCallback Callback)
{
    auto Request = std::make_shared<chsaggregation::DuplicateSpaceRequest>();
    Request->SetSpaceId(SpaceId);
    Request->SetNewGroupOwnerId(SystemsManager::Get().GetUserSystem()->GetLoginState().UserId);
    Request->SetNewUniqueName(NewName);
    Request->SetDiscoverable(HasFlag(NewAttributes, csp::systems::SpaceAttributes::IsDiscoverable));
    Request->SetRequiresInvite(HasFlag(NewAttributes, csp::systems::SpaceAttributes::RequiresInvite));
    Request->SetShallowCopy(ShallowCopy);

    if (MemberGroupIds.HasValue())
    {
        std::vector<String> GroupIds;
        GroupIds.reserve(MemberGroupIds->Size());

        for (size_t i = 0; i < MemberGroupIds->Size(); ++i)
        {
            GroupIds.push_back(MemberGroupIds->operator[](i));
        }

        Request->SetMemberGroupIds(GroupIds);
    }

    csp::services::ResponseHandlerPtr ResponseHandler
        = SpaceAPI->CreateHandler<csp::systems::SpaceResultCallback, csp::systems::SpaceResult, void, chs::GroupDto>(Callback, nullptr);

    static_cast<chsaggregation::SpaceApi*>(SpaceAPI)->spacesSpaceIdDuplicatePost(SpaceId, // spaceId
        false, // asyncCall
        Request, // RequestBody
        ResponseHandler // ResponseHandler
    );
}
} // namespace csp::systems
