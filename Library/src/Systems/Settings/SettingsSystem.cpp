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
#include "CSP/Systems/Settings/SettingsSystem.h"

#include "CSP/Common/Array.h"
#include "CSP/Common/List.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/Assets/AssetSystem.h"
#include "CSP/Systems/Users/UserSystem.h"
#include "Services/ApiBase/ApiBase.h"
#include "Services/UserService/Api.h"
#include "Services/UserService/Dto.h"
#include "Systems/Spaces/SpaceSystemHelpers.h"

#include <iostream>
#include <sstream>

#define MAX_RECENT_SPACES 10

constexpr const char* AVATAR_PORTRAIT_ASSET_NAME			= "AVATAR_PORTRAIT_ASSET_";
constexpr const char* AVATAR_PORTRAIT_ASSET_COLLECTION_NAME = "AVATAR_PORTRAIT_ASSET_COLLECTION_";

namespace chs = csp::services::generated::userservice;


namespace csp::systems
{

SettingsSystem::SettingsSystem() : SystemBase(), SettingsAPI(nullptr)
{
}

SettingsSystem::SettingsSystem(csp::web::WebClient* InWebClient) : SystemBase(InWebClient)
{
	SettingsAPI = CSP_NEW chs::SettingsApi(InWebClient);
}

SettingsSystem::~SettingsSystem()
{
	CSP_DELETE(SettingsAPI);
}

void SettingsSystem::SetSettingValue(const csp::common::String& InUserId,
									 const csp::common::String& InContext,
									 const csp::common::String& InKey,
									 const csp::common::String& InValue,
									 NullResultCallback Callback) const
{
	auto& SystemsManager   = csp::systems::SystemsManager::Get();
	const auto* UserSystem = SystemsManager.GetUserSystem();

	const auto InSettings = std::make_shared<chs::SettingsDto>();
	std::map<csp::common::String, csp::common::String> NewSettings;
	NewSettings.clear();
	NewSettings.insert(std::make_pair(InKey, InValue));
	InSettings->SetSettings(NewSettings);
	InSettings->SetContext(InContext);
	InSettings->SetUserId(InUserId);

	SettingsResultCallback InternalCallback = [InKey, Callback](const csp::systems::SettingsCollectionResult& Result)
	{
		if (Result.GetResultCode() == csp::services::EResultCode::InProgress)
		{
			return;
		}

		NullResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

		Callback(InternalResult);
	};

	csp::services::ResponseHandlerPtr SettingsResponseHandler
		= SettingsAPI->CreateHandler<SettingsResultCallback, SettingsCollectionResult, void, chs::SettingsDto>(InternalCallback,
																											   nullptr,
																											   csp::web::EResponseCodes::ResponseOK);

	static_cast<chs::SettingsApi*>(SettingsAPI)->apiV1UsersUserIdSettingsContextPut(InUserId, InContext, InSettings, SettingsResponseHandler);
}

void SettingsSystem::GetSettingValue(const csp::common::String& InUserId,
									 const csp::common::String& InContext,
									 const csp::common::String& InKey,
									 StringResultCallback Callback) const
{
	auto& SystemsManager				   = csp::systems::SystemsManager::Get();
	const auto* UserSystem				   = SystemsManager.GetUserSystem();
	std::vector<csp::common::String> MyKey = {InKey};

	SettingsResultCallback InternalCallback = [InKey, Callback](const csp::systems::SettingsCollectionResult& Result)
	{
		StringResult StringResult(Result.GetResultCode(), Result.GetHttpResultCode());

		if (Result.GetResultCode() == csp::services::EResultCode::InProgress)
		{
			return;
		}
		else if (Result.GetResultCode() == csp::services::EResultCode::Success)
		{
			// Only attempt to read settings if result is valid

			auto& Settings = Result.GetSettingsCollection().Settings;

			if (Settings.HasKey(InKey))
			{
				StringResult.SetValue(Settings[InKey]);
			}
			else
			{
				StringResult.SetValue("");
			}
		}
		else
		{
			// Handle failure state
			StringResult.SetValue("");
		}

		Callback(StringResult);
	};

	csp::services::ResponseHandlerPtr SettingsResponseHandler
		= SettingsAPI->CreateHandler<SettingsResultCallback, SettingsCollectionResult, void, chs::SettingsDto>(InternalCallback,
																											   nullptr,
																											   csp::web::EResponseCodes::ResponseOK);

	static_cast<chs::SettingsApi*>(SettingsAPI)->apiV1UsersUserIdSettingsContextGet(InUserId, InContext, MyKey, SettingsResponseHandler);
}

void SettingsSystem::SetNDAStatus(const csp::common::String& InUserId, bool InValue, NullResultCallback Callback)
{
	const csp::common::String NDAStatus = InValue ? "true" : "false";

	SetSettingValue(InUserId, "UserSettings", "NDAStatus", NDAStatus, Callback);
}

void SettingsSystem::GetNDAStatus(const csp::common::String& InUserId, BooleanResultCallback Callback)
{
	StringResultCallback GetSettingCallback = [=](const StringResult& Result)
	{
		BooleanResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
		InternalResult.SetValue(Result.GetValue() == "true");

		Callback(InternalResult);
	};

	GetSettingValue(InUserId, "UserSettings", "NDAStatus", GetSettingCallback);
}

void SettingsSystem::SetNewsletterStatus(const csp::common::String& InUserId, bool InValue, NullResultCallback Callback)
{
	const csp::common::String NewsletterStatus = InValue ? "true" : "false";

	SetSettingValue(InUserId, "UserSettings", "Newsletter", NewsletterStatus, Callback);
}

void SettingsSystem::GetNewsletterStatus(const csp::common::String& InUserId, BooleanResultCallback Callback)
{
	StringResultCallback GetSettingCallback = [=](const StringResult& Result)
	{
		BooleanResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());
		InternalResult.SetValue(Result.GetValue() == "true");

		Callback(InternalResult);
	};

