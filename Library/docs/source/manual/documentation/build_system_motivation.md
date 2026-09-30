# Build System Architecture

## Motivation

CSP is a cross-platform C++ library which uses multiple dependencies, and supports multiple platforms, including desktop, mobile, and wasm.

Our previous build system made it difficult to manage these things easily. We needed a build system that could:

- Support multiple compilers, platforms, and SDKs.
- Handle cross-compilation tools e.g Android NDK and Emscripten.
- Manage a large number of third-party dependencies.
- Manage dependency versions across platforms.
- Integrate easily with clients.

To fufill these requirements, we moved to a build system using **Conan** and **CMake**.

---

## CMake

CMake was chosen because it is widely adopted across the C++ ecosystem. This allows CSP to easily support third-party dependencies and gives consumers a standard way to include CSP in their own build systems.

This is important for us because the library is consumed by a various applications and engines, while also supporting multiple third-party dependencies.

In our previous Premake build system, we had to maintain custom build scripts for our third-party dependencies. This involved copying build scripts, source files, and prebuilt binaries into our repository, with local modifications.
Because of this, dependencies became harder to track and maintain, and we often lost track of the upstream version being used.

CSP was also difficult to consume from other CMake projects. Instead of using the standard CMake package interface, consumers had to manually create CMake targets and wire up CSP's libraries and include paths themselves.

## Conan

Conan was chosen because CSP has a alot of dependencies and needs to support multiple platforms, compilers, and toolchains.

Keeping environment configuration and dependency retrieval separate from the core build logic makes the build system easier to understand, maintain, and extend. It allows platform and dependency concerns to be handled seperatly from how CSP is built.

Conan profiles allow us to seperate the enviornment and toolchain configuration seperate, while our Conan package file allows us to seperate dependency configurations.

## Further reading

To learn how to use the build system, a document is available [here](../building/build-system.md).
