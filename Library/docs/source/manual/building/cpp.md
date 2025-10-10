# Building CSP for Windows

This page details how to build CSP and its associated C++ DLLs locally for Windows users. Note that the CSP Solution targets Visual Studio 2022 by default, so we will be using this IDE for this walkthrough.

## Prerequisites

This section details the step 2 of the [Build Instructions section of this tutorial](#build-instructions).

### Default Packages

Please run the install script `install_prerequisites.ps1` in PowerShell as administrator.
This script will install the Windows Package Manager **Chocolatey** as well as the following **packages**:
- git
- vscode
- python3
- llvm
- docker-desktop
- cmake

### Clang

The Clang subproject is part of the LLVM system that have been installed as a package previously. Once the prerequisites above have been installed, please set your custom install path for **clang-format** in Visual Studio. 
- Go to `Tools > Options > Text Editor > C/C++ > Code Style > Formatting`.
- Check the last option `Use custom path to clang-format.exe`, then click `Browse` and search for the LLVM executable (should be located at  `C:\Users\username\AppData\Local\Temp\chocolatey\llvm\17.0.1\LLVM-17.0.1-win64.exe` by default).

### SDK versions

Additionally, you will need three specific Individual Components from the Visual Studio Installer, including two **SDKs**:
- MSVC v143 - VS 2022 C++ x64/x86 build tools (Latest) - required to compile the C++ code.
- Windows 10 SDK (10.0.19041.0) - required to build the C++ code.
- .NET Framework 4.7.1 SDK and targeting pack - both required to load and build the C# tests.

These SDK versions might not be available from within the Visual Studio Installer directly. If so, they will need to be downloaded from the website and installed separately:
- Windows SDK: click [here](https://developer.microsoft.com/en-us/windows/downloads/sdk-archive/) and search for version "2004", or run the Chocolatey command `choco install windows-sdk-10-version-2004-all`.
- .NET Developer Pack: click [here](https://dotnet.microsoft.com/en-us/download/visual-studio-sdks?cid=getdotnetsdk) and search for version "4.7.1", or run the Chocolatey command `choco install netfx-4.7.1`.

### Running Docker Desktop with WSL2

**Docker Desktop** is known to have issues with WSL, so some further steps are needed for it to function as expected:
1. Open PowerShell as Administrator, and install WSL2 by running the command `wsl --install`.
2. Download the WSL2 Linux kernel update package for x64 machines [here](https://wslstorestorage.blob.core.windows.net/wslblob/wsl_update_x64.msi).
3. Follow the installation guide.
3. Restart your computer.

***

## Build Instructions
Follow the steps below to build the **Connected Spaces Platform** project:

1. Clone the `connected-spaces-platform` remote repository by running the Git command `git clone --recurse-submodules https://github.com/magnopus-opensource/connected-spaces-platform.git`.

2. Make sure you have installed all the prerequisites by following [the Prerequisites section of this tutorial](#prerequisites-windows-only), if you haven't already.

    > If you would like to run Docker Desktop using WSL, please make sure sure you followed [this subsection of the tutorial](#running-docker-desktop-with-wsl2).

3. Open a Command Line Prompt from your `connected-spaces-platform` local repository, and run `generate_solution.bat` to generate the CSP Solution.

    > If you get an error "premake not found", this is only the first module built, so you will need to checkout the submodules by running the Git command `git submodule update --init --recursive`.\
    You may also need to build premake manually to get past this step, by opening `"..\modules\premake\build\bootstrap\Premake5.sln"` in Visual Studio and building the Solution.

4. Double-click `ConnectedSpacesPlatform.sln` to open up the CSP Solution.
    
    > This will take some time, but once loaded, it should look like this:\
    ![image info](../../_static/building/cpp_sln.png)

5. Before building the Solution by clicking on the Green Arrow, ensure you have:
    - `ReleaseDLL` option as your Solution Configurations,
    - `x64` as your Solution Platforms.
    - `Tests` project selected as your Startup Project,
    
    > ![image info](../../_static/building/cpp_cfg.png)

***

## Build Output

Once CSP has finished building, you will find the **resulting DLL** at `connected-spaces-platform\Library\Binaries\x64\ReleaseDLL`.