	GetSettingValue(InUserId, "UserSettings", "Newsletter", GetSettingCallback);
}

void SettingsSystem::AddRecentlyVisitedSpace(const csp::common::String& InUserId, const csp::common::String InSpaceID, NullResultCallback Callback)
{
	StringArrayResultCallback GetRecentSpacesCallback = [=](StringArrayResult Result)
	{
		if (Result.GetResultCode() == csp::services::EResultCode::Failed)
		{
			NullResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

			Callback(InternalResult);

			return;
		}

		auto RecentSpacesArray = Result.GetValue();

		auto RecentSpaces = RecentSpacesArray.ToList();
		RecentSpaces.Insert(0, InSpaceID);

		// Remove duplicate entry
		for (size_t i = RecentSpaces.Size() - 1; i > 0; --i)
		{
			if (RecentSpaces[i] == InSpaceID)
			{
				RecentSpaces.Remove(i);

				// Only a single duplicate entry (if any) should be found
				break;
			}
		}

		// Remove last entry if max number of spaces reached
		if (RecentSpaces.Size() > MAX_RECENT_SPACES)
		{
			RecentSpaces.Remove(MAX_RECENT_SPACES);
		}

		auto RecentSpacesString = csp::common::String::Join(RecentSpaces, ',');

		SetSettingValue(InUserId, "UserSettings", "RecentSpaces", RecentSpacesString, Callback);
	};

	GetRecentlyVisitedSpaces(InUserId, GetRecentSpacesCallback);
}

void SettingsSystem::GetRecentlyVisitedSpaces(const csp::common::String& InUserId, StringArrayResultCallback Callback)
{
	StringResultCallback GetSettingCallback = [=](const csp::systems::StringResult& Result)
	{
		StringArrayResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

		if (Result.GetResultCode() == csp::services::EResultCode::Success)
		{
			auto& RecentSpacesString = Result.GetValue();

			if (RecentSpacesString.IsEmpty())
			{
				InternalResult.SetValue({});
			}
			else
			{
				auto RecentSpacesList = RecentSpacesString.Split(',');
				auto RecentSpaces	  = RecentSpacesList.ToArray();
				InternalResult.SetValue(RecentSpaces);
			}
		}

		Callback(InternalResult);
	};

	GetSettingValue(InUserId, "UserSettings", "RecentSpaces", GetSettingCallback);
}

