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
#include "CSP/Common/StringFormat.h"
#include "CSP/Multiplayer/MultiPlayerConnection.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/SystemsManager.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "CallHelpers.h"
#include "Debug/Logging.h"
#include "Events/EventSystem.h"
#include "Services/AggregationService/Api.h"
#include "Services/AggregationService/Dto.h"
#include "Services/UserService/Api.h"
#include "Services/UserService/Dto.h"
#include "Systems/ResultHelpers.h"
#include "Systems/Spaces/SpaceSystemHelpers.h"
#include "Systems/Spatial/PointOfInterestInternalSystem.h"

#include <iostream>
#include <rapidjson/rapidjson.h>


using namespace csp;
using namespace csp::common;

namespace chs			 = csp::services::generated::userservice;
namespace chsaggregation = csp::services::generated::aggregationservice;


namespace
{

constexpr const int MAX_SPACES_RESULTS = 100;

void CreateSpace(chs::GroupApi* GroupAPI,
				 const String& Name,
				 const String& Description,
				 csp::systems::SpaceAttributes Attributes,
				 csp::systems::SpaceResultCallback Callback)
{
	auto GroupInfo = systems::SpaceSystemHelpers::DefaultGroupInfo();
	GroupInfo->SetName(Name);
	GroupInfo->SetDescription(Description);
	GroupInfo->SetDiscoverable(HasFlag(Attributes, csp::systems::SpaceAttributes::IsDiscoverable));
	GroupInfo->SetRequiresInvite(HasFlag(Attributes, csp::systems::SpaceAttributes::RequiresInvite));

	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<csp::systems::SpaceResultCallback, csp::systems::SpaceResult, void, chs::GroupDto>(Callback, nullptr);

	GroupAPI->apiV1GroupsPost(GroupInfo, ResponseHandler);
}

} // namespace


