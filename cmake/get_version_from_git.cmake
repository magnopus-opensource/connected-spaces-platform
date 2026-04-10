# Populates properties used to generate version.h
function(get_version_from_git)
    # Ensure we can find git
    find_package(Git QUIET)
    if(NOT Git_FOUND)
        message(WARNING "Git not found")
        return()
    endif()

    # Get last commit hash
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE LAST_COMMIT_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Populate last commit hash
    set(CSP_COMMIT_ID ${LAST_COMMIT_ID})
    set(CSP_COMMIT_ID ${LAST_COMMIT_ID} PARENT_SCOPE)

    # Generate build id
    string(TIMESTAMP BUILD_TIME "%y%m%d_%H%M%S" UTC)
    set(CSP_BUILD_ID "${BUILD_TIME}_${CSP_COMMIT_ID}")

    option(IS_CI_BUILD "Is this a CI build?" OFF)

    if(NOT IS_CI_BUILD)
        set(CSP_BUILD_ID "${CSP_BUILD_ID}/PERSONAL")
    endif()

    set(CSP_BUILD_ID "${CSP_BUILD_ID}" PARENT_SCOPE)

    # Get version tag
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE GIT_RESULT
    )

    if(NOT GIT_RESULT EQUAL 0)
        message(WARNING "Failed to get git tag")
        return()
    endif()

    # Convert to correct format
    string(REGEX REPLACE "^v" "" CLEAN_TAG "${GIT_TAG}")
    if(CLEAN_TAG MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-.*)?$")

        set(CSP_VERSION_MAJOR ${CMAKE_MATCH_1})
        set(CSP_VERSION_MAJOR ${CMAKE_MATCH_1} PARENT_SCOPE)
        set(CSP_VERSION_MINOR ${CMAKE_MATCH_2})
        set(CSP_VERSION_MINOR ${CMAKE_MATCH_2} PARENT_SCOPE)
        set(CSP_VERSION_PATCH ${CMAKE_MATCH_3})
        set(CSP_VERSION_PATCH ${CMAKE_MATCH_3} PARENT_SCOPE)

        set(CSP_VERSION "${CSP_VERSION_MAJOR}.${CSP_VERSION_MINOR}.${CSP_VERSION_PATCH}") 
        set(CSP_VERSION "${CSP_VERSION_MAJOR}.${CSP_VERSION_MINOR}.${CSP_VERSION_PATCH}" PARENT_SCOPE)
    else()
        message(WARNING "Tag '${CLEAN_TAG}' does not match semver format")
    endif()
   
endfunction()