void SettingsSystem::ClearRecentlyVisitedSpaces(const csp::common::String& InUserId, NullResultCallback Callback)
{
	SetSettingValue(InUserId, "UserSettings", "RecentSpaces", "", Callback);
}

void SettingsSystem::AddBlockedSpace(const csp::common::String& InUserId, const csp::common::String InSpaceID, NullResultCallback Callback)
{
	StringArrayResultCallback GetBlockedSpacesCallback = [=](StringArrayResult Result)
	{
		if (Result.GetResultCode() == csp::services::EResultCode::Failed)
		{
			NullResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

			Callback(InternalResult);

			return;
		}

		auto BlockedSpacesArray = Result.GetValue();

		// Ignore if space already blocked
		for (int i = 0; i < BlockedSpacesArray.Size(); ++i)
		{
			if (BlockedSpacesArray[i] == InSpaceID)
			{
				NullResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

				Callback(InternalResult);

				return;
			}
		}

		auto BlockedSpaces = BlockedSpacesArray.ToList();
		BlockedSpaces.Insert(0, InSpaceID);

		auto BlockedSpacesString = csp::common::String::Join(BlockedSpaces, ',');

		SetSettingValue(InUserId, "UserSettings", "BlockedSpaces", BlockedSpacesString, Callback);
	};

	GetBlockedSpaces(InUserId, GetBlockedSpacesCallback);
}

void SettingsSystem::RemoveBlockedSpace(const csp::common::String& InUserId, const csp::common::String InSpaceID, NullResultCallback Callback)
{
	StringArrayResultCallback GetBlockedSpacesCallback = [=](StringArrayResult Result)
	{
		if (Result.GetResultCode() == csp::services::EResultCode::Failed)
		{
			NullResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

			Callback(InternalResult);

			return;
		}

		auto BlockedSpacesArray = Result.GetValue();
		auto FoundSpace			= false;

		// Ignore if space not blocked
		for (int i = 0; i < BlockedSpacesArray.Size(); ++i)
		{
			if (BlockedSpacesArray[i] == InSpaceID)
			{
				FoundSpace = true;

				break;
			}
		}

		if (!FoundSpace)
		{
			NullResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

			Callback(InternalResult);

			return;
		}

		auto BlockedSpaces = BlockedSpacesArray.ToList();
		BlockedSpaces.RemoveItem(InSpaceID);

		auto BlockedSpacesString = csp::common::String::Join(BlockedSpaces, ',');

		SetSettingValue(InUserId, "UserSettings", "BlockedSpaces", BlockedSpacesString, Callback);
	};

	GetBlockedSpaces(InUserId, GetBlockedSpacesCallback);
}

void SettingsSystem::GetBlockedSpaces(const csp::common::String& InUserId, StringArrayResultCallback Callback)
{
	StringResultCallback GetSettingCallback = [=](const csp::systems::StringResult& Result)
	{
		StringArrayResult InternalResult(Result.GetResultCode(), Result.GetHttpResultCode());

		if (Result.GetResultCode() == csp::services::EResultCode::Success)
		{
			auto BlockedSpacesString = Result.GetValue();

			if (BlockedSpacesString.IsEmpty())
			{
				InternalResult.SetValue({});
			}
			else
			{
				auto BlockedSpacesList = BlockedSpacesString.Split(',');
				auto BlockedSpaces	   = BlockedSpacesList.ToArray();
				InternalResult.SetValue(BlockedSpaces);
			}
		}

		Callback(InternalResult);
	};

	GetSettingValue(InUserId, "UserSettings", "BlockedSpaces", GetSettingCallback);
}

void SettingsSystem::ClearBlockedSpaces(const csp::common::String& InUserId, NullResultCallback Callback)
{
	SetSettingValue(InUserId, "UserSettings", "BlockedSpaces", "", Callback);
}