namespace csp::systems
{

SpaceSystem::SpaceSystem() : SystemBase(), GroupAPI(nullptr), SpaceAPI(nullptr)
{
}

SpaceSystem::SpaceSystem(csp::web::WebClient* InWebClient) : SystemBase(InWebClient), CurrentSpace()
{
	GroupAPI = CSP_NEW chs::GroupApi(InWebClient);
	SpaceAPI = CSP_NEW chsaggregation::SpaceApi(InWebClient);
}

SpaceSystem::~SpaceSystem()
{
	CSP_DELETE(GroupAPI);
}

void SpaceSystem::EnterSpace(const String& SpaceId, NullResultCallback Callback)
{
	SpaceResultCallback GetSpaceCallback = [Callback, SpaceId, this](const SpaceResult& GetSpaceResult)
	{
		if (GetSpaceResult.GetResultCode() == EResultCode::InProgress)
		{
			return;
		}

		if (GetSpaceResult.GetResultCode() == EResultCode::Failed)
		{
			NullResult InternalResult(GetSpaceResult.GetResultCode(), GetSpaceResult.GetHttpResultCode());
			INVOKE_IF_NOT_NULL(Callback, InternalResult);

			return;
		}

		const auto& RefreshedSpace = GetSpaceResult.GetSpace();

		CSP_LOG_FORMAT(LogLevel::Log, "Entering Space %s", RefreshedSpace.Name.c_str());

		const String UserId = SystemsManager::Get().GetUserSystem()->GetLoginState().UserId;

		if (!HasFlag(RefreshedSpace.Attributes, SpaceAttributes::RequiresInvite))
		{
			AddUserToSpace(SpaceId,
						   UserId,
						   [Callback, SpaceId, GetSpaceResult, RefreshedSpace, this](const SpaceResult& Result)
						   {
							   if (Result.GetResultCode() == EResultCode::InProgress)
							   {
								   return;
							   }

							   NullResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

							   if (Result.GetResultCode() == EResultCode::Success)
							   {
								   CurrentSpace = RefreshedSpace;

								   csp::events::Event* EnterSpaceEvent
									   = csp::events::EventSystem::Get().AllocateEvent(csp::events::SPACESYSTEM_ENTER_SPACE_EVENT_ID);
								   EnterSpaceEvent->AddString("SpaceId", SpaceId);
								   csp::events::EventSystem::Get().EnqueueEvent(EnterSpaceEvent);
							   }

							   INVOKE_IF_NOT_NULL(Callback, InternalResult);
						   });
		}
		else
		{
			// First check if the user is the owner
			bool EnterSuccess = RefreshedSpace.OwnerId == UserId;

			// If the user is not the owner check are they a moderator
			if (!EnterSuccess)
			{
				EnterSuccess = systems::SpaceSystemHelpers::IdCheck(UserId, RefreshedSpace.ModeratorIds);
			}

			// Finally check all users in the group
			if (!EnterSuccess)
			{
				EnterSuccess = systems::SpaceSystemHelpers::IdCheck(UserId, RefreshedSpace.UserIds);
			}

			NullResult InternalResult(GetSpaceResult.GetResultCode(), GetSpaceResult.GetHttpResultCode());

			if (EnterSuccess)
			{
				CurrentSpace						= GetSpaceResult.GetSpace();
				csp::events::Event* EnterSpaceEvent = csp::events::EventSystem::Get().AllocateEvent(csp::events::SPACESYSTEM_ENTER_SPACE_EVENT_ID);
				EnterSpaceEvent->AddString("SpaceId", SpaceId);
				csp::events::EventSystem::Get().EnqueueEvent(EnterSpaceEvent);
			}

			INVOKE_IF_NOT_NULL(Callback, InternalResult);
		}
	};

	GetSpace(SpaceId, GetSpaceCallback);
}

void SpaceSystem::ExitSpace()
{
	CSP_LOG_FORMAT(LogLevel::Log, "Exiting Space %s", CurrentSpace.Name.c_str());

	csp::events::Event* ExitSpaceEvent = csp::events::EventSystem::Get().AllocateEvent(csp::events::SPACESYSTEM_EXIT_SPACE_EVENT_ID);
	ExitSpaceEvent->AddString("SpaceId", CurrentSpace.Id);

	csp::events::EventSystem::Get().EnqueueEvent(ExitSpaceEvent);
}

bool SpaceSystem::IsInSpace()
{
	return !CurrentSpace.Id.IsEmpty();
}

const Space& SpaceSystem::GetCurrentSpace() const
{
	return CurrentSpace;
}

void SpaceSystem::CreateSpace(const String& Name,
							  const String& Description,
							  SpaceAttributes Attributes,
							  const Optional<InviteUserRoleInfoCollection>& InviteUsers,
							  const Map<String, String>& Metadata,
							  const Optional<FileAssetDataSource>& Thumbnail,
							  SpaceResultCallback Callback)
{
	CSP_PROFILE_SCOPED();

	SpaceResultCallback CreateSpaceCallback = [Callback, InviteUsers, Thumbnail, Metadata, this](const SpaceResult& CreateSpaceResult)
	{
		if (CreateSpaceResult.GetResultCode() == EResultCode::InProgress)
		{
			return;
		}

		if (CreateSpaceResult.GetResultCode() == EResultCode::Failed)
		{
			INVOKE_IF_NOT_NULL(Callback, CreateSpaceResult);

			return;
		}

		const auto& Space	= CreateSpaceResult.GetSpace();
		const auto& SpaceId = Space.Id;

		NullResultCallback BulkInviteCallback = [Callback, SpaceId, CreateSpaceResult, this](const NullResult& _BulkInviteResult)
		{
			if (_BulkInviteResult.GetResultCode() == EResultCode::InProgress)
			{
				return;
			}

			SpaceResult InternalResult(_BulkInviteResult);

			if (_BulkInviteResult.GetResultCode() == EResultCode::Failed)
			{
				// Delete the space, its metadata and tuhmbnail as the space wasn't created how the user requested
				RemoveSpaceThumbnail(SpaceId, nullptr);
				RemoveMetadata(SpaceId, nullptr);
				DeleteSpace(SpaceId, nullptr);
			}

			if (_BulkInviteResult.GetResultCode() == EResultCode::Success)
			{
				InternalResult.SetSpace(CreateSpaceResult.GetSpace());
			}

			INVOKE_IF_NOT_NULL(Callback, InternalResult);
		};

		NullResultCallback UploadSpaceThumbnailCallback
			= [Callback, SpaceId, InviteUsers, CreateSpaceResult, BulkInviteCallback, this](const NullResult& _UploadThumbnailResult)
		{
			if (_UploadThumbnailResult.GetResultCode() == EResultCode::InProgress)
			{
				return;
			}

			SpaceResult InternalResult(_UploadThumbnailResult);

			if (_UploadThumbnailResult.GetResultCode() == EResultCode::Failed)
			{
				// Delete the space and its metadata as the space wasn't created how the user requested
				RemoveMetadata(SpaceId, nullptr);
				DeleteSpace(SpaceId, nullptr);

				INVOKE_IF_NOT_NULL(Callback, InternalResult);

				return;
			}

			if (InviteUsers.HasValue() && !InviteUsers->InviteUserRoleInfos.IsEmpty())
			{
				BulkInviteToSpace(SpaceId, *InviteUsers, BulkInviteCallback);
			}
			else
			{
				InternalResult.SetSpace(CreateSpaceResult.GetSpace());
				INVOKE_IF_NOT_NULL(Callback, InternalResult);
			}
		};

		NullResultCallback AddMetadataCallback
			= [Callback, SpaceId, Thumbnail, InviteUsers, CreateSpaceResult, UploadSpaceThumbnailCallback, BulkInviteCallback, this](
				  const NullResult& _AddMetadataResult)
		{
			if (_AddMetadataResult.GetResultCode() == EResultCode::InProgress)
			{
				return;
			}

			SpaceResult InternalResult(_AddMetadataResult);

			if (_AddMetadataResult.GetResultCode() == EResultCode::Failed)
			{
				// Delete the space as it can be considered broken without any space metadata
				DeleteSpace(SpaceId, nullptr);

				INVOKE_IF_NOT_NULL(Callback, InternalResult);

				return;
			}

			if (Thumbnail.HasValue())
			{
				AddSpaceThumbnail(SpaceId, *Thumbnail, UploadSpaceThumbnailCallback);
			}
			else if (InviteUsers.HasValue() && !InviteUsers->InviteUserRoleInfos.IsEmpty())
			{
				BulkInviteToSpace(SpaceId, *InviteUsers, BulkInviteCallback);
			}
			else
			{
				InternalResult.SetSpace(CreateSpaceResult.GetSpace());
				INVOKE_IF_NOT_NULL(Callback, InternalResult);
			}
		};

		AddMetadata(Space.Id, Metadata, AddMetadataCallback);
	};

	::CreateSpace(static_cast<chs::GroupApi*>(GroupAPI), Name, Description, Attributes, CreateSpaceCallback);
}

void SpaceSystem::CreateSpaceWithBuffer(const String& Name,
										const String& Description,
										SpaceAttributes Attributes,
										const Optional<InviteUserRoleInfoCollection>& InviteUsers,
										const Map<String, String>& Metadata,
										const BufferAssetDataSource& Thumbnail,
										SpaceResultCallback Callback)
{
	CSP_PROFILE_SCOPED();

	SpaceResultCallback CreateSpaceCallback = [Callback, InviteUsers, Thumbnail, Metadata, this](const SpaceResult& CreateSpaceResult)
	{
		if (CreateSpaceResult.GetResultCode() == EResultCode::InProgress)
		{
			return;
		}

		if (CreateSpaceResult.GetResultCode() == EResultCode::Failed)
		{
			INVOKE_IF_NOT_NULL(Callback, CreateSpaceResult);

			return;
		}

		const auto& Space	= CreateSpaceResult.GetSpace();
		const auto& SpaceId = Space.Id;

		NullResultCallback BulkInviteCallback = [Callback, SpaceId, CreateSpaceResult, this](const NullResult& _BulkInviteResult)
		{
			if (CreateSpaceResult.GetResultCode() == EResultCode::InProgress)
			{
				return;
			}

			SpaceResult InternalResult(_BulkInviteResult);

			if (_BulkInviteResult.GetResultCode() == EResultCode::Failed)
			{
				// Delete the space, its metadata and tuhmbnail as the space wasn't created how the user requested
				RemoveSpaceThumbnail(SpaceId, nullptr);
				RemoveMetadata(SpaceId, nullptr);
				DeleteSpace(SpaceId, nullptr);
			}
			else
			{
				InternalResult.SetSpace(CreateSpaceResult.GetSpace());
			}

			INVOKE_IF_NOT_NULL(Callback, InternalResult);
		};

		NullResultCallback UploadSpaceThumbnailCallback
			= [Callback, SpaceId, InviteUsers, CreateSpaceResult, BulkInviteCallback, this](const NullResult& _UploadThumbnailResult)
		{
			if (_UploadThumbnailResult.GetResultCode() == EResultCode::InProgress)
			{
				return;
			}

			SpaceResult InternalResult(_UploadThumbnailResult);

			if (_UploadThumbnailResult.GetResultCode() == EResultCode::Failed)
			{
				// Delete the space and its metadata and the space wasn't created how the user requested
				RemoveMetadata(SpaceId, nullptr);
				DeleteSpace(SpaceId, nullptr);

				INVOKE_IF_NOT_NULL(Callback, InternalResult);

				return;
			}

			if (InviteUsers.HasValue() && !InviteUsers->InviteUserRoleInfos.IsEmpty())
			{
				BulkInviteToSpace(SpaceId, *InviteUsers, BulkInviteCallback);
			}
			else
			{
				InternalResult.SetSpace(CreateSpaceResult.GetSpace());
				INVOKE_IF_NOT_NULL(Callback, InternalResult);
			}
		};

		NullResultCallback AddMetadataCallback
			= [Callback, SpaceId, Thumbnail, UploadSpaceThumbnailCallback, this](const NullResult& _AddMetadataResult)
		{
			if (_AddMetadataResult.GetResultCode() == EResultCode::InProgress)
			{
				return;
			}

			SpaceResult InternalResult(_AddMetadataResult);

			if (_AddMetadataResult.GetResultCode() == EResultCode::Failed)
			{
				// Delete the space as it can be considered broken without any space metadata
				DeleteSpace(SpaceId, nullptr);

				INVOKE_IF_NOT_NULL(Callback, InternalResult);

				return;
			}

			AddSpaceThumbnailWithBuffer(SpaceId, Thumbnail, UploadSpaceThumbnailCallback);
		};

		AddMetadata(Space.Id, Metadata, AddMetadataCallback);
	};

	::CreateSpace(static_cast<chs::GroupApi*>(GroupAPI), Name, Description, Attributes, CreateSpaceCallback);
}

void SpaceSystem::UpdateSpace(const String& SpaceId,
							  const Optional<String>& Name,
							  const Optional<String>& Description,
							  const Optional<SpaceAttributes>& Attributes,
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

	if (Attributes.HasValue())
	{
		LiteGroupInfo->SetDiscoverable(HasFlag(*Attributes, SpaceAttributes::IsDiscoverable));
		LiteGroupInfo->SetRequiresInvite(HasFlag(*Attributes, SpaceAttributes::RequiresInvite));

		LiteGroupInfo->SetAutoModerator(false);
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<BasicSpaceResultCallback, BasicSpaceResult, void, chs::GroupLiteDto>(Callback, nullptr);

	static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsGroupIdLitePut(SpaceId, LiteGroupInfo, ResponseHandler);
}

void SpaceSystem::DeleteSpace(const csp::common::String& SpaceId, NullResultCallback Callback)
{
	CSP_PROFILE_SCOPED();

	const String InGroupId = SpaceId;

	// Delete space metadata AssetCollection first, as users without super-user will not be able to do so after the space is deleted
	NullResultCallback RemoveMetadataCallback = [Callback, InGroupId, this](const NullResult& RemoveMetadataResult)
	{
		if (RemoveMetadataResult.GetResultCode() == EResultCode::InProgress)
		{
			return;
		}

		if (RemoveMetadataResult.GetResultCode() == EResultCode::Failed)
		{
			INVOKE_IF_NOT_NULL(Callback, RemoveMetadataResult);

			return;
		}

		NullResultCallback RemoveSpaceThumbnailCallback = [Callback, InGroupId, this](const NullResult& RemoveSpaceThumbnailResult)
		{
			if (RemoveSpaceThumbnailResult.GetResultCode() == EResultCode::InProgress)
			{
				return;
			}

			if (RemoveSpaceThumbnailResult.GetResultCode() == EResultCode::Failed)
			{
				INVOKE_IF_NOT_NULL(Callback, RemoveSpaceThumbnailResult);

				return;
			}

			csp::services::ResponseHandlerPtr ResponseHandler
				= GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback,
																										nullptr,
																										csp::web::EResponseCodes::ResponseNoContent);

			static_cast<chsaggregation::SpaceApi*>(SpaceAPI)->apiV1SpacesSpaceIdDelete(InGroupId, ResponseHandler);
		};

		RemoveSpaceThumbnail(InGroupId, RemoveSpaceThumbnailCallback);
	};

	RemoveMetadata(InGroupId, RemoveMetadataCallback);
}

void SpaceSystem::GetSpaces(SpacesResultCallback Callback)
{
	const auto* UserSystem = SystemsManager::Get().GetUserSystem();
	const String InUserId  = UserSystem->GetLoginState().UserId;

	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<SpacesResultCallback, SpacesResult, void, csp::services::DtoArray<chs::GroupDto>>(Callback, nullptr);

	static_cast<chs::GroupApi*>(GroupAPI)->apiV1UsersUserIdGroupsGet(InUserId, ResponseHandler);
}

void SpaceSystem::GetSpacesByAttributes(const Optional<bool>& InIsDiscoverable,
										const Optional<bool>& InRequiresInvite,
										const Optional<int>& InResultsSkip,
										const Optional<int>& InResultsMax,
										BasicSpacesResultCallback Callback)
{
	auto IsDiscoverable				  = InIsDiscoverable.HasValue() ? *InIsDiscoverable : std::optional<bool>(std::nullopt);
	auto RequiresInvite				  = InRequiresInvite.HasValue() ? *InRequiresInvite : std::optional<bool>(std::nullopt);
	auto ResultsSkip				  = InResultsSkip.HasValue() ? *InResultsSkip : std::optional<int32_t>(std::nullopt);
	std::optional<int32_t> ResultsMax = InResultsMax.HasValue() ? std::min(MAX_SPACES_RESULTS, *InResultsMax) : MAX_SPACES_RESULTS;

	if (InResultsMax.HasValue() && *InResultsMax > MAX_SPACES_RESULTS)
	{
		CSP_LOG_WARN_FORMAT("Provided value `%i` for ResultsMax exceeded max value and was reduced to `%i`.", *InResultsMax, MAX_SPACES_RESULTS);
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<BasicSpacesResultCallback, BasicSpacesResult, void, csp::services::DtoArray<chs::GroupLiteDto>>(Callback, nullptr);

	static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsLiteGet(std::nullopt, // Ids
															  std::nullopt, // GroupTypes
															  std::nullopt, // Names
															  std::nullopt, // PartialName
															  std::nullopt, // GroupOwnerIds
															  std::nullopt, // ExcludeGroupOwnerIds
															  std::nullopt, // Users
															  IsDiscoverable,
															  std::nullopt, // AutoModerator
															  RequiresInvite,
															  ResultsSkip,
															  ResultsMax,
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

	for (auto idx = 0; idx < RequestedSpaceIDs.Size(); ++idx)
	{
		SpaceIds.push_back(RequestedSpaceIDs[idx]);
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<SpacesResultCallback, SpacesResult, void, csp::services::DtoArray<chs::GroupDto>>(Callback, nullptr);

	static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsGet(SpaceIds, ResponseHandler);
}

void SpaceSystem::GetSpacesForUserId(const String& UserId, SpacesResultCallback Callback)
{
	const String InUserId = UserId;

	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<SpacesResultCallback, SpacesResult, void, csp::services::DtoArray<chs::GroupDto>>(Callback, nullptr);

	static_cast<chs::GroupApi*>(GroupAPI)->apiV1UsersUserIdGroupsGet(UserId, ResponseHandler);
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
		= GroupAPI->CreateHandler<SpaceResultCallback, SpaceResult, void, chs::GroupDto>(Callback, nullptr);

	static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsGroupIdGet(SpaceId, ResponseHandler);
}

void SpaceSystem::InviteToSpace(const csp::common::String& SpaceId,
								const String& Email,
								const Optional<bool>& IsModeratorRole,
								const Optional<String>& EmailLinkUrl,
								const Optional<String>& SignupUrl,
								NullResultCallback Callback)
{
	auto GroupInviteInfo = std::make_shared<chs::GroupInviteDto>();
	GroupInviteInfo->SetEmail(Email);

	if (IsModeratorRole.HasValue())
	{
		GroupInviteInfo->SetAsModerator(*IsModeratorRole);
	}

	auto EmailLinkUrlParam = EmailLinkUrl.HasValue() && !EmailLinkUrl->IsEmpty() ? (*EmailLinkUrl) : std::optional<String>(std::nullopt);
	auto SignupUrlParam	   = SignupUrl.HasValue() && !SignupUrl->IsEmpty() ? (*SignupUrl) : std::optional<String>(std::nullopt);

	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback,
																								nullptr,
																								csp::web::EResponseCodes::ResponseNoContent);

	static_cast<chs::GroupApi*>(GroupAPI)
		->apiV1GroupsGroupIdEmailInvitesPost(SpaceId, std::nullopt, EmailLinkUrlParam, SignupUrlParam, GroupInviteInfo, ResponseHandler);
}

void SpaceSystem::BulkInviteToSpace(const String& SpaceId, const InviteUserRoleInfoCollection& InviteUsers, NullResultCallback Callback)
{
	std::vector<std::shared_ptr<chs::GroupInviteDto>> GroupInvites
		= systems::SpaceSystemHelpers::GenerateGroupInvites(InviteUsers.InviteUserRoleInfos);

	auto EmailLinkUrlParam = !InviteUsers.EmailLinkUrl.IsEmpty() ? (InviteUsers.EmailLinkUrl) : std::optional<String>(std::nullopt);
	auto SignupUrlParam	   = !InviteUsers.SignupUrl.IsEmpty() ? (InviteUsers.SignupUrl) : std::optional<String>(std::nullopt);

	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback,
																								nullptr,
																								csp::web::EResponseCodes::ResponseNoContent);

	static_cast<chs::GroupApi*>(GroupAPI)
		->apiV1GroupsGroupIdEmailInvitesBulkPost(SpaceId, std::nullopt, EmailLinkUrlParam, SignupUrlParam, GroupInvites, ResponseHandler);
}

void SpaceSystem::GetPendingUserInvites(const String& SpaceId, PendingInvitesResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<PendingInvitesResultCallback, PendingInvitesResult, void, csp::services::DtoArray<chs::GroupInviteDto>>(Callback,
																																		  nullptr);

	static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsGroupIdEmailInvitesGet(SpaceId, ResponseHandler);
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

				 static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupCodesGroupCodeUsersUserIdPut(SpaceCode, UserId, ResponseHandler);
			 });
}

