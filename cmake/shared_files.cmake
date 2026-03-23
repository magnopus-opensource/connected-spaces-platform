set(CSP_SHARED_INCLUDE_DIR ${CSP_INCLUDE_DIR}/CSP/Common)
set(CSP_SHARED_SOURCE_DIR  ${CSP_SOURCE_DIR}/Common)

set(CSP_SHARED_PUBLIC_INCLUDES 
    ${CSP_SHARED_INCLUDE_DIR}/Array.h
    ${CSP_SHARED_INCLUDE_DIR}/CancellationToken.h
    ${CSP_SHARED_INCLUDE_DIR}/ContinuationUtils.h
    ${CSP_SHARED_INCLUDE_DIR}/CSPAsyncScheduler.h
    ${CSP_SHARED_INCLUDE_DIR}/fmt_Formatters.h
    ${CSP_SHARED_INCLUDE_DIR}/Hash.h
    ${CSP_SHARED_INCLUDE_DIR}/List.h
    ${CSP_SHARED_INCLUDE_DIR}/LoginState.h
    ${CSP_SHARED_INCLUDE_DIR}/Map.h
    ${CSP_SHARED_INCLUDE_DIR}/MimeTypeHelper.h
    ${CSP_SHARED_INCLUDE_DIR}/NetworkEventData.h
    ${CSP_SHARED_INCLUDE_DIR}/Optional.h
    ${CSP_SHARED_INCLUDE_DIR}/ReplicatedValue.h
    ${CSP_SHARED_INCLUDE_DIR}/ReplicatedValueException.h
    ${CSP_SHARED_INCLUDE_DIR}/Settings.h
    ${CSP_SHARED_INCLUDE_DIR}/SharedConstants.h
    ${CSP_SHARED_INCLUDE_DIR}/SharedEnums.h
    ${CSP_SHARED_INCLUDE_DIR}/String.h
    ${CSP_SHARED_INCLUDE_DIR}/StringFormat.h
    ${CSP_SHARED_INCLUDE_DIR}/Variant.h
    ${CSP_SHARED_INCLUDE_DIR}/Vector.h

    ${CSP_SHARED_INCLUDE_DIR}/Interfaces/IAuthContext.h
    ${CSP_SHARED_INCLUDE_DIR}/Interfaces/IJSScriptRunner.h
    ${CSP_SHARED_INCLUDE_DIR}/Interfaces/InvalidInterfaceUserError.h
    ${CSP_SHARED_INCLUDE_DIR}/Interfaces/IRealtimeEngine.h
    ${CSP_SHARED_INCLUDE_DIR}/Interfaces/IScriptBinding.h

    ${CSP_SHARED_INCLUDE_DIR}/Systems/Log/LogLevels.h
    ${CSP_SHARED_INCLUDE_DIR}/Systems/Log/LogSystem.h
)

set(CSP_SHARED_SOURCES
    ${CSP_SHARED_SOURCE_DIR}/Algorithm.cpp
    ${CSP_SHARED_SOURCE_DIR}/CancellationToken.cpp
    ${CSP_SHARED_SOURCE_DIR}/DateTime.cpp
    ${CSP_SHARED_SOURCE_DIR}/Encode.cpp
    ${CSP_SHARED_SOURCE_DIR}/Hash.cpp
    ${CSP_SHARED_SOURCE_DIR}/LoginState.cpp
    ${CSP_SHARED_SOURCE_DIR}/MimeTypeHelper.cpp
    ${CSP_SHARED_SOURCE_DIR}/ReplicatedValue.cpp
    ${CSP_SHARED_SOURCE_DIR}/Scheduler.cpp
    ${CSP_SHARED_SOURCE_DIR}/Settings.cpp
    ${CSP_SHARED_SOURCE_DIR}/String.cpp
    ${CSP_SHARED_SOURCE_DIR}/Variant.cpp
    ${CSP_SHARED_SOURCE_DIR}/Vector.cpp

    ${CSP_SHARED_SOURCE_DIR}/Systems/Log/LogSystem.cpp

    ${CSP_SHARED_SOURCE_DIR}/Web/HttpAuth.cpp
    ${CSP_SHARED_SOURCE_DIR}/Web/HttpPayload.cpp
    ${CSP_SHARED_SOURCE_DIR}/Web/HttpProgress.cpp
    ${CSP_SHARED_SOURCE_DIR}/Web/HttpRequest.cpp
    ${CSP_SHARED_SOURCE_DIR}/Web/HttpResponse.cpp
    ${CSP_SHARED_SOURCE_DIR}/Web/Json.cpp
    ${CSP_SHARED_SOURCE_DIR}/Web/Uri.cpp
    ${CSP_SHARED_SOURCE_DIR}/Web/WebClient.cpp

    ${CSP_SHARED_SOURCE_DIR}/Web/EmscriptenWebClient/EmscriptenWebClient.cpp

    ${CSP_SHARED_SOURCE_DIR}/Web/POCOWebClient/POCOWebClient.cpp

)

set(CSP_SHARED_PRIVATE_INCLUDES 
    ${CSP_SHARED_SOURCE_DIR}/Convert.h
    ${CSP_SHARED_SOURCE_DIR}/DateTime.h
    ${CSP_SHARED_SOURCE_DIR}/Encode.h
    ${CSP_SHARED_SOURCE_DIR}/Logger.h
    ${CSP_SHARED_SOURCE_DIR}/NumberFormatter.h
    ${CSP_SHARED_SOURCE_DIR}/Queue.h
    ${CSP_SHARED_SOURCE_DIR}/Scheduler.h
    ${CSP_SHARED_SOURCE_DIR}/ThreadPool.h
    ${CSP_SHARED_SOURCE_DIR}/UUIDGenerator.h
    ${CSP_SHARED_SOURCE_DIR}/Wrappers.h

    ${CSP_SHARED_SOURCE_DIR}/Web/HttpAuth.h
    ${CSP_SHARED_SOURCE_DIR}/Web/HttpPayload.h
    ${CSP_SHARED_SOURCE_DIR}/Web/HttpProgress.h
    ${CSP_SHARED_SOURCE_DIR}/Web/HttpRequest.h
    ${CSP_SHARED_SOURCE_DIR}/Web/HttpResponse.h
    ${CSP_SHARED_SOURCE_DIR}/Web/Json.h
    ${CSP_SHARED_SOURCE_DIR}/Web/Json_HttpPayload.h
    ${CSP_SHARED_SOURCE_DIR}/Web/Uri.h
    ${CSP_SHARED_SOURCE_DIR}/Web/WebClient.h

    ${CSP_SHARED_SOURCE_DIR}/Web/EmscriptenWebClient/EmscriptenWebClient.h

    ${CSP_SHARED_SOURCE_DIR}/Web/POCOWebClient/POCOWebClient.h
)