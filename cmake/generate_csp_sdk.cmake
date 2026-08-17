# This file is responsible for generating a CSP SDK, meaning a redistributable statically linked package that includes all link dependencies
# In order for this to work, the following needs to be provided:
#
# - CSP_GENERATE_SDK - Set to On, to generate the sdk.
# - CSP_SDK_DEPENDENCIES_DIR - A staging directory for third party dependencies containing the following:
#   - sdk-dependencies/
#   -   lib/ - Directory containing all third-party libs to link
#   -   include/ - Includes for all csp public dependencies
#   -   CSPThirdPartyDependencies.cmake - A file defining the following CMakeVariabes:
#           - CSP_THIRD_PARTY_DEPENDENCY_LIBRARY_FILENAMES - A list containing all CSP third party library names to package 
#           - CSP_THIRD_PARTY_PUBLIC_INCLUDE_DIRECTORIES - A list of all csp public third-party include header directories
#
#   We use a custom conan deployer to generate this directory structure. This can be used by passing:
#    --deployer=./conan/csp_deployer.py --deployer-folder=build/windows-debug-static/sdk-dependencies
#
#   You can generate this directory manually if you aren't using conan.
#
# - CSP_SDK_CONFIGURATION - Used to determine directory and file names for installation. [Debug, Release, RelWithDebInfo]
#   This only needs to be set if using a multi-config generator.
#   This is because CMAKE_BUILD_TYPE is set at configure time for single-config generators, so we can use this to determine the correct config.

if(NOT CSP_GENERATE_SDK)
    message(FATAL_ERROR
        "Cannot run generate_csp_sdk file when CSP_GENERATE_SDK is not set."
    )
endif()

# Validate inputs ----------------------------------------------------------------------------------------------------------------------------

# Ensures CSP_SDK_DEPENDENCIES_DIR is set and that it is a directory.
set(
    CSP_SDK_DEPENDENCIES_DIR
    ""
    CACHE PATH
    "The generated third-party staging directory"
)

if(NOT CSP_SDK_DEPENDENCIES_DIR)
    message(FATAL_ERROR
        "CSP_SDK_DEPENDENCIES_DIR must point to the third-party dependency directory"
    )
endif()

if(NOT IS_DIRECTORY "${CSP_SDK_DEPENDENCIES_DIR}")
    message(FATAL_ERROR
        "CSP SDK dependency directory does not exist:\n"
        "  ${CSP_SDK_DEPENDENCIES_DIR}"
    )
endif()

# Ensures CSP_SDK_CONFIGURATION is set.
# This will set it automatically for single-config generators using CMAKE_BUILD_TYPE
set(
    CSP_SDK_CONFIGURATION
    ""
    CACHE STRING
    "Configuration contained in this SDK"
)

# Ensure CMake sdk config is valid
if(NOT CSP_SDK_CONFIGURATION)
    # Get the CSP_SDK_CONFIGURATION from the CMAKE_BUILD_TYPE.
    # This will only be set at configure time for single config generators using CMAKE_BUILD_TYPE.
    if(CMAKE_BUILD_TYPE)
        set(
            CSP_SDK_CONFIGURATION
            "${CMAKE_BUILD_TYPE}"
            CACHE STRING
            "Configuration contained in this SDK"
            FORCE
        )
    else()
        message(FATAL_ERROR
            "CSP_SDK_CONFIGURATION must be specified when generating "
            "an SDK with a multi-config generator."
        )
    endif()
endif()

# This step validates the provided CSPThirdPartyDependencies file
set(
    CSP_THIRD_PARTY_DEPENDENCY_MANIFEST
    "${CSP_SDK_DEPENDENCIES_DIR}/CSPThirdPartyDependencies.cmake"
)

# Ensure the manifest file exists.
if(NOT EXISTS "${CSP_THIRD_PARTY_DEPENDENCY_MANIFEST}")
    message(FATAL_ERROR
        "CSP third-party dependency manifest was not found:\n"
        "  ${CSP_THIRD_PARTY_DEPENDENCY_MANIFEST}\n"
        "CSP_SDK_DEPENDENCIES_DIR must point to the third-party dependency directory"
    )
endif()

include("${CSP_THIRD_PARTY_DEPENDENCY_MANIFEST}")

# Ensures CSP_THIRD_PARTY_DEPENDENCY_LIBRARY_FILENAMES exists.
# This should be populated from CSPThirdPartyDependencies.cmake included inside CSP_SDK_DEPENDENCIES_DIR
if(NOT CSP_THIRD_PARTY_DEPENDENCY_LIBRARY_FILENAMES)
    message(FATAL_ERROR
        "The manifest did not define any dependency libraries.\n"
        "Expected CSP_THIRD_PARTY_DEPENDENCY_LIBRARY_FILENAMES to be populated."
    )
endif()

# Ensure the root lib directory exists
set(
    _csp_deployed_library_directory
    "${CSP_SDK_DEPENDENCIES_DIR}/lib"
)

if(NOT IS_DIRECTORY "${_csp_deployed_library_directory}")
    message(FATAL_ERROR
        "The third-party /lib directory does not exist:\n"
        "  ${_csp_deployed_library_directory}"
    )
endif()