void SpaceSystem::RemoveUserFromSpace(const String& SpaceId, const String& UserId, NullResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback, nullptr);

	static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsGroupIdUsersUserIdDelete(SpaceId, UserId, ResponseHandler);
}

void SpaceSystem::AddSiteInfo(const String& SpaceId, Site& SiteInfo, SiteResultCallback Callback)
{
	auto& SystemsManager	= SystemsManager::Get();
	auto* POIInternalSystem = static_cast<PointOfInterestInternalSystem*>(SystemsManager.GetPointOfInterestSystem());

	SiteInfo.SpaceId = SpaceId;
	POIInternalSystem->CreateSite(SiteInfo, Callback);
}

void SpaceSystem::RemoveSiteInfo(const String& SpaceId, Site& SiteInfo, NullResultCallback Callback)
{
	auto& SystemsManager	= SystemsManager::Get();
	auto* POIInternalSystem = static_cast<PointOfInterestInternalSystem*>(SystemsManager.GetPointOfInterestSystem());

	SiteInfo.SpaceId = SpaceId;
	POIInternalSystem->DeleteSite(SiteInfo, Callback);
}

void SpaceSystem::GetSitesInfo(const String& SpaceId, SitesCollectionResultCallback Callback)
{
	auto& SystemsManager	= SystemsManager::Get();
	auto* POIInternalSystem = static_cast<PointOfInterestInternalSystem*>(SystemsManager.GetPointOfInterestSystem());

	POIInternalSystem->GetSites(SpaceId, Callback);
}

