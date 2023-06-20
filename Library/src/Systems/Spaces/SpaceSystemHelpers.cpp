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
#include "Systems/Spaces/SpaceSystemHelpers.h"

#include "Debug/Logging.h"


namespace csp::systems
{

namespace SpaceSystemHelpers
{

String GetSpaceMetadataAssetCollectionName(const String& SpaceId)
{
	return String(SPACE_METADATA_ASSET_COLLECTION_NAME_PREFIX) + SpaceId;
}

String GetSpaceIdFromMetadataAssetCollectionName(const String& MetadataAssetCollectionName)
{
	std::string ReturnValue = MetadataAssetCollectionName.c_str();
	ReturnValue.erase(0, strlen(SPACE_METADATA_ASSET_COLLECTION_NAME_PREFIX));
	return String(ReturnValue.c_str());
}

Map<String, String> ConvertSpaceMetadataToAssetCollectionMetadata(const String& Metadata)
{
	Map<String, String> MetadataMap;
	MetadataMap[SPACE_METADATA_KEY] = Metadata;

	return MetadataMap;
}

String GetSpaceThumbnailAssetCollectionName(const String& SpaceId)
{
	return String(SPACE_THUMBNAIL_ASSET_COLLECTION_NAME_PREFIX) + SpaceId;
}

String GetUniqueSpaceThumbnailAssetName(const String& SpaceId)
{
	return csp::common::String(SPACE_THUMBNAIL_ASSET_NAME_PREFIX) + SpaceId;
}

String GetUniqueAvatarThumbnailAssetName(const String& Extension)
{
	return csp::common::String(AVATAR_THUMBNAIL_ASSET_NAME_PREFIX) + Extension;
}

String GetAssetFileExtension(const String& MimeType)
{
	if (MimeType == "image/png")
		return ".png";
	else if (MimeType == "image/jpeg")
		return ".jpeg";
	else if (MimeType == "image/gif")
		return ".gif";
	else if (MimeType == "image/apng")
		return ".apng";
	else if (MimeType == "image/avif")
		return ".avif";
	else if (MimeType == "image/svg+xml")
		return ".svg";
	else if (MimeType == "image/webp")
		return ".webp";
	else
	{
		FOUNDATION_LOG_MSG(LogLevel::Error, "Mimetype File Extension Not Supported");
		return ".buffer";
	}
}

} // namespace SpaceSystemHelpers

} // namespace csp::systems
