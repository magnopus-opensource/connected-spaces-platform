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
cd Tools/Emscripten