void SpaceSystem::UpdateUserRole(const String& SpaceId, const UserRoleInfo& NewUserRoleInfo, NullResultCallback Callback)
{
	const auto NewUserRole = NewUserRoleInfo.UserRole;
	const auto& UserId	   = NewUserRoleInfo.UserId;

	if (NewUserRole == SpaceUserRole::Owner)
	{
		csp::services::ResponseHandlerPtr ResponseHandler
			= GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback, nullptr);

		static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsGroupIdOwnerNewGroupOwnerIdPut(SpaceId, UserId, ResponseHandler);
	}
	else if (NewUserRole == SpaceUserRole::Moderator)
	{
		csp::services::ResponseHandlerPtr ResponseHandler
			= GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback,
																									nullptr,
																									csp::web::EResponseCodes::ResponseNoContent);

		static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsGroupIdModeratorsUserIdPut(SpaceId, UserId, ResponseHandler);
	}
	else if (NewUserRole == SpaceUserRole::User)
	{
		// TODO: When the Client will be able to change the space owner role get a fresh Space object to see if the NewUserRoleInfo.UserId is still a
		// space owner
		if (SpaceId == NewUserRoleInfo.UserId)
		{
			// An owner must firstly pass the space ownership to someone else before it can become a user
			INVOKE_IF_NOT_NULL(Callback, MakeInvalid<NullResult>());

			return;
		}

		csp::services::ResponseHandlerPtr ResponseHandler
			= GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback,
																									nullptr,
																									csp::web::EResponseCodes::ResponseNoContent);

		static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsGroupIdModeratorsUserIdDelete(SpaceId, UserId, ResponseHandler);
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
		AssetSystem->UpdateAssetCollectionMetadata(AssetCollection, NewMetadata, UpdateAssetCollCallback);
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
			const auto& AssetCollections = Result.GetAssetCollections();

			for (int i = 0; i < AssetCollections.Size(); ++i)
			{
				const auto& AssetCollection = AssetCollections[i];

				auto SpaceId = SpaceSystemHelpers::GetSpaceIdFromMetadataAssetCollectionName(AssetCollection.Name);

				SpacesMetadata[SpaceId] = systems::SpaceSystemHelpers::LegacyAssetConversion(AssetCollection);
			}

			InternalResult.SetMetadata(SpacesMetadata);
		}

		INVOKE_IF_NOT_NULL(Callback, InternalResult);
	};

	GetMetadataAssetCollections(SpaceIds, MetadataAssetCollCallback);
}

