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

#include "CSP/Common/Map.h"
#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"
#include "CSP/Multiplayer/Conversation/Conversation.h"
#include "CSP/Multiplayer/SpaceTransform.h"

using namespace csp::common;

namespace csp::systems
{

class AssetCollection;

}

namespace csp::multiplayer
{

class MessageInfo;
class ConversationInfo;

class ConversationSystemHelpers
{

public:
    static String GetUniqueConversationContainerAssetCollectionName(const String& SpaceId, const String& CreatorUserId);
    static String GetUniqueMessageAssetCollectionName(const String& SpaceId, const String& CreatorUserId);
    static Map<String, String> GenerateMessageAssetCollectionMetadata(const MessageInfo& MessageData);
    static Map<String, String> GenerateConversationAssetCollectionMetadata(const ConversationInfo& ConversationData);
    static ConversationInfo GetConvosationInfoFromConvosationAssetCollection(const csp::systems::AssetCollection& ConversationAssetCollection);
    static MessageInfo GetMessageInfoFromMessageAssetCollection(const csp::systems::AssetCollection& MessageAssetCollection);
    static String BoolToString(const bool value);
    static bool StringToBool(const String& value);
    static String Vector3ToString(const Vector3& value);
    static Vector3 StringToVector3(const String& value);
    static String Vector4ToString(const Vector4& value);
    static Vector4 StringToVector4(const String& value);
    static SpaceTransform StringToSpaceTransform(const String& value);
    static String SpaceTransformToString(const SpaceTransform& value);

private:
    static String GetUniqueAssetCollectionSuffix(const String& SpaceId, const String& CreatorUserId);

}; // ConversationSystemHelpers

} // namespace csp::multiplayer
