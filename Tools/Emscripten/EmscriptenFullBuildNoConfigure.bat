@echo off

REM *** Command Line Arguments *** 
REM [1] - Configuration - release || debug

for /f "delims=" %%x in (emsdk_version.txt) do (
    set emsdk_version=%%x
    goto :Build
)

:Build

cd ../..

if "%~1"=="" (goto ArgError) else (goto ArgOk)

:ArgError
echo No configuration name provided
exit /b 1

:ArgOk
docker run -w /src -v %cd%:/src --rm emscripten/emsdk:%emsdk_version% emmake make -j 8 config=%~1_wasm
if %ERRORLEVEL% NEQ 0 (goto Error)
xcopy /y /s /e Library\Binaries\WASM\%~1\ Tools\WrapperGenerator\Output\TypeScript\connected-spaces-platform.web\%~1\
if %ERRORLEVEL% NEQ 0 (goto Error) else (goto Success)

:Error
cd Tools/Emscripten
echo ERROR: Build failed. Have you started the docker engine?
exit /b 1

:Success
cd Tools/Emscripten
echo Success! Build located in: ..\WrapperGenerator\Output\TypeScript\connected-spaces-platform.web
exit /b 0