void SettingsSystem::UpdateAvatarPortrait(const csp::common::String& UserId,
										  const csp::systems::FileAssetDataSource& NewAvatarPortrait,
										  NullResultCallback Callback)
{
	AssetCollectionsResultCallback AvatarPortraitAssetCollCallback = [=](const AssetCollectionsResult& AssetCollResult)
	{
		if (AssetCollResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			const auto& AssetCollections = AssetCollResult.GetAssetCollections();

			if (AssetCollections.IsEmpty())
			{
				// space without a thumbnail
				AddAvatarPortrait(UserId, NewAvatarPortrait, Callback);
			}
			else
			{
				const auto& AvatarPortraitAssetCollection = AssetCollections[0];

				AssetsResultCallback AvatarPortraitAssetCallback = [=](const AssetsResult& AssetsResult)
				{
					if (AssetsResult.GetResultCode() == csp::services::EResultCode::Success)
					{
						UriResultCallback UploadCallback = [=](const UriResult& UploadResult)
						{
							if (UploadResult.GetResultCode() == csp::services::EResultCode::Failed)
							{
								CSP_LOG_FORMAT(LogLevel::Log,
											   "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
											   (int) UploadResult.GetResultCode(),
											   UploadResult.GetHttpResultCode());
							}

							const NullResult InternalResult(UploadResult);
							Callback(InternalResult);
						};

						auto& AvatarPortraitAsset	 = ((csp::systems::AssetsResult&) AssetsResult).GetAssets()[0];
						AvatarPortraitAsset.MimeType = NewAvatarPortrait.GetMimeType();
						const auto AssetSystem		 = SystemsManager::Get().GetAssetSystem();

						AssetSystem->UploadAssetData(AvatarPortraitAssetCollection, AvatarPortraitAsset, NewAvatarPortrait, UploadCallback);
					}
					else
					{
						NullResult InternalResult(AssetsResult);
						Callback(InternalResult);
					}
				};

				GetAvatarPortraitAsset(AvatarPortraitAssetCollection, AvatarPortraitAssetCallback);
			}
		}
		else
		{
			const NullResult InternalResult(AssetCollResult);
			Callback(InternalResult);
		}
	};

	GetAvatarPortraitAssetCollection(UserId, AvatarPortraitAssetCollCallback);
}

void SettingsSystem::GetAvatarPortrait(const csp::common::String& UserId, UriResultCallback Callback)
{
	AssetCollectionsResultCallback AvatarPortraitAssetCollCallback = [=](const AssetCollectionsResult& AssetCollResult)
	{
		if (AssetCollResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			const auto& AssetCollections = AssetCollResult.GetAssetCollections();

			if (AssetCollections.IsEmpty())
			{
				// User Doesn't have a Avatar Portrait
				const UriResult InternalResult(csp::services::EResultCode::Success,
											   static_cast<uint16_t>(csp::web::EResponseCodes::ResponseNotFound));
				Callback(InternalResult);
			}
			else
			{
				const auto& AvatarPortraitAssetCollection = AssetCollections[0];

				AssetsResultCallback AvatarPortraitAssetCallback = [=](const AssetsResult& AssetsResult)
				{
					if (AssetsResult.GetResultCode() == csp::services::EResultCode::Success)
					{
						const auto& Assets = AssetsResult.GetAssets();

						if (Assets.Size() > 0)
						{
							const UriResult InternalResult(AssetsResult.GetAssets()[0].Uri);
							Callback(InternalResult);
						}
						else
						{
							UriResult InternalResult(csp::services::EResultCode::Failed, 200);
							InternalResult.SetResponseBody(
								"Invalid avatar portrait AssetCollection. AssetCollection should contain an Asset but does not!");
							InternalResult.Uri = "";

							Callback(InternalResult);
						}
					}
					else
					{
						const UriResult InternalResult(AssetsResult.GetResultCode(), AssetsResult.GetHttpResultCode());
						Callback(InternalResult);
					}
				};

				GetAvatarPortraitAsset(AvatarPortraitAssetCollection, AvatarPortraitAssetCallback);
			}
		}
		else
		{
			const UriResult InternalResult(AssetCollResult.GetResultCode(), AssetCollResult.GetHttpResultCode());
			Callback(InternalResult);
		}
	};

	GetAvatarPortraitAssetCollection(UserId, AvatarPortraitAssetCollCallback);
}