void SpaceSystem::GetSpaceMetadata(const String& SpaceId, SpaceMetadataResultCallback Callback)
{
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
					CSP_LOG_FORMAT(LogLevel::Log,
								   "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
								   (int) UploadResult.GetResultCode(),
								   UploadResult.GetHttpResultCode());
				}

				NullResult InternalResult(UploadResult);
				INVOKE_IF_NOT_NULL(Callback, InternalResult);
			};

			auto& ThumbnailAsset	= ((csp::systems::AssetsResult&) AssetsResult).GetAssets()[0];
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
					CSP_LOG_FORMAT(LogLevel::Log,
								   "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
								   (int) UploadResult.GetResultCode(),
								   UploadResult.GetHttpResultCode());
				}

				NullResult InternalResult(UploadResult);
				INVOKE_IF_NOT_NULL(Callback, InternalResult);
			};

			auto& ThumbnailAsset = ((csp::systems::AssetsResult&) AssetsResult).GetAssets()[0];
			ThumbnailAsset.FileName
				= SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(SpaceSystemHelpers::GetAssetFileExtension(NewThumbnail.GetMimeType()));
			ThumbnailAsset.MimeType = NewThumbnail.GetMimeType();
			const auto AssetSystem	= SystemsManager::Get().GetAssetSystem();
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
	static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsGroupIdBannedUsersUserIdPut(SpaceId, RequestedUserId, ResponseHandler);
}

