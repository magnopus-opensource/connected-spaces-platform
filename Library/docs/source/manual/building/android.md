# Building CSP for Android

This page details how to Build foundation locally and build the Android DLLs.

## Prerequisites 


Windows: Please run the install script "install_prerequisites.ps1" in PowerShell as administrator.
This script will install the Windows Package Manager Chocolatey and the following packages:
 - git
 - vscode
 - python3
 - llvm
 - docker-desktop
 - cmake

***

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
4. Double click `OlympusFoundation.sln` to open up the Foundation Project in your chosen ide (For this walk through we will be using Rider).
>  This will take some time but its loaded it should look like this:

> ![image info](../../_static/building/android_sln.png)

5. Select the `ReleaseDLL` option in the build configuration settings alongside `x64` click the **Green Hammer** to start the build.
> ![image info](../../_static/building/android_cfg.png)

## Where is the file output?

Once Foundation has finished building you will find the DLL has been generated in `connected-spaces-platform\Library\Binaries\Android\ReleaseDLL`.