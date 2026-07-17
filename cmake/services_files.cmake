# Contains our internal REST API services files that come from https://github.com/magnopus-opensource/csp-services.git.

set(CSP_SERVICES_DIR ${csp-services_SOURCE_DIR}/generated)

# We glob only in the services file due to the way we consume them.
# Whenever a new version is released, we will need to update the version in the parent CMake file
# which will cause cmake to reconfigure without needting to touch the files.
file(GLOB_RECURSE CSP_SERVICES_SOURCES CONFIGURE_DEPENDS
    ${CSP_SERVICES_DIR}/*.h
    ${CSP_SERVICES_DIR}/*.cpp
)