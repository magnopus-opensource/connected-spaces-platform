set(CSP_CORE_INCLUDE_DIR ${CSP_INCLUDE_DIR}/CSP/Core)
set(CSP_CORE_SOURCE_DIR  ${CSP_SOURCE_DIR}/Core)

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

    ${CSP_CORE_INCLUDE_DIR}/Assets/EventTicketing.h
    ${CSP_CORE_INCLUDE_DIR}/Assets/EventTicketingSystem.h

    ${CSP_CORE_INCLUDE_DIR}/Assets/EventTicketing.h

)

set(CSP_CORE_SOURCES
   
)

set(CSP_CORE_PRIVATE_INCLUDES 

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