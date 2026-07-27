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
    "description": "Adds static 3D models to a SpaceEntity. It displays non-animated objects, such as furniture, buildings, or decorative items within a space. The static model defines the visual appearance but has no animations or dynamic behaviors.",
    "properties": [
      {
        "key": 0,
        "reserved": true,
        "description": "Was name, superseded by component-level naming"
      },
      {
        "key": 1,
        "deprecated": true,
        "name": "externalResourceAssetId",
        "description": "Due to the introduction of LODs it doesn't make sense to set a specific asset anymore",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "externalResourceAssetCollectionId",
        "description": "To retrieve this component's static asset, both the Asset ID and the Asset Collection ID are required.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Scale of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Whether the component is visible when in default mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 7,
        "name": "isARVisible",
        "description": "Whether the component is visible when in AR mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 8,
        "name": "thirdPartyComponentRef",
        "description": "Third party component reference.",
        "type": "string",
        "defaultValue": "",
        "scripting": false
      },
      {
        "key": 9,
        "name": "isShadowCaster",
        "description": "Whether the mesh casts shadows.",
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
        "description": "Whether the component is visible when in Virtual mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 12,
        "name": "showAsHoldoutInAR",
        "description": "Whether the component is shown as holdout when in AR mode.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 13,
        "name": "showAsHoldoutInVirtual",
        "description": "Whether the component is shown as holdout when in Virtual mode.",
        "type": "bool",
        "defaultValue": false
      }
    ]
  },
  {
    "typeId": 4,
    "name": "AnimatedModel",
    "description": "Adds animated skeletal meshes to a SpaceEntity. These are used for objects that need to move or act within the space, such as characters or animated props. This component makes it possible to play, pause, or loop animations.",
    "properties": [
      {
        "key": 0,
        "reserved": true,
        "description": "Was name, superseded by component-level naming"
      },
      {
        "key": 1,
        "deprecated": true,
        "name": "externalResourceAssetId",
        "description": "Due to the introduction of LODs it doesn't make sense to set a specific asset anymore",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "externalResourceAssetCollectionId",
        "description": "To retrieve this component's animated asset, both the Asset ID and the Asset Collection ID are required.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Scale of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Whether the animation of this animated model is in loop.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 7,
        "name": "isPlaying",
        "description": "Whether the animation of this animated model is playing.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 8,
        "name": "isVisible",
        "description": "Whether the component is visible when in default mode.",
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
        "description": "Index of the currently active animation.",
        "type": "int",
        "defaultValue": -1
      },
      {
        "key": 11,
        "name": "isARVisible",
        "description": "Whether the component is visible when in AR mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 12,
        "name": "thirdPartyComponentRef",
        "description": "Third party component reference.",
        "type": "string",
        "defaultValue": "",
        "scripting": false
      },
      {
        "key": 13,
        "name": "isShadowCaster",
        "description": "Whether the mesh casts shadows.",
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
        "description": "Whether the component is visible when in Virtual mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 16,
        "name": "showAsHoldoutInAR",
        "description": "Whether the component is shown as holdout when in AR mode.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 17,
        "name": "showAsHoldoutInVirtual",
        "description": "Whether the component is shown as holdout when in Virtual mode.",
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
    "description": "Enables the playback of video content within the space. You can use it to stream videos from a URL or play videos stored as assets in CSP, allowing users to watch videos directly within the virtual environment.",
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
        "description": "ID of the video asset associated with this video player.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "videoAssetURL",
        "description": "URL of the video asset associated with this video player.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "assetCollectionId",
        "description": "To retrieve this component's video asset, both the Asset ID and the Asset Collection ID are required.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 4,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Scale of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Whether playback state is shared with other users through replication.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 8,
        "name": "isAutoPlay",
        "description": "Whether the video plays automatically on load.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 9,
        "name": "isLoopPlayback",
        "description": "Whether the video loops (i.e. starts over on end).",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 10,
        "name": "isAutoResize",
        "description": "Whether the video auto-resizes if its frame has different dimensions.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 11,
        "name": "attenuationRadius",
        "description": "Attenuation for the audio when a spatial audio type. The radius is the minimum distance between the origin of this audio component and the position of the player, from within which the player can start hearing the spatial audio in range. The radius is expressed in meters.",
        "type": "float",
        "defaultValue": 10.0
      },
      {
        "key": 12,
        "name": "playbackState",
        "description": "Playback state of the video of this component.",
        "type": "int",
        "defaultValue": 0,
        "options": [
          {
            "name": "Reset",
            "value": 0
          },
          {
            "name": "Pause",
            "value": 1
          },
          {
            "name": "Play",
            "value": 2
          }
        ]
      },
      {
        "key": 13,
        "name": "currentPlayheadPosition",
        "description": "Current playhead position of the played video.",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 14,
        "name": "timeSincePlay",
        "description": "Time in Unix timestamp format that identifies the moment when the video started to play.",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 15,
        "name": "videoPlayerSourceType",
        "description": "Type of source the video of this component uses.",
        "type": "int",
        "defaultValue": 1,
        "options": [
          {
            "name": "URLSource",
            "value": 0
          },
          {
            "name": "AssetSource",
            "value": 1
          },
          {
            "name": "WowzaStreamSource",
            "value": 2
          }
        ]
      },
      {
        "key": 16,
        "name": "isVisible",
        "description": "Whether the component is visible when in default mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 17,
        "name": "isARVisible",
        "description": "Whether the component is visible when in AR mode.",
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
        "description": "Whether the component is enabled.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 20,
        "name": "isVirtualVisible",
        "description": "Whether the component is visible when in Virtual mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 21,
        "name": "stereoVideoType",
        "description": "Type of stereo the video of this component uses.",
        "type": "int",
        "defaultValue": 0,
        "options": [
          {
            "name": "None",
            "value": 0
          },
          {
            "name": "SideBySide",
            "value": 1
          },
          {
            "name": "TopBottom",
            "value": 2
          }
        ]
      },
      {
        "key": 22,
        "name": "isStereoFlipped",
        "description": "Whether the stereo video left and right are flipped.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 23,
        "name": "volume",
        "description": "Volume of the audio in a ratio between 0 and 1. Volume 1 represents the full volume of the audio clip of this component.",
        "type": "float",
        "defaultValue": 1.0,
        "range": {
          "min": 0.0,
          "max": 1.0
        }
      },
      {
        "key": 24,
        "name": "audioType",
        "description": "Type of the audio of this audio component.",
        "type": "int",
        "defaultValue": 1,
        "options": [
          {
            "name": "Global",
            "value": 0
          },
          {
            "name": "Spatial",
            "value": 1
          }
        ]
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
    "description": "Used to handle external URLs that can be opened from within a space.",
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
        "description": "URL address to which this external link component redirects the user on trigger.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Scale of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Text that will be displayed by the component as hyperlink to the URL it redirects to.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 6,
        "name": "isEnabled",
        "description": "Whether the component is enabled.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 7,
        "name": "isVisible",
        "description": "Whether the component is visible when in default mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 8,
        "name": "isARVisible",
        "description": "Whether the component is visible when in AR mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 9,
        "name": "isVirtualVisible",
        "description": "Whether the component is visible when in Virtual mode.",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 9,
    "name": "Avatar",
    "description": "Data representation of an AvatarSpaceComponent.",
    "properties": [
      {
        "key": 0,
        "name": "avatarId",
        "description": "Used for selecting a specific avatar depending on the user's preferences.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 1,
        "name": "userId",
        "description": "ID of the user that controls this avatar.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "state",
        "description": "State of the current avatar.",
        "type": "int",
        "defaultValue": 0,
        "options": [
          {
            "name": "Idle",
            "value": 0
          },
          {
            "name": "Walking",
            "value": 1
          },
          {
            "name": "Running",
            "value": 2
          },
          {
            "name": "Flying",
            "value": 3
          },
          {
            "name": "Jumping",
            "value": 4
          },
          {
            "name": "Falling",
            "value": 5
          }
        ]
      },
      {
        "key": 3,
        "reserved": true,
        "description": "Was avatarMeshIndex"
      },
      {
        "key": 4,
        "name": "agoraUserId",
        "description": "ID of the Agora user bounded to this avatar. When using voice chat, an Agora user is associated with a specific avatar component, so that it is possible to associate the person speaking via the Agora voice chat through the relative avatar.",
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
        "description": "Whether the Hands Inverse Kinematics (IK) are enabled for this avatar. Intended for use in VR or with virtual hands controllers.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 7,
        "name": "targetHandIKTargetLocation",
        "description": "Location of the target used for the hands IK. Used in combination with hand IK if enabled.",
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
        "description": "Rotation of the avatar hand.",
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
        "description": "Rotation of the avatar head.",
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
        "description": "Blending between walk and run states expressed in percentage. When 0 the avatar is fully walking, when 100 the avatar is fully running.",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 11,
        "name": "torsoTwistAlpha",
        "description": "Angle to use to twist the avatar's torso. Positive value if the torso is turning right, negative if avatar is turning left.",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 12,
        "name": "avatarPlayMode",
        "description": "Play mode used by this avatar.",
        "type": "int",
        "defaultValue": 0,
        "options": [
          {
            "name": "Default",
            "value": 0
          },
          {
            "name": "AR",
            "value": 1
          },
          {
            "name": "VR",
            "value": 2
          },
          {
            "name": "Creator",
            "value": 3
          }
        ]
      },
      {
        "key": 13,
        "name": "movementDirection",
        "description": "A vector that represents the movement direction of the avatar.",
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
        "description": "Specifies which locomotion model this avatar component is using.",
        "type": "int",
        "defaultValue": 0,
        "options": [
          {
            "name": "Grounded",
            "value": 0
          },
          {
            "name": "FreeCamera",
            "value": 1
          }
        ]
      },
      {
        "key": 15,
        "name": "isVisible",
        "description": "Whether the component is visible when in default mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 16,
        "name": "isARVisible",
        "description": "Whether the component is visible when in AR mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 17,
        "name": "isVirtualVisible",
        "description": "Whether the component is visible when in Virtual mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 18,
        "name": "avatarUrl",
        "description": "URL of a mesh for this avatar. This is intended for use with external avatar managers, such as ReadyPlayerMe.",
        "type": "string",
        "defaultValue": ""
      }
    ]
  },
  {
    "typeId": 10,
    "name": "Light",
    "description": "Adds various types of lighting to a SpaceEntity, such as directional, point, or spotlights. This component is essential for creating realistic lighting effects and controlling how objects are illuminated within the space.",
    "properties": [
      {
        "key": 0,
        "reserved": true,
        "description": "Was name, superseded by component-level naming"
      },
      {
        "key": 1,
        "name": "lightType",
        "description": "Type of light of this light component.",
        "type": "int",
        "defaultValue": 1,
        "options": [
          {
            "name": "Directional",
            "value": 0
          },
          {
            "name": "Point",
            "value": 1
          },
          {
            "name": "Spot",
            "value": 2
          }
        ]
      },
      {
        "key": 2,
        "name": "color",
        "description": "Color of the light of this component.",
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
        "description": "Intensity of the light of this component.",
        "type": "float",
        "defaultValue": 5000.0
      },
      {
        "key": 4,
        "name": "range",
        "description": "Range within which the light of this component affects the surrounding 3D scene.",
        "type": "float",
        "defaultValue": 1000.0
      },
      {
        "key": 5,
        "name": "innerConeAngle",
        "description": "Angle of the inner cone in a spotlight.",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 6,
        "name": "outerConeAngle",
        "description": "Angle of the outer cone in a spotlight.",
        "type": "float",
        "defaultValue": 0.78539816339
      },
      {
        "key": 7,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Whether the component is visible when in default mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 10,
        "name": "cookieAssetId",
        "description": "ID of the asset used for the light cookie of this light component.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 11,
        "name": "cookieAssetCollectionId",
        "description": "ID of the asset collection used for the light cookie of this light component.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 12,
        "name": "lightCookieType",
        "description": "Type of the light cookie used by this light component.",
        "type": "int",
        "defaultValue": 2,
        "options": [
          {
            "name": "ImageCookie",
            "value": 0
          },
          {
            "name": "VideoCookie",
            "value": 1
          },
          {
            "name": "NoCookie",
            "value": 2
          }
        ]
      },
      {
        "key": 13,
        "name": "isARVisible",
        "description": "Whether the component is visible when in AR mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 14,
        "name": "thirdPartyComponentRef",
        "description": "Third party component reference.",
        "type": "string",
        "defaultValue": "",
        "scripting": false
      },
      {
        "key": 15,
        "name": "lightShadowType",
        "description": "Type of light shadow of this light component.",
        "type": "int",
        "defaultValue": 0,
        "options": [
          {
            "name": "None",
            "value": 0
          },
          {
            "name": "Static",
            "value": 1
          },
          {
            "name": "Realtime",
            "value": 2
          }
        ]
      },
      {
        "key": 16,
        "name": "isVirtualVisible",
        "description": "Whether the component is visible when in Virtual mode.",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 11,
    "name": "Button",
    "description": "Add a clickable button to your space. Button click events can be responded to via scripts.",
    "properties": [
      {
        "key": 0,
        "reserved": true,
        "description": "Was name, superseded by component-level naming"
      },
      {
        "key": 1,
        "name": "labelText",
        "description": "Text of the label of this button.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "iconAssetId",
        "description": "ID of the icon asset associated with the button of this component. This is used to show a specific icon on the button by ID.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "assetCollectionId",
        "description": "To retrieve this component's button asset, both the Asset ID and the Asset Collection ID are required.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 4,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Scale of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Whether the component is visible when in default mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 8,
        "name": "isEnabled",
        "description": "Whether the component is enabled.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 9,
        "name": "isARVisible",
        "description": "Whether the component is visible when in AR mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 10,
        "name": "isVirtualVisible",
        "description": "Whether the component is visible when in Virtual mode.",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 12,
    "name": "Image",
    "description": "Add an image to your space.",
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
        "description": "ID of the image asset this image component refers to.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "assetCollectionId",
        "description": "To retrieve this component's image asset, both the Asset ID and the Asset Collection ID are required.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Scale of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Whether the component is visible when in default mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 7,
        "name": "billboardMode",
        "description": "Billboard mode used by this image component.",
        "type": "int",
        "defaultValue": 0,
        "options": [
          {
            "name": "Off",
            "value": 0
          },
          {
            "name": "Billboard",
            "value": 1
          },
          {
            "name": "YawLockedBillboard",
            "value": 2
          }
        ]
      },
      {
        "key": 8,
        "name": "displayMode",
        "description": "Display mode used by this image component.",
        "type": "int",
        "defaultValue": 1,
        "options": [
          {
            "name": "SingleSided",
            "value": 0
          },
          {
            "name": "DoubleSided",
            "value": 1
          },
          {
            "name": "DoubleSidedReversed",
            "value": 2
          }
        ]
      },
      {
        "key": 9,
        "name": "isARVisible",
        "description": "Whether the component is visible when in AR mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 10,
        "name": "isEmissive",
        "description": "Whether the image of this image component is emissive.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 11,
        "name": "isVirtualVisible",
        "description": "Whether the component is visible when in Virtual mode.",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 13,
    "name": "Script",
    "description": "Enables custom behavior through scripting. This component allows developers to author scripts that control how entities and components behave based on specific conditions or user actions. Scripts can modify entity properties, trigger events, or respond to user inputs.",
    "scripting": false,
    "properties": [
      {
        "key": 1,
        "name": "scriptSource",
        "description": "Source of the script of this script component.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "ownerId",
        "description": "ID of the owner of this script component.",
        "type": "int",
        "defaultValue": 0
      },
      {
        "key": 3,
        "name": "scriptScope",
        "description": "Scope within which this script operates.",
        "type": "int",
        "defaultValue": 1,
        "options": [
          {
            "name": "Local",
            "value": 0
          },
          {
            "name": "Owner",
            "value": 1
          }
        ]
      }
    ]
  },
  {
    "typeId": 14,
    "name": "Custom",
    "description": "Can be used to prototype new component types or to support the replication of custom data. This still requires using the CustomSpaceComponent class, as user-defined properties are stored at dynamic hash-derived ids that aren't statically representable in a schema.",
    "properties": [
      {
        "key": 0,
        "name": "applicationOrigin",
        "description": "The application origin for which this component has been generated.",
        "type": "string",
        "defaultValue": ""
      }
    ]
  },
  {
    "typeId": 15,
    "name": "Conversation",
    "description": "Add a conversation with comment thread to your space. These conversations have a spatial representation.",
    "properties": [
      {
        "key": 0,
        "name": "conversationId",
        "type": "string",
        "defaultValue": "",
        "scripting": false
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
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Title of the conversation.",
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
        "description": "Resolved value of the conversation.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 9,
        "name": "conversationCameraPosition",
        "description": "Camera position of the conversation.",
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
        "description": "Camera rotation of the conversation.",
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
    "description": "Add a portal to your space that can be used to teleport users to another configured Space. To ensure the connection to the new space is successful, clients should: 1. Store the new space Id by calling PortalSpaceComponent::GetSpaceId(). 2. Exit the current space via the space system. 3. Enter the new one (also via the space system).",
    "properties": [
      {
        "key": 0,
        "name": "spaceId",
        "description": "Space ID that this portal points to. When the user uses the portal, it should be able to leave the current space and enter the one identified by this property.",
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
        "description": "Whether the component is enabled.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 5,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Radius of this portal.",
        "type": "float",
        "defaultValue": 1.5
      }
    ]
  },
  {
    "typeId": 17,
    "name": "Audio",
    "description": "Adds spatial audio to a SpaceEntity. This component creates immersive soundscapes by playing audio that reacts to the user's position in the space. Whether it's background music, sound effects, or voiceovers, the AudioSpaceComponent makes sound more engaging by positioning it in 3D space.",
    "properties": [
      {
        "key": 0,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Current playback state of the audio of this audio component.",
        "type": "int",
        "defaultValue": 0,
        "options": [
          {
            "name": "Reset",
            "value": 0
          },
          {
            "name": "Pause",
            "value": 1
          },
          {
            "name": "Play",
            "value": 2
          }
        ]
      },
      {
        "key": 2,
        "name": "audioType",
        "description": "Type of the audio of this audio component.",
        "type": "int",
        "defaultValue": 0,
        "options": [
          {
            "name": "Global",
            "value": 0
          },
          {
            "name": "Spatial",
            "value": 1
          }
        ]
      },
      {
        "key": 3,
        "name": "audioAssetId",
        "description": "Asset ID for this audio asset.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 4,
        "name": "assetCollectionId",
        "description": "To retrieve this component's audio asset, both the Asset ID and the Asset Collection ID are required.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 5,
        "name": "attenuationRadius",
        "description": "Attenuation for the audio when a spatial audio type. The radius is the minimum distance between the origin of this audio component and the position of the player, from within which the player can start hearing the spatial audio in range. The radius is expressed in meters.",
        "type": "float",
        "defaultValue": 10.0
      },
      {
        "key": 6,
        "name": "isLoopPlayback",
        "description": "Whether the audio playback is looping. True if the audio loops (i.e. starts from the beginning when ended), false otherwise.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 7,
        "name": "timeSincePlay",
        "description": "Timestamp recorded from the moment when the audio clip started playing, in Unix timestamp format.",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 8,
        "name": "volume",
        "description": "Volume of the audio in a ratio between 0 and 1. Volume 1 represents the full volume of the audio clip of this component.",
        "type": "float",
        "defaultValue": 1.0,
        "range": {
          "min": 0.0,
          "max": 1.0
        }
      },
      {
        "key": 9,
        "name": "isEnabled",
        "description": "Whether the component is enabled.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 10,
        "name": "thirdPartyComponentRef",
        "description": "Third party component reference.",
        "type": "string",
        "defaultValue": "",
        "scripting": false
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
    "name": "Collision",
    "description": "Add box, mesh, capsule and sphere colliders to objects in your Space. These colliders can act as triggers, which can be used in conjunction with Scripts to drive behavior.",
    "scripting": false,
    "properties": [
      {
        "key": 0,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Scale of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 3,
        "name": "collisionShape",
        "description": "Collision shape used by this collision component.",
        "type": "int",
        "defaultValue": 0,
        "options": [
          {
            "name": "Box",
            "value": 0
          },
          {
            "name": "Mesh",
            "value": 1
          },
          {
            "name": "Capsule",
            "value": 2
          },
          {
            "name": "Sphere",
            "value": 3
          }
        ]
      },
      {
        "key": 4,
        "name": "collisionMode",
        "description": "Collision mode used by this collision component.",
        "type": "int",
        "defaultValue": 0,
        "options": [
          {
            "name": "Collision",
            "value": 0
          },
          {
            "name": "Trigger",
            "value": 1
          }
        ]
      },
      {
        "key": 5,
        "name": "collisionAssetId",
        "description": "ID of the collision asset used by this collision component.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 6,
        "name": "assetCollectionId",
        "description": "To retrieve this component's collision asset, both the Asset ID and the Asset Collection ID are required.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 7,
        "name": "thirdPartyComponentRef",
        "description": "Third party component reference.",
        "type": "string",
        "defaultValue": "",
        "scripting": false
      },
      {
        "key": 8,
        "name": "isEnabled",
        "description": "Whether the component is enabled.",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 20,
    "name": "Reflection",
    "description": "Add sphere and box reflection captures to your Space which can be used by objects with reflective materials.",
    "scripting": false,
    "properties": [
      {
        "key": 0,
        "reserved": true,
        "description": "Was name, superseded by component-level naming"
      },
      {
        "key": 1,
        "name": "reflectionAssetId",
        "description": "Asset Id for the Reflection texture asset.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "assetCollectionId",
        "description": "To retrieve this component's reflection asset, both the Asset ID and the Asset Collection ID are required.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "name": "scale",
        "description": "Scale of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
        "type": "vec3",
        "defaultValue": [
          1.0,
          1.0,
          1.0
        ]
      },
      {
        "key": 6,
        "name": "reflectionShape",
        "description": "UnitBox: Projects a texture in a planar fashion from all six directions (like an inward facing cube). UnitSphere: Warps the texture into a spherical shape and projects it onto a surface.",
        "type": "int",
        "defaultValue": 1,
        "options": [
          {
            "name": "UnitSphere",
            "value": 0
          },
          {
            "name": "UnitBox",
            "value": 1
          }
        ]
      },
      {
        "key": 7,
        "name": "thirdPartyComponentRef",
        "description": "Third party component reference.",
        "type": "string",
        "defaultValue": "",
        "scripting": false
      }
    ]
  },
  {
    "typeId": 21,
    "name": "Fog",
    "description": "Add a depth-based fog volume to your space.",
    "properties": [
      {
        "key": 0,
        "name": "fogMode",
        "description": "Type of fog currently used by this fog component.",
        "type": "int",
        "defaultValue": 1,
        "options": [
          {
            "name": "Linear",
            "value": 0
          },
          {
            "name": "Exponential",
            "value": 1
          },
          {
            "name": "Exponential2",
            "value": 2
          }
        ]
      },
      {
        "key": 1,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Scale of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Start distance. Distance from camera that the fog will start. 0 = this property has no effect.",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 5,
        "name": "endDistance",
        "description": "End distance. Objects passed this distance will not be affected by fog. 0 = this property has no effect.",
        "type": "float",
        "defaultValue": 0.0
      },
      {
        "key": 6,
        "name": "color",
        "description": "Fog color.",
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
        "description": "Global density factor.",
        "type": "float",
        "defaultValue": 0.4
      },
      {
        "key": 8,
        "name": "heightFalloff",
        "description": "Height density factor. Controls how the density increases and height decreases. Smaller values make the visible transition larger.",
        "type": "float",
        "defaultValue": 0.2
      },
      {
        "key": 9,
        "name": "maxOpacity",
        "description": "Maximum opacity of the fog. 1 = fog becomes fully opaque at a distance and replaces the scene colour completely. 0 = fog colour will have no impact.",
        "type": "float",
        "defaultValue": 1.0
      },
      {
        "key": 10,
        "name": "isVolumetric",
        "description": "Whether fog is volumetric.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 11,
        "name": "isVisible",
        "description": "Whether the component is visible when in default mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 12,
        "name": "isARVisible",
        "description": "Whether the component is visible when in AR mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 13,
        "name": "thirdPartyComponentRef",
        "description": "Third party component reference.",
        "type": "string",
        "defaultValue": "",
        "scripting": false
      },
      {
        "key": 14,
        "name": "isVirtualVisible",
        "description": "Whether the component is visible when in Virtual mode.",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 22,
    "name": "ECommerce",
    "description": "Can be used alongside CSP's Stripe integration to add e-commerce to your space. This component is used to represent physical objects that can be purchased as virtual items in the environment.",
    "properties": [
      {
        "key": 0,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Product ID associated with the ECommerce component.",
        "type": "string",
        "defaultValue": ""
      }
    ]
  },
  {
    "typeId": 23,
    "name": "FiducialMarker",
    "description": "As an alternative to cloud-based anchors, fiducial markers can be used to anchor your space to a physical location.",
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
        "description": "ID of the image asset this fiducial marker component refers to.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "assetCollectionId",
        "description": "To retrieve this component's image asset, both the Asset ID and the Asset Collection ID are required.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Scale of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Whether the component is visible when in default mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 7,
        "name": "isARVisible",
        "description": "Whether the component is visible when in AR mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 8,
        "name": "isVirtualVisible",
        "description": "Whether the component is visible when in Virtual mode.",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 24,
    "name": "GaussianSplat",
    "description": "Add Gaussian Splats to your space. Gaussian Splatting is a technique for real-time 3D reconstruction and rendering of an object or environment using images taken from multiple points of view. Rather than representing the object as a mesh of triangles, it is instead represented as a volume comprising a point cloud of splats, each of which has a position, colour (with alpha) and covariance.",
    "properties": [
      {
        "key": 0,
        "reserved": true,
        "description": "Was name, superseded by component-level naming"
      },
      {
        "key": 1,
        "name": "externalResourceAssetId",
        "description": "To retrieve this component's gaussian splat asset, both the Asset ID and the Asset Collection ID are required.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "externalResourceAssetCollectionId",
        "description": "To retrieve this component's gaussian splat asset, both the Asset ID and the Asset Collection ID are required.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Scale of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Whether the component is visible when in default mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 7,
        "name": "isARVisible",
        "description": "Whether the component is visible when in AR mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 8,
        "deprecated": true,
        "name": "isShadowCaster",
        "type": "bool",
        "defaultValue": true,
        "scripting": false
      },
      {
        "key": 9,
        "name": "tint",
        "description": "Tint that should be globally applied to the Gaussian Splat associated with this component. Expected to be in RGB color space, with each value normalised between 0 and 1.",
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
        "description": "Whether the component is visible when in Virtual mode.",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 25,
    "name": "Text",
    "description": "Add a spatial representation of text to your space.",
    "properties": [
      {
        "key": 0,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Scale of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Text this text component refers to.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 4,
        "name": "textColor",
        "description": "Text color. Expected to be in RGB color space, with each value normalised between 0 and 1.",
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
        "description": "Background color applied to the text component. Expected to be in RGB color space, with each value normalised between 0 and 1.",
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
        "description": "Billboard mode used by this text component.",
        "type": "int",
        "defaultValue": 0,
        "options": [
          {
            "name": "Off",
            "value": 0
          },
          {
            "name": "Billboard",
            "value": 1
          },
          {
            "name": "YawLockedBillboard",
            "value": 2
          }
        ]
      },
      {
        "key": 10,
        "name": "isVisible",
        "description": "Whether the component is visible when in default mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 11,
        "name": "isARVisible",
        "description": "Whether the component is visible when in AR mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 12,
        "name": "isVirtualVisible",
        "description": "Whether the component is visible when in Virtual mode.",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 26,
    "name": "Hotspot",
    "description": "Data representation of an HotspotSpaceComponent.",
    "properties": [
      {
        "key": 0,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Whether this Hotspot is a teleport point.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 4,
        "name": "isSpawnPoint",
        "description": "Whether this Hotspot is a spawn point.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 5,
        "name": "isVisible",
        "description": "Whether the component is visible when in default mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 6,
        "name": "isARVisible",
        "description": "Whether the component is visible when in AR mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 7,
        "name": "isVirtualVisible",
        "description": "Whether the component is visible when in Virtual mode.",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 27,
    "name": "CinematicCamera",
    "description": "Data representation of a CinematicCameraSpaceComponent.",
    "properties": [
      {
        "key": 0,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Whether the component is enabled.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 3,
        "name": "focalLength",
        "description": "Focal length. Used alongside sensorSize and aspectRatio to compute field of view (historically exposed as GetFov()).",
        "type": "float",
        "defaultValue": 0.035
      },
      {
        "key": 4,
        "name": "aspectRatio",
        "description": "Current aspect ratio.",
        "type": "float",
        "defaultValue": 1.778
      },
      {
        "key": 5,
        "name": "sensorSize",
        "description": "Sensor size.",
        "type": "vec2",
        "defaultValue": [
          0.036,
          0.024
        ]
      },
      {
        "key": 6,
        "name": "nearClip",
        "description": "Near clip. On platforms that don't support reversedZ, near clip should be used to control the clipping distance.",
        "type": "float",
        "defaultValue": 0.1
      },
      {
        "key": 7,
        "name": "farClip",
        "description": "Far clip. On platforms that don't support reversedZ, far clip should be used to control the clipping distance.",
        "type": "float",
        "defaultValue": 20000.0
      },
      {
        "key": 8,
        "name": "iso",
        "description": "ISO sensitivity for controlling exposure.",
        "type": "float",
        "defaultValue": 400.0
      },
      {
        "key": 9,
        "name": "shutterSpeed",
        "description": "Shutter speed.",
        "type": "float",
        "defaultValue": 0.0167
      },
      {
        "key": 10,
        "name": "aperture",
        "description": "Aperture.",
        "type": "float",
        "defaultValue": 4.0
      },
      {
        "key": 11,
        "name": "isViewerCamera",
        "description": "Whether this camera acts as the viewer camera.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 12,
        "name": "thirdPartyComponentRef",
        "description": "Third party component reference.",
        "type": "string",
        "defaultValue": "",
        "scripting": false
      },
      {
        "key": 13,
        "name": "focusDistance",
        "description": "Focus distance.",
        "type": "float",
        "defaultValue": 5.0
      },
      {
        "key": 14,
        "name": "depthOfFieldEnabled",
        "description": "Whether depth of field is enabled.",
        "type": "bool",
        "defaultValue": false
      }
    ]
  },
  {
    "typeId": 28,
    "name": "ScreenSharing",
    "description": "Enables screen sharing within the space. The component provides properties to define a default image to be displayed when no users are sharing their screen, as well as a UserId property to store the Id of the user currently sharing their screen.",
    "properties": [
      {
        "key": 0,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Rotation of this component's origin as a quaternion, expressed in radians. Right handed coordinate system, positive rotation is counterclockwise. North: +Z, East: -X, South: -Z, West: +X.",
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
        "description": "Scale of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Whether the component is visible when in default mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 4,
        "name": "isARVisible",
        "description": "Whether the component is visible when in AR mode.",
        "type": "bool",
        "defaultValue": true
      },
      {
        "key": 5,
        "name": "isShadowCaster",
        "description": "Whether the mesh casts shadows.",
        "type": "bool",
        "defaultValue": false
      },
      {
        "key": 6,
        "name": "userId",
        "description": "ID of the user who is currently sharing their screen to this component. An empty string means that no user is currently sharing their screen.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 7,
        "name": "defaultImageCollectionId",
        "description": "To retrieve this component's default image, both the DefaultImageCollectionId and the DefaultImageAssetId are required.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 8,
        "name": "defaultImageAssetId",
        "description": "To retrieve this component's default image, both the DefaultImageCollectionId and the DefaultImageAssetId are required.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 9,
        "name": "attenuationRadius",
        "description": "Radius from this component origin within which the audio of this video can be heard by the user. Only when the user position is within this radius should the audio be heard.",
        "type": "float",
        "defaultValue": 10.0
      },
      {
        "key": 10,
        "name": "isVirtualVisible",
        "description": "Whether the component is visible when in Virtual mode.",
        "type": "bool",
        "defaultValue": true
      }
    ]
  },
  {
    "typeId": 29,
    "name": "AIChatbot",
    "description": "AI chatbot component with text-to-speech voice capability and configurable guardrails.",
    "properties": [
      {
        "key": 0,
        "name": "position",
        "description": "Position of this component's origin in world space. The coordinate system used follows the glTF 2.0 specification, in meters. Right handed coordinate system, +Y is UP, +X is left (facing forward), +Z is forward.",
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
        "description": "Voice name of the TTS model associated with this AI chatbot.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 2,
        "name": "guardrailAssetCollectionId",
        "description": "ID of the guardrail asset collection associated with this AI chatbot.",
        "type": "string",
        "defaultValue": ""
      },
      {
        "key": 3,
        "name": "visualState",
        "description": "Visual state of the AI chatbot for this component.",
        "type": "int",
        "defaultValue": 0,
        "options": [
          {
            "name": "Waiting",
            "value": 0
          },
          {
            "name": "Listening",
            "value": 1
          },
          {
            "name": "Thinking",
            "value": 2
          },
          {
            "name": "Speaking",
            "value": 3
          },
          {
            "name": "Unknown",
            "value": 4
          }
        ]
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