void SettingsSystem::UpdateAvatarPortraitWithBuffer(const csp::common::String& UserId,
													const csp::systems::BufferAssetDataSource& NewAvatarPortrait,
													NullResultCallback Callback)
{
	AssetCollectionsResultCallback ThumbnailAssetCollCallback = [=](const AssetCollectionsResult& AssetCollResult)
	{
		if (AssetCollResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			const auto& AssetCollections = AssetCollResult.GetAssetCollections();

			if (AssetCollections.IsEmpty())
			{
				// space without a thumbnail
				AddAvatarPortraitWithBuffer(UserId, NewAvatarPortrait, Callback);
			}
			else
			{
				const auto& ThumbnailAssetCollection = AssetCollections[0];

				AssetsResultCallback ThumbnailAssetCallback = [=](const AssetsResult& AssetsResult)
				{
					if (AssetsResult.GetResultCode() == csp::services::EResultCode::Success)
					{
						UriResultCallback UploadCallback = [=](const UriResult& UploadResult)
						{
							if (UploadResult.GetResultCode() == csp::services::EResultCode::Failed)
							{
								CSP_LOG_FORMAT(LogLevel::Log,
											   "The Space thumbnail upload data has failed. ResCode: %d, HttpResCode: %d",
											   (int) UploadResult.GetResultCode(),
											   UploadResult.GetHttpResultCode());
							}

							const NullResult InternalResult(UploadResult);
							Callback(InternalResult);
						};

						auto& ThumbnailAsset	= ((csp::systems::AssetsResult&) AssetsResult).GetAssets()[0];
						ThumbnailAsset.FileName = SpaceSystemHelpers::GetUniqueAvatarThumbnailAssetName(
							SpaceSystemHelpers::GetAssetFileExtension(NewAvatarPortrait.GetMimeType()));
						ThumbnailAsset.MimeType = NewAvatarPortrait.GetMimeType();
						const auto AssetSystem	= SystemsManager::Get().GetAssetSystem();
						AssetSystem->UploadAssetData(ThumbnailAssetCollection, ThumbnailAsset, NewAvatarPortrait, UploadCallback);
					}
					else
					{
						NullResult InternalResult(AssetsResult);
						Callback(InternalResult);
					}
				};

				GetAvatarPortraitAsset(ThumbnailAssetCollection, ThumbnailAssetCallback);
			}
		}
		else
		{
			const NullResult InternalResult(AssetCollResult);
			Callback(InternalResult);
		}
	};

	GetAvatarPortraitAssetCollection(UserId, ThumbnailAssetCollCallback);
}

void SettingsSystem::AddAvatarPortrait(const csp::common::String& UserId,
									   const csp::systems::FileAssetDataSource& ImageDataSource,
									   NullResultCallback Callback)
{
	const auto AssetSystem								  = SystemsManager::Get().GetAssetSystem();
	AssetCollectionResultCallback CreateAssetCollCallback = [=](const AssetCollectionResult& AssetCollResult)
	{
		if (AssetCollResult.GetResultCode() == csp::services::EResultCode::InProgress)
		{
			return;
		}

		if (AssetCollResult.GetResultCode() == csp::services::EResultCode::Success)
		{
			const auto& PortraitAssetColl = AssetCollResult.GetAssetCollection();

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
							CSP_LOG_FORMAT(LogLevel::Error,
										   "The Avatar Portrait upload data has failed. ResCode: %d, HttpResCode: %d",
										   (int) UploadResult.GetResultCode(),
										   UploadResult.GetHttpResultCode());
						}

						const NullResult InternalResult(UploadResult);
						Callback(InternalResult);
					};

					AssetSystem->UploadAssetData(PortraitAssetColl, CreateAssetResult.GetAsset(), ImageDataSource, UploadCallback);
				}
				else
				{
					CSP_LOG_FORMAT(LogLevel::Error,
								   "The Avatar Portrait asset creation was not successful. ResCode: %d, HttpResCode: %d",
								   (int) CreateAssetResult.GetResultCode(),
								   CreateAssetResult.GetHttpResultCode());

					const NullResult InternalResult(CreateAssetResult);
					Callback(InternalResult);
				}
			};

			const auto UniqueAssetName = AVATAR_PORTRAIT_ASSET_NAME + UserId;
			AssetSystem->CreateAsset(PortraitAssetColl, UniqueAssetName, nullptr, nullptr, EAssetType::IMAGE, CreateAssetCallback);
		}
		else
		{
			CSP_LOG_FORMAT(LogLevel::Error,
						   "The Avatar Portrait asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
						   (int) AssetCollResult.GetResultCode(),
						   AssetCollResult.GetHttpResultCode());

			const NullResult InternalResult(AssetCollResult);
			Callback(InternalResult);
		}
	};

	const auto AvatarPortraitAssetCollectionName = AVATAR_PORTRAIT_ASSET_COLLECTION_NAME + UserId;

	AssetSystem->CreateAssetCollection(nullptr,
									   nullptr,
									   AvatarPortraitAssetCollectionName,
									   nullptr,
									   EAssetCollectionType::DEFAULT,
									   nullptr,
									   CreateAssetCollCallback);
}

