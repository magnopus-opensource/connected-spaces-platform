# Building CSP for MacOS

This page details how to Build foundation locally and build the MacOS DLLs on OSX.

## Prerequisites 

Please install the following packages:

* Install [HomeBrew](https://docs.brew.sh/Installation)
* xcode (install through App Store)
 - `brew install git`
 - `brew install git-lfs`
 - `brew install --cask visual-studio-code`
 - `brew install python3`
 - `brew install llvm`
 - `brew install --cask docker`
 - `brew install cmake`
 - `pip3 install chevron`
 - `pip3 install jinja2`
 - `pip3 install gitpython`

***

## Build Instructions
Build instructions for the Connected Spaces Platform project are below.
First of all you'll need to run the following script:
1. Clone the Connected Spaces Platform Repositiory `git clone --recurse-submodules https://github.com/magnopus-opensource/connected-spaces-platform.git`.
3. Open Terminal and run `generate_solution_mac` to generate the Foundation solution.
 > If you get an error with premake not found, this is the first module built so you'll need to checkout the submodules with `git submodule update --init --recursive`.
4. Open up Terminal in the Foundation Root Folder and run :

`xcodebuild BITCODE_GENERATION_MODE=bitcode OTHER_CFLAGS="-fembed-bitcode" -configuration ReleaseDLL -project Library/ConnectedSpacesPlatform.xcodeproj`

_Note: To Build Debug DLLs replace `ReleaseDLL` with `DebugDLL` and likewise with the directory location._

## Where is the file output?

Once Foundation has finished building you will find the DLL has been generated in `connected-spaces-platform\Library\Binaries\macosx\ReleaseDLL`.