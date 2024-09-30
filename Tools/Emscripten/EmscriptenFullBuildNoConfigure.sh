read -r emsdk_version < emsdk_version.txt
export emsdk_version=$emsdk_version;

cd ../..

if [ -z "$1" ]
  then
    echo "No configuration name provided"
	exit
fi

docker run -w /src -v `pwd`:/src --rm emscripten/emsdk:$emsdk_version emmake make -j 8 config="$1"_wasm

python3 Tools/WrapperGenerator/WrapperGenerator.py --generate_typescript
cd Tools/Emscripten