void SettingsSystem::AddAvatarPortraitWithBuffer(const csp::common::String& UserId,
												 const csp::systems::BufferAssetDataSource& ImageDataSource,
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
			const auto& PortraitAvatarAssetColl = AssetCollResult.GetAssetCollection();

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
							CSP_LOG_FORMAT(LogLevel::Error,
										   "The Avatar Portrait upload data has failed. ResCode: %d, HttpResCode: %d",
										   (int) UploadResult.GetResultCode(),
										   UploadResult.GetHttpResultCode());
						}

						const NullResult InternalResult(UploadResult);
						Callback(InternalResult);
					};

					Asset AvatarPortraitAsset = CreateAssetResult.GetAsset();

					AvatarPortraitAsset.FileName
						= AVATAR_PORTRAIT_ASSET_NAME + UserId + SpaceSystemHelpers::GetAssetFileExtension(ImageDataSource.GetMimeType());
					AvatarPortraitAsset.MimeType = ImageDataSource.GetMimeType();
					AssetSystem->UploadAssetData(PortraitAvatarAssetColl, AvatarPortraitAsset, ImageDataSource, UploadCallback);
				}
				else
				{
					CSP_LOG_FORMAT(LogLevel::Error,
								   "The Avatar Portrait asset creation was not successful. ResCode: %d, HttpResCode: %d",
								   (int) CreateAssetResult.GetResultCode(),
								   CreateAssetResult.GetHttpResultCode());

					const NullResult InternalResult(CreateAssetResult);
					Callback(InternalResult);
				}
			};

			const auto UniqueAssetName = AVATAR_PORTRAIT_ASSET_NAME + UserId;
			AssetSystem->CreateAsset(PortraitAvatarAssetColl, UniqueAssetName, nullptr, nullptr, EAssetType::IMAGE, CreateAssetCallback);
		}
		else
		{
			CSP_LOG_FORMAT(LogLevel::Error,
						   "The Avatar Portrait asset collection creation was not successful. ResCode: %d, HttpResCode: %d",
						   (int) AssetCollResult.GetResultCode(),
						   AssetCollResult.GetHttpResultCode());

			const NullResult InternalResult(AssetCollResult);
			Callback(InternalResult);
		}
	};



	const String SpaceThumbnailAssetCollectionName = AVATAR_PORTRAIT_ASSET_COLLECTION_NAME + UserId;

	// don't associate this asset collection with a particular space so that it can be retrieved by guest users that have not joined this space
	AssetSystem->CreateAssetCollection(nullptr,
									   nullptr,
									   SpaceThumbnailAssetCollectionName,
									   nullptr,
									   EAssetCollectionType::DEFAULT,
									   nullptr,
									   CreateAssetCollCallback);
}

