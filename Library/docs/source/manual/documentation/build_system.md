# Build System Architecture

The CSP build system is based around CMake, which handles configuration, compilation, linking, testing, and installation.

Conan is recommended for dependency management, toolchain configuration, and packaging of third-party libraries. We provide Conan profiles and custom tooling to make these workflows easier, but Conan is not required by the CMake build system.

## High-Level Architecture

When using Conan for dependency management, the typical build system flow is:

```
Conan Profile
      │
      ▼
 conan install
      │
      ├── Resolve third-party dependencies
      │
      └── Generate CMake toolchain
              │
              ▼
         CMake configure
              │
              ▼
          CMake build
              │
              ▼
         CMake install
```

## Conan

Our Conan recipe declares the external packages required by CSP, such as:

```
RapidJSON
fmt
Async++
msgpack-cxx
tinyspline
GLM
Poco
```

Platform-specific dependencies are also handled here. For example, Poco is not used for Emscripten builds.

The recipe also configures individual dependencies to work with CSP correctly, such as:

```
Disabling unused Poco components.
Disabling Boost support in msgpack-cxx.
Disabling the tinyspline C++ interface.
Forcing dependencies to use static linkage.
Enabling position-independent code.
```

This means the majority of dependency specific knowledge belongs in Conan rather than being duplicated throughout the CMake project.

## Conan-Generated CMake Integration

The recipe uses two Conan generators:

```
CMakeDeps
CMakeToolchain
```

CMakeDeps generates the CMake package information required so we can find dependencies like so:

```find_package(Poco REQUIRED)```

CMakeToolchain takes the selected Conan configuration and creates a CMake toolchain.

The way to understand how these systems inetract is:

> Conan determines what dependencies and build environment CMake sees. CMake determines how CSP is built.


## Conan Profiles

Profiles define the build environment for each supported platform and toolchain combination.

Current profiles include targets such as:

```
android_arm
android_x86_64
emscripten
ios_arm64
linux_clang
linux_gcc
linux_unreal_56
osx_arm
visionos_arm64
windows_msvc
windows_ninja
```

A profile provides configuration such as:

```
Operating system
Architecture
Compiler
Compiler version
Runtime
C++ standard
CMake generator
Platform-specific toolchain settings
```

This means our conanfile does not need to contain a large amount of platform-specific compiler configuration. A simple way to understand this is:

> Profiles describe the target environment. The recipe describes the dependency graph.

## CMake

CMake owns the actual CSP build graph.

Once Conan has made its dependencies available, CMake is responsible for:

```
Locating dependencies.
Creating the CSP library.
Building CSP's vendored and internal libraries.
Applying compiler and platform-specific build options.
Building tests.
Installing headers and binaries.
Generating CSP's CMake package configuration.
```

Vendored CSP dependencies such as SignalR and QuickJS are included using ```add_subdirectory()```.

Other source dependencies, such as csp-services, are brought into the build using FetchContent.

The main CSP target is:

```csp-lib```

It can be built as either:

```STATIC```

or:

```SHARED```

depending on the value of:

```CSP_BUILD_SHARED```

## Platform Abstraction

The build system uses compiler expressions rather than creating multiple build projects for each platform.

Compiler-specific behaviour is selected for compilers such as:

```
MSVC
GNU
Clang
AppleClang
```

Platform-specific behaviour is selected for platforms such as:

```
Windows
Android
Darwin
Emscripten
```

This allows the same CMake target graph to be used across all supported platforms.

## Third-Party Dependency Packaging

CSP static builds introduce the need to link against our third-party dependencies.

When CSP is distributed as a shared library, most implementation dependencies are hidden behind the CSP shared-library boundary.

With a static library, the final application's linker will also require libraries that CSP itself depends upon.

For example:

```
Application
    │
    ▼
ConnectedSpacesPlatform.lib
    │
    ├── fmt
    ├── Poco
    ├── Async++
    ├── tinyspline
    └── ...
```

For this reason, CSP provides a custom Conan deployer that works by iterating Conan's resolved dependency graph and collects the libraries to copy into the final install package.

## Further reading

To learn how to use the build system, a document is available [here](../building/build-system.md).