void SpaceSystem::DeleteUserFromSpaceBanList(const String& SpaceId, const String& RequestedUserId, NullResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback, nullptr);
	static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsGroupIdBannedUsersUserIdDelete(SpaceId, RequestedUserId, ResponseHandler);
}

void SpaceSystem::GetMetadataAssetCollection(const String& SpaceId, AssetCollectionResultCallback Callback)
{
	auto* AssetSystem				 = SystemsManager::Get().GetAssetSystem();
	auto MetadataAssetCollectionName = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(SpaceId);

	AssetSystem->GetAssetCollectionByName(MetadataAssetCollectionName, Callback);
}

void SpaceSystem::GetMetadataAssetCollections(const Array<csp::common::String>& SpaceIds, AssetCollectionsResultCallback Callback)
{
	auto* AssetSystem = SystemsManager::Get().GetAssetSystem();
	Array<String> PrototypeNames(SpaceIds.Size());

	for (auto item = 0; item < SpaceIds.Size(); ++item)
	{
		PrototypeNames[item] = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(SpaceIds[item]);
	}

	AssetSystem->FindAssetCollections(nullptr, nullptr, PrototypeNames, nullptr, nullptr, nullptr, nullptr, nullptr, Callback);
}

void SpaceSystem::AddMetadata(const csp::common::String& SpaceId, const Map<String, String>& Metadata, NullResultCallback Callback)
{
	AssetCollectionResultCallback CreateAssetCollCallback = [Callback](const AssetCollectionResult& Result)
	{
		NullResult InternalResult(Result);
		INVOKE_IF_NOT_NULL(Callback, InternalResult);
	};

	auto MetadataAssetCollectionName = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(SpaceId);
	auto* AssetSystem				 = SystemsManager::Get().GetAssetSystem();

	// Don't assign this AssetCollection to a space so any user can retrieve the metadata without joining the space
	AssetSystem->CreateAssetCollection(SpaceId,
									   nullptr,
									   MetadataAssetCollectionName,
									   Metadata,
									   EAssetCollectionType::FOUNDATION_INTERNAL,
									   nullptr,
									   CreateAssetCollCallback);
}

