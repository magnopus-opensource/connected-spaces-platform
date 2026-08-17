# Generating the CSP Static SDK

CSP supports generating a redistributable static SDK containing CSP alongside its third-party libraries and public headers required to link against it.

This allows consumers to use the static CSP package without needing to install Conan or provide CSP's third-party dependencies.

This is optional and only enabled when `CSP_GENERATE_SDK` is set at configure time. Normal development builds do not require any of the SDK configuration variables.

## Overview

Generating the static SDK consists of two stages:

1. Stage CSP's third-party dependencies
2. Configure, build, and install CSP with SDK generation enabled.

The Conan deployer produces a staging directory containing the dependency libraries, selected public headers, licence files, and a generated dependency manifest. This directory can be created manually if Conan isn't being used.

CMake then consumes this staging directory and installs the required files into the final CSP SDK layout.

Each generated SDK contains a single platform, architecture, linkage mode, and build config.

## Requirements

The SDK can currently only be generated for static CSP builds.

The following CMake option must be enabled at configure time:

```text
CSP_GENERATE_SDK=ON
```

`CSP_BUILD_SHARED` must be disabled.

SDK generation also requires the following values:

```text
CSP_SDK_DEPENDENCIES_DIR
CSP_SDK_CONFIGURATION
```

### `CSP_SDK_DEPENDENCIES_DIR`

The path to the generated staging directory.

If using the Conan deployer, this should be the same directory as:

```text
--deployer-folder
```

### `CSP_SDK_CONFIGURATION`

The configuration being packaged into the SDK, for example:

```text
Debug
Release
RelWithDebInfo
```

If using a single-config generator, this doesn't need to be provided, as it is populated from the `CMAKE_BUILD_TYPE` internally.

For multi-config generators, it must be provided explicitly.

## Stage Third-Party Dependencies

Run Conan using the CSP SDK deployer before configuring the SDK build.

For example:

```bash
conan install . \
    -of build/conan \
    -s build_type=Release \
    --build=missing \
    --profile:host=profiles/host/<profile> \
    --deployer=./conan/csp_deployer.py \
    --deployer-folder=build/sdk-dependencies
```

The resulting staging directory contains the folllowing structure:

```text
sdk-dependencies/
├── lib/
│   ├── <third-party static libraries>
│   └── ...
├── include/
│   ├── <package>/
│   │   └── <public headers>
│   └── ...
├── licenses/
│   ├── <package>/
│   │   └── <licence and attribution files>
│   └── ...
├── CSPThirdPartyDependencies.cmake
└── csp-sdk-dependencies.json
```

## Configure CSP

Once the dependency staging directory has been generated, configure CSP with SDK generation enabled:

```bash
cmake --preset <configure-preset> \
    -DCSP_GENERATE_SDK=ON \
    -DCSP_SDK_DEPENDENCIES_DIR="<path/to/sdk/dependencies>" \
    -DCSP_SDK_CONFIGURATION=Release
```

Example:

```bash
cmake --preset windows-release-static \
    -DCSP_GENERATE_SDK=ON \
    -DCSP_SDK_DEPENDENCIES_DIR="${PWD}/build/sdk-dependencies" \
    -DCSP_SDK_CONFIGURATION=Release
```

## Build CSP

Build CSP normally using the corresponding build preset:

```bash
cmake --build --preset <build-preset>
```

Example:

```bash
cmake --build --preset release-static
```

## Install the SDK

Install the build into the final install directory:

```bash
cmake --install <build-directory> \
    --config <configuration> \
    --prefix <install-directory>
```

Example:

```bash
cmake --install build/release-static \
    --config Release \
    --prefix install/windows/release-static
```

The install step copies:

* CSP lib.
* CSP public headers.
* Vendored CSP dependencies such as `csp-signalr` and `csp-quickjs`.
* Staged third-party static libraries.
* Third-party public headers.
* Third-party licence files.
* CSP CMake package files.
* Static dependency data used by consumers.

## Installed SDK Layout

A generated SDK has the following structure:

```text
install/
├── include/
│   ├── CSP/
│   └── third-party/
│       ├── fmt/
│       ├── rapidjson/
│       └── ...
├── lib/
│   ├── ConnectedSpacesPlatform.lib
│   ├── third-party/
│   │   ├── csp/
│   │   │   ├── csp-signalr.lib
│   │   │   └── csp-quickjs.lib
│   │   ├── <third-party library>
│   │   └── ...
│   └── cmake/
│       └── CSP/
│           ├── CSPConfig.cmake
│           ├── CSPTargets.cmake
│           └── CSPStaticDependencies.cmake
├── licenses/
│   └── third-party/
│       └── ...
└── link/
    └── libraries.txt
```

## Consuming the SDK with CMake

Consumers using CMake can use `find_package` to find the CMake install output:

```cmake
find_package(CSP REQUIRED)

target_link_libraries(
    MyApp
    PRIVATE
        CSP::csp-lib
)
```

For the generated SDK, `CSP::csp-lib` exposes the bundled dependencies through the internal `CSPThirdParty::Dependencies` target.

Required platform libraries are also linked where necessary.

## Manual Linking

The SDK also generates:

```text
link/libraries.txt
```

The file includes CSP's static libraries, bundled third-party libraries, and required platform system libraries to link.