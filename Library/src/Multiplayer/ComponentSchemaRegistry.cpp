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

#include "ComponentSchemaRegistry.h"

#include "CSP/Common/Systems/Log/LogSystem.h"
#include "CSP/Multiplayer/ComponentSchema.h"

#include <fmt/format.h>

#include <cassert>
#include <limits>
#include <type_traits>

namespace csp::multiplayer
{

constexpr const auto* BuiltinSchemasJson = R"(
[
  {
    "typeId": 0,
    "reserved": true,
    "description": "Invalid. Sentinel value, not a real component type."
  },
  {
    "typeId": 1,
    "reserved": true,
    "description": "Was Core, removed"
  },
  {
    "typeId": 2,
    "reserved": true,
    "description": "Was UIController, removed"
  },
  {
    "typeId": 3,
    "name": "StaticModel",
    "properties": [
      {
        "key": 0,
        "reserved": true,
        "description": "Was name, superseded by component-level naming"
      },
      {
        "key": 1,
        "name": "externalResourceAssetId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "externalResourceAssetCollectionId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 4,
        "name": "rotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 5,
        "name": "scale",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 6,
        "name": "isVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 7,
        "name": "isARVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 8,
        "name": "",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 9,
        "name": "isShadowCaster",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 10,
        "reserved": true,
        "description": "materialOverrides: Map<String, String>. No equivalent type currently supported."
      },
      {
        "key": 11,
        "name": "isVirtualVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 12,
        "name": "showAsHoldoutInAR",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 13,
        "name": "showAsHoldoutInVirtual",
        "type": "bool",
        "defaultValue": false
      }
    ]
  },
  {
    "typeId": 4,
    "name": "AnimatedModel",
    "properties": [
      {
        "key": 0,
        "reserved": true,
        "description": "Was name, superseded by component-level naming"
      },
      {
        "key": 1,
        "name": "externalResourceAssetId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "externalResourceAssetCollectionId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 4,
        "name": "rotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 5,
        "name": "scale",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 6,
        "name": "isLoopPlayback",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 7,
        "name": "isPlaying",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 8,
        "name": "isVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 9,
        "reserved": true,
        "description": "Reserved slot in AnimatedModelPropertyKeys"
      },
      {
        "key": 10,
        "name": "animationIndex",
        "type": "int",
        "defaultValue": -1
      },
      {
        "key": 11,
        "name": "isARVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 12,
        "name": "",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 13,
        "name": "isShadowCaster",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 14,
        "reserved": true,
        "description": "materialOverrides: Map<String, String>. No equivalent type currently supported."
      },
      {
        "key": 15,
        "name": "isVirtualVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 16,
        "name": "showAsHoldoutInAR",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 17,
        "name": "showAsHoldoutInVirtual",
        "type": "bool",
        "defaultValue": false
      }
    ]
  },
  {
    "typeId": 5,
    "reserved": true,
    "description": "Was MediaSurface, removed"
  },
  {
    "typeId": 6,
    "name": "VideoPlayer",
    "properties": [
      {
        "key": 0,
        "name": "name",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 1,
        "name": "videoAssetId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "videoAssetURL",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "assetCollectionId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 4,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 5,
        "name": "rotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 6,
        "name": "scale",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 7,
        "name": "isStateShared",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 8,
        "name": "isAutoPlay",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 9,
        "name": "isLoopPlayback",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 10,
        "name": "isAutoResize",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 11,
        "name": "attenuationRadius",
        "type": "float",
        "defaultValue": 10.0
      },
      {
        "key": 12,
        "name": "playbackState",
        "type": "int",
        "defaultValue": 0
      },
      {
        "key": 13,
        "name": "currentPlayheadPosition",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 14,
        "name": "timeSincePlay",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 15,
        "name": "videoPlayerSourceType",
        "type": "int",
        "defaultValue": 1
      },
      {
        "key": 16,
        "name": "isVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 17,
        "name": "isARVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 18,
        "reserved": true,
        "description": "Unused key `MeshComponentId` from original `VideoPlayerPropertyKeys` enum"
      },
      {
        "key": 19,
        "name": "isEnabled",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 20,
        "name": "isVirtualVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 21,
        "name": "stereoVideoType",
        "type": "int",
        "defaultValue": 0
      },
      {
        "key": 22,
        "name": "isStereoFlipped",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 23,
        "name": "",
        "type": "float",
        "defaultValue": 1.0
      },
      {
        "key": 24,
        "name": "audioType",
        "type": "int",
        "defaultValue": 1
      }
    ]
  },
  {
    "typeId": 7,
    "reserved": true,
    "description": "Was ImageSequencer, removed"
  },
  {
    "typeId": 8,
    "name": "ExternalLink",
    "properties": [
      {
        "key": 0,
        "name": "name",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 1,
        "name": "linkUrl",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 3,
        "name": "rotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 4,
        "name": "scale",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 5,
        "name": "displayText",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 6,
        "name": "isEnabled",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 7,
        "name": "isVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 8,
        "name": "isARVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 9,
        "name": "isVirtualVisible",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 9,
    "name": "Avatar",
    "properties": [
      {
        "key": 0,
        "name": "avatarId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 1,
        "name": "userId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "state",
        "type": "int",
        "defaultValue": 0
      },
      {
        "key": 3,
        "reserved": true,
        "description": "Was avatarMeshIndex"
      },
      {
        "key": 4,
        "name": "agoraUserId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 5,
        "reserved": true,
        "description": "Was customAvatarUrl"
      },
      {
        "key": 6,
        "name": "isHandIKEnabled",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 7,
        "name": "targetHandIKTargetLocation",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 8,
        "name": "handRotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 9,
        "name": "headRotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 10,
        "name": "walkRunBlendPercentage",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 11,
        "name": "torsoTwistAlpha",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 12,
        "name": "avatarPlayMode",
        "type": "int",
        "defaultValue": 0
      },
      {
        "key": 13,
        "name": "movementDirection",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 14,
        "name": "locomotionModel",
        "type": "int",
        "defaultValue": 0
      },
      {
        "key": 15,
        "name": "isVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 16,
        "name": "isARVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 17,
        "name": "isVirtualVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 18,
        "name": "avatarUrl",
        "type": "string",
        "defaultValue": ""
      }
    ]
  },
  {
    "typeId": 10,
    "name": "Light",
    "properties": [
      {
        "key": 0,
        "reserved": true,
        "description": "Was name, superseded by component-level naming"
      },
      {
        "key": 1,
        "name": "lightType",
        "type": "int",
        "defaultValue": 1
      },
      {
        "key": 2,
        "name": "color",
        "type": "vec3",
        "defaultValue": [
          255.0,
          255.0,
          255.0
        ]
      },
      {
        "key": 3,
        "name": "Intensity",
        "type": "float",
        "defaultValue": 5000.0
      },
      {
        "key": 4,
        "name": "range",
        "type": "float",
        "defaultValue": 1000.0
      },
      {
        "key": 5,
        "name": "innerConeAngle",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 6,
        "name": "outerConeAngle",
        "type": "float",
        "defaultValue": 0.78539816339
      },
      {
        "key": 7,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 8,
        "name": "rotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 9,
        "name": "isVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 10,
        "name": "cookieAssetId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 11,
        "name": "cookieAssetCollectionId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 12,
        "name": "lightCookieType",
        "type": "int",
        "defaultValue": 2
      },
      {
        "key": 13,
        "name": "isARVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 14,
        "name": "",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 15,
        "name": "",
        "type": "int",
        "defaultValue": 0
      },
      {
        "key": 16,
        "name": "isVirtualVisible",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 11,
    "name": "Button",
    "properties": [
      {
        "key": 0,
        "reserved": true,
        "description": "Was name, superseded by component-level naming"
      },
      {
        "key": 1,
        "name": "labelText",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "iconAssetId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "assetCollectionId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 4,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 5,
        "name": "rotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 6,
        "name": "scale",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 7,
        "name": "isVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 8,
        "name": "isEnabled",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 9,
        "name": "isARVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 10,
        "name": "isVirtualVisible",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 12,
    "name": "Image",
    "properties": [
      {
        "key": 0,
        "name": "name",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 1,
        "name": "imageAssetId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "assetCollectionId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 4,
        "name": "rotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 5,
        "name": "scale",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 6,
        "name": "isVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 7,
        "name": "billboardMode",
        "type": "int",
        "defaultValue": 0
      },
      {
        "key": 8,
        "name": "displayMode",
        "type": "int",
        "defaultValue": 1
      },
      {
        "key": 9,
        "name": "isARVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 10,
        "name": "isEmissive",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 11,
        "name": "isVirtualVisible",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 13,
    "name": "",
    "properties": [
      {
        "key": 1,
        "name": "",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "",
        "type": "int",
        "defaultValue": 0
      },
      {
        "key": 3,
        "name": "",
        "type": "int",
        "defaultValue": 1
      }
    ]
  },
  {
    "typeId": 14,
    "name": "Custom",
    "properties": [
      {
        "key": 0,
        "name": "applicationOrigin",
        "type": "string",
        "defaultValue": ""
      }
    ]
  },
  {
    "typeId": 15,
    "name": "Conversation",
    "properties": [
      {
        "key": 0,
        "name": "",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 1,
        "name": "isVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 2,
        "name": "isActive",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 3,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 4,
        "name": "rotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 5,
        "name": "title",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 6,
        "reserved": true,
        "description": "Was Date, deprecated and removed"
      },
      {
        "key": 7,
        "reserved": true,
        "description": "Was NumberOfReplies, deprecated and removed"
      },
      {
        "key": 8,
        "name": "resolved",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 9,
        "name": "conversationCameraPosition",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 10,
        "name": "conversationCameraRotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      }
    ]
  },
  {
    "typeId": 16,
    "name": "Portal",
    "properties": [
      {
        "key": 0,
        "name": "spaceId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 1,
        "reserved": true,
        "description": "Was IsVisible, retained for wire compatibility"
      },
      {
        "key": 2,
        "reserved": true,
        "description": "Was IsActive, retained for wire compatibility"
      },
      {
        "key": 3,
        "reserved": true,
        "description": "Was IsARVisible, retained for wire compatibility"
      },
      {
        "key": 4,
        "name": "isEnabled",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 5,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 6,
        "name": "radius",
        "type": "float",
        "defaultValue": 1.5
      }
    ]
  },
  {
    "typeId": 17,
    "name": "Audio",
    "properties": [
      {
        "key": 0,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 1,
        "name": "playbackState",
        "type": "int",
        "defaultValue": 0
      },
      {
        "key": 2,
        "name": "audioType",
        "type": "int",
        "defaultValue": 0
      },
      {
        "key": 3,
        "name": "audioAssetId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 4,
        "name": "assetCollectionId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 5,
        "name": "attenuationRadius",
        "type": "float",
        "defaultValue": 10.0
      },
      {
        "key": 6,
        "name": "isLoopPlayback",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 7,
        "name": "timeSincePlay",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 8,
        "name": "",
        "type": "float",
        "defaultValue": 1.0
      },
      {
        "key": 9,
        "name": "isEnabled",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 10,
        "name": "",
        "type": "string",
        "defaultValue": ""
      }
    ]
  },
  {
    "typeId": 18,
    "reserved": true,
    "description": "Spline component. Key 0 stores waypoint count, keys 1..N store Vector3 waypoints at runtime. Dynamic keys are not unrepresentable as a static schema."
  },
  {
    "typeId": 19,
    "name": "",
    "properties": [
      {
        "key": 0,
        "name": "",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 1,
        "name": "",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 2,
        "name": "",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 3,
        "name": "",
        "type": "int",
        "defaultValue": 0
      },
      {
        "key": 4,
        "name": "",
        "type": "int",
        "defaultValue": 0
      },
      {
        "key": 5,
        "name": "",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 6,
        "name": "",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 7,
        "name": "",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 8,
        "name": "",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 20,
    "name": "",
    "properties": [
      {
        "key": 0,
        "reserved": true,
        "description": "Was name, superseded by component-level naming"
      },
      {
        "key": 1,
        "name": "",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 4,
        "reserved": true,
        "description": "Was Rotation, reserved as Rotation_NOT_USED in ReflectionPropertyKeys"
      },
      {
        "key": 5,
        "name": "",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 6,
        "name": "",
        "type": "int",
        "defaultValue": 1
      },
      {
        "key": 7,
        "name": "",
        "type": "string",
        "defaultValue": ""
      }
    ]
  },
  {
    "typeId": 21,
    "name": "Fog",
    "properties": [
      {
        "key": 0,
        "name": "fogMode",
        "type": "int",
        "defaultValue": 1
      },
      {
        "key": 1,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 2,
        "name": "rotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 3,
        "name": "scale",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 4,
        "name": "startDistance",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 5,
        "name": "endDistance",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 6,
        "name": "color",
        "type": "vec3",
        "defaultValue": [
          0.8,
          0.9,
          1.0
        ]
      },
      {
        "key": 7,
        "name": "density",
        "type": "float",
        "defaultValue": 0.4
      },
      {
        "key": 8,
        "name": "heightFalloff",
        "type": "float",
        "defaultValue": 0.2
      },
      {
        "key": 9,
        "name": "maxOpacity",
        "type": "float",
        "defaultValue": 1.0
      },
      {
        "key": 10,
        "name": "isVolumetric",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 11,
        "name": "isVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 12,
        "name": "isARVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 13,
        "name": "",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 14,
        "name": "isVirtualVisible",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 22,
    "name": "ECommerce",
    "properties": [
      {
        "key": 0,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 1,
        "name": "productId",
        "type": "string",
        "defaultValue": ""
      }
    ]
  },
  {
    "typeId": 23,
    "name": "FiducialMarker",
    "properties": [
      {
        "key": 0,
        "name": "name",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 1,
        "name": "markerAssetId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "assetCollectionId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 4,
        "name": "rotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 5,
        "name": "scale",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 6,
        "name": "isVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 7,
        "name": "isARVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 8,
        "name": "isVirtualVisible",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 24,
    "name": "GaussianSplat",
    "properties": [
      {
        "key": 0,
        "reserved": true,
        "description": "Was name, superseded by component-level naming"
      },
      {
        "key": 1,
        "name": "externalResourceAssetId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "externalResourceAssetCollectionId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 4,
        "name": "rotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 5,
        "name": "scale",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 6,
        "name": "isVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 7,
        "name": "isARVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 8,
        "name": "",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 9,
        "name": "tint",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 10,
        "name": "isVirtualVisible",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 25,
    "name": "Text",
    "properties": [
      {
        "key": 0,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 1,
        "name": "rotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 2,
        "name": "scale",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 3,
        "name": "text",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 4,
        "name": "textColor",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 5,
        "name": "backgroundColor",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 6,
        "name": "isBackgroundVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 7,
        "name": "width",
        "type": "float",
        "defaultValue": 1.0
      },
      {
        "key": 8,
        "name": "height",
        "type": "float",
        "defaultValue": 1.0
      },
      {
        "key": 9,
        "name": "billboardMode",
        "type": "int",
        "defaultValue": 0
      },
      {
        "key": 10,
        "name": "isVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 11,
        "name": "isARVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 12,
        "name": "isVirtualVisible",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 26,
    "name": "Hotspot",
    "properties": [
      {
        "key": 0,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 1,
        "name": "rotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 2,
        "reserved": true,
        "description": "Was name, superseded by component-level naming"
      },
      {
        "key": 3,
        "name": "isTeleportPoint",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 4,
        "name": "isSpawnPoint",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 5,
        "name": "isVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 6,
        "name": "isARVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 7,
        "name": "isVirtualVisible",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 27,
    "name": "CinematicCamera",
    "properties": [
      {
        "key": 0,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 1,
        "name": "rotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 2,
        "name": "isEnabled",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 3,
        "name": "focalLength",
        "type": "float",
        "defaultValue": 0.035
      },
      {
        "key": 4,
        "name": "aspectRatio",
        "type": "float",
        "defaultValue": 1.778
      },
      {
        "key": 5,
        "name": "sensorSize",
        "type": "vec2",
        "defaultValue": [
          0.036,
          0.024
        ]
      },
      {
        "key": 6,
        "name": "nearClip",
        "type": "float",
        "defaultValue": 0.1
      },
      {
        "key": 7,
        "name": "farClip",
        "type": "float",
        "defaultValue": 20000.0
      },
      {
        "key": 8,
        "name": "iso",
        "type": "float",
        "defaultValue": 400.0
      },
      {
        "key": 9,
        "name": "shutterSpeed",
        "type": "float",
        "defaultValue": 0.0167
      },
      {
        "key": 10,
        "name": "aperture",
        "type": "float",
        "defaultValue": 4.0
      },
      {
        "key": 11,
        "name": "isViewerCamera",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 12,
        "name": "",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 13,
        "name": "focusDistance",
        "type": "float",
        "defaultValue": 5.0
      },
      {
        "key": 14,
        "name": "depthOfFieldEnabled",
        "type": "bool",
        "defaultValue": false
      }
    ]
  },
  {
    "typeId": 28,
    "name": "ScreenSharing",
    "properties": [
      {
        "key": 0,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 1,
        "name": "rotation",
        "type": "vec4",
        "defaultValue": [
          0.0,
          0.0,
          0.0,
          1.0
        ]
      },
      {
        "key": 2,
        "name": "scale",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 3,
        "name": "isVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 4,
        "name": "isARVisible",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 5,
        "name": "isShadowCaster",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 6,
        "name": "userId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 7,
        "name": "defaultImageCollectionId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 8,
        "name": "defaultImageAssetId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 9,
        "name": "attenuationRadius",
        "type": "float",
        "defaultValue": 10.0
      },
      {
        "key": 10,
        "name": "isVirtualVisible",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 29,
    "name": "AIChatbot",
    "properties": [
      {
        "key": 0,
        "name": "position",
        "type": "vec3",
        "defaultValue": [
          0.0,
          0.0,
          0.0
        ]
      },
      {
        "key": 1,
        "name": "voice",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "guardrailAssetCollectionId",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "visualState",
        "type": "int",
        "defaultValue": 0
      }
    ]
  },
  {
    "typeId": 56,
    "reserved": true,
    "description": "An implementation detail of component deletion on the wire. Not a real component type."
  }
]
)";

static csp::common::Array<ComponentSchema> ParseBuiltinSchemas()
{
    auto Docs = csp::common::List<csp::common::String> {};
    Docs.Append(BuiltinSchemasJson);
    const auto Schemas = ComponentSchemasFromJson(Docs);
    assert(Schemas.Size() == 31 && "Not all built-in schemas could be parsed from JSON");
    return Schemas;
}

const auto AllSchemas = ParseBuiltinSchemas();

const ComponentSchema& GetStaticModelSchema() { return AllSchemas[3]; }
const ComponentSchema& GetAnimatedModelSchema() { return AllSchemas[4]; }
const ComponentSchema& GetVideoPlayerSchema() { return AllSchemas[6]; }
const ComponentSchema& GetExternalLinkSchema() { return AllSchemas[8]; }
const ComponentSchema& GetAvatarSchema() { return AllSchemas[9]; }
const ComponentSchema& GetLightSchema() { return AllSchemas[10]; }
const ComponentSchema& GetButtonSchema() { return AllSchemas[11]; }
const ComponentSchema& GetImageSchema() { return AllSchemas[12]; }
const ComponentSchema& GetScriptSchema() { return AllSchemas[13]; }
const ComponentSchema& GetCustomSchema() { return AllSchemas[14]; }
const ComponentSchema& GetConversationSchema() { return AllSchemas[15]; }
const ComponentSchema& GetPortalSchema() { return AllSchemas[16]; }
const ComponentSchema& GetAudioSchema() { return AllSchemas[17]; }
const ComponentSchema& GetSplineSchema() { return AllSchemas[18]; }
const ComponentSchema& GetCollisionSchema() { return AllSchemas[19]; }
const ComponentSchema& GetReflectionSchema() { return AllSchemas[20]; }
const ComponentSchema& GetFogSchema() { return AllSchemas[21]; }
const ComponentSchema& GetECommerceSchema() { return AllSchemas[22]; }
const ComponentSchema& GetFiducialMarkerSchema() { return AllSchemas[23]; }
const ComponentSchema& GetGaussianSplatSchema() { return AllSchemas[24]; }
const ComponentSchema& GetTextSchema() { return AllSchemas[25]; }
const ComponentSchema& GetHotspotSchema() { return AllSchemas[26]; }
const ComponentSchema& GetCinematicCameraSchema() { return AllSchemas[27]; }
const ComponentSchema& GetScreenSharingSchema() { return AllSchemas[28]; }
const ComponentSchema& GetAIChatbotSchema() { return AllSchemas[29]; }

ComponentSchemaRegistryImpl::ComponentSchemaRegistryImpl(
    csp::common::LogSystem& LogSystem, const csp::common::Array<ComponentSchema>& AdditionalComponents)
{
    const auto AddSchema = [this, &LogSystem](const ComponentSchema& Schema)
    {
        const auto Result = SchemaMap.insert_or_assign(Schema.TypeId, Schema);
        const auto DidReplace = !Result.second;

        if (DidReplace)
        {
            LogSystem.LogMsg(
                csp::common::LogLevel::Warning, fmt::format("Replaced a previously registered schema for TypeId: {}", Schema.TypeId).c_str());
        }
    };

    for (const auto& Schema : AllSchemas)
    {
        AddSchema(Schema);
    }

    for (const auto& Schema : AdditionalComponents)
    {
        if (const auto It = SchemaMap.find(Schema.TypeId); It != SchemaMap.end() && !IsCompatible(It->second, Schema, &LogSystem))
        {
            LogSystem.LogMsg(csp::common::LogLevel::Warning,
                fmt::format("Injected schema for TypeId {} is not compatible with the built-in schema and will be ignored.", Schema.TypeId).c_str());
            continue;
        }

        AddSchema(Schema);
    }
}

csp::common::Array<ComponentSchema> ComponentSchemaRegistryImpl::GetAll() const
{
    csp::common::Array<ComponentSchema> Result(SchemaMap.size());
    size_t Index = 0;

    for (const auto& [TypeId, Schema] : SchemaMap)
    {
        Result[Index++] = Schema;
    }

    return Result;
}

const ComponentSchema* ComponentSchemaRegistryImpl::Find(uint64_t TypeId) const
{
    const auto It = SchemaMap.find(TypeId);
    return It != SchemaMap.end() ? &It->second : nullptr;
}

std::optional<ComponentType> ToComponentType(uint64_t TypeId)
{
    using Underlying = std::underlying_type_t<ComponentType>;
    static_assert(std::is_unsigned_v<Underlying>);

    if (TypeId > static_cast<uint64_t>(std::numeric_limits<Underlying>::max()))
    {
        return std::nullopt;
    }

    switch (static_cast<ComponentType>(TypeId))
    {
    case ComponentType::Invalid:
    case ComponentType::Core:
    case ComponentType::UIController_DEPRECATED:
    case ComponentType::StaticModel:
    case ComponentType::AnimatedModel:
    case ComponentType::MediaSurface_DEPRECATED:
    case ComponentType::VideoPlayer:
    case ComponentType::ImageSequencer_DEPRECATED:
    case ComponentType::ExternalLink:
    case ComponentType::AvatarData:
    case ComponentType::Light:
    case ComponentType::Button:
    case ComponentType::Image:
    case ComponentType::ScriptData:
    case ComponentType::Custom:
    case ComponentType::Conversation:
    case ComponentType::Portal:
    case ComponentType::Audio:
    case ComponentType::Spline:
    case ComponentType::Collision:
    case ComponentType::Reflection:
    case ComponentType::Fog:
    case ComponentType::ECommerce:
    case ComponentType::FiducialMarker:
    case ComponentType::GaussianSplat:
    case ComponentType::Text:
    case ComponentType::Hotspot:
    case ComponentType::CinematicCamera:
    case ComponentType::ScreenSharing:
    case ComponentType::AIChatbot:
    case ComponentType::Delete:
        return static_cast<ComponentType>(TypeId);
    }

    return std::nullopt;
}

bool IsLegacyComponentTypeId(uint64_t TypeId) { return ToComponentType(TypeId).has_value(); }

} // namespace csp::multiplayer