void SpaceSystem::RemoveMetadata(const String& SpaceId, NullResultCallback Callback)
{
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
			CSP_LOG_FORMAT(LogLevel::Log,
						   "The Space thumbnail asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
						   (int) AssetCollResult.GetResultCode(),
						   AssetCollResult.GetHttpResultCode());

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
				CSP_LOG_FORMAT(LogLevel::Log,
							   "The Space thumbnail asset creation was not successful. ResCode: %d, HttpResCode: %d",
							   (int) CreateAssetResult.GetResultCode(),
							   CreateAssetResult.GetHttpResultCode());

				NullResult InternalResult(CreateAssetResult);
				INVOKE_IF_NOT_NULL(Callback, InternalResult);

				return;
			}

			UriResultCallback UploadCallback = [Callback](const UriResult& UploadResult)
			{
				if (UploadResult.GetResultCode() == EResultCode::Failed)
				{
					CSP_LOG_FORMAT(LogLevel::Log,
								   "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
								   (int) UploadResult.GetResultCode(),
								   UploadResult.GetHttpResultCode());
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
	const auto Tag								   = Array<String>({SpaceId});

	// don't associate this asset collection with a particular space so that it can be retrieved by guest users that have not joined this space
	AssetSystem->CreateAssetCollection(SpaceId,
									   nullptr,
									   SpaceThumbnailAssetCollectionName,
									   nullptr,
									   EAssetCollectionType::SPACE_THUMBNAIL,
									   Tag,
									   CreateAssetCollCallback);
}

void SpaceSystem::AddSpaceThumbnailWithBuffer(const csp::common::String& SpaceId,
											  const BufferAssetDataSource& ImageDataSource,
											  NullResultCallback Callback)
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
			CSP_LOG_FORMAT(LogLevel::Log,
						   "The Space thumbnail asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
						   (int) AssetCollResult.GetResultCode(),
						   AssetCollResult.GetHttpResultCode());

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
				CSP_LOG_FORMAT(LogLevel::Log,
							   "The Space thumbnail asset creation was not successful. ResCode: %d, HttpResCode: %d",
							   (int) CreateAssetResult.GetResultCode(),
							   CreateAssetResult.GetHttpResultCode());

				NullResult InternalResult(CreateAssetResult);
				INVOKE_IF_NOT_NULL(Callback, InternalResult);

				return;
			}

			UriResultCallback UploadCallback = [Callback](const UriResult& UploadResult)
			{
				if (UploadResult.GetResultCode() == EResultCode::Failed)
				{
					CSP_LOG_FORMAT(LogLevel::Log,
								   "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
								   (int) UploadResult.GetResultCode(),
								   UploadResult.GetHttpResultCode());
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
	const Array<String> Tag({SpaceId});

	AssetSystem->CreateAssetCollection(SpaceId,
									   nullptr,
									   SpaceThumbnailAssetCollectionName,
									   nullptr,
									   EAssetCollectionType::SPACE_THUMBNAIL,
									   Tag,
									   CreateAssetCollCallback);
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
			CSP_LOG_FORMAT(LogLevel::Log,
						   "The Space thumbnail asset collection retrieval has failed. ResCode: %d, HttpResCode: %d",
						   (int) AssetCollResult.GetResultCode(),
						   AssetCollResult.GetHttpResultCode());
		}

		INVOKE_IF_NOT_NULL(Callback, AssetCollResult);
	};

	auto* AssetSystem				 = SystemsManager::Get().GetAssetSystem();
	auto MetadataAssetCollectionName = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(SpaceId);

	Array<csp::systems::EAssetCollectionType> PrototypeTypes = {EAssetCollectionType::SPACE_THUMBNAIL};
	Array<String> PrototypeTags								 = {SpaceId};
	Array<String> GroupIds									 = {SpaceId};

	AssetSystem->FindAssetCollections(nullptr, nullptr, nullptr, PrototypeTypes, PrototypeTags, GroupIds, nullptr, nullptr, GetAssetCollCallback);
}

