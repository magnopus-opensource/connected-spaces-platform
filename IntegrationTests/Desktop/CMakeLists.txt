cmake_minimum_required(VERSION 3.15)
project(integration_test LANGUAGES CXX)

enable_testing()

find_package(CSP CONFIG REQUIRED)

add_executable(integration_test Main.cpp)

target_link_libraries(integration_test PRIVATE CSP::csp-lib)

add_test(NAME integration_test COMMAND $<TARGET_FILE:integration_test>)