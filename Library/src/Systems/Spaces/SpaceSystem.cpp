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
#include "Debug/Logging.h"
#include "Events/EventSystem.h"
#include "Services/UserService/Api.h"
#include "Services/UserService/Dto.h"
#include "Systems/Spaces/SpaceSystemHelpers.h"
#include "Systems/Spatial/PointOfInterestInternalSystem.h"

#include <iostream>
#include <rapidjson/rapidjson.h>


using namespace csp;

namespace chs = csp::services::generated::userservice;


namespace
{

constexpr const int MAX_SPACES_RESULTS = 100;

void ConvertJsonMetadataToMapMetadata(const String& JsonMetadata, Map<String, String>& OutMapMetadata)
{
	rapidjson::Document Json;
	Json.Parse(JsonMetadata.c_str());

	if (!Json.IsObject())
	{
		FOUNDATION_LOG_MSG(csp::systems::LogLevel::Verbose, "Space JSON metadata is not an object! Returning default metadata values...");

		OutMapMetadata["site"]				 = "Void";
		OutMapMetadata["multiplayerVersion"] = "3"; // 2 represents double-msg-packed serialiser spaces, 3 represents the change to dictionary packing

		return;
	}

	for (const auto& Member : Json.GetObject())
	{
		if (Member.value.IsString())
		{
			OutMapMetadata[Member.name.GetString()] = Member.value.GetString();
		}
		else if (Member.value.IsInt())
		{
			OutMapMetadata[Member.name.GetString()] = std::to_string(Member.value.GetInt()).c_str();
		}
		else if (Member.value.IsNull())
		{
			OutMapMetadata[Member.name.GetString()] = "";
		}
		else
		{
			FOUNDATION_LOG_FORMAT(csp::systems::LogLevel::Error,
								  "Unsupported JSON type in space metadata! (Key = %s, Value Type = %d)",
								  Member.name.GetString(),
								  Member.value.GetType());
		}
	}
}

void CreateSpace(chs::GroupApi* GroupAPI,
				 const String& Name,
				 const String& Description,
				 csp::systems::SpaceAttributes Attributes,
				 csp::systems::SpaceResultCallback Callback)
{
	auto GroupInfo = std::make_shared<chs::GroupDto>();
	GroupInfo->SetName(Name);
	GroupInfo->SetDescription(Description);
	GroupInfo->SetGroupType("Space");

	GroupInfo->SetDiscoverable(HasFlag(Attributes, csp::systems::SpaceAttributes::IsDiscoverable));
	GroupInfo->SetRequiresInvite(HasFlag(Attributes, csp::systems::SpaceAttributes::RequiresInvite));

	GroupInfo->SetAutoModerator(false);

	const auto* UserSystem = csp::systems::SystemsManager::Get().GetUserSystem();
	GroupInfo->SetGroupOwnerId(UserSystem->GetLoginState().UserId);

	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<csp::systems::SpaceResultCallback, csp::systems::SpaceResult, void, chs::GroupDto>(Callback, nullptr);

	GroupAPI->apiV1GroupsPost(GroupInfo, ResponseHandler);
}

} // namespace


