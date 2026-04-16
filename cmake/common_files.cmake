# Contains files that are used in both multiplayer and core.
# This has yet to be formally modularized into a separate library.

set(CSP_ROOT_INCLUDE_DIR ${CSP_INCLUDE_DIR}/CSP)
set(CSP_COMMON_INCLUDE_DIR ${CSP_INCLUDE_DIR}/CSP/Common)
set(CSP_COMMON_SOURCE_DIR  ${CSP_SOURCE_DIR}/Common)

set(CSP_COMMON_PUBLIC_INCLUDES 
    ${CSP_COMMON_INCLUDE_DIR}/Array.h
    ${CSP_COMMON_INCLUDE_DIR}/CancellationToken.h
    ${CSP_COMMON_INCLUDE_DIR}/ContinuationUtils.h
    ${CSP_COMMON_INCLUDE_DIR}/CSPAsyncScheduler.h
    ${CSP_COMMON_INCLUDE_DIR}/fmt_Formatters.h
    ${CSP_COMMON_INCLUDE_DIR}/Hash.h
    ${CSP_COMMON_INCLUDE_DIR}/List.h
    ${CSP_COMMON_INCLUDE_DIR}/LoginState.h
    ${CSP_COMMON_INCLUDE_DIR}/Map.h
    ${CSP_COMMON_INCLUDE_DIR}/MimeTypeHelper.h
    ${CSP_COMMON_INCLUDE_DIR}/NetworkEventData.h
    ${CSP_COMMON_INCLUDE_DIR}/Optional.h
    ${CSP_COMMON_INCLUDE_DIR}/ReplicatedValue.h
    ${CSP_COMMON_INCLUDE_DIR}/ReplicatedValueException.h
    ${CSP_COMMON_INCLUDE_DIR}/Settings.h
    ${CSP_COMMON_INCLUDE_DIR}/SharedConstants.h
    ${CSP_COMMON_INCLUDE_DIR}/SharedEnums.h
    ${CSP_COMMON_INCLUDE_DIR}/String.h
    ${CSP_COMMON_INCLUDE_DIR}/StringFormat.h
    ${CSP_COMMON_INCLUDE_DIR}/Variant.h
    ${CSP_COMMON_INCLUDE_DIR}/Vector.h

    ${CSP_COMMON_INCLUDE_DIR}/Interfaces/IAuthContext.h
    ${CSP_COMMON_INCLUDE_DIR}/Interfaces/IJSScriptRunner.h
    ${CSP_COMMON_INCLUDE_DIR}/Interfaces/InvalidInterfaceUserError.h
    ${CSP_COMMON_INCLUDE_DIR}/Interfaces/IRealtimeEngine.h
    ${CSP_COMMON_INCLUDE_DIR}/Interfaces/IScriptBinding.h

    ${CSP_COMMON_INCLUDE_DIR}/Systems/Log/LogLevels.h
    ${CSP_COMMON_INCLUDE_DIR}/Systems/Log/LogSystem.h

    # Files that exist at the root of the Library folder.
    # We will need to sort these files when we do the formal modularization work.
    ${CSP_ROOT_INCLUDE_DIR}/AssetHash.h
    ${CSP_ROOT_INCLUDE_DIR}/CSPCommon.h
    ${CSP_ROOT_INCLUDE_DIR}/CSPFoundation.h
    ${CSP_ROOT_INCLUDE_DIR}/version.h
)

set(CSP_COMMON_SOURCES
    ${CSP_COMMON_SOURCE_DIR}/Algorithm.cpp
    ${CSP_COMMON_SOURCE_DIR}/CancellationToken.cpp
    ${CSP_COMMON_SOURCE_DIR}/DateTime.cpp
    ${CSP_COMMON_SOURCE_DIR}/Encode.cpp
    ${CSP_COMMON_SOURCE_DIR}/Hash.cpp
    ${CSP_COMMON_SOURCE_DIR}/LoginState.cpp
    ${CSP_COMMON_SOURCE_DIR}/MimeTypeHelper.cpp
    ${CSP_COMMON_SOURCE_DIR}/ReplicatedValue.cpp
    ${CSP_COMMON_SOURCE_DIR}/Scheduler.cpp
    ${CSP_COMMON_SOURCE_DIR}/Settings.cpp
    ${CSP_COMMON_SOURCE_DIR}/String.cpp
    ${CSP_COMMON_SOURCE_DIR}/Variant.cpp
    ${CSP_COMMON_SOURCE_DIR}/Vector.cpp

    ${CSP_COMMON_SOURCE_DIR}/Systems/Log/LogSystem.cpp

    ${CSP_COMMON_SOURCE_DIR}/Web/HttpAuth.cpp
    ${CSP_COMMON_SOURCE_DIR}/Web/HttpPayload.cpp
    ${CSP_COMMON_SOURCE_DIR}/Web/HttpProgress.cpp
    ${CSP_COMMON_SOURCE_DIR}/Web/HttpRequest.cpp
    ${CSP_COMMON_SOURCE_DIR}/Web/HttpResponse.cpp
    ${CSP_COMMON_SOURCE_DIR}/Web/Json.cpp
    ${CSP_COMMON_SOURCE_DIR}/Web/Uri.cpp
    ${CSP_COMMON_SOURCE_DIR}/Web/WebClient.cpp

    ${CSP_COMMON_SOURCE_DIR}/Web/EmscriptenWebClient/EmscriptenWebClient.cpp

    ${CSP_COMMON_SOURCE_DIR}/Web/POCOWebClient/POCOWebClient.cpp

    # Files that exist at the root of the Library folder.
    # We will need to sort these files when we do the formal modularization work.
    ${CSP_SOURCE_DIR}/AssetHash.cpp
    ${CSP_SOURCE_DIR}/CSPFoundation.cpp
    ${CSP_SOURCE_DIR}/ExplicitTypes.cpp

    ${CSP_SOURCE_DIR}/EmscriptenBindings/CallbackQueue.cpp

    ${CSP_SOURCE_DIR}/Events/Event.cpp
    ${CSP_SOURCE_DIR}/Events/EventDispatcher.cpp
    ${CSP_SOURCE_DIR}/Events/EventId.cpp
    ${CSP_SOURCE_DIR}/Events/EventSystem.cpp

    ${CSP_SOURCE_DIR}/Json/JsonParseHelper.cpp
    ${CSP_SOURCE_DIR}/Json/JsonSerializer.cpp

    ${CSP_SOURCE_DIR}/Services/ApiBase/ApiBase.cpp

    ${CSP_SOURCE_DIR}/Services/DtoBase/DtoBase.cpp

    ${CSP_SOURCE_DIR}/Services/PrototypeService/AssetFileDto.cpp

    ${CSP_SOURCE_DIR}/Web/RemoteFileManager.cpp

    ${CSP_SOURCE_DIR}/Web/GraphQLApi/GraphQLApi.cpp

    ${CSP_SOURCE_DIR}/Web/MaintenanceApi/MaintenanceApi.cpp


)

