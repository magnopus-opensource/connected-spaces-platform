# CSP Build System

> ⚠️ **Warning:** This build system is still in development. We do not recommend using it for production or active development.

This guide explains how to configure and build CSP for supported platforms.

---

## Prerequisites

Ensure the following tools are installed and available in your `PATH`:

* CMake (version 3.31 or higher)
  https://cmake.org/download/
* Conan (version 2.x)
  https://docs.conan.io/2/installation.html

---

## Choosing a Build Environment

Before running any commands, decide whether to use a **single-config** or **multi-config** generator:

* **Single-config generators**
  Build one configuration at a time (e.g. `Debug` *or* `Release`).
  Recommended for command-line workflows.

* **Multi-config generators**
  Support multiple configurations in the same build (e.g. `Debug` *and* `Release`).
  Recommended when using an IDE (e.g. Visual Studio, Xcode).

---

## Installing Dependencies

Install project dependencies using Conan:

```sh
conan install . -s build_type=<CONFIG> --build=missing --profile:host=profiles/host/<PROFILE>
```

### Parameters

* `<CONFIG>`
  Build configuration: `Debug` or `Release`

* `<PROFILE>`
  Target platform profile (see `profiles/host/`)

### Notes

* When switching between different profiles, delete the generated `ConanPresets.json` file in the repository root.
  Otherwise, you may encounter conflicting preset errors.
* If using a **multi-config generator**, install dependencies for **both `Debug` and `Release`**.
* If using the **Xcode generator**, you must explicitly set it:

```sh
-c tools.cmake.cmaketoolchain:generator=Xcode
```

Failing to do so may result in incorrect directory layouts.

---

## Configuring the Build

CMake presets are used to configure the build system.

List available presets with:

```sh
cmake --list-presets
```

### Steps

1. Choose an appropriate preset

   * Use a **multi-config preset** if you selected a multi-config generator

2. Configure the project:

```sh
cmake --preset <PRESET>
```

After this step, the build system will be generated in the `build/` directory.

### Notes

* Tests are **not supported when building shared libraries**
  → Use a **static build** if you need to run tests
* If configuring for Emscripten, be sure to append:

```sh
 -DBUILD_TESTING=OFF
```

Failing to do so will cause build errors due to Emscripten not supporting dependencies used for testing.

---

## Custom CMake Properties

* If not using presets, our custom cmake variables can be used directly:

| Property           | Type | Description                                                                 |
|------------------|------|-----------------------------------------------------------------------------|
| CSP_BUILD_SHARED | BOOL | Builds CSP as a shared library.             |
| BUILD_TESTING    | BOOL | Enables or disables building of test targets.This should be set to false for shared builds.       |

## Building

You can build the project using CMake build presets.

List available build presets:

```sh
cmake --build --list-presets
```

### Steps

1. Select a build preset

   * Ensure it matches your generator type (single vs multi-config)

2. Build the project:

```sh
cmake --build --preset <PRESET>
```

---

## Build Targets

| Target                 | Type        | Description                                                                 |
|----------------------|------------|-----------------------------------------------------------------------------|
| csp-lib              | Library     | Core CSP library. Built as either static or shared depending on `CSP_BUILD_SHARED`. |
| csp-tests            | Executable  | Test suite for CSP. Only available when `BUILD_TESTING=ON` and using a static build. |
| MultiplayerTestRunner| Executable  | Utility for running multiplayer test scenarios. Used by `csp-tests` via inter-process communication. |
| csp-signalr          | Library     | Custom SignalR library used for multiplayer connections. Linked statically into CSP. |
| csp-quickjs          | Library     | Custom QuickJS library used for scripting. Linked statically into CSP.      |

## Output

 A CSP binary will be output in the build folder.

## Installing

After building the project, you can install it using:

```sh
cmake --install <BUILD_DIR> --config <CONFIG> --prefix <INSTALL_DIR>
```

### Parameters

* `<BUILD_DIR>`
  Path to the build directory

* `<CONFIG>`
  Build configuration: `Debug` or `Release`

* `<INSTALL_DIR>`
 Destination directory for the installation

This will install the CSP library, including: headers, and configured runtime binaries.