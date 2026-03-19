set(CSP_MISC_INCLUDE_DIR ${CSP_INCLUDE_DIR}/CSP)
set(CSP_MISC_SOURCE_DIR  ${CSP_SOURCE_DIR})

set(CSP_MISC_PUBLIC_INCLUDES 
    ${CSP_MISC_INCLUDE_DIR}/AssetHash.h
    ${CSP_MISC_INCLUDE_DIR}/CSPCommon.h
    ${CSP_MISC_INCLUDE_DIR}/CSPFoundation.h
    ${CSP_MISC_INCLUDE_DIR}/version.h
)

set(CSP_MISC_SOURCES
    ${CSP_MISC_SOURCE_DIR}/AssetHash.cpp
    ${CSP_MISC_SOURCE_DIR}/CSPFoundation.cpp
    ${CSP_MISC_SOURCE_DIR}/ExplicitTypes.cpp

    ${CSP_MISC_SOURCE_DIR}/EmscriptenBindings/CallbackQueue.cpp

    ${CSP_MISC_SOURCE_DIR}/Events/Event.cpp
    ${CSP_MISC_SOURCE_DIR}/Events/EventDispatcher.cpp
    ${CSP_MISC_SOURCE_DIR}/Events/EventId.cpp
    ${CSP_MISC_SOURCE_DIR}/Events/EventSystem.cpp

    ${CSP_MISC_SOURCE_DIR}/Json/JsonParseHelper.cpp
    ${CSP_MISC_SOURCE_DIR}/Json/JsonSerializer.cpp

    ${CSP_MISC_SOURCE_DIR}/Services/ApiBase/ApiBase.cpp

    ${CSP_MISC_SOURCE_DIR}/Services/DtoBase/DtoBase.cpp

    ${CSP_MISC_SOURCE_DIR}/Services/PrototypeService/AssetFileDto.cpp

    ${CSP_MISC_SOURCE_DIR}/Web/RemoteFileManager.cpp

    ${CSP_MISC_SOURCE_DIR}/Web/GraphQLApi/GraphQLApi.cpp

    ${CSP_MISC_SOURCE_DIR}/Web/MaintenanceApi/MaintenanceApi.cpp
)

set(CSP_MISC_PRIVATE_INCLUDES 
    ${CSP_MISC_SOURCE_DIR}/CallHelpers.h
    ${CSP_MISC_SOURCE_DIR}/WrapperGenUtils.h

    ${CSP_MISC_SOURCE_DIR}/Debug/Logging.h

    ${CSP_MISC_SOURCE_DIR}/EmscriptenBindings/CallbackQueue.h

    ${CSP_MISC_SOURCE_DIR}/Events/Event.h
    ${CSP_MISC_SOURCE_DIR}/Events/EventDispatcher.h
    ${CSP_MISC_SOURCE_DIR}/Events/EventId.h
    ${CSP_MISC_SOURCE_DIR}/Events/EventListener.h
    ${CSP_MISC_SOURCE_DIR}/Events/EventSystem.h

    ${CSP_MISC_SOURCE_DIR}/Json/JsonParseHelper.h
    ${CSP_MISC_SOURCE_DIR}/Json/JsonSerializer.h

    ${CSP_MISC_SOURCE_DIR}/Services/ApiBase/ApiBase.h

    ${CSP_MISC_SOURCE_DIR}/Services/DtoBase/DtoBase.h

    ${CSP_MISC_SOURCE_DIR}/Services/PrototypeService/AssetFileDto.h

    ${CSP_MISC_SOURCE_DIR}/Storage/FileCache.h

    ${CSP_MISC_SOURCE_DIR}/Web/RemoteFileManager.h

    ${CSP_MISC_SOURCE_DIR}/Web/GraphQLApi/GraphQLApi.h

    ${CSP_MISC_SOURCE_DIR}/Web/MaintenanceApi/MaintenanceApi.h
)

# Public
target_sources(csp-lib PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CSP_INCLUDE_DIR}
    FILES ${CSP_MISC_PUBLIC_INCLUDES}
)

# Private
target_sources(csp-lib PRIVATE
    ${CSP_MISC_SOURCES}
    ${CSP_MISC_PRIVATE_INCLUDES}
)