namespace csp::systems
{

SpaceSystem::SpaceSystem() : SystemBase(), GroupAPI(nullptr)
{
}

SpaceSystem::SpaceSystem(csp::web::WebClient* InWebClient) : SystemBase(InWebClient), CurrentSpace()
{
	GroupAPI = CSP_NEW chs::GroupApi(InWebClient);
}

SpaceSystem::~SpaceSystem()
{
	CSP_DELETE(GroupAPI);
}

void SpaceSystem::EnterSpace(const String& SpaceId, bool AutoConnect, EnterSpaceResultCallback Callback)
{
	SpaceResultCallback GetSpaceCallback = [=](const SpaceResult& GetSpaceResult)
	{
		if (GetSpaceResult.GetResultCode() == csp::services::EResultCode::Failed)
		{
			EnterSpaceResult InternalResult(GetSpaceResult.GetResultCode(), GetSpaceResult.GetHttpResultCode());
			Callback(InternalResult);
		}
		else if (GetSpaceResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			auto RefreshedSpace = GetSpaceResult.GetSpace();

			FOUNDATION_LOG_FORMAT(LogLevel::Log, "Entering Space %s", RefreshedSpace.Name.c_str());

			const String UserID = SystemsManager::Get().GetUserSystem()->GetLoginState().UserId;

			if (!HasFlag(RefreshedSpace.Attributes, SpaceAttributes::RequiresInvite))
			{
				AddUserToSpace(SpaceId,
							   UserID,
							   [=](const SpaceResult& Result)
							   {
								   if (Result.GetResultCode() == csp::services::EResultCode::Success)
								   {
									   CurrentSpace = RefreshedSpace;

									   csp::events::Event* EnterSpaceEvent
										   = csp::events::EventSystem::Get().AllocateEvent(csp::events::SPACESYSTEM_ENTER_SPACE_EVENT_ID);
									   EnterSpaceEvent->AddString("SpaceId", SpaceId);
									   csp::events::EventSystem::Get().EnqueueEvent(EnterSpaceEvent);
									   EnterSpaceResult InternalResult(GetSpaceResult.GetResultCode(), GetSpaceResult.GetHttpResultCode());

									   if (AutoConnect)
									   {
										   auto* Connection = new csp::multiplayer::MultiplayerConnection(SpaceId);

										   SetConnectionCallbacks(Connection);

										   Connection->Connect(
											   [=](bool Ok)
											   {
												   if (Ok)
												   {
													   Connection->InitialiseConnection(
														   [=](bool Ok)
														   {
															   if (Ok)
															   {
																   EnterSpaceResult InternalResult(GetSpaceResult.GetResultCode(),
																								   GetSpaceResult.GetHttpResultCode());
																   InternalResult.SetConnection(Connection);
																   Callback(InternalResult);
															   }
															   else
															   {
																   FOUNDATION_LOG_ERROR_MSG("Failed to Connect to SignalR Server");
																   Callback(EnterSpaceResult(GetSpaceResult.GetResultCode(),
																							 GetSpaceResult.GetHttpResultCode()));
															   }
														   });
												   }
												   else
												   {
													   FOUNDATION_LOG_ERROR_MSG("Failed to Connect to SignalR Server")
													   Callback(EnterSpaceResult(GetSpaceResult.GetResultCode(), GetSpaceResult.GetHttpResultCode()));
												   }
											   });
									   }
									   else
									   {
										   Callback(InternalResult);
									   }
								   }
							   });
			}
			else
			{
				// First check if the user is the owner
				bool EnterSuccess = RefreshedSpace.OwnerId == UserID;

				// If the user is not the owner check are they a moderator
				if (!EnterSuccess)
				{
					for (int i = 0; i < RefreshedSpace.ModeratorIds.Size(); ++i)
					{
						if (RefreshedSpace.ModeratorIds[i] == UserID)
						{
							EnterSuccess = true;
							break;
						}
					}
				}

				// Finally check all users in the group
				if (!EnterSuccess)
				{
					for (int i = 0; i < RefreshedSpace.UserIds.Size(); ++i)
					{
						if (RefreshedSpace.UserIds[i] == UserID)
						{
							EnterSuccess = true;
						}
					}
				}
				if (EnterSuccess)
				{
					CurrentSpace = GetSpaceResult.GetSpace();
					csp::events::Event* EnterSpaceEvent
						= csp::events::EventSystem::Get().AllocateEvent(csp::events::SPACESYSTEM_ENTER_SPACE_EVENT_ID);
					EnterSpaceEvent->AddString("SpaceId", SpaceId);
					csp::events::EventSystem::Get().EnqueueEvent(EnterSpaceEvent);

					if (AutoConnect)
					{
						auto Connection = new csp::multiplayer::MultiplayerConnection(SpaceId);

						SetConnectionCallbacks(Connection);

						Connection->Connect(
							[=](bool Ok)
							{
								if (Ok)
								{
									Connection->InitialiseConnection(
										[=](bool Ok)
										{
											if (Ok)
											{
												EnterSpaceResult InternalResult(GetSpaceResult.GetResultCode(), GetSpaceResult.GetHttpResultCode());
												InternalResult.SetConnection(Connection);
												Callback(InternalResult);
											}
											else
											{
												FOUNDATION_LOG_ERROR_MSG("Failed to Connect to SignalR Server");
												Callback(EnterSpaceResult(GetSpaceResult.GetResultCode(), GetSpaceResult.GetHttpResultCode()));
											}
										});
								}
								else
								{
									FOUNDATION_LOG_ERROR_MSG("Failed to Connect to SignalR Server")
									Callback(EnterSpaceResult(GetSpaceResult.GetResultCode(), GetSpaceResult.GetHttpResultCode()));
								}
							});
					}
					else
					{
						Callback(EnterSpaceResult(GetSpaceResult.GetResultCode(), GetSpaceResult.GetHttpResultCode()));
					}
				}
			}
		}
	};

	GetSpace(SpaceId, GetSpaceCallback);
}

void SpaceSystem::ExitSpaceAndDisconnect(csp::multiplayer::MultiplayerConnection* Connection, BoolCallback Callback)
{
	FOUNDATION_LOG_FORMAT(LogLevel::Log, "Exiting Space %s", CurrentSpace.Name.c_str());

	csp::events::Event* ExitSpaceEvent = csp::events::EventSystem::Get().AllocateEvent(csp::events::SPACESYSTEM_EXIT_SPACE_EVENT_ID);
	ExitSpaceEvent->AddString("SpaceId", CurrentSpace.Id);
	csp::events::EventSystem::Get().EnqueueEvent(ExitSpaceEvent);

	if (Connection->GetConnectionState() != csp::multiplayer::ConnectionState::Disconnected
		&& Connection->GetConnectionState() != csp::multiplayer::ConnectionState::Disconnecting)
	{
		Connection->Disconnect(
			[=](bool Ok)
			{
				if (Ok)
				{
					CurrentSpace = Space();
					Callback(true);
				}
				else
				{
					FOUNDATION_LOG_ERROR_MSG("Failed to Exit Space: Disconnect Failed");

					Callback(false);
				}
			});
	}
}

void SpaceSystem::ExitSpace()
{
	FOUNDATION_LOG_FORMAT(LogLevel::Log, "Exiting Space %s", CurrentSpace.Name.c_str());

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

CSP_EVENT void SpaceSystem::SetEntityCreatedCallback(csp::multiplayer::SpaceEntitySystem::EntityCreatedCallback Callback)
{
	EntityCreatedCallback = Callback;
}

CSP_EVENT void SpaceSystem::SetInitialEntitiesRetrievedCallback(csp::multiplayer::SpaceEntitySystem::CallbackHandler Callback)
{
	InitialEntitiesRetrievedCallback = Callback;
}

CSP_EVENT void SpaceSystem::SetScriptSystemReadyCallback(csp::multiplayer::SpaceEntitySystem::CallbackHandler Callback)
{
	ScriptSystemReadyCallback = Callback;
}

void SpaceSystem::CreateSpace(const String& Name,
							  const String& Description,
							  SpaceAttributes Attributes,
							  const csp::common::Map<csp::common::String, csp::common::String>& Metadata,
							  SpaceResultCallback Callback)
{
	FOUNDATION_PROFILE_SCOPED();

	SpaceResultCallback CreateSpaceCallback = [=](const SpaceResult& CreateSpaceResult)
	{
		if (CreateSpaceResult.GetResultCode() != csp::services::EResultCode::Success)
		{
			Callback(CreateSpaceResult);

			return;
		}

		const auto& Space = CreateSpaceResult.GetSpace();

		NullResultCallback AddMetadataCallback = [=](const NullResult& _AddMetadataResult)
		{
			SpaceResult InternalResult(_AddMetadataResult);

			if (_AddMetadataResult.GetResultCode() == csp::services::EResultCode::Failed)
			{
				// Delete the space as it can be considered broken without any space metadata
				DeleteSpace(Space.Id, nullptr);
			}

			if (_AddMetadataResult.GetResultCode() != csp::services::EResultCode::Success)
			{
				Callback(InternalResult);

				return;
			}
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
	FOUNDATION_PROFILE_SCOPED();

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
		LiteGroupInfo->SetDiscoverable(HasFlag(*Attributes, csp::systems::SpaceAttributes::IsDiscoverable));
		LiteGroupInfo->SetRequiresInvite(HasFlag(*Attributes, csp::systems::SpaceAttributes::RequiresInvite));

		LiteGroupInfo->SetAutoModerator(false);
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<BasicSpaceResultCallback, BasicSpaceResult, void, chs::GroupLiteDto>(Callback, nullptr);

	static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsGroupIdLitePut(SpaceId, LiteGroupInfo, ResponseHandler);
}

void SpaceSystem::DeleteSpace(const csp::common::String& SpaceId, NullResultCallback Callback)
{
	FOUNDATION_PROFILE_SCOPED();

	const String InGroupId = SpaceId;

	// Delete space metadata AssetCollection first, as users without super-user will not be able to do so after the space is deleted
	NullResultCallback RemoveMetadataCallback = [=](const NullResult& RemoveMetadataResult)
	{
		if (RemoveMetadataResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			NullResultCallback RemoveSpaceThumbnailCallback = [=](const NullResult& RemoveSpaceThumbnailResult)
			{
				if (RemoveSpaceThumbnailResult.GetResultCode() == csp::services::EResultCode::Success)
				{
					csp::services::ResponseHandlerPtr ResponseHandler
						= GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(
							Callback,
							nullptr,
							csp::web::EResponseCodes::ResponseNoContent);

					static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsGroupIdDelete(InGroupId, ResponseHandler);
				}
				else
				{
					Callback(RemoveSpaceThumbnailResult);
				}
			};

			RemoveSpaceThumbnail(InGroupId, RemoveSpaceThumbnailCallback);
		}
		else
		{
			Callback(RemoveMetadataResult);
		}
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
		FOUNDATION_LOG_WARN_FORMAT("Provided value `%i` for ResultsMax exceeded max value and was reduced to `%i`.",
								   *InResultsMax,
								   MAX_SPACES_RESULTS);
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
		FOUNDATION_LOG_ERROR_MSG("No space ids given");
		Callback(SpacesResult::Invalid());

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
		FOUNDATION_LOG_ERROR_MSG("No space id given");
		Callback(SpaceResult(csp::services::EResultCode::Failed, 400));

		return;
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<SpaceResultCallback, SpaceResult, void, chs::GroupDto>(Callback, nullptr);

	static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsGroupIdGet(SpaceId, ResponseHandler);
}

void SpaceSystem::InviteToSpace(const csp::common::String& SpaceId,
								const String& Email,
								const Optional<bool>& IsModeratorRole,
								NullResultCallback Callback)
{
	auto GroupInviteInfo = std::make_shared<chs::GroupInviteDto>();
	GroupInviteInfo->SetEmail(Email);

	if (IsModeratorRole.HasValue())
	{
		GroupInviteInfo->SetAsModerator(*IsModeratorRole);
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback,
																								nullptr,
																								csp::web::EResponseCodes::ResponseNoContent);

	static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsGroupIdEmailInvitePost(SpaceId, std::nullopt, GroupInviteInfo, ResponseHandler);
}

void SpaceSystem::BulkInviteToSpace(const String& SpaceId, const Array<InviteUserRoleInfo>& InviteUsers, NullResultCallback Callback)
{
	std::vector<std::shared_ptr<chs::GroupInviteDto>> GroupInvites;
	GroupInvites.reserve(InviteUsers.Size());

	for (auto i = 0; i < InviteUsers.Size(); ++i)
	{
		auto InviteUser = InviteUsers[i];

		auto GroupInvite = std::make_shared<chs::GroupInviteDto>();
		GroupInvite->SetEmail(InviteUser.UserEmail);
		GroupInvite->SetAsModerator(InviteUser.UserRole == SpaceUserRole::Moderator);

		GroupInvites.push_back(GroupInvite);
	}

	csp::services::ResponseHandlerPtr ResponseHandler
		= GroupAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback,
																								nullptr,
																								csp::web::EResponseCodes::ResponseNoContent);

	static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupsGroupIdEmailInvitesBulkPost(SpaceId, std::nullopt, GroupInvites, ResponseHandler);
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
	// This function right here is the only place in the whole of fdn that needs to use group code.
	// So, rather than bloat our `Space` class with the property, and give clients something that they have zero use for,
	// we prefer to pay the cost of an additional call to the cloud in the one place we need it, in order to retrieve it.
	// This function is not expected to be on any hot code path, so the perf cost is expected to be low. It's worth it for the api quality.

	GetSpace(SpaceId,
			 [UserId, Callback, this](const SpaceResult& Result)
			 {
				 if (Result.GetResultCode() == csp::services::EResultCode::Success)
				 {
					 const csp::common::String& SpaceCode = Result.GetSpaceCode();

					 csp::services::ResponseHandlerPtr ResponseHandler
						 = GroupAPI->CreateHandler<SpaceResultCallback, SpaceResult, void, chs::GroupDto>(Callback, nullptr);

					 static_cast<chs::GroupApi*>(GroupAPI)->apiV1GroupCodesGroupCodeUsersUserIdPut(SpaceCode, UserId, ResponseHandler);
				 }
				 else if (Result.GetResultCode() == csp::services::EResultCode::Failed)
				 {
					 Callback(Result);
				 }
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
	auto& SystemsManager	= csp::systems::SystemsManager::Get();
	auto* POIInternalSystem = static_cast<PointOfInterestInternalSystem*>(SystemsManager.GetPointOfInterestSystem());

	SiteInfo.SpaceId = SpaceId;
	POIInternalSystem->CreateSite(SiteInfo, Callback);
}

void SpaceSystem::RemoveSiteInfo(const String& SpaceId, Site& SiteInfo, NullResultCallback Callback)
{
	auto& SystemsManager	= csp::systems::SystemsManager::Get();
	auto* POIInternalSystem = static_cast<PointOfInterestInternalSystem*>(SystemsManager.GetPointOfInterestSystem());

	SiteInfo.SpaceId = SpaceId;
	POIInternalSystem->DeleteSite(SiteInfo, Callback);
}

void SpaceSystem::GetSitesInfo(const String& SpaceId, SitesCollectionResultCallback Callback)
{
	auto& SystemsManager	= csp::systems::SystemsManager::Get();
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
			// an owner must firstly pass the space ownership to someone else before it can become a user
			NullResult InternalResult;
			InternalResult.SetResult(csp::services::EResultCode::Failed, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNotAcceptable));
			Callback(InternalResult);
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
		assert("Unsupported User Role!");
	}
}

void SpaceSystem::GetUsersRoles(const String& SpaceId, const Array<String>& RequestedUserIds, UserRoleCollectionCallback Callback)
{
	SpaceResultCallback GetSpaceCallback = [=](const SpaceResult& SpaceResult)
	{
		if (SpaceResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			auto& Space = SpaceResult.GetSpace();

			UserRoleCollectionResult Result;
			Result.FillUsersRoles(Space, RequestedUserIds);
			Callback(Result);
		}
		else
		{
			UserRoleCollectionResult InternalResult(SpaceResult.GetResultCode(), SpaceResult.GetHttpResultCode());
			Callback(InternalResult);
		}
	};

	GetSpace(SpaceId, GetSpaceCallback);
}

void SpaceSystem::UpdateSpaceMetadata(const String& SpaceId, const Map<String, String>& NewMetadata, NullResultCallback Callback)
{
	AssetCollectionResultCallback MetadataAssetCollCallback = [=](const AssetCollectionResult& Result)
	{
		auto AssetSystem = SystemsManager::Get().GetAssetSystem();

		if (Result.GetResultCode() != csp::services::EResultCode::Success)
		{
			NullResult InternalResult(Result);
			Callback(InternalResult);

			return;
		}

		AssetCollectionResultCallback UpdateAssetCollCallback = [=](const AssetCollectionResult& _Result)
		{
			NullResult InternalResult(_Result);
			Callback(InternalResult);
		};

		const auto& AssetCollection = Result.GetAssetCollection();
		AssetSystem->UpdateAssetCollectionMetadata(AssetCollection, NewMetadata, UpdateAssetCollCallback);
	};

	GetMetadataAssetCollection(SpaceId, MetadataAssetCollCallback);
}

void SpaceSystem::GetSpacesMetadata(const Array<String>& SpaceIds, SpacesMetadataResultCallback Callback)
{
	AssetCollectionsResultCallback MetadataAssetCollCallback = [=](const AssetCollectionsResult& Result)
	{
		SpacesMetadataResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

		if (Result.GetResultCode() == csp::services::EResultCode::Success)
		{
			Map<String, Map<String, String>> SpacesMetadata;
			const auto& AssetCollections = Result.GetAssetCollections();

			for (int i = 0; i < AssetCollections.Size(); ++i)
			{
				const auto& AssetCollection = AssetCollections[i];
				const auto& Metadata		= AssetCollection.GetMetadataImmutable();

				auto SpaceId = SpaceSystemHelpers::GetSpaceIdFromMetadataAssetCollectionName(AssetCollection.Name);

				// Convert old JSON metadata to key-value metadata
				if (Metadata.HasKey(SpaceSystemHelpers::SPACE_METADATA_KEY) && !Metadata.HasKey("site"))
				{
					FOUNDATION_LOG_FORMAT(LogLevel::Verbose, "Converting old space metadata (Space ID: %s)", SpaceId.c_str());

					const auto& Json = Metadata[SpaceSystemHelpers::SPACE_METADATA_KEY];
					Map<String, String> NewMetadata;
					::ConvertJsonMetadataToMapMetadata(Json, NewMetadata);

					SpacesMetadata[SpaceId] = NewMetadata;
				}
				else
				{
					SpacesMetadata[SpaceId] = Metadata;
				}
			}

			InternalResult.SetMetadata(SpacesMetadata);
		}

		Callback(InternalResult);
	};

	GetMetadataAssetCollections(SpaceIds, MetadataAssetCollCallback);
}

void SpaceSystem::GetSpaceMetadata(const String& SpaceId, SpaceMetadataResultCallback Callback)
{
	AssetCollectionResultCallback MetadataAssetCollCallback = [=](const AssetCollectionResult& Result)
	{
		SpaceMetadataResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

		if (Result.GetResultCode() == csp::services::EResultCode::Success)
		{
			const auto& AssetCollection = Result.GetAssetCollection();
			const auto& Metadata		= AssetCollection.GetMetadataImmutable();

			// Convert old JSON metadata to key-value metadata
			if (Metadata.HasKey(SpaceSystemHelpers::SPACE_METADATA_KEY) && !Metadata.HasKey("site"))
			{
				FOUNDATION_LOG_FORMAT(LogLevel::Verbose, "Converting old space metadata (Space ID: %s)", SpaceId.c_str());

				const auto& Json = Metadata[SpaceSystemHelpers::SPACE_METADATA_KEY];
				Map<String, String> NewMetadata;
				::ConvertJsonMetadataToMapMetadata(Json, NewMetadata);

				InternalResult.SetMetadata(NewMetadata);
			}
			else
			{
				InternalResult.SetMetadata(Metadata);
			}
		}

		Callback(InternalResult);
	};

	GetMetadataAssetCollection(SpaceId, MetadataAssetCollCallback);
}

void SpaceSystem::GetSpaceThumbnail(const String& SpaceId, UriResultCallback Callback)
{
	AssetCollectionsResultCallback ThumbnailAssetCollCallback = [=](const AssetCollectionsResult& AssetCollResult)
	{
		if (AssetCollResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			const auto& AssetCollections = AssetCollResult.GetAssetCollections();

			if (AssetCollections.IsEmpty())
			{
				// space doesn't have a thumbnail
				const UriResult InternalResult(csp::services::EResultCode::Success,
											   static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNotFound));
				Callback(InternalResult);
			}
			else
			{
				const auto& ThumbnailAssetCollection = AssetCollections[0];

				AssetsResultCallback ThumbnailAssetCallback = [=](const AssetsResult& AssetsResult)
				{
					if (AssetsResult.GetResultCode() == csp::services::EResultCode::Success)
					{
						const UriResult InternalResult(AssetsResult.GetAssets()[0].Uri);
						Callback(InternalResult);
					}
					else
					{
						const UriResult InternalResult(AssetsResult.GetResultCode(), AssetsResult.GetHttpResultCode());
						Callback(InternalResult);
					}
				};

				GetSpaceThumbnailAsset(ThumbnailAssetCollection, ThumbnailAssetCallback);
			}
		}
		else
		{
			const UriResult InternalResult(AssetCollResult.GetResultCode(), AssetCollResult.GetHttpResultCode());
			Callback(InternalResult);
		}
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
	auto AssetSystem				 = csp::systems::SystemsManager::Get().GetAssetSystem();
	auto MetadataAssetCollectionName = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(SpaceId);

	AssetSystem->GetAssetCollectionByName(MetadataAssetCollectionName, Callback);
}

void SpaceSystem::GetMetadataAssetCollections(const Array<csp::common::String>& SpaceIds, AssetCollectionsResultCallback Callback)
{
	auto AssetSystem = SystemsManager::Get().GetAssetSystem();
	Array<String> AssetCollectionNames(SpaceIds.Size());

	for (auto item = 0; item < SpaceIds.Size(); ++item)
	{
		AssetCollectionNames[item] = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(SpaceIds[item]);
	}

	AssetSystem->GetAssetCollectionsByCriteria(nullptr, nullptr, nullptr, nullptr, AssetCollectionNames, nullptr, nullptr, Callback);
}

void SpaceSystem::AddMetadata(const csp::common::String& SpaceId, const Map<String, String>& Metadata, NullResultCallback Callback)
{
	AssetCollectionResultCallback CreateAssetCollCallback = [=](const AssetCollectionResult& Result)
	{
		NullResult InternalResult(Result);
		Callback(InternalResult);
	};

	auto MetadataAssetCollectionName = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(SpaceId);
	auto AssetSystem				 = SystemsManager::Get().GetAssetSystem();

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
	AssetCollectionResultCallback GetAssetCollCallback = [=](const AssetCollectionResult& AssetCollResult)
	{
		if (AssetCollResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			NullResultCallback DeleteAssetCollCallback = [=](const NullResult& Result)
			{
				NullResult InternalResult(Result);
				Callback(Result);
			};

			auto AssetSystem = SystemsManager::Get().GetAssetSystem();
			AssetSystem->DeleteAssetCollection(AssetCollResult.GetAssetCollection(), DeleteAssetCollCallback);
		}
		else
		{
			NullResult InternalResult(AssetCollResult);
			Callback(InternalResult);
		}
	};

	GetMetadataAssetCollection(SpaceId, GetAssetCollCallback);
}



void SpaceSystem::AddSpaceThumbnail(const csp::common::String& SpaceId, const FileAssetDataSource& ImageDataSource, NullResultCallback Callback)
{
	const auto AssetSystem = SystemsManager::Get().GetAssetSystem();

	AssetCollectionResultCallback CreateAssetCollCallback = [=](const AssetCollectionResult& AssetCollResult)
	{
		if (AssetCollResult.GetResultCode() == csp::services::EResultCode::InProgress)
		{
			return;
		}

		if (AssetCollResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			const auto& ThumbnailAssetColl = AssetCollResult.GetAssetCollection();

			AssetResultCallback CreateAssetCallback = [=](const AssetResult& CreateAssetResult)
			{
				if (CreateAssetResult.GetResultCode() == csp::services::EResultCode::InProgress)
				{
					return;
				}

				if (CreateAssetResult.GetResultCode() == csp::services::EResultCode::Success)
				{
					UriResultCallback UploadCallback = [=](const UriResult& UploadResult)
					{
						if (UploadResult.GetResultCode() == csp::services::EResultCode::Failed)
						{
							FOUNDATION_LOG_FORMAT(LogLevel::Log,
												  "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
												  (int) UploadResult.GetResultCode(),
												  UploadResult.GetHttpResultCode());
						}

						const NullResult InternalResult(UploadResult);
						Callback(InternalResult);
					};

					AssetSystem->UploadAssetData(ThumbnailAssetColl, CreateAssetResult.GetAsset(), ImageDataSource, UploadCallback);
				}
				else
				{
					FOUNDATION_LOG_FORMAT(LogLevel::Log,
										  "The Space thumbnail asset creation was not successful. ResCode: %d, HttpResCode: %d",
										  (int) CreateAssetResult.GetResultCode(),
										  CreateAssetResult.GetHttpResultCode());

					const NullResult InternalResult(CreateAssetResult);
					Callback(InternalResult);
				}
			};

			const auto UniqueAssetName = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(SpaceId);
			AssetSystem->CreateAsset(ThumbnailAssetColl, UniqueAssetName, nullptr, nullptr, EAssetType::IMAGE, CreateAssetCallback);
		}
		else
		{
			FOUNDATION_LOG_FORMAT(LogLevel::Log,
								  "The Space thumbnail asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
								  (int) AssetCollResult.GetResultCode(),
								  AssetCollResult.GetHttpResultCode());

			const NullResult InternalResult(AssetCollResult);
			Callback(InternalResult);
		}
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
	const auto AssetSystem = SystemsManager::Get().GetAssetSystem();

	AssetCollectionResultCallback CreateAssetCollCallback = [=](const AssetCollectionResult& AssetCollResult)
	{
		if (AssetCollResult.GetResultCode() == csp::services::EResultCode::InProgress)
		{
			return;
		}

		if (AssetCollResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			const auto& ThumbnailAssetColl = AssetCollResult.GetAssetCollection();

			AssetResultCallback CreateAssetCallback = [=](const AssetResult& CreateAssetResult)
			{
				if (CreateAssetResult.GetResultCode() == csp::services::EResultCode::InProgress)
				{
					return;
				}

				if (CreateAssetResult.GetResultCode() == csp::services::EResultCode::Success)
				{
					UriResultCallback UploadCallback = [=](const UriResult& UploadResult)
					{
						if (UploadResult.GetResultCode() == csp::services::EResultCode::Failed)
						{
							FOUNDATION_LOG_FORMAT(LogLevel::Log,
												  "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
												  (int) UploadResult.GetResultCode(),
												  UploadResult.GetHttpResultCode());
						}

						const NullResult InternalResult(UploadResult);
						Callback(InternalResult);
					};

					Asset ThumbnailAsset = CreateAssetResult.GetAsset();

					ThumbnailAsset.FileName = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(
						SpaceSystemHelpers::GetAssetFileExtension(ImageDataSource.GetMimeType()));
					ThumbnailAsset.MimeType = ImageDataSource.GetMimeType();
					AssetSystem->UploadAssetData(ThumbnailAssetColl, ThumbnailAsset, ImageDataSource, UploadCallback);
				}
				else
				{
					FOUNDATION_LOG_FORMAT(LogLevel::Log,
										  "The Space thumbnail asset creation was not successful. ResCode: %d, HttpResCode: %d",
										  (int) CreateAssetResult.GetResultCode(),
										  CreateAssetResult.GetHttpResultCode());

					const NullResult InternalResult(CreateAssetResult);
					Callback(InternalResult);
				}
			};

			const auto UniqueAssetName = SpaceSystemHelpers::GetUniqueSpaceThumbnailAssetName(SpaceId);
			AssetSystem->CreateAsset(ThumbnailAssetColl, UniqueAssetName, nullptr, nullptr, EAssetType::IMAGE, CreateAssetCallback);
		}
		else
		{
			FOUNDATION_LOG_FORMAT(LogLevel::Log,
								  "The Space thumbnail asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
								  (int) AssetCollResult.GetResultCode(),
								  AssetCollResult.GetHttpResultCode());

			const NullResult InternalResult(AssetCollResult);
			Callback(InternalResult);
		}
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
	AssetCollectionsResultCallback GetAssetCollCallback = [=](const AssetCollectionsResult& AssetCollResult)
	{
		if (AssetCollResult.GetResultCode() == csp::services::EResultCode::Failed)
		{
			FOUNDATION_LOG_FORMAT(LogLevel::Log,
								  "The Space thumbnail asset collection retrieval has failed. ResCode: %d, HttpResCode: %d",
								  (int) AssetCollResult.GetResultCode(),
								  AssetCollResult.GetHttpResultCode());
		}

		Callback(AssetCollResult);
	};

	auto AssetSystem				 = SystemsManager::Get().GetAssetSystem();
	auto MetadataAssetCollectionName = SpaceSystemHelpers::GetSpaceMetadataAssetCollectionName(SpaceId);

	const Array<String> Tag({SpaceId});

	AssetSystem->GetAssetCollectionsByCriteria(SpaceId,
											   nullptr,
											   EAssetCollectionType::SPACE_THUMBNAIL,
											   Tag,
											   nullptr,
											   nullptr,
											   nullptr,
											   GetAssetCollCallback);
}

void SpaceSystem::GetSpaceThumbnailAsset(const AssetCollection& ThumbnailAssetCollection, AssetsResultCallback Callback)
{
	AssetsResultCallback ThumbnailAssetCallback = [=](const AssetsResult& AssetsResult)
	{
		if (AssetsResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			assert(!AssetsResult.GetAssets().IsEmpty() && "Space thumbnail asset should exist");
			assert((AssetsResult.GetAssets().Size() == 1) && "There should be only one Space thumbnail asset");
		}
		else if (AssetsResult.GetResultCode() == csp::services::EResultCode::Failed)
		{
			FOUNDATION_LOG_FORMAT(LogLevel::Log,
								  "The Space thumbnail asset retrieval has failed. ResCode: %d, HttpResCode: %d",
								  (int) AssetsResult.GetResultCode(),
								  AssetsResult.GetHttpResultCode());
		}

		Callback(AssetsResult);
	};

	const auto AssetSystem = SystemsManager::Get().GetAssetSystem();
	AssetSystem->GetAssetsInCollection(ThumbnailAssetCollection, ThumbnailAssetCallback);
}

void SpaceSystem::RemoveSpaceThumbnail(const csp::common::String& SpaceId, NullResultCallback Callback)
{
	const auto AssetSystem = SystemsManager::Get().GetAssetSystem();

	AssetCollectionsResultCallback ThumbnailAssetCollCallback = [=](const AssetCollectionsResult& AssetCollResult)
	{
		if (AssetCollResult.GetResultCode() == csp::services::EResultCode::InProgress)
		{
			return;
		}

		if (AssetCollResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			const auto& AssetCollections = AssetCollResult.GetAssetCollections();

			if (AssetCollections.IsEmpty())
			{
				// space doesn't have a thumbnail so we're done
				const NullResult InternalResult(AssetCollResult);
				Callback(InternalResult);
			}
			else
			{
				const auto& ThumbnailAssetCollection = AssetCollections[0];

				AssetsResultCallback ThumbnailAssetCallback = [=](const AssetsResult& AssetsResult)
				{
					if (AssetsResult.GetResultCode() == csp::services::EResultCode::InProgress)
					{
						return;
					}

					if (AssetsResult.GetResultCode() == csp::services::EResultCode::Success)
					{
						NullResultCallback DeleteAssetCallback = [=](const NullResult& DeleteAssetResult)
						{
							if (DeleteAssetResult.GetResultCode() == csp::services::EResultCode::InProgress)
							{
								return;
							}

							if (DeleteAssetResult.GetResultCode() == csp::services::EResultCode::Success)
							{
								NullResultCallback DeleteAssetCollCallback = [=](const NullResult& DeleteAssetCollResult)
								{
									if (DeleteAssetCollResult.GetResultCode() == csp::services::EResultCode::Failed)
									{
										FOUNDATION_LOG_FORMAT(
											LogLevel::Log,
											"The Space thumbnail asset collection deletion has failed. ResCode: %d, HttpResCode: %d",
											(int) DeleteAssetResult.GetResultCode(),
											DeleteAssetResult.GetHttpResultCode());
									}

									NullResult InternalResult(DeleteAssetCollResult);
									Callback(DeleteAssetCollResult);
								};

								AssetSystem->DeleteAssetCollection(ThumbnailAssetCollection, DeleteAssetCollCallback);
							}
							else
							{
								FOUNDATION_LOG_FORMAT(LogLevel::Log,
													  "The Space thumbnail asset deletion was not successful. ResCode: %d, HttpResCode: %d",
													  (int) DeleteAssetResult.GetResultCode(),
													  DeleteAssetResult.GetHttpResultCode());

								NullResult InternalResult(DeleteAssetResult);
								Callback(DeleteAssetResult);
							}
						};

						const auto& ThumbnailAsset = AssetsResult.GetAssets()[0];
						AssetSystem->DeleteAsset(ThumbnailAssetCollection, ThumbnailAsset, DeleteAssetCallback);
					}
					else
					{
						const NullResult InternalResult(AssetsResult);
						Callback(InternalResult);
					}
				};

				GetSpaceThumbnailAsset(ThumbnailAssetCollection, ThumbnailAssetCallback);
			}
		}
		else
		{
			const NullResult InternalResult(AssetCollResult);
			Callback(InternalResult);
		}
	};

	GetSpaceThumbnailAssetCollection(SpaceId, ThumbnailAssetCollCallback);
}

void SpaceSystem::GetSpaceGeoLocationInternal(const csp::common::String& SpaceId, SpaceGeoLocationResultCallback Callback)
{
	auto& SystemsManager	= csp::systems::SystemsManager::Get();
	auto* POIInternalSystem = static_cast<PointOfInterestInternalSystem*>(SystemsManager.GetPointOfInterestSystem());
	POIInternalSystem->GetSpaceGeoLocation(SpaceId, Callback);
}

void SpaceSystem::SetConnectionCallbacks(csp::multiplayer::MultiplayerConnection* Connection)
{
	if (!EntityCreatedCallback || !InitialEntitiesRetrievedCallback || !ScriptSystemReadyCallback)
	{
		FOUNDATION_LOG_WARN_MSG("Space connection callbacks have not been set.");
		return;
	}

	if (Connection)
	{
		Connection->GetSpaceEntitySystem()->SetEntityCreatedCallback(EntityCreatedCallback);
		Connection->GetSpaceEntitySystem()->SetInitialEntitiesRetrievedCallback(InitialEntitiesRetrievedCallback);
		Connection->GetSpaceEntitySystem()->SetScriptSystemReadyCallback(ScriptSystemReadyCallback);
	}
}

void SpaceSystem::GetSpaceGeoLocation(const csp::common::String& SpaceId, SpaceGeoLocationResultCallback Callback)
{
	// First refresh the space to ensure the user has access to the space
	SpaceResultCallback GetSpaceCallback = [=](const SpaceResult& GetSpaceResult)
	{
		if (GetSpaceResult.GetResultCode() == csp::services::EResultCode::Failed)
		{
			SpaceGeoLocationResult Result(GetSpaceResult.GetResultCode(), GetSpaceResult.GetHttpResultCode());
			Callback(Result);
		}
		else if (GetSpaceResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			const auto RefreshedSpace = GetSpaceResult.GetSpace();

			const auto UserId = SystemsManager::Get().GetUserSystem()->GetLoginState().UserId;

			// First check if the user is the owner
			bool UserCanAccessSpaceDetails
				= !(bool) (RefreshedSpace.Attributes & SpaceAttributes::RequiresInvite) || RefreshedSpace.OwnerId == UserId;

			// If the user is not the owner check are they a moderator
			if (!UserCanAccessSpaceDetails)
			{
				for (int i = 0; i < RefreshedSpace.ModeratorIds.Size(); ++i)
				{
					if (RefreshedSpace.ModeratorIds[i] == UserId)
					{
						UserCanAccessSpaceDetails = true;
						break;
					}
				}
			}

			// If the user is not the owner or a moderator check are the full user list
			if (!UserCanAccessSpaceDetails)
			{
				for (int i = 0; i < RefreshedSpace.UserIds.Size(); ++i)
				{
					if (RefreshedSpace.UserIds[i] == UserId)
					{
						UserCanAccessSpaceDetails = true;
						break;
					}
				}
			}

			if (UserCanAccessSpaceDetails)
			{
				GetSpaceGeoLocationInternal(RefreshedSpace.Id, Callback);
			}
			else
			{
				SpaceGeoLocationResult Result(csp::services::EResultCode::Failed, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));
				Callback(Result);
			}
		}
	};

	GetSpace(SpaceId, GetSpaceCallback);
}

void SpaceSystem::DeleteSpaceGeoLocation(const csp::common::String& SpaceId, NullResultCallback Callback)
{
	SpaceGeoLocationResultCallback GetSpaceGeoLocationCallback = [=](const SpaceGeoLocationResult& GetGeoLocationResult)
	{
		if (GetGeoLocationResult.GetResultCode() == csp::services::EResultCode::Failed
			|| GetGeoLocationResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			auto& SystemsManager	= csp::systems::SystemsManager::Get();
			auto* POIInternalSystem = static_cast<PointOfInterestInternalSystem*>(SystemsManager.GetPointOfInterestSystem());

			if (GetGeoLocationResult.HasGeoLocation)
			{
				POIInternalSystem->DeleteSpaceGeoLocation(GetGeoLocationResult.GetSpaceGeoLocation().Id, Callback);
			}
			else
			{
				Callback(NullResult::Invalid());
			}
		}
	};

	// First refresh the space to ensure the user has access to the space
	SpaceResultCallback GetSpaceCallback = [=](const SpaceResult& GetSpaceResult)
	{
		if (GetSpaceResult.GetResultCode() == csp::services::EResultCode::Failed)
		{
			NullResult Result(GetSpaceResult.GetResultCode(), GetSpaceResult.GetHttpResultCode());
			Callback(Result);
		}
		else if (GetSpaceResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			const auto RefreshedSpace = GetSpaceResult.GetSpace();

			const auto UserID = SystemsManager::Get().GetUserSystem()->GetLoginState().UserId;

			// First check if the user is the owner
			bool UserCanModifySpace = RefreshedSpace.OwnerId == UserID;

			// If the user is not the owner check are they a moderator
			if (!UserCanModifySpace)
			{
				for (int i = 0; i < RefreshedSpace.ModeratorIds.Size(); ++i)
				{
					if (RefreshedSpace.ModeratorIds[i] == UserID)
					{
						UserCanModifySpace = true;
						break;
					}
				}
			}

			if (UserCanModifySpace)
			{
				GetSpaceGeoLocationInternal(RefreshedSpace.Id, GetSpaceGeoLocationCallback);
			}
			else
			{
				NullResult Result(csp::services::EResultCode::Failed, static_cast<uint16_t>(csp::web::EResponseCodes::ResponseForbidden));
				Callback(Result);
			}
		}
	};

	GetSpace(SpaceId, GetSpaceCallback);
}

} // namespace csp::systems
