@echo off

REM *** Command Line Arguments *** 
REM [1] - Configuration - release || debug
REM [2] - Optional, append withNode to build with Node support, example ./EmscriptenFullBuildAndConfigure debug withNode

git config core.hooksPath .githooks
git config commit.template .githooks/commit-template.txt

IF NOT EXIST "../../modules/premake/README.md" (
	git submodule update --recursive
)

IF NOT EXIST "../../modules/premake/bin/release/premake5.exe" (
	cd ..\..\modules\premake
	call ..\..\teamcity\BootstrapPremake.bat 2022
	cd ..\..\Tools\Emscripten
)

for /f "delims=" %%x in (emsdk_version.txt) do (
    set emsdk_version=%%x
    goto :Build
)

:Build

cd ../..
del Makefile

if "%~1"=="" (goto ArgNone) else (goto ArgCheck)

:ArgNone
echo No configuration name provided, please provide "debug" or "release" after ./EmscriptenFullBuildAndConfigure
exit /b 1

:ArgCheck
for %%a in ("debug" "release") do if "%~1"==%%a (goto ArgOk)
echo Unsupported configuration "%~1". Supported configurations are "debug" and "release". Please try again.
exit /b 1

:ArgOk
REM withNode is an optional arg
set "WITH_NODE=0"
if /i "%~2"=="withNode" set "WITH_NODE=1"

if "%WITH_NODE%"=="1" (
    echo Building with Node.js support
    "modules/premake/bin/release/premake5" gmake2 --generate_wasm --wasm_with_node
) else (
    echo Building without Node.js support
    "modules/premake/bin/release/premake5" gmake2 --generate_wasm
)
goto Wasm

:Wasm
python Tools/Emscripten/ReplaceComSpec.py
if %ERRORLEVEL% NEQ 0 (goto Error)

docker run -w /src -v %cd%:/src --rm emscripten/emsdk:%emsdk_version% emmake make config=%~1_wasm clean
if %ERRORLEVEL% NEQ 0 (goto DockerError)

docker run -w /src -v %cd%:/src --rm emscripten/emsdk:%emsdk_version% emmake make -j 8 config=%~1_wasm
if %ERRORLEVEL% NEQ 0 (goto DockerError)

python .\teamcity\GenerateReadMeWithLink.py
if %ERRORLEVEL% NEQ 0 (goto Error)

python .\teamcity\BuildNPMWebPackage.py --npm_publish_flag=False
if %ERRORLEVEL% NEQ 0 (goto Error) else (goto Success)

:DockerError
cd Tools\Emscripten
echo ERROR: Build failed. Have you started the docker engine?
exit /b 1

:Error
cd Tools\Emscripten
echo ERROR: Build failed: failed to generate package.
exit /b 1

:Success
cd Tools\Emscripten
echo Success! Build located in: ..\Library\Binaries\package\wasm\connected-spaces-platform.web
exit /b 0