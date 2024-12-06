# Building CSP for Web

## Prerequisites
* git
* vscode
* python3
* llvm
* docker-desktop (must be started as Administrator)
* cmake

## Running the build

To build with emscripten on Windows, simply run the bat file contained within `Tools/Emscripten`.

These bat files also require an argument provided when running them, either _debug_ or _release_.

EmscriptenFullBuildAndConfigure.bat will run generate_solution.bat and Premake. 

e.g. EmscriptenFullBuildAndConfigure.bat release

To run the Emscripten server locally call the python script: `Tools/Emscripten/StartEmscriptenServer.py`. 

## Where is the file output?

After the build is successful the local files will appear in `Tools/WrapperGenerator/Output/TypeScript`.

You will also need to copy the Debug and/or Release folder from `Library/Binaries/wasm`.