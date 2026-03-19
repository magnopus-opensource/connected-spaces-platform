set(CSP_CORE_INCLUDE_DIR ${CSP_INCLUDE_DIR}/CSP/Systems)
set(CSP_CORE_SOURCE_DIR  ${CSP_SOURCE_DIR}/Systems)

set(CSP_CORE_PUBLIC_INCLUDES 
    ${CSP_CORE_INCLUDE_DIR}/ContinuationUtils.h
    ${CSP_CORE_INCLUDE_DIR}/CSPSceneData.h
    ${CSP_CORE_INCLUDE_DIR}/ServiceStatus.h
    ${CSP_CORE_INCLUDE_DIR}/SystemBase.h
    ${CSP_CORE_INCLUDE_DIR}/SystemsManager.h
    ${CSP_CORE_INCLUDE_DIR}/SystemsResult.h
    ${CSP_CORE_INCLUDE_DIR}/WebService.h

    ${CSP_CORE_INCLUDE_DIR}/Analytics/AnalyticsSystem.h

    ${CSP_CORE_INCLUDE_DIR}/Assets/AlphaVideoMaterial.h
    ${CSP_CORE_INCLUDE_DIR}/Assets/Asset.h
    ${CSP_CORE_INCLUDE_DIR}/Assets/AssetCollection.h
    ${CSP_CORE_INCLUDE_DIR}/Assets/AssetSystem.h
    ${CSP_CORE_INCLUDE_DIR}/Assets/GLTFMaterial.h
    ${CSP_CORE_INCLUDE_DIR}/Assets/LOD.h
    ${CSP_CORE_INCLUDE_DIR}/Assets/Material.h
    ${CSP_CORE_INCLUDE_DIR}/Assets/TextureInfo.h

    ${CSP_CORE_INCLUDE_DIR}/ECommerce/ECommerce.h
    ${CSP_CORE_INCLUDE_DIR}/ECommerce/ECommerceSystem.h

    ${CSP_CORE_INCLUDE_DIR}/EventTicketing/EventTicketing.h
    ${CSP_CORE_INCLUDE_DIR}/EventTicketing/EventTicketingSystem.h

    ${CSP_CORE_INCLUDE_DIR}/ExternalServices/ExternalServiceInvocation.h
    ${CSP_CORE_INCLUDE_DIR}/ExternalServices/ExternalServiceProxySystem.h

    ${CSP_CORE_INCLUDE_DIR}/GraphQL/GraphQL.h
    ${CSP_CORE_INCLUDE_DIR}/GraphQL/GraphQLSystem.h

    ${CSP_CORE_INCLUDE_DIR}/HotspotSequence/HotspotGroup.h
    ${CSP_CORE_INCLUDE_DIR}/HotspotSequence/HotspotSequenceSystem.h

    ${CSP_CORE_INCLUDE_DIR}/Maintenance/Maintenance.h
    ${CSP_CORE_INCLUDE_DIR}/Maintenance/MaintenanceSystem.h

    ${CSP_CORE_INCLUDE_DIR}/MCS/MCSSceneData.h

    ${CSP_CORE_INCLUDE_DIR}/Multiplayer/MultiplayerSystem.h
    ${CSP_CORE_INCLUDE_DIR}/Multiplayer/Scope.h
    ${CSP_CORE_INCLUDE_DIR}/Multiplayer/ScopeLeader.h

    ${CSP_CORE_INCLUDE_DIR}/Quota/Quota.h
    ${CSP_CORE_INCLUDE_DIR}/Quota/QuotaSystem.h

    ${CSP_CORE_INCLUDE_DIR}/Script/ScriptSystem.h

    ${CSP_CORE_INCLUDE_DIR}/Sequence/Sequence.h
    ${CSP_CORE_INCLUDE_DIR}/Sequence/SequenceSystem.h

    ${CSP_CORE_INCLUDE_DIR}/Settings/ApplicationSettings.h
    ${CSP_CORE_INCLUDE_DIR}/Settings/ApplicationSettingsSystem.h
    ${CSP_CORE_INCLUDE_DIR}/Settings/SettingsCollection.h
    ${CSP_CORE_INCLUDE_DIR}/Settings/SettingsSystem.h

    ${CSP_CORE_INCLUDE_DIR}/Spaces/Site.h
    ${CSP_CORE_INCLUDE_DIR}/Spaces/Space.h
    ${CSP_CORE_INCLUDE_DIR}/Spaces/SpaceSystem.h
    ${CSP_CORE_INCLUDE_DIR}/Spaces/UserRoles.h

    ${CSP_CORE_INCLUDE_DIR}/Spatial/Anchor.h
    ${CSP_CORE_INCLUDE_DIR}/Spatial/AnchorSystem.h
    ${CSP_CORE_INCLUDE_DIR}/Spatial/PointOfInterest.h
    ${CSP_CORE_INCLUDE_DIR}/Spatial/PointOfInterestSystem.h
    ${CSP_CORE_INCLUDE_DIR}/Spatial/SpatialDataTypes.h

    ${CSP_CORE_INCLUDE_DIR}/Users/Authentication.h
    ${CSP_CORE_INCLUDE_DIR}/Users/Profile.h
    ${CSP_CORE_INCLUDE_DIR}/Users/ThirdPartyAuthentication.h
    ${CSP_CORE_INCLUDE_DIR}/Users/UserSystem.h

    ${CSP_CORE_INCLUDE_DIR}/Voip/VoipSystem.h
)

