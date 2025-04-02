@echo off

REM *** Command Line Arguments *** 
REM [1] - Configuration - release || debug

git config core.hooksPath .githooks
git config commit.template .githooks/commit-template.txt

IF NOT EXIST "../../modules/premake/README.md" (
	git submodule update --recursive
)

IF NOT EXIST "../../modules/premake/bin/release/premake5.exe" (
	cd ..\..\modules\premake
	call Bootstrap.bat
	cd ..\..\Tools\Emscripten
)

for /f "delims=" %%x in (emsdk_version.txt) do (
    set emsdk_version=%%x
    goto :Build
)

:Build

cd ../..
del Makefile

if "%~1"=="" (goto ArgError) else (goto ArgOk)

:ArgError
echo No configuration name provided
exit /b 1

:ArgOk
"modules/premake/bin/release/premake5" gmake2 --generate_wasm
goto Wasm

:Wasm
python Tools/Emscripten/ReplaceComSpec.py
docker run -w /src -v %cd%:/src --rm emscripten/emsdk:%emsdk_version% emmake make config=%~1_wasm clean
if %ERRORLEVEL% NEQ 0 (goto Error)
docker run -w /src -v %cd%:/src --rm emscripten/emsdk:%emsdk_version% emmake make -j 8 config=%~1_wasm
if %ERRORLEVEL% NEQ 0 (goto Error)
xcopy /s /e Library\Binaries\WASM\%~1\ Tools\WrapperGenerator\Output\TypeScript\connected-spaces-platform.web\%~1\
if %ERRORLEVEL% NEQ 0 (goto Error) else (goto Success)

:Error
cd Tools/Emscripten
echo ERROR: Build failed. Have you started the docker engine?
exit /b 1

:Success
cd Tools/Emscripten
echo Success! Build located in: ..\WrapperGenerator\Output\TypeScript\connected-spaces-platform.web
exit /b 0