void SpaceSystem::GetSpaceThumbnailAsset(const AssetCollection& ThumbnailAssetCollection, AssetsResultCallback Callback)
{
	AssetsResultCallback ThumbnailAssetCallback = [Callback](const AssetsResult& AssetsResult)
	{
		if (AssetsResult.GetResultCode() == EResultCode::Failed)
		{
			CSP_LOG_FORMAT(LogLevel::Log,
						   "The Space thumbnail asset retrieval has failed. ResCode: %d, HttpResCode: %d",
						   (int) AssetsResult.GetResultCode(),
						   AssetsResult.GetHttpResultCode());
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
					CSP_LOG_FORMAT(LogLevel::Log,
								   "The Space thumbnail asset deletion was not successful. ResCode: %d, HttpResCode: %d",
								   (int) DeleteAssetResult.GetResultCode(),
								   DeleteAssetResult.GetHttpResultCode());

					NullResult InternalResult(DeleteAssetResult);
					INVOKE_IF_NOT_NULL(Callback, DeleteAssetResult);

					return;
				}

				NullResultCallback DeleteAssetCollCallback = [Callback, DeleteAssetResult, AssetSystem](const NullResult& DeleteAssetCollResult)
				{
					if (DeleteAssetCollResult.GetResultCode() == EResultCode::InProgress)
					{
						return;
					}

					if (DeleteAssetCollResult.GetResultCode() == EResultCode::Failed)
					{
						CSP_LOG_FORMAT(LogLevel::Log,
									   "The Space thumbnail asset collection deletion has failed. ResCode: %d, HttpResCode: %d",
									   (int) DeleteAssetResult.GetResultCode(),
									   DeleteAssetResult.GetHttpResultCode());
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
	auto& SystemsManager	= SystemsManager::Get();
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
		const auto& UserId		   = SystemsManager::Get().GetUserSystem()->GetLoginState().UserId;

		// First check if the user is the owner
		bool UserCanAccessSpaceDetails = !(bool) (RefreshedSpace.Attributes & SpaceAttributes::RequiresInvite) || RefreshedSpace.OwnerId == UserId;

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

void SpaceSystem::UpdateSpaceGeoLocation(const csp::common::String& SpaceId,
										 const csp::common::Optional<GeoLocation>& Location,
										 const csp::common::Optional<float>& Orientation,
										 const csp::common::Optional<csp::common::Array<GeoLocation>>& GeoFence,
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

		auto& SystemsManager	= SystemsManager::Get();
		auto* POIInternalSystem = static_cast<PointOfInterestInternalSystem*>(SystemsManager.GetPointOfInterestSystem());

		if (GetGeoLocationResult.HasGeoLocation)
		{
			POIInternalSystem
				->UpdateSpaceGeoLocation(SpaceId, GetGeoLocationResult.GetSpaceGeoLocation().Id, Location, Orientation, GeoFence, Callback);
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
		const auto& UserId		   = SystemsManager::Get().GetUserSystem()->GetLoginState().UserId;

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

		auto& SystemsManager	= SystemsManager::Get();
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
		const auto& UserId		   = SystemsManager::Get().GetUserSystem()->GetLoginState().UserId;

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

} // namespace csp::systems
