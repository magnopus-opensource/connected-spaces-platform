set(CSP_SERVICES_DIR ${csp-services_SOURCE_DIR}/generated)

file(GLOB_RECURSE CSP_SERVICES_SOURCES CONFIGURE_DEPENDS
    ${CSP_SERVICES_DIR}/*.h
    ${CSP_SERVICES_DIR}/*.cpp
)

# Private
target_sources(csp-lib PRIVATE
    ${CSP_SERVICES_SOURCES}
)