set(CSP_CORE_SOURCES
    ${CSP_CORE_SOURCE_DIR}/CSPSceneData.cpp
    ${CSP_CORE_SOURCE_DIR}/ServiceStatus.cpp
    ${CSP_CORE_SOURCE_DIR}/SystemBase.cpp
    ${CSP_CORE_SOURCE_DIR}/SystemsManager.cpp
    ${CSP_CORE_SOURCE_DIR}/SystemsResult.cpp
    ${CSP_CORE_SOURCE_DIR}/WebService.cpp

    ${CSP_CORE_SOURCE_DIR}/Analytics/AnalyticsSystem.cpp

    ${CSP_CORE_SOURCE_DIR}/Assets/AlphaVideoMaterial.cpp
    ${CSP_CORE_SOURCE_DIR}/Assets/Asset.cpp
    ${CSP_CORE_SOURCE_DIR}/Assets/AssetCollection.cpp
    ${CSP_CORE_SOURCE_DIR}/Assets/AssetSystem.cpp
    ${CSP_CORE_SOURCE_DIR}/Assets/GLTFMaterial.cpp
    ${CSP_CORE_SOURCE_DIR}/Assets/LOD.cpp
    ${CSP_CORE_SOURCE_DIR}/Assets/LODHelpers.cpp
    ${CSP_CORE_SOURCE_DIR}/Assets/Material.cpp
    ${CSP_CORE_SOURCE_DIR}/Assets/TextureInfo.cpp

    ${CSP_CORE_SOURCE_DIR}/Conversation/ConversationSystemHelpers.cpp
    ${CSP_CORE_SOURCE_DIR}/Conversation/ConversationSystemInternal.cpp

    ${CSP_CORE_SOURCE_DIR}/ECommerce/ECommerce.cpp
    ${CSP_CORE_SOURCE_DIR}/ECommerce/ECommerceSystem.cpp
    ${CSP_CORE_SOURCE_DIR}/ECommerce/ECommerceSystemHelpers.cpp

    ${CSP_CORE_SOURCE_DIR}/EventTicketing/EventTicketing.cpp
    ${CSP_CORE_SOURCE_DIR}/EventTicketing/EventTicketingSystem.cpp

    ${CSP_CORE_SOURCE_DIR}/ExternalServices/ExternalServiceInvocation.cpp
    ${CSP_CORE_SOURCE_DIR}/ExternalServices/ExternalServiceProxySystem.cpp

    ${CSP_CORE_SOURCE_DIR}/GraphQL/GraphQL.cpp
    ${CSP_CORE_SOURCE_DIR}/GraphQL/GraphQLSystem.cpp

    ${CSP_CORE_SOURCE_DIR}/HotspotSequence/HotspotGroup.cpp
    ${CSP_CORE_SOURCE_DIR}/HotspotSequence/HotspotSequenceSystem.cpp

    ${CSP_CORE_SOURCE_DIR}/Maintenance/Maintenance.cpp
    ${CSP_CORE_SOURCE_DIR}/Maintenance/MaintenanceSystem.cpp

    ${CSP_CORE_SOURCE_DIR}/Multiplayer/MultiplayerSystem.cpp
    ${CSP_CORE_SOURCE_DIR}/Multiplayer/Scope.cpp
    ${CSP_CORE_SOURCE_DIR}/Multiplayer/ScopeLeader.cpp

    ${CSP_CORE_SOURCE_DIR}/Quota/Quota.cpp
    ${CSP_CORE_SOURCE_DIR}/Quota/QuotaSystem.cpp

    ${CSP_CORE_SOURCE_DIR}/Script/ScriptContext.cpp
    ${CSP_CORE_SOURCE_DIR}/Script/ScriptRuntime.cpp
    ${CSP_CORE_SOURCE_DIR}/Script/ScriptSystem.cpp

    ${CSP_CORE_SOURCE_DIR}/Sequence/Sequence.cpp
    ${CSP_CORE_SOURCE_DIR}/Sequence/SequenceSystem.cpp

    ${CSP_CORE_SOURCE_DIR}/Settings/ApplicationSettings.cpp
    ${CSP_CORE_SOURCE_DIR}/Settings/ApplicationSettingsSystem.cpp
    ${CSP_CORE_SOURCE_DIR}/Settings/SettingsCollection.cpp
    ${CSP_CORE_SOURCE_DIR}/Settings/SettingsSystem.cpp

    ${CSP_CORE_SOURCE_DIR}/Spaces/Site.cpp
    ${CSP_CORE_SOURCE_DIR}/Spaces/Space.cpp
    ${CSP_CORE_SOURCE_DIR}/Spaces/SpaceSystem.cpp
    ${CSP_CORE_SOURCE_DIR}/Spaces/SpaceSystemHelpers.cpp
    ${CSP_CORE_SOURCE_DIR}/Spaces/UserRoles.cpp

    ${CSP_CORE_SOURCE_DIR}/Spatial/Anchor.cpp
    ${CSP_CORE_SOURCE_DIR}/Spatial/AnchorSystem.cpp
    ${CSP_CORE_SOURCE_DIR}/Spatial/PointOfInterest.cpp
    ${CSP_CORE_SOURCE_DIR}/Spatial/PointOfInterestSystem.cpp
    ${CSP_CORE_SOURCE_DIR}/Spatial/SpatialDataTypes.cpp

    ${CSP_CORE_SOURCE_DIR}/Users/Authentication.cpp
    ${CSP_CORE_SOURCE_DIR}/Users/Profile.cpp
    ${CSP_CORE_SOURCE_DIR}/Users/ThirdPartyAuthentication.cpp
    ${CSP_CORE_SOURCE_DIR}/Users/UserSystem.cpp

    ${CSP_CORE_SOURCE_DIR}/Voip/VoipSystem.cpp
)

