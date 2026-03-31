# Contains files that hande our realtime/multiplayer functionality.
# This has yet to be formally modularized into a separate library.

set(CSP_MULTIPLAYER_INCLUDE_DIR ${CSP_INCLUDE_DIR}/CSP/Multiplayer)
set(CSP_MULTIPLAYER_SOURCE_DIR  ${CSP_SOURCE_DIR}/Multiplayer)

set(CSP_MULTIPLAYER_PUBLIC_INCLUDES 
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/ComponentBase.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/ContinuationUtils.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/ContinuationUtils.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/CSPSceneDescription.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/MultiPlayerConnection.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/MultiplayerHubMethods.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/NetworkEventBus.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/OfflineRealtimeEngine.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/OnlineRealtimeEngine.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/PatchTypes.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/SpaceEntity.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/SpaceTransform.h

    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/AIChatbotComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/AnimatedModelSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/AudioSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/AvatarSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/BillBoardModeEnum.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/ButtonSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/CinematicCameraSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/CollisionSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/ConversationSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/CustomSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/ECommerceSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/ExternalLinkSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/FiducialMarkerSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/FogSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/GaussianSplatSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/HotspotSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/ImageSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/LightSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/PortalSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/ReflectionSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/ScreenSharingSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/ScriptSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/SplineSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/StaticModelSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/TextSpaceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/VideoPlayerSpaceComponent.h

    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/Interfaces/IEnableableComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/Interfaces/IExternalResourceComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/Interfaces/IPositionComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/Interfaces/IRenderBehaviourComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/Interfaces/IRotationComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/Interfaces/IScaleComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/Interfaces/IShadowCasterComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/Interfaces/IThirdPartyComponentRef.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/Interfaces/ITransformComponent.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Components/Interfaces/IVisibleComponent.h

    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Conversation/Conversation.h

    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Script/EntityScript.h
    ${CSP_MULTIPLAYER_INCLUDE_DIR}/Script/EntityScriptMessages.h
)

set(CSP_MULTIPLAYER_SOURCES
    ${CSP_MULTIPLAYER_SOURCE_DIR}/ComponentBase.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/CSPSceneDescription.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/MCSComponentPacker.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/MultiplayerConnection.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/MultiplayerConstants.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/MultiplayerHubMethods.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/NetworkEventBus.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/NetworkEventManagerImpl.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/NetworkEventSerialisation.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/OfflineRealtimeEngine.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/OnlineRealtimeEngine.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/RealtimeEngineUtils.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/SignalRSerializer.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/SpaceEntity.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/SpaceEntityStatePatcher.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/SpaceTransform.cpp

    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/AIChatbotComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/AnimatedModelSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/AudioSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/AvatarSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/ButtonSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/CinematicCameraSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/CollisionSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/ConversationSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/CustomSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/ECommerceSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/ExternalLinkSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/FiducialMarkerSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/FogSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/GaussianSplatSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/HotspotSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/ImageSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/LightSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/PortalSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/ReflectionSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/ScreenSharingSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/ScriptSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/SplineSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/StaticModelSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/TextSpaceComponent.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Components/VideoPlayerSpaceComponent.cpp

    ${CSP_MULTIPLAYER_SOURCE_DIR}/Conversation/Conversation.cpp

    ${CSP_MULTIPLAYER_SOURCE_DIR}/Election/ClientElectionManager.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Election/ClientProxy.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Election/ScopeLeadershipManager.cpp

    ${CSP_MULTIPLAYER_SOURCE_DIR}/MCS/MCSSceneDescription.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/MCS/MCSTypes.cpp

    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/EntityScript.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/EntityScriptBinding.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/EntityScriptInterface.cpp

    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/AIChatbotComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/AnimatedModelSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/AudioSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/AvatarSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/ButtonSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/CinematicCameraSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/CollisionSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/ConversationSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/CustomSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/ECommerceSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/ExternalLinkSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/FiducialMarkerSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/FogSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/GaussianSplatSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/HotspotSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/ImageSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/LightSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/PortalSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/ReflectionSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/ScreenSharingSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/SplineSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/StaticModelSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/TextSpaceComponentScriptInterface.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/VideoPlayerSpaceComponentScriptInterface.cpp

    ${CSP_MULTIPLAYER_SOURCE_DIR}/SignalR/SignalRClient.cpp
    ${CSP_MULTIPLAYER_SOURCE_DIR}/SignalR/SignalRConnection.cpp

    ${CSP_MULTIPLAYER_SOURCE_DIR}/SignalR/EmscriptenSignalRClient/EmscriptenSignalRClient.cpp

    ${CSP_MULTIPLAYER_SOURCE_DIR}/SignalR/POCOSignalRClient/POCOSignalRClient.cpp
)

set(CSP_MULTIPLAYER_PRIVATE_INCLUDES 
    ${CSP_MULTIPLAYER_SOURCE_DIR}/ComponentBaseKeys.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/MCSComponentPacker.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/MultiplayerConstants.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/NetworkEventManagerImpl.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/NetworkEventSerialisation.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/PatchUtils.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/RealtimeEngineUtils.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/SignalRSerializer.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/SignalRSerializerTypeTraits.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/SpaceEntityKeys.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/SpaceEntityStatePatcher.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/WebSocketClient.h

    ${CSP_MULTIPLAYER_SOURCE_DIR}/Election/ClientElectionManager.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Election/ClientProxy.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Election/ScopeLeadershipManager.h

    ${CSP_MULTIPLAYER_SOURCE_DIR}/MCS/MCSSceneDescription.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/MCS/MCSTypes.h

    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentScriptMacros.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/EntityScriptBinding.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/EntityScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ScriptHelpers.h

    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/AIChatbotComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/AnimatedModelSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/AudioSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/AvatarSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/ButtonSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/CinematicCameraSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/CollisionSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/ConversationSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/CustomSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/ECommerceSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/ExternalLinkSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/FiducialMarkerSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/FogSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/GaussianSplatSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/HotspotSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/ImageSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/LightSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/PortalSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/ReflectionSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/ScreenSharingSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/SplineSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/StaticModelSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/TextSpaceComponentScriptInterface.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/Script/ComponentBinding/VideoPlayerSpaceComponentScriptInterface.h

    ${CSP_MULTIPLAYER_SOURCE_DIR}/SignalR/ISignalRConnection.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/SignalR/SignalRClient.h
    ${CSP_MULTIPLAYER_SOURCE_DIR}/SignalR/SignalRConnection.h

    ${CSP_MULTIPLAYER_SOURCE_DIR}/SignalR/EmscriptenSignalRClient/EmscriptenSignalRClient.h

    ${CSP_MULTIPLAYER_SOURCE_DIR}/SignalR/POCOSignalRClient/POCOSignalRClient.h
)