void SettingsSystem::GetAvatarPortraitAssetCollection(const csp::common::String& UserId, AssetCollectionsResultCallback Callback)
{
	AssetCollectionsResultCallback GetAssetCollCallback = [=](const AssetCollectionsResult& AssetCollResult)
	{
		if (AssetCollResult.GetResultCode() == csp::services::EResultCode::Failed)
		{
			CSP_LOG_FORMAT(LogLevel::Error,
						   "The Avatar Portrait asset collection retrieval has failed. ResCode: %d, HttpResCode: %d",
						   (int) AssetCollResult.GetResultCode(),
						   AssetCollResult.GetHttpResultCode());
		}

		Callback(AssetCollResult);
	};

	auto AssetSystem														  = SystemsManager::Get().GetAssetSystem();
	csp::common::Array<csp::common::String> AvatarPortraitAssetCollectionName = {AVATAR_PORTRAIT_ASSET_COLLECTION_NAME + UserId};

	AssetSystem->GetAssetCollectionsByCriteria(nullptr,
											   nullptr,
											   EAssetCollectionType::DEFAULT,
											   nullptr,
											   AvatarPortraitAssetCollectionName,
											   nullptr,
											   nullptr,
											   GetAssetCollCallback);
}

void SettingsSystem::GetAvatarPortraitAsset(const AssetCollection& AvatarPortraitAssetCollection, AssetsResultCallback Callback)
{
	AssetsResultCallback ThumbnailAssetCallback = [=](const AssetsResult& AssetsResult)
	{
		if (AssetsResult.GetResultCode() == csp::services::EResultCode::Failed)
		{
			CSP_LOG_FORMAT(LogLevel::Error,
						   "The Avatar Portrait asset retrieval has failed. ResCode: %d, HttpResCode: %d",
						   (int) AssetsResult.GetResultCode(),
						   AssetsResult.GetHttpResultCode());
		}

		Callback(AssetsResult);
	};

	const auto AssetSystem = SystemsManager::Get().GetAssetSystem();
	AssetSystem->GetAssetsInCollection(AvatarPortraitAssetCollection, ThumbnailAssetCallback);
}

void SettingsSystem::RemoveAvatarPortrait(const csp::common::String& UserId, NullResultCallback Callback)
{
	const auto AssetSystem = SystemsManager::Get().GetAssetSystem();

	AssetCollectionsResultCallback PortraitAvatarAssetCollCallback = [=](const AssetCollectionsResult& AssetCollResult)
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
				// user doesnt have a avatar portrait
				const NullResult InternalResult(AssetCollResult);
				Callback(InternalResult);
			}
			else
			{
				const auto& PortraitAvatarAssetCollection = AssetCollections[0];

				AssetsResultCallback PortraitAvatarAssetCallback = [=](const AssetsResult& AssetsResult)
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
										CSP_LOG_FORMAT(LogLevel::Error,
													   "The Portrait Avatar asset collection deletion has failed. ResCode: %d, HttpResCode: %d",
													   (int) DeleteAssetResult.GetResultCode(),
													   DeleteAssetResult.GetHttpResultCode());
									}

									NullResult InternalResult(DeleteAssetCollResult);
									Callback(DeleteAssetCollResult);
								};

								AssetSystem->DeleteAssetCollection(PortraitAvatarAssetCollection, DeleteAssetCollCallback);
							}
							else
							{
								CSP_LOG_FORMAT(LogLevel::Error,
											   "The Portrait Avatar asset deletion was not successful. ResCode: %d, HttpResCode: %d",
											   (int) DeleteAssetResult.GetResultCode(),
											   DeleteAssetResult.GetHttpResultCode());

								NullResult InternalResult(DeleteAssetResult);
								Callback(DeleteAssetResult);
							}
						};

						const auto& PortraitAvatarAsset = AssetsResult.GetAssets()[0];
						AssetSystem->DeleteAsset(PortraitAvatarAssetCollection, PortraitAvatarAsset, DeleteAssetCallback);
					}
					else
					{
						const NullResult InternalResult(AssetsResult);
						Callback(InternalResult);
					}
				};

				GetAvatarPortraitAsset(PortraitAvatarAssetCollection, PortraitAvatarAssetCallback);
			}
		}
		else
		{
			const NullResult InternalResult(AssetCollResult);
			Callback(InternalResult);
		}
	};

	GetAvatarPortraitAssetCollection(UserId, PortraitAvatarAssetCollCallback);
}
} // namespace csp::systems
