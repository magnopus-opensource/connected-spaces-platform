# Building CSP for C++

This page details how to Build foundation locally and build the C++ DLLs.

## Prerequisites 

The CSP solution targets Visual Studio 2022 by default.

Windows: Please run the install script "install_prerequisites.ps1" in PowerShell as administrator.
This script will install the Windows Package Manager Chocolatey and the following packages:
 - git
 - vscode
 - python3
 - llvm
 - docker-desktop
 - cmake

***

Once the prerequisites have been installed, please set your custom install path for **clang-format** in Visual Studio.
This can be set via the Options menu here - Text Editor > C/C++ > Code Style > Formatting.
Check the last option for `Use custom path to clang-format.exe` and then click browse to locate the executable in your llvm installation folder.

Additionally, you will also need two specific SDK versions:
* Windows SDK version 1809 (10.0.17763.0) - Required to build the C++ code.

* .Net Developer Pack 4.7.1 - required to load and build the C# tests.

These SDK versions are not available from within VS 2022 and will need to be downloaded directly:

Windows SDK: https://developer.microsoft.com/en-us/windows/downloads/sdk-archive/ search the page for "1809"

or use chocolatey: `choco install windows-sdk-10-version-1809-all`

.Net developer pack: https://dotnet.microsoft.com/en-us/download/visual-studio-sdks?cid=getdotnetsdk search the page for "4.7.1"

or use chocolatey: `choco install netfx-4.7.1`

You will also need to install the MSVC build tools v142 as an individual component in the Visual Studio Installer.

## Running docker-desktop with WSL2 (Windows)
Windows: Docker-Desktop is known to have issues with WSL so some further steps are needed to function as expected
1. Open PowerShell as Administrator and run `wsl --install` This will install WSL2
2. Download WSL2 kernel patch [WSL2 Linux kernel update package for x64 machines](https://wslstorestorage.blob.core.windows.net/wslblob/wsl_update_x64.msi)
3. Follow installation guide.
3. Restart your computer

***

## Build Instructions
Build instructions for the Connected Spaces Platform project are below.
First of all you'll need to run the following script:
1. Clone the Connected Spaces Platform Repositiory `git clone --recurse-submodules https://github.com/magnopus-opensource/connected-spaces-platform.git`.
2. Open PowerShell as Administrator and run `install_prerequisites.ps1` to install Prerequisites if you haven't already.
> If you would like to run docker-desktop using wsl please make sure sure you follow **Running docker-desktop with WSL2** above .
3. Open Command Line and run `generate_solution.bat` to generate the Foundation solution.
 > If you get an error with premake not found, this is the first module built so you'll need to checkout the submodules with `git submodule update --init --recursive`.
 > You may need to build premake manually to get past this step, by opening `"..\modules\premake\build\bootstrap\Premake5.sln"` in Visual Studio and building the solution.
4. Double click `ConnectedSpacesPlatform.sln` to open up the Foundation Project in your chosen IDE (For this walk through we will be using Rider).
>  This will take some time but its loaded it should look like this:

> ![image info](../../_static/building/cpp_sln.png)

5. Ensuring you have the `Tests` projected selected as your Startup Project, select the `ReleaseDLL` option in the build configuration settings alongside `x64` click the **Green Hammer** to start the build.
> ![image info](../../_static/building/cpp_cfg.png)

## Where is the file output?

Once Foundation has finished building you will find the DLL has been generated in `connected-spaces-platform\Library\Binaries\x64\ReleaseDLL`.