set(CSP_COMMON_PRIVATE_INCLUDES 
    ${CSP_COMMON_SOURCE_DIR}/Convert.h
    ${CSP_COMMON_SOURCE_DIR}/DateTime.h
    ${CSP_COMMON_SOURCE_DIR}/Encode.h
    ${CSP_COMMON_SOURCE_DIR}/Logger.h
    ${CSP_COMMON_SOURCE_DIR}/NumberFormatter.h
    ${CSP_COMMON_SOURCE_DIR}/Queue.h
    ${CSP_COMMON_SOURCE_DIR}/Scheduler.h
    ${CSP_COMMON_SOURCE_DIR}/ThreadPool.h
    ${CSP_COMMON_SOURCE_DIR}/UUIDGenerator.h
    ${CSP_COMMON_SOURCE_DIR}/Wrappers.h

    ${CSP_COMMON_SOURCE_DIR}/Web/HttpAuth.h
    ${CSP_COMMON_SOURCE_DIR}/Web/HttpPayload.h
    ${CSP_COMMON_SOURCE_DIR}/Web/HttpProgress.h
    ${CSP_COMMON_SOURCE_DIR}/Web/HttpRequest.h
    ${CSP_COMMON_SOURCE_DIR}/Web/HttpResponse.h
    ${CSP_COMMON_SOURCE_DIR}/Web/Json.h
    ${CSP_COMMON_SOURCE_DIR}/Web/Json_HttpPayload.h
    ${CSP_COMMON_SOURCE_DIR}/Web/Uri.h
    ${CSP_COMMON_SOURCE_DIR}/Web/WebClient.h

    ${CSP_COMMON_SOURCE_DIR}/Web/EmscriptenWebClient/EmscriptenWebClient.h

    ${CSP_COMMON_SOURCE_DIR}/Web/POCOWebClient/POCOWebClient.h


    # Files that exist at the root of the Library folder.
    # We will need to sort these files when we do the formal modularization work.
    ${CSP_SOURCE_DIR}/CallHelpers.h
    ${CSP_SOURCE_DIR}/WrapperGenUtils.h

    ${CSP_SOURCE_DIR}/Debug/Logging.h

    ${CSP_SOURCE_DIR}/EmscriptenBindings/CallbackQueue.h

    ${CSP_SOURCE_DIR}/Events/Event.h
    ${CSP_SOURCE_DIR}/Events/EventDispatcher.h
    ${CSP_SOURCE_DIR}/Events/EventId.h
    ${CSP_SOURCE_DIR}/Events/EventListener.h
    ${CSP_SOURCE_DIR}/Events/EventSystem.h

    ${CSP_SOURCE_DIR}/Json/JsonParseHelper.h
    ${CSP_SOURCE_DIR}/Json/JsonSerializer.h

    ${CSP_SOURCE_DIR}/Services/ApiBase/ApiBase.h

    ${CSP_SOURCE_DIR}/Services/DtoBase/DtoBase.h

    ${CSP_SOURCE_DIR}/Services/PrototypeService/AssetFileDto.h

    ${CSP_SOURCE_DIR}/Storage/FileCache.h

    ${CSP_SOURCE_DIR}/Web/RemoteFileManager.h

    ${CSP_SOURCE_DIR}/Web/GraphQLApi/GraphQLApi.h

    ${CSP_SOURCE_DIR}/Web/MaintenanceApi/MaintenanceApi.h
)