set(CSP_CORE_PRIVATE_INCLUDES 
    ${CSP_CORE_SOURCE_DIR}/ResultHelpers.h

    ${CSP_CORE_SOURCE_DIR}/Assets/LODHelpers.h

    ${CSP_CORE_SOURCE_DIR}/Conversation/ConversationSystemHelpers.h
    ${CSP_CORE_SOURCE_DIR}/Conversation/ConversationSystemInternal.h

    ${CSP_CORE_SOURCE_DIR}/ECommerce/ECommerceSystemHelpers.h

    ${CSP_CORE_SOURCE_DIR}/Script/ScriptContext.h
    ${CSP_CORE_SOURCE_DIR}/Script/ScriptRuntime.h

    ${CSP_CORE_SOURCE_DIR}/Spaces/SpaceSystemHelpers.h

    ${CSP_CORE_SOURCE_DIR}/Spatial/PointOfInterestHelpers.h
    ${CSP_CORE_SOURCE_DIR}/Spatial/PointOfInterestInternalSystem.h

    ${CSP_CORE_SOURCE_DIR}/Users/Authentication.h

    )   

# Public
target_sources(csp-lib PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CSP_INCLUDE_DIR}
    FILES ${CSP_CORE_PUBLIC_INCLUDES}
)

# Private
target_sources(csp-lib PRIVATE
    ${CSP_CORE_SOURCES}
    ${CSP_CORE_PRIVATE_INCLUDES}
)