# Ensure a lib directory exists for every lib specified in CSPThirdPartyDependencies.cmake
foreach(
    _library_filename
    IN LISTS CSP_THIRD_PARTY_DEPENDENCY_LIBRARY_FILENAMES
)
    set(
        _library_path
        "${_csp_deployed_library_directory}/${_library_filename}"
    )

    if(NOT EXISTS "${_library_path}")
        message(FATAL_ERROR
            "A library listed in CSPThirdPartyDependencies.cmake "
            "was not found:\n"
            "  ${_library_path}"
        )
    endif()
endforeach()

# Get lib names for vendored libs -----------------------------------------------------------------------------------------------------------

# This gets the lib base name and optionally appends _D for debug builds so the correct lib is found.
# e.g csp-signalr_D, csp-quickjs_D
get_target_property(
    CSP_SIGNALR_OUTPUT_NAME
    csp-signalr
    OUTPUT_NAME
)

if(NOT CSP_SIGNALR_OUTPUT_NAME)
    set(CSP_SIGNALR_OUTPUT_NAME "csp-signalr")
endif()

get_target_property(
    CSP_QUICKJS_OUTPUT_NAME
    csp-quickjs
    OUTPUT_NAME
)

if(NOT CSP_QUICKJS_OUTPUT_NAME)
    set(CSP_QUICKJS_OUTPUT_NAME "csp-quickjs")
endif()

get_target_property(
    CSP_SIGNALR_DEBUG_POSTFIX
    csp-signalr
    DEBUG_POSTFIX
)

if(NOT CSP_SIGNALR_DEBUG_POSTFIX)
    set(CSP_SIGNALR_DEBUG_POSTFIX "")
endif()

get_target_property(
    CSP_QUICKJS_DEBUG_POSTFIX
    csp-quickjs
    DEBUG_POSTFIX
)

if(NOT CSP_QUICKJS_DEBUG_POSTFIX)
    set(CSP_QUICKJS_DEBUG_POSTFIX "")
endif()

if(CSP_SDK_CONFIGURATION STREQUAL "Debug")
    set(_csp_signalr_postfix "${CSP_SIGNALR_DEBUG_POSTFIX}")
    set(_csp_quickjs_postfix "${CSP_QUICKJS_DEBUG_POSTFIX}")
else()
    set(_csp_signalr_postfix "")
    set(_csp_quickjs_postfix "")
endif()

set(
    CSP_SIGNALR_LIBRARY_FILENAME
    "${CMAKE_STATIC_LIBRARY_PREFIX}${CSP_SIGNALR_OUTPUT_NAME}${_csp_signalr_postfix}${CMAKE_STATIC_LIBRARY_SUFFIX}"
)
set(
    CSP_QUICKJS_LIBRARY_FILENAME
    "${CMAKE_STATIC_LIBRARY_PREFIX}${CSP_QUICKJS_OUTPUT_NAME}${_csp_quickjs_postfix}${CMAKE_STATIC_LIBRARY_SUFFIX}"
)

# Copy files into final sdk package ----------------------------------------------------------------------------------------------------------

# Install lib, third-party and licenses directories from the provided CSP_SDK_DEPENDENCIES_DIR
install(
    DIRECTORY
        "${CSP_SDK_DEPENDENCIES_DIR}/lib/"
    DESTINATION
        "${CMAKE_INSTALL_LIBDIR}/third-party"
)
install(
    DIRECTORY
        "${CSP_SDK_DEPENDENCIES_DIR}/include/"
    DESTINATION
        "${CMAKE_INSTALL_INCLUDEDIR}/third-party"
    OPTIONAL
)
install(
    DIRECTORY
        "${CSP_SDK_DEPENDENCIES_DIR}/licenses/"
    DESTINATION
        "licenses/third-party"
    OPTIONAL
)

# Configure CSPStaticDependencies.cmake.in with defined CMake vars and install
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CSPStaticDependencies.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CSPStaticDependencies.cmake"
    @ONLY
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/CSPStaticDependencies.cmake"
    DESTINATION
        "${CSP_CONFIG_INSTALL_DIR}"
)

# Generate link manifest file.
# This makes it easier for clients to know what libs to link.
set(_csp_manual_link_manifest "")

string(
    APPEND
    _csp_manual_link_manifest
    "$<TARGET_FILE_NAME:csp-lib>\n"
    "$<TARGET_FILE_NAME:csp-signalr>\n"
    "$<TARGET_FILE_NAME:csp-quickjs>\n"
)

foreach(
    _library
    IN LISTS CSP_THIRD_PARTY_DEPENDENCY_LIBRARY_FILENAMES
)
    string(
        APPEND
        _csp_manual_link_manifest
        "${_library}\n"
    )
endforeach()

# Add platform specific lib files that need to be linked.
if(WIN32)
    string(
        APPEND
        _csp_manual_link_manifest
        "ws2_32.lib\n"
        "iphlpapi.lib\n"
        "crypt32.lib\n"
        "bcrypt.lib\n"
    )
endif()

# Generate and copy manifest file into sdk package
file(
    GENERATE
    OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/link/$<CONFIG>/libraries.txt"
    CONTENT
        "${_csp_manual_link_manifest}"
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/link/${CSP_SDK_CONFIGURATION}/libraries.txt"
    DESTINATION
        "link"
    CONFIGURATIONS
        "${CSP_SDK_CONFIGURATION}"
)