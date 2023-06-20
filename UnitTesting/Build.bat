rem Make sure we can run msbuild

rem Try VS2019 first

if not defined DevEnvDir (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat" amd64
)

rem If VS2019 is not installed, try VS2022

if not defined DevEnvDir (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" amd64
)

rem Build Windows Foundation lib

msbuild WrapperGeneratorUnitTesting.sln


rem Build Foundation WASM lib

for /f "delims=" %%x in (../Tools/Emscripten/emsdk_version.txt) do (
    set emsdk_version=%%x
    goto :BuildWASM
)

:BuildWASM

echo %cd%

docker run -w /src -v %cd%:/src --rm emscripten/emsdk:%emsdk_version% emmake make config=debug_wasm clean
docker run -w /src/UnitTesting -v %cd%/..:/src --rm emscripten/emsdk:%emsdk_version% emmake make -j 8 config=debug_wasm