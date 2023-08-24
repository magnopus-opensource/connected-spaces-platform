rem Build Connected Spaces Platform WASM lib

for /f "delims=" %%x in (../Tools/Emscripten/emsdk_version.txt) do (
    set emsdk_version=%%x
    goto :BuildWASM
)

:BuildWASM

echo %cd%

docker run -w /src -v %cd%:/src --rm emscripten/emsdk:%emsdk_version% emmake make config=debug_wasm clean
docker run -w /src/UnitTesting -v %cd%/..:/src --rm emscripten/emsdk:%emsdk_version% emmake make -j 8 config=debug_wasm