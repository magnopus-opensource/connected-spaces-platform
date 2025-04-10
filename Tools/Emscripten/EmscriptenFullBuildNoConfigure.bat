@echo off

REM *** Command Line Arguments *** 
REM [1] - Configuration - release || debug

for /f "delims=" %%x in (emsdk_version.txt) do (
    set emsdk_version=%%x
    goto :Build
)

:Build

cd ../..

if "%~1"=="" (goto ArgNone) else (goto ArgCheck)

:ArgNone
echo No configuration name provided
exit /b 1

:ArgCheck
for %%a in ("debug" "release") do if "%~1"==%%a (goto ArgOk)
echo Unsupported configuration "%~1". Supported configurations are "debug" and "release". Please try again.
exit /b